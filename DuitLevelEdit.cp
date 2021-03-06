/* 
 | Implementation of DuitLevel
*/

#include "DuitLevelEdit.h"

#define DOC_WIND		128

#define PPAT_BACK		128
#define PPAT_FILL		129
#define PPAT_OUTLINE	130
#define PPAT_SELECT     131
#define PPAT_METASEL    132
#define PPAT_CONTROL	150
#define PPAT_DEBUG0		160

#define CURSOR_MAG		1000

static void GetSize( short nVertex, WadVertex *vertices, ScrollsRect *bounds, short *scale )
{
	WadVertex *thisVertex;
	
	if (vertices == 0) {
		bounds->left 	= 0;
		bounds->right 	= 1000;
		bounds->top 	= 0;
		bounds->bottom 	= 1000;
		return;
	}
	
	thisVertex = vertices;
	bounds->left = bounds->right = thisVertex->x;
	bounds->top = bounds->bottom = thisVertex->y;
	
	while( ++thisVertex < vertices+nVertex ) {
		if (thisVertex->x < bounds->left)
			bounds->left = thisVertex->x;
		else if (thisVertex->x > bounds->right)
			bounds->right = thisVertex->x;
			
		if (thisVertex->y < bounds->bottom)
			bounds->bottom = thisVertex->y;
		else if (thisVertex->y > bounds->top)
			bounds->top = thisVertex->y;
	}
	
	bounds->left 	-= 20;
	bounds->right 	+= 20;
	bounds->top		= -bounds->top - 20;
	bounds->bottom	= -bounds->bottom + 20;
	
	*scale = -1;
	
	bounds->left 	/= 2;
	bounds->right	/= 2;
	bounds->top		/= 2;
	bounds->bottom	/= 2;
	
	return;
}
	

/*
 | DuitLevelEdit (Creater)
 | Open the specified file and display its main window
*/

DuitLevelEdit::DuitLevelEdit( unsigned char *title, LevelDesc *descrip ) : Scroll( DOC_WIND )
{
	ScrollDesc	scrollDesc;
	long		*CurDirStore = (long *)0x398;
	unsigned char	pasTitle[9], *tchar, *pchar;
	
	/* I really think c++ stinks. About as robust as a house of cards. */
	
	toolPal 	= 0;			
	needsSaving = 0;
	sectDesc	= 0;
	
	pPatBack		= 0;
	pPatFill		= 0;
	pPatOutline		= 0;
	pPatControl 	= 0;
	pPatSelect  	= 0;
	pPatMetaSelect 	= 0;
	
	level = descrip;
	
	debugLindex = 0;
	debugLines  = 0;
	
	/* Allocate scratch regions */
	
	toxor1 = NewRgn();
	toxor2 = NewRgn();
	if (toxor2 == 0) {
		ioError = 1;
		return;
	}
	
	/* Get color pen patterns */
	
	pPatBack    	= GetPixPat( PPAT_BACK );
	pPatFill    	= GetPixPat( PPAT_FILL );
	pPatOutline 	= GetPixPat( PPAT_OUTLINE );
	pPatControl 	= GetPixPat( PPAT_CONTROL );
	pPatSelect 		= GetPixPat( PPAT_SELECT );
	pPatMetaSelect	= GetPixPat( PPAT_METASEL );
	
	/* Get cursors */
	
	magCursor	= GetCursor( CURSOR_MAG );
		
	/* Build sector descriptors */
	
	ioError = BuildSectDesc();
	if (ioError) return;
	
	selected = sectDesc;
	
	/* Decide on scale and size of screen */
	
	GetSize( level->nVertex, level->vertices, &bounds, &scale );
	
	scaleFact = (scale < 0) ? (1 << (-scale)) : (1 << scale);
	
	/* Make our tool palette */
	
	toolPal = new Palette( (short)1000, (short)4, 0, (short)1, pPatControl );
	
	/* Build a pascal string. We know that titles cannot be greater than eight characters */
	
	tchar = title+7;
	while( (*tchar != 0) && (tchar > title) ) tchar--;
	
	pasTitle[0] = tchar-title+1;
	pchar = pasTitle + pasTitle[0];
	while( pchar > pasTitle ) 
		*pchar-- = *tchar--;
	
	/* Open up a scroll window */
	
	scrollDesc.leftMargin	= 32;
	scrollDesc.topMargin	= 0;
	scrollDesc.minWidth		= 80+scrollDesc.leftMargin;
	scrollDesc.minHeight	= 80;
	scrollDesc.imageRect	= bounds;
	scrollDesc.title		= pasTitle;
	scrollDesc.startVert	= scrollCenter;
	scrollDesc.startHorz	= scrollCenter;
	
	Setup( &scrollDesc );
	
	return;
}


