/*
 | Reject.c
 | Implementation of reject map building package
*/

#include "Reject.h"
#include "RejectPrivate.h"

//
// BlockBlock
// Decide if block1 covers a corner of block2 and make the necessary modifications
// of block2
//
static int BlockBlock( Block *block1, Block *block2 )
{
	BlockLine face;
	float	sB, sF;
	int		answer = 0;

	// Build a line representing the face of block1
	
	face.x0 = block2->bl.x0;
	face.y0 = block2->bl.y0;
	face.tx = block2->br.x0 - face.x0;
	face.ty = block2->br.y0 - face.y0;

	// Check intersection with right block line
	
	if (GetIntersect( &block2->br, &face, &sB, &sF )==0) {
	
		if ((sB >= 0) && (sF < 0) && (sF <= 1.0)) {
		
			// Intersection found. Update right block
			
			block2->br.tx = block2->br.x0 - block1->bl.x0;
			block2->br.ty = block2->br.y0 - block1->bl.y0;
			block2->covered |= 2;
			
			answer = 1;
		}
	}

	// Check intersection with left block line
	
	if (GetIntersect( &block2->bl, &face, &sB, &sF )==0) {
	
		if ((sB >= 0) && (sF < 0) && (sF <= 1.0)) {
		
			// Intersection found. Update right block
			
			block2->bl.tx = block2->bl.x0 - block1->br.x0;
			block2->bl.ty = block2->bl.y0 - block1->br.y0;
			block2->covered |= 1;
			
			answer = 1;
		}
	}
	
	return(answer);
}

//
// GetIntersect
// Return distance along two lines to their intersection.
// Returns non-zero if the intersection does not exist (i.e. lines are parallel)
//
static int GetIntersect( BlockLine *line1, BlockLine *line2, float *s1, float *s2 )
{
	long 	den;
	float	fDen, nom1, nom2;
	
	den = line1->tx*line2->ty - line1->ty*line2->tx;
	if (den==0) return(1);
	
	fDen = den;
	
	nom1 = (line2->x0 - line1->x0)*line2->ty - (line2->y0 - line1->y0)*line2->tx;
	nom2 = (line2->x0 - line1->x0)*line1->ty - (line2->y0 - line1->y0)*line1->tx;
	
	*s1 = nom1/fDen;
	*s2 = nom2/fDen;
	return(0);
}

//
// Return cross product of line1 with start of line2
//
static long	CrossOut1( BlockLine *line1, BlockLine *line2 )
{
	return( line1->tx*(line2->y0-line1->y0) - line1->ty*(line2->x0-line1->x0) );
}

//
// Return cross product of line1 with end of line2
//
static long CrossOut2( BlockLine *line1, BlockLine *line2 )
{
	return( line1->tx*(line2->y0+line2->ty-line1->y0) - 
	        line1->ty*(line2->x0+line2->tx-line1->x0)   );
}

//
// Oblique
//
static long Oblique( BlockLine *line1, BlockLine *line2 )
{
	return( line1->tx*line2->ty - line1->ty*line2->tx );
}

