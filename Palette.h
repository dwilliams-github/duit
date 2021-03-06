/*
 | Definition of Palette class
*/

#ifndef __PALETTE__
#define __PALETTE__

#include "PalItem.h"

typedef struct {
	PalItem	*item;
} PalPals;

class Palette {
	public:
	short	selected;

	private:
	GrafPtr	port;
	PalPals	*pals;
	short	nItem;
	Rect	outLine;
	Str255	*label;
	Point	labelPlace;
	PixPatHandle backPpat;

	public:
	Palette( short iconRase, short nIcon, Str255 *title, short selected, PixPatHandle backPpat );
	~Palette();
	
	void Show( GrafPtr port, Point topLeft );
	void Hide();
	
	void Select( short selected );
	int	CheckPress( Point where );
	void Draw();
};

#endif