DuitLevelEdit::~DuitLevelEdit()
{
	if (toolPal) 	delete toolPal;
	
	if (toxor1)		DisposeRgn(toxor1);
	if (toxor2)		DisposeRgn(toxor2);
	
	if (pPatBack)		DisposPixPat( pPatBack );
	if (pPatFill)		DisposPixPat( pPatFill );
	if (pPatOutline)	DisposPixPat( pPatOutline );
	if (pPatControl)	DisposPixPat( pPatControl );
	if (pPatSelect)		DisposPixPat( pPatSelect );
	if (pPatMetaSelect)	DisposPixPat( pPatMetaSelect );
	
	if (sectDesc) {
		SectDesc	*thisSect = sectDesc + level->nSector;
		
		if (thisSect->region) DisposeRgn( thisSect->region );
		
		while(--thisSect >= sectDesc) {
			SectVList	*curr = thisSect->vLists;
			while( curr ) {
				SectVList *next = curr->next;
				SectVerts *vect = curr->head;
				do {
					SectVerts *nextVect = vect->next;
					delete vect;
					vect = nextVect;					// Careful!! vect might be zero
				} while(vect && (vect != curr->head));	// for unclosed (bad) sectors
				delete curr;
				curr = next;
			}
		}
	}
	
	if (debugLines) {
		delete debugLines;
		if (pPatDebug[0]) DisposPixPat( pPatDebug[0] );
		if (pPatDebug[1]) DisposPixPat( pPatDebug[1] );
		if (pPatDebug[2]) DisposPixPat( pPatDebug[2] );
	}	
	return;
}



//
// Coordinate systems/definitions
//
// Screen coordinates: (0,0) to (showRect.right,shortRect.top)
// WAD coordinates: v->x  v->y   (short)
//
// Conversion: Wad --> Screen
//        if scale < 0:  xS = + v->x/scaleFact - topLeftSect.h
//                       yS = - v->y/scaleFact - topLeftSect.v
//        if scale > 0:  xS = + v->x*scaleFact - topLeftSect.h
//                       yS = - v->y*scaleFact - topLeftSect.v
//        if scale = 0:  xS = + v->x - topLeftSect.h      
//                       yS = - v->y + topLeftSect.v
//
// Conversion: Screen --> Wad
//        if scale < 0:  v->x = +(xS + topLeftSect.h)*scaleFact
//                       v->y = -(yS + topLeftSect.v)*scaleFact
//        if scale > 0:  v->x = +(xS + topLeftSect.h)/scaleFact
//                       v->y = -(yS + topLeftSect.v)/scaleFact
//        if scale = 0:  v->x = +(xS + topLeftSect.h)
//                       v->y = -(yS + topLeftSect.v)
//


/*
 | XtoScreen
 | Convert X WAD coordinate to screen coordinate. For integer overflows (from either
 | very large WADs or very high magnifications) the value is concatonated to
 | SCROLL_SMALLEST_SHORT or SCROLL_LARGEST_SHORT, as appropriate.
*/
short	DuitLevelEdit::XtoScreen( short x )
{
	long answer  = x;

	if (scale > 0)
		answer *= scaleFact;
	else if (scale < 0)
		answer /= scaleFact;
		
	answer -= topLeftSect.h;
		
	if (answer < SCROLL_SMALLEST_SHORT) return( SCROLL_SMALLEST_SHORT );
	if (answer > SCROLL_LARGEST_SHORT)  return( SCROLL_LARGEST_SHORT  );
	
	return( (short)answer );
}

/*
 | YtoScreen
 | Convert Y WAD coordinate to screen coordinate. For integer overflows (from either
 | very large WADs or very high magnifications) the value is concatonated to
 | SCROLL_SMALLEST_SHORT or SCROLL_LARGEST_SHORT, as appropriate.
*/
short	DuitLevelEdit::YtoScreen( short y )
{
	long answer  = -y;

	if (scale > 0)
		answer *= scaleFact;
	else if (scale < 0)
		answer /= scaleFact;
		
	answer -= topLeftSect.v;
		
	if (answer < SCROLL_SMALLEST_SHORT) return( SCROLL_SMALLEST_SHORT );
	if (answer > SCROLL_LARGEST_SHORT)  return( SCROLL_LARGEST_SHORT  );
	
	return( (short)answer );
}