//
// Return start and end blocked along a length of a target line.
// Return value is non-zero if there is any block.
//
int	AmIBlocked( BlockLine *line, Block *block, float *bStart, float *bEnd )
{
	float	lo, hi, bs;
	int		cBlock, cFace;
	BlockLine face;
	
	// Build face line
	
	face.x0 = block->bl.x0;
	face.y0 = block->bl.y0;
	face.tx = block->br.x0 - face.x0;
	face.ty = block->br.y0 - face.y0;
	
	// Check starting point and left block line

	cBlock = CrossOut1( &block->bl, line );
	cFace  = CrossOut1( &face, line );

	if (Oblique( &block->bl, &face ) > 0) {
	
		// The left block is "oblique"
		
		lo = ((cBlock > 0) && (cFace < 0)) ? 1 : 0;
	}
	else
		lo = ((cBlock > 0) || (cFace < 0)) ? 1 : 0;
		
	if (lo == 1) {
	
		// Left point is outside: find intersection
		
		if (GetIntersect( line, &block->bl, &lo, &bs )) return(0);
		
		if (bs < 0) return(0);
	}
	
	// Check ending point and right block line

	cBlock = CrossOut2( &block->br, line );
	cFace  = CrossOut2( &face, line );

	if (Oblique( &block->br, &face ) > 0) {
	
		// The right block is "oblique"
		
		hi = ((cBlock < 0) && (cFace < 0)) ? 0 : 1;
	}
	else
		hi = ((cBlock < 0) || (cFace < 0)) ? 0 : 1;
		
	if (hi == 0) {
	
		// Right point is outside: find intersection
		
		if (GetIntersect( line, &block->br, &hi, &bs )) return(0);
		
		if (bs < 0) return(0);
	}

	
	// Take care of roundoff error by moving values by epsilon to the less
	// conservative (less uncovered) direction.
	
	*bStart = (lo<=0) ? 0 : lo - 0.00001;
	*bEnd   = (hi>=1) ? 1 : hi + 0.00001;

	return( *bStart < *bEnd );
}

//
// GetSeen
// Return those portions of target line that are not blocked out
//
static void GetSeen( Block *blocks, short nBlock, WadLine *skipMe,
                     WadVertex *start, WadVertex *end, Seen *seens, Seen **head )
{
	Seen 	*nextFree = seens,
			*prev, *curr;
	long 	x1, y1, tx1, ty1;
	float	bStart, bEnd;
	Block	*thisBlock;
	BlockLine line1;
	
	// Get line data
	
	line1.x0 = start->x;
	line1.y0 = start->y;
	line1.tx = end->x - start->x;
	line1.ty = end->y - start->y;
	
	// Initialize list 
	
	*head = nextFree;
	
	nextFree->start = 0;
	nextFree->end   = 1.0;
	nextFree->next  = 0;
	
	// Now, loop through blocks

	thisBlock = blocks + nBlock;
	while( --thisBlock >= blocks ) {
	
		// Don't let a line block itself!
		
		if (thisBlock->line == skipMe) continue;
	
		// Get portion blocked by this block (if any)
		
		if (AmIBlocked( &line1, thisBlock, &bStart, &bEnd )) {
		
			// See what portions, if any, of the seen area this blocks out
			
			curr = *head;
			prev = 0;
			while( curr ) {
				if (bStart <= curr->start) {
				
					// Block preceeds this seen
					
					if (bEnd >= curr->end) {
					
						// Entire seen is blocked: erase it
						
						if (prev)
							prev->next = curr->next;
						else
							*head = curr->next;
							
						// Here we *don't* want to update prev
							
						curr = curr->next;
						continue;
					}
					else if (bEnd > curr->start) {
					
						// Front piece is shadowed: update appropriately
						
						curr->start = bEnd;
					}
				}
				else if (bStart < curr->end) {
				
					// Block starts inside this seen
					
					if (bEnd > curr->end) {
					
						// End piece is shadowed: update appropriately
						
						curr->end = bStart;
					}
					else if (bEnd < curr->end) {
					
						// A middle section is shadowed: make new seen
						
						nextFree++;

						nextFree->end = curr->end;
						nextFree->start = bEnd;
						curr->end = bStart;
						if (prev)
							prev->next = nextFree;
						else
							*head = nextFree;
							
						nextFree->next = curr;
						
						// We can stop here
						
						break;
					}
				}
					
				// Default loop action: goto next seen
				
				prev = curr;
				curr = curr->next;
			}
			
			// If nothings left, we might as well return now
			
			if (*head == 0) return;
		}
	}
}


