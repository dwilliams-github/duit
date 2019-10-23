/*
 | Implement of class PalItem
 |
 | (c) David C. Williams
*/

#include "PalItem.h"

PalItem::PalItem( short iconRid, short aSelected, PixPatHandle theBackPpat )
{
	port = 0;
	selected = aSelected;
	backPPat = theBackPpat;
	
	icon = GetCIcon( iconRid );
}

void PalItem::Show( GrafPtr aPort, Point place )
{
	port = aPort;
	destRect.top 	= place.v;
	destRect.bottom = destRect.top + 16;
	destRect.left 	= place.h;
	destRect.right	= destRect.left + 16;
	
	selRect.top 	= destRect.top 		- 3;
	selRect.bottom	= destRect.bottom	+ 4;
	selRect.left	= destRect.left		- 4;
	selRect.right	= destRect.right	+ 3;
	
	Draw();
}
	
void PalItem::Hide()
{
	port = 0;
}

void PalItem::Draw()
{
	GrafPtr savePort;
	
	/* Don't do anything if we're hiding */
	
	if (!port) return;
	
	/* Save current port and set selected port */
	
	GetPort(&savePort);
	SetPort(port);
	
	/* Clear the draw area */
	
	FillCRect( &selRect, backPPat );
	
	/* If selected, draw a square around the icon */
	
	if (selected) {
		PenPat( &qd.black );
		PenSize(2,2);
		FrameRect( &selRect );
		PenSize(1,1);
	}
	
	/* Plot the icon and its shadow */
		
	PlotCIcon( &destRect, icon );
	
	FrameRect( &destRect );
	MoveTo( destRect.left-1, destRect.top+1 );
	LineTo( destRect.left-1, destRect.bottom );
	LineTo( destRect.right-2, destRect.bottom );
	
	/* Restore port and return */
	
	SetPort(savePort);
}


void PalItem::Select()
{
	selected = 1;
}

void PalItem::DeSelect()
{
	selected = 0;
}

int PalItem::PointIn( Point where )
{
	if (!port) return(0);
	return(PtInRect( where, &destRect ));
}

	