/*
 | XtoWad
 | Convert X screen coordinate to WAD coordinate
*/
short	DuitLevelEdit::XtoWad( short x )
{
	long answer = x;
	
	answer = answer + topLeftSect.h;
	if (scale > 0)
		answer /= scaleFact;
	else if (scale < 0)
		answer *= scaleFact;
		
	if (answer < SCROLL_SMALLEST_SHORT) return( SCROLL_SMALLEST_SHORT );
	if (answer > SCROLL_LARGEST_SHORT)  return( SCROLL_LARGEST_SHORT  );

	return(answer);
}

/*
 | YtoWad
 | Convert Y screen coordinate to WAD coordinate
*/
short	DuitLevelEdit::YtoWad( short y )
{
	long answer = y;
	
	answer = answer + topLeftSect.v;
	if (scale > 0)
		answer /= scaleFact;
	else if (scale < 0)
		answer *= scaleFact;
	
	answer = -answer;
		
	if (answer < SCROLL_SMALLEST_SHORT) return( SCROLL_SMALLEST_SHORT );
	if (answer > SCROLL_LARGEST_SHORT)  return( SCROLL_LARGEST_SHORT  );

	return(answer);
}



/*
 | RebuildRgn
 | Rebuild region corresponding to one sector
*/
void MakeSectRegion( SectVList *list, RgnHandle region, short scale );

void DuitLevelEdit::RebuildRgn( SectDesc *thisSect )
{
	SectVList	*list = thisSect->vLists;
	
	/* Build the sector belonging to the first list */
	
	MakeSectRegion( list, thisSect->region );
	
	/* Remove any sub-sectors */
	
	while( list=list->next ) {
	
		/* Addition list (sector within sector). Build it and do an XOR */
		
		CopyRgn( thisSect->region, toxor1 );
		MakeSectRegion( list, toxor2 );
		
		XorRgn( toxor1, toxor2, thisSect->region );
	}
	
	thisSect->onScreen = on;
}

void DuitLevelEdit::MakeSectRegion( SectVList *list, RgnHandle region )
{
	SectVerts *vert = list->head;
	
	OpenRgn();
	MoveTo( XtoScreen( vert->vertex->x ), YtoScreen( vert->vertex->y ) );
	
	do {
		vert = vert->next;
		LineTo( XtoScreen( vert->vertex->x ), YtoScreen( vert->vertex->y ) );
	} while(vert!=list->head);
	
	CloseRgn( region );
	
	if (EmptyRgn(region)) {
		vert = 0;
	}
}


