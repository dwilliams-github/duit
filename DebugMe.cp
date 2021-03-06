//
// Implementation of debug window
//

#include "DebugMe.h"
#include <math.h>
#include <stdlib.h>

#define DEBUG_WIND 130
#define PPAT_DEBUG0		160

DebugMe::DebugMe() : Scroll( DEBUG_WIND )
{	
	ScrollDesc	scrollDesc;
	
	nTest = 0;

	pPat1 = GetPixPat( PPAT_DEBUG0+1 );
	pPat2 = GetPixPat( PPAT_DEBUG0+2 );
	pPat3 = GetPixPat( PPAT_DEBUG0+3 );


	scrollDesc.leftMargin	= 0;
	scrollDesc.topMargin	= 0;
	scrollDesc.minWidth		= 80;
	scrollDesc.minHeight	= 80;
	scrollDesc.imageRect.top    = -200;
	scrollDesc.imageRect.left   = -200;
	scrollDesc.imageRect.bottom =  200;
	scrollDesc.imageRect.right  =  200;
	scrollDesc.title		= "\pDUIT Debug";
	scrollDesc.startVert	= scrollCenter;
	scrollDesc.startHorz	= scrollCenter;
	
	Setup( &scrollDesc );
}

DebugMe::~DebugMe()
{
	DisposPixPat( pPat1 );
	DisposPixPat( pPat2 );
	DisposPixPat( pPat3 );
}


static void DrawArrow( long x0, long y0, long x1, long y1, ScrollsPoint *topLeft )
{
	float tnorm;
	long dx = x1-x0;
	long dy = y1-y0;
	
	tnorm = sqrt( (float)(dx*dx + dy*dy) );
	dx = 5.0*dx/tnorm;
	dy = 5.0*dy/tnorm;
	
	x0 += 200 - topLeft->h;
	x1 += 200 - topLeft->h;
	y0 = -y0 + 200 - topLeft->v;
	y1 = -y1 + 200 - topLeft->v;

	MoveTo( x0, y0 );
	LineTo( x1, y1 );
	LineTo( x1-dx+dy, y1+dy+dx );
	MoveTo( x1, y1 );
	LineTo( x1-dx-dy, y1+dy-dx );
}


static short MyRandom()
{
	return( rand()*360/RAND_MAX - 180 );
}

static short MyUniqueRandom(short notThis)
{
	short answer;
	
	do {
	 	answer = MyRandom();
	} while( answer == notThis );
	
	return(answer);
}



void DebugMe::DrawDebug()
{
	// Draw bound 
	
	PenPixPat( pPat1 );
	
	DrawArrow( block.bl.x0, block.bl.y0, 
	           block.bl.x0+block.bl.tx, block.bl.y0+block.bl.ty,
	           &topLeftSect );
	DrawArrow( block.br.x0, block.br.y0, 
	           block.br.x0+block.br.tx, block.br.y0+block.br.ty,
	           &topLeftSect );
    DrawArrow( block.bl.x0, block.bl.y0, 
               block.br.x0, block.br.y0,
	           &topLeftSect );
    
    // Draw target
    
    PenPixPat( pPat2 );
    DrawArrow( line.x0, line.y0, line.x0+line.tx, line.y0+line.ty, &topLeftSect );
    
    // Draw block
    
    if (anyBlock) {
    	DrawArrow( -185, 180,  175, 180, &topLeftSect );
    
    	PenPixPat( pPat3 );
		DrawArrow( seen.x0, seen.y0, seen.x0+seen.tx, seen.y0+seen.ty, &topLeftSect );
		
		DrawArrow( (long)(-185.0+bStart*360.0), 178, 
		           (long)(-185.0+  bEnd*360.0), 178, &topLeftSect );
	}
}


void DebugMe::Recall()
{
	void 	*startHndl = StartDrawScroll();
    
    if (nTest==0) return;
    
    anyBlock = AmIBlocked( &line, &block, &bStart, &bEnd );
    
    if (anyBlock) {
    	float ftx = line.tx, fty = line.ty;
    
    	seen.x0 = line.x0 + bStart*ftx+0.5;
    	seen.y0 = line.y0 + bStart*fty+0.5;
    	seen.tx = bEnd*ftx+0.5;
    	seen.ty = bEnd*fty+0.5;
    }
    
	EraseRgn(window->visRgn);
	DrawDebug();
	FinishDrawing( startHndl );
	return;

}


void DebugMe::Next()
{
	short	x1, y1;
	
	nTest++;
	
	block.bl.x0 = -50;
	block.bl.y0 =   0;
	x1 = MyUniqueRandom( block.bl.x0 );
	y1 = MyUniqueRandom( block.bl.y0 );
	block.bl.tx = x1-block.bl.x0;
	block.bl.ty = y1-block.bl.y0;

	block.br.x0 =  50;
	block.br.y0 =   0;
	x1 = MyUniqueRandom( block.br.x0 );
	y1 = MyUniqueRandom( block.br.y0 );
	block.br.tx = x1-block.br.x0;
	block.br.ty = y1-block.br.y0;

    line.x0 = MyRandom();
    line.y0 = MyRandom();
    x1 = MyUniqueRandom( line.x0 );
    y1 = MyUniqueRandom( line.y0 );
    line.tx = x1-line.x0;
    line.ty = y1-line.y0;
    
	Recall();
	return;
}

void DebugMe::Draw( ScrollsPoint topLeft, RgnHandle updateRgn, RgnHandle visRgn, short newFrame )
{
	topLeftSect = topLeft;
	
	if (nTest==0) return;
	
	DrawDebug();
}
