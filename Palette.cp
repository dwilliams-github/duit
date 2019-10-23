/*
 | Implement of class Palette
*/

#include "Palette.h"

Palette::Palette( short iconBase, short nIcon, Str255 *title, 
                  short preSelect, PixPatHandle theBackPpat )
{
	PalPals *thisPal;
	short	i;
	
	/* Save info */
	
	label = title;
	nItem = nIcon;
	backPpat = theBackPpat;
	if (preSelect >= 0 && preSelect < nIcon) 
		selected = preSelect;
	else
		selected = 0;
	
	/* Allocate space to hold pal items */
	
	pals = new PalPals[nIcon];
	
	/* Build them, one at a time */
	
	for( i = 0, thisPal = pals; i < nItem; i++, thisPal++ ) 
		thisPal->item = new PalItem( iconBase+i, (i==selected), backPpat );
}

Palette::~Palette()
{
	PalPals *thisPal;
	
	/* Loop over our pals, deleting them as we go */
	
	thisPal = pals + nItem - 1;
	do {
		delete thisPal->item;
	} while( thisPal-- > pals );
	
	/* Delete pal array */
	
	delete pals;
}

void Palette::Show( GrafPtr aPort, Point topLeft )
{
	PalPals *thisPal;
	Point	itemPlace;

	/* Save port and set outline rectangle, label place */
	
	port = aPort;
	outLine.top		= topLeft.v;
	outLine.bottom	= outLine.top + 22*nItem + 6;
	outLine.left	= topLeft.h;
	outLine.right	= outLine.left + 28;
	
	if (label) {
		labelPlace.v    = outLine.top + 12;
		labelPlace.h	= outLine.left + 14 - (StringWidth(*label)>>1);
		outLine.bottom += 12;
	}
	
	/* Install each item in turn */
	
	thisPal = pals + nItem - 1;
	itemPlace.h = outLine.left + 6;
	itemPlace.v = outLine.bottom - 22;
	do {
		thisPal->item->Show( port, itemPlace );
	} while( itemPlace.v -= 22, thisPal-- > pals );
	
	/* Draw it */
	
	Draw();
}

void Palette::Hide()
{
	PalPals *thisPal;

	port = 0;
	
	thisPal = pals + nItem - 1;
	do {
		thisPal->item->Hide();
	} while( thisPal-- > pals );
}

void Palette::Select( short toSelect )
{
	if (selected==toSelect) return;
	
	PalPals	*prev = pals + selected,
			*next = pals + toSelect;
			
	prev->item->DeSelect();
	next->item->Select();

	if (port) {
		prev->item->Draw();
		next->item->Draw();
	}
	
	selected = toSelect;
}

int Palette::CheckPress( Point where )
{
	if (!port) return(0);
	
	short	i = nItem - 1;
	PalPals *thisPal = pals + i;
	do {
		if (thisPal->item->PointIn( where )) {
			Select(i);
			return(1);
		}
	} while( i--, thisPal-- > pals );
	
	return(0);
}

void Palette::Draw()
{
	GrafPtr savePort;
	
	if (!port) return;
	
	/* Save current port and set selected one */
	
	GetPort( &savePort );
	SetPort( port );
	
	/* Draw outline rectangle */
	
	PenPat( &qd.black );
	FrameRect( &outLine );
	MoveTo( outLine.left-1, outLine.top+1 );
	LineTo( outLine.left-1, outLine.bottom );
	LineTo( outLine.right-2, outLine.bottom );
		
	if (label) {
	
		/* Stick down the label */
	
		TextSize(9);
		TextFont(2);
		TextFace(1);
		TextMode(0);
		
		MoveTo( labelPlace.h, labelPlace.v );
		DrawString( *label );
	}
	
	/* Draw each icon in turn */

	PalPals *thisPal = pals + nItem - 1;
	do {
		thisPal->item->Draw();
	} while( thisPal-- > pals );
}