void DuitLevelEdit::Draw( ScrollsPoint topLeft, RgnHandle updateRgn, RgnHandle visRgn, short newFrame )
{
	SectDesc	*thisSect;
	Rect		frame;
 	long		ibit;
	
	FillCRgn( updateRgn, pPatBack );
	
	// Well... anything to draw???

	if (sectDesc==0) return;
	
	// Had anything moved?
	
	if (newFrame) {
	
		// How much (if any) has the coordinate origin changed?
		
		long xShift = topLeftSect.h - topLeft.h;
		long yShift = topLeftSect.v - topLeft.v;
		long noShift = (xShift == 0 && yShift == 0);
		
		topLeftSect = topLeft;
	
		// What are the WAD coordinates of the current visible area?
		
		frame.left 		= XtoWad( 0 ) - 1;
		frame.right		= XtoWad( showRect.right ) + 1;
		frame.top    	= YtoWad( showRect.bottom ) + 1;
		frame.bottom	= YtoWad( 0 ) - 1;
		
		// Loop over sectors, looking for those that need to be drawn
		
		thisSect = sectDesc + level->nSector;
		while( --thisSect >= sectDesc ) {
			
			// Is this sector somewhere near the screen?
			
			if ( (frame.left   > thisSect->bounds.right  ) ||
			     (frame.right  < thisSect->bounds.left   ) ||
			     (frame.top    > thisSect->bounds.bottom ) ||
			     (frame.bottom < thisSect->bounds.top    )    ) {
				thisSect->onScreen = off;
				continue;
			}
						
			// Okay: Rebuild or shift the region, as appropriate
			
			if (thisSect->onScreen) {
				if (noShift) continue;
				
				// Will this sector give us overflow problems at this screen origin?
					
				if ((XtoScreen(thisSect->bounds.left)   == SCROLL_SMALLEST_SHORT) ||
			  	    (XtoScreen(thisSect->bounds.right)  == SCROLL_LARGEST_SHORT)  ||
			  	    (YtoScreen(thisSect->bounds.top)    == SCROLL_LARGEST_SHORT)  ||
			   	    (YtoScreen(thisSect->bounds.bottom) == SCROLL_SMALLEST_SHORT)    ) {
			   	    
			   	    // Yeah? That's one big f**king sector...
			   	    
			   	    RebuildRgn( thisSect );
			   	}
			   	else
			   		OffsetRgn( thisSect->region, (short)xShift, (short)yShift );
			}
			else 
				RebuildRgn( thisSect );

		}
	}
	
	// Draw away!
	
	PenPixPat( pPatOutline );
	
	ibit = selected - sectDesc + level->nSector*level->nSector;
	thisSect = sectDesc + level->nSector;
	while( ibit -= level->nSector, --thisSect >= sectDesc ) {
		if (thisSect->onScreen) {
			RgnPtr	u = *updateRgn,
			        s = *thisSect->region;
			PixPatHandle fill;
			unsigned char	*rejectChar = level->reject + (ibit>>3);
			unsigned char	mask = 1 << (ibit&0x007);
		
			if (u->rgnBBox.left   > s->rgnBBox.right ) continue;
			if (u->rgnBBox.right  < s->rgnBBox.left  ) continue;
			if (u->rgnBBox.top    > s->rgnBBox.bottom) continue;
			if (u->rgnBBox.bottom < s->rgnBBox.top   ) continue;
		
			if (thisSect==selected)
				fill = pPatMetaSelect;
			else if ((*rejectChar)&mask)
				fill = pPatSelect;
			else
				fill = pPatFill;
		
			FillCRgn( thisSect->region, fill );
			FrameRgn( thisSect->region );
		}
	}
	
	// Debug active??
	
	if (debugLines) DrawDebug( 0, 0 );
	
	return;
}

void DuitLevelEdit::DrawBorder( RgnHandle updateRgn, short newFrame )
{
	Point	toolPoint = {1,-30};
	Rect	marginRect = showRect;
	
	marginRect.right = 0;
	marginRect.bottom += 16;

	FillCRect( &marginRect, pPatControl );
	
	toolPal->Show( window, toolPoint );
	
	return;
}

