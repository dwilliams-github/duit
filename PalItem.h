/*
 | Definition of class PalItem
*/

#ifndef __PALITEM__
#define __PALITEM__

class PalItem {
	private:
	CIconHandle	icon;
	GrafPtr	port;
	Rect	destRect,
			selRect;
	short	selected;
	PixPatHandle backPPat;
	
	public:
	PalItem( short iconRid, short selected, PixPatHandle backPpat );
	void Show( GrafPtr port, Point place );
	void Hide();
	void Draw();
	void Select();
	void DeSelect();
	int PointIn( Point where );
};




#endif 