/*
 | InView (local routine)
 | Decide if line2 can be seen by line1
*/
static int InView( WadVertex *vertA, WadVertex *vertB, WadVertex *vertC, WadVertex *vertD )
{
	long Ax, Ay, Bx, By, Cx, Cy, Dx, Dy;

	Ax = vertA->x; Ay = vertA->y;
	Bx = vertB->x; By = vertB->y;
	Cx = vertC->x; Cy = vertC->y;
	Dx = vertD->x; Dy = vertD->y;

	// Is the "face" of the line visible?
	
	if ((Cx-Ax)*(Dy-Cy) - (Cy-Ay)*(Dx-Cx) > 0) return(0);
	if ((Dx-Bx)*(Dy-Cy) - (Dy-By)*(Dx-Cx) > 0) return(0);
	
	// Is line2 even partially in front of line1?
		 
	if ((Cx-Ax)*(By-Ay) - (Cy-Ay)*(Bx-Ax) > 0) return(1);
	if ((Dx-Ax)*(By-Ay) - (Dy-Ay)*(Bx-Ax) > 0) return(1);
		
	return(0);
}


int RejectDebug( LevelDesc *level, short iLine,  
				 RejectDebugLine *debugLines, short *nDebug,
				 RejectDebugLine2 *debugLines2, short *nDebug2 )
{
	RejectDebugLine *thisDebug;
	RejectDebugLine2 *thisDebug2;
	WadLine			*targetLine, *thisLine;
	Block			*blocks = new Block[level->nLine],
					*thisBlock;
	Seen			*seens = new Seen[level->nLine],
					*seenHead,
					*thisSeen;
	WadVertex		*targFrom, *targTo;
	short			nBlock;
	float			delx, dely, dell;
	
	// Memory problems?
	
	if (blocks == 0) return(1);
	
	// Any lines?
	
	if (level->nLine < 1) {*nDebug = 0; return(0);}
	
	thisDebug = debugLines;
	targetLine = level->lines + iLine;
	
	targFrom = level->vertices + targetLine->from;
	thisDebug->x1 = targFrom->x;
	thisDebug->y1 = targFrom->y;
	
	targTo = level->vertices + targetLine->to;
	thisDebug->x2 = targTo->x;
	thisDebug->y2 = targTo->y;
	
	thisDebug->color = 1;
	
	// First pass: loop over all lines that could block others
	
	thisBlock = blocks;
	nBlock = 0;
	
	thisLine = level->lines;
	do {
		WadVertex *from, *to;
	
		// Decide if this line is relevant
	
		if (thisLine == targetLine) continue;
		if (thisLine->left != -1) continue;
		
		// Is this line in view of the target line?
		
		from = level->vertices + thisLine->from;
		to   = level->vertices + thisLine->to;

		if (InView( targFrom, targTo, from, to )) {
			Block	*prevBlock;
			int		lockL, lockR;

			// Okay: make a new block
			
			thisBlock->line = thisLine;
			
			thisBlock->bl.vert = from;
			thisBlock->bl.x0 = from->x;
			thisBlock->bl.y0 = from->y;
			if (from == targTo) {
				thisBlock->bl.tx = from->x - targFrom->x;
				thisBlock->bl.ty = from->y - targFrom->y;
			}
			else {
				thisBlock->bl.tx = from->x - targTo->x;
				thisBlock->bl.ty = from->y - targTo->y;
			}
			thisBlock->bbl.tx = thisBlock->bl.tx;
			thisBlock->bbl.ty = thisBlock->bl.ty;

			thisBlock->br.vert = to;
			thisBlock->br.x0 = to->x;
			thisBlock->br.y0 = to->y;
			if (to == targFrom) {
				thisBlock->br.tx = to->x - targTo->x;
				thisBlock->br.ty = to->y - targTo->y;
			}
			else {
				thisBlock->br.tx = to->x - targFrom->x;
				thisBlock->br.ty = to->y - targFrom->y;
			}
			thisBlock->bbr.tx = thisBlock->br.tx;
			thisBlock->bbr.ty = thisBlock->br.ty;
			
			thisBlock->covered = 0;
			
			// Loop through the blocks we've made so far
			
			prevBlock = thisBlock;
			while( --prevBlock >= blocks ) {
			
				// Check for matching vertices
			
				if (prevBlock->bl.vert == thisBlock->br.vert) {
					thisBlock->br.tx = prevBlock->bbl.tx;
					thisBlock->br.ty = prevBlock->bbl.ty;
					prevBlock->bl.tx = thisBlock->bbr.tx;
					prevBlock->bl.ty = thisBlock->bbr.ty;
					thisBlock->covered |= 2;
					continue;
				}
				else if (prevBlock->br.vert == thisBlock->bl.vert) {
					thisBlock->bl.tx = prevBlock->bbr.tx;
					thisBlock->bl.ty = prevBlock->bbr.ty;
					prevBlock->br.tx = thisBlock->bbl.tx;
					prevBlock->br.ty = thisBlock->bbl.ty;
					thisBlock->covered |= 1;
					continue;
				}			
				
				// Check to see if prevBlock covers thisBlock
				
				if (BlockBlock( prevBlock, thisBlock )) continue;
				
				// And vice-versa
				
				if (BlockBlock( thisBlock, prevBlock )) continue;
			} 

			thisBlock++; nBlock++;
		}
	} while( ++thisLine < level->lines + level->nLine );
	
	// Prepare second pass: look for seen lines
	
	thisLine = level->lines;
	do {
		WadVertex *from, *to;
	
		// Decide if this line is relevant
	
		if (thisLine == targetLine) continue;

		from = level->vertices + thisLine->from,
		to   = level->vertices + thisLine->to;
					  
		// Check right side
		
		delx = to->x - from->x;
		dely = to->y - from->y;
		
		if (InView( targFrom, targTo, from, to )) {
			GetSeen( blocks, nBlock, thisLine, from, to, seens, &seenHead );
			
			thisSeen = seenHead;
			while( thisSeen ) {
				if (thisDebug-debugLines >= (level->nLine<<1)) break;
				thisDebug++;
		
				thisDebug->x1 = from->x + thisSeen->start*delx;
				thisDebug->y1 = from->y + thisSeen->start*dely;
				thisDebug->x2 = from->x + thisSeen->end*delx;
				thisDebug->y2 = from->y + thisSeen->end*dely;
				thisDebug->color = thisLine->left == -1 ? 2 : 3;
			
				thisSeen = thisSeen->next;
			}
		}
		
		// Check left side (if there is one)

		if (thisLine->left != -1) {
			if (InView( targFrom, targTo, to, from )) {
				GetSeen( blocks, nBlock, thisLine, to, from, seens, &seenHead );
							
				thisSeen = seenHead;
				while( thisSeen ) {
					if (thisDebug-debugLines >= (level->nLine<<1)) break;
					thisDebug++;
			
					thisDebug->x1 = to->x - thisSeen->start*delx;
					thisDebug->y1 = to->y - thisSeen->start*dely;
					thisDebug->x2 = to->x - thisSeen->end*delx;
					thisDebug->y2 = to->y - thisSeen->end*dely;
					thisDebug->color = 3;
				
					thisSeen = thisSeen->next;
				}
			}
		}
	} while( ++thisLine < level->lines + level->nLine );
	
	thisDebug2 = debugLines2;
	thisBlock = blocks;
	do {
		thisDebug2->x1 = thisBlock->bl.x0+thisBlock->bl.tx;
		thisDebug2->y1 = thisBlock->bl.y0+thisBlock->bl.ty;
		thisDebug2->x2 = thisBlock->bl.x0;
		thisDebug2->y2 = thisBlock->bl.y0;
		thisDebug2->x3 = thisBlock->br.x0;
		thisDebug2->y3 = thisBlock->br.y0;
		thisDebug2->x4 = thisBlock->br.x0+thisBlock->br.tx;
		thisDebug2->y4 = thisBlock->br.y0+thisBlock->br.ty;
		thisDebug2->color = 2;
		thisDebug2++;
	
	} while( ++thisBlock < blocks + nBlock );
	
	delete blocks;
	delete seens;

	*nDebug = thisDebug-debugLines+1;
	*nDebug2 = thisDebug2-debugLines2;
	
	return(0);
}