void DuitLevelEdit::Mouse( ScrollsPoint topLeft, Point where, short modifiers )
{
	SectDesc	*thisSect;

	if (toolPal->selected == 3) {
		Point	keepHere;
		
		// Scale tool: where were we in WAD coordinates?
		
		keepHere.h = XtoWad( where.h );
		keepHere.v = YtoWad( where.v );
		
		// New scale? Simple press is magnify, shift-press is demagnify
	
		if (modifiers&shiftKey) {
			scale = scale - 1;
			bounds.left 	/= 2;
			bounds.right 	/= 2;
			bounds.top		/= 2;
			bounds.bottom	/= 2;
		}
		else {
			scale = scale + 1;
			bounds.left 	*= 2;
			bounds.right 	*= 2;
			bounds.top		*= 2;
			bounds.bottom	*= 2;
		}
		
		scaleFact = (scale < 0) ? (1 << (-scale)) : (1 << scale);
				
		// Reset sector regions
		
		thisSect = sectDesc + level->nSector;
		while( --thisSect >= sectDesc ) 
			thisSect->onScreen = off;
		
		// Reset scroll region such that current WAD point is unmoved on screen.
		// Do this by taking coordinate conversion equations 
		//   (i.e. xS = + v->x/scaleFact - topLeftSect.h )
		// and solving for topLeft.
		
		if (scale < 0) {
			topLeft.h = +keepHere.h/scaleFact - where.h;
			topLeft.v = -keepHere.v/scaleFact - where.v;
		}
		else if (scale > 0) {
			topLeft.h = +keepHere.h*scaleFact - where.h;
			topLeft.v = -keepHere.v*scaleFact - where.v;
		}
		else {
			topLeft.h = +keepHere.h - where.h;
			topLeft.v = -keepHere.v - where.v;
		}

		ResetImageSize( bounds, topLeft );
	}
	else if (toolPal->selected == 2) {
		SectDesc *newSelect = 0;
	
		// Reject select: which sector on screen does the mouse press correspond to?
	
		thisSect = sectDesc + level->nSector;
		while( --thisSect >= sectDesc ) 
			if (thisSect->onScreen) {
				if (PtInRgn( where, thisSect->region )) {
					newSelect = thisSect;
					break;
				}
			}
			
		if (newSelect==0) 
			SysBeep(30);
		else if (newSelect != selected) {
			long	ibit1, ibit2;
			void 	*startHndl = StartDrawScroll();
			
			PenPixPat( pPatOutline );

			FillCRgn( selected->region, pPatFill );
			FrameRgn( selected->region );
			
			FillCRgn( newSelect->region, pPatMetaSelect );
			FrameRgn( newSelect->region );
			
			ibit1 = selected  - sectDesc + level->nSector*level->nSector;
			ibit2 = newSelect - sectDesc + level->nSector*level->nSector;
			thisSect = sectDesc + level->nSector;
			while( ibit1 -= level->nSector, ibit2 -= level->nSector, --thisSect >= sectDesc ) 
				if (thisSect->onScreen && (thisSect != newSelect) ) {
					unsigned char	*rej1 = level->reject + (ibit1>>3),
									*rej2 = level->reject + (ibit2>>3),
									mask1 = 1 << (ibit1&0x07),
									mask2 = 1 << (ibit2&0x07);
									
					unsigned char 	sel1 = (*rej1)&mask1,
									sel2 = (*rej2)&mask2;
				
					if (sel1 != sel2) {
						FillCRgn( thisSect->region, sel2 ? pPatSelect : pPatFill );
						FrameRgn( thisSect->region );
					}	
				}
			

			selected = newSelect;
			FinishDrawing( startHndl );
		}
			
	}

	return;
}


void DuitLevelEdit::MouseBorder( Point where, short modifiers )
{
	if (toolPal->CheckPress( where )) return;

	SysBeep( 30 );
	return;
}


/*
 | CallMeALot
 | A routine to be called routinely (in the main eventloop)
*/
void DuitLevelEdit::CallMeALot()
{
	int mouseIsIn = MouseInScroll();
	if (mouseIsIn != mouseWasIn) {
		mouseWasIn = mouseIsIn;
		if (mouseIsIn)
			SetCursor( *magCursor );
		else
			SetCursor( &qd.arrow );
	}

	if (debugLines) FlashDebug();
}


/*
 | SetBehind
 | Place this level's window behind the specified one
*/
void DuitLevelEdit::SetBehind( WindowPtr frontWindow )
{
	WindowPeek peek = (WindowPeek)window;
	
	SendBehind( window, frontWindow );
	PaintOne( (GrafPtr)peek, peek->strucRgn );
	CalcVis( (GrafPtr)peek );
}


/*
 | BuildSectDesc
 | Construct sector descriptors.
 |   Returns:      0  all okay
 |                -1  out of memory
 |                 N  number problem sectors
*/
int AddLineToList( WadVertex *v1, WadVertex *v2, SectDesc *sector );

int DuitLevelEdit::BuildSectDesc()
{	
	WadLine		*thisLine;
	SectDesc	*thisDesc;
	int			errRet;
	short		nError = 0;
	
	/* Allocate memory for sector descriptors */
	
	sectDesc = new SectDesc[level->nSector];
	if (sectDesc==0) return(-1);
	
	/* Initialize */
	
	thisDesc = sectDesc + level->nSector;
	while( --thisDesc >= sectDesc ) {
		thisDesc->region = 0;
		thisDesc->vLists = 0;
		thisDesc->error  = 0;
		thisDesc->region = NewRgn();
		if (thisDesc->region == 0) return(-1);
		thisDesc->onScreen = off;
	}	
	
	/* Loop over linedefs */
	
	thisLine = level->lines + level->nLine;
	while( --thisLine >= level->lines ) {
		WadVertex 	*v1 = level->vertices + thisLine->from;
		WadVertex 	*v2 = level->vertices + thisLine->to;
		WadSide 	*s1 = thisLine->right == -1 ? 0 : level->sides + thisLine->right;
		WadSide		*s2 = thisLine->left  == -1 ? 0 : level->sides + thisLine->left;
				
		if (s1) {
			if (AddLineToList( v1, v2, sectDesc+s1->sector )) return(-1);
		}
		else {
			nError++;
		}
		
		if (s2) {
			if (AddLineToList( v2, v1, sectDesc+s2->sector )) return(-1);
		}
	}
	
	/* Check for unclosed sectors */
	
	thisDesc = sectDesc + level->nSector;
	while( --thisDesc >= sectDesc ) {
		SectVList *list = thisDesc->vLists;
		
		if (list == 0) {
			thisDesc->error = 2;
			nError++;
		}
		else {
			while (list) {
				if (list->head != list->tail) {
					nError++;
					thisDesc->error = 1;
					break;
				}
				list = list->next;
			}
		}
	}
			
	return(nError);
}	

int AddLineToList( WadVertex *v1, WadVertex *v2, SectDesc *sector )
{
	SectVList	*thisList;
	SectVList	*prev;
	SectVList	*topsThis = 0;
	SectVList	*topsThisPrev = 0;
	SectVList	*botsThis = 0;
	
	/* See if our new pair of vertices belong to a current list */
	
	prev = 0;
	thisList = sector->vLists;
	while( thisList ) {
		if (thisList->head != thisList->tail) {
			if (thisList->head->vertex == v2) {
			
				/* This new pair belongs to the front of this list: make note */
				
				topsThis = thisList;
				topsThisPrev = prev;
				if (botsThis) break;
			}
			if (thisList->tail->vertex == v1) {
			
				/* This new pair belongs to the bottom of this list: make note */
				
				botsThis = thisList;
				if (topsThis) break;
			}
		}
		prev = thisList;
		thisList = thisList->next;
	}
	
	/* Update bounds */
	
	if (botsThis==0) {
		if (sector->vLists == 0) {
			sector->bounds.right  = sector->bounds.left  = v1->x;
			sector->bounds.bottom = sector->bounds.top   = v1->y;
		}
		else {
			if (v1->x < sector->bounds.left)
				sector->bounds.left = v1->x;
			else if (v1->x > sector->bounds.right)
				sector->bounds.right = v1->x;
				
			if (v1->y < sector->bounds.top)
				sector->bounds.top = v1->y;
			else if (v1->y > sector->bounds.bottom)
				sector->bounds.bottom = v1->y;
		}
	}
	
	if (topsThis==0) {
		if (v2->x < sector->bounds.left)
			sector->bounds.left = v2->x;
		else if (v2->x > sector->bounds.right)
			sector->bounds.right = v2->x;
			
		if (v2->y < sector->bounds.top)
			sector->bounds.top = v2->y;
		else if (v2->y > sector->bounds.bottom)
			sector->bounds.bottom = v2->y;
	}
	
	/* Update lists */
	
	if ((botsThis == 0) && (topsThis == 0)) {
	
		/* The vertex pair is new: make a new vertex list */
		
		thisList = new SectVList;
		if (thisList == 0) return(-1);
		
		thisList->next = sector->vLists;
		sector->vLists = thisList;
		
		thisList->head = new SectVerts;
		if (thisList->head == 0) return(-1);
		
		thisList->tail = new SectVerts;
		if (thisList->tail == 0) return(-1);
		
		thisList->head->vertex = v1;
		thisList->head->next = thisList->tail;
		thisList->tail->vertex = v2;
		thisList->tail->next = 0;
		
		thisList->nVert = 2;
	}
	else if (botsThis == 0) {
		SectVerts	*newVert;
	
		/* The vertex pair belongs only on the top of a current list: add it */
		
		newVert = new SectVerts;
		if (newVert == 0) return(-1);
		
		newVert->next = topsThis->head;
		topsThis->head = newVert;
		newVert->vertex = v1;
		
		topsThis->nVert++;
	}
	else if (topsThis == 0) {
		SectVerts	*newVert;
	
		/* The vertex pair belongs only on the bottom of a current list: add it */
		
		newVert = new SectVerts;
		if (newVert == 0) return(-1);
		
		botsThis->tail->next = newVert;
		botsThis->tail = newVert;
		newVert->vertex = v2;
		newVert->next = 0;
		
		botsThis->nVert++;
	}
	else if (topsThis == botsThis) {
	
		/* This vertex pair belongs on the top and bottom of the same list */
		
		topsThis->tail->next = topsThis->head;
		topsThis->tail = topsThis->head;
	}
	else {
	
		/* This vertex connects what was two disconnected vertex lists: merge them */
		
		botsThis->tail->next = topsThis->head;
		botsThis->tail = topsThis->tail;
		botsThis->nVert += topsThis->nVert;
		
		if (topsThisPrev) 
			topsThisPrev->next = topsThis->next;
		else
			sector->vLists = topsThis->next;
			
		delete topsThis;
	}
	
	return(0);
}
		

/*
 | NextDebug2
 | Debug option 2
*/

void DuitLevelEdit::NextDebug2()
{
	void 	*startHndl;
	
	if (debugLines==0) return;
	
	startHndl = StartDrawScroll();

	// Erase previous and get next
	
	if (debugNext2) {
		DrawDebug2( 1, debugNext2 );
		debugNext2++;
		if (debugNext2 >= debugLines2 + nDebugLine2) debugNext2 = debugLines2;
	}
	else
		debugNext2 = debugLines2;
	
	// Draw away

	DrawDebug2( 0, debugNext2 );

	FinishDrawing( startHndl );
	return;
}


/*
 | NextDebug
 | Issue next debug option. Redrawing might be required.
*/

void DuitLevelEdit::NextDebug()
{
	void 	*startHndl = StartDrawScroll();

	if (debugLines==0) {
		debugLines = new RejectDebugLine[level->nLine<<1];
		debugLindex = 0;
		debugLines2 = new RejectDebugLine2[level->nLine];
		
		pPatDebug[0] = GetPixPat( PPAT_DEBUG0+1 );
		pPatDebug[1] = GetPixPat( PPAT_DEBUG0+2 );
		pPatDebug[2] = GetPixPat( PPAT_DEBUG0+3 );
	}
	else {
		debugLindex++;
		if (debugLindex==level->nLine) debugLindex = 0;
		
		DrawDebug( 1, 0 );
		if (debugNext2) DrawDebug2( 1, debugNext2 );
	}
	
	RejectDebug( level, debugLindex, debugLines, &nDebugLine, debugLines2, &nDebugLine2 );
	debugNext2 = 0;
	
	DrawDebug( 0, 0 );
	FinishDrawing( startHndl );
	return;
}
		
/*
 | DrawDebug
 | Issue next debug option. Redrawing might be required.
*/

void DuitLevelEdit::DrawDebug( int erase, int wait )
{
	RejectDebugLine *thisLine = debugLines;
	long	notUsed;
	
	do {
		if (thisLine->color==0) continue;
		if (wait) Delay( wait, &notUsed );
	
		PenPixPat( erase ? pPatOutline : pPatDebug[thisLine->color-1] );
		
		MoveTo( XtoScreen( thisLine->x1 ), YtoScreen( thisLine->y1 ) );
		LineTo( XtoScreen( thisLine->x2 ), YtoScreen( thisLine->y2 ) );
	} while( ++thisLine < debugLines + nDebugLine );
}

/*
 | DrawDebug2
*/

void DuitLevelEdit::DrawDebug2( int erase, RejectDebugLine2 *thisLine )
{
	PenPixPat( erase ? pPatBack : pPatDebug[thisLine->color-1] );
		
	MoveTo( XtoScreen( thisLine->x1 ), YtoScreen( thisLine->y1 ) );
	LineTo( XtoScreen( thisLine->x2 ), YtoScreen( thisLine->y2 ) );
	LineTo( XtoScreen( thisLine->x3 ), YtoScreen( thisLine->y3 ) );
	LineTo( XtoScreen( thisLine->x4 ), YtoScreen( thisLine->y4 ) );
}

/*
 | FlashDebug
 | Flash first debug line, to be called a lot
*/
void DuitLevelEdit::FlashDebug()
{
	void			*startHndl;
	RejectDebugLine *thisLine = debugLines;
	long			ticks = TickCount();
	long 			flashState = ticks & 0x20;
	
	if (flashState != debugFlash) {
		debugFlash = flashState;
		
		startHndl = StartDrawScroll();
		PenPixPat( flashState ? pPatOutline : pPatDebug[thisLine->color-1] );
		
		MoveTo( XtoScreen( thisLine->x1 ), YtoScreen( thisLine->y1 ) );
		LineTo( XtoScreen( thisLine->x2 ), YtoScreen( thisLine->y2 ) );
		FinishDrawing( startHndl );
	}
}
		
		
