/*
 | Class definition for a scrollable window with optional borders
 |
 | (c) 1995, David C. Williams.  All rights reserved.
 |
 | History:
 |    May 11, 1996: Now implemented as a base class
*/

#ifndef __SCROLL__
#define __SCROLL__

#define SCROLL_YOU_GET_IT	0
#define SCROLL_I_GOT_IT		1
#define SCROLL_I_QUIT		2

/*
 | Define equivalent of Rect and Point, but with long values
*/

typedef struct {
	long	top, left, bottom, right;
} ScrollsRect;

typedef struct {
	long	h,v;
} ScrollsPoint;

/*
 | Window description
*/
typedef enum { scrollTopLeft, scrollCenter, scrollBottomRight } StartType;

typedef struct {
	short			leftMargin,
					topMargin,
					minWidth,
					minHeight;
	ScrollsRect		imageRect;			// Image size
	StartType 		startVert;			// Where does the vertical scroll bar start?
	StartType 		startHorz;			// Where does the horizontal scroll bar start?
	unsigned char 	*title;
} ScrollDesc;

#define		SCROLL_SMALLEST_SHORT  	(-32768)
#define		SCROLL_LARGEST_SHORT	32767

class Scroll {
	public:
	WindowPtr		window;			// The window 
	
	protected:
	Rect			maxWindow;		// imageRect + space for scroll bars, short concatenated
	ScrollsPoint	maxSize;		// imageRect + space for scroll bars
	int				leftMargin,		// Left margin (extra space at left of window)
					topMargin;		// Top margin (extra space at top of window)
	RgnHandle		tempRgn1,		// Temp region
					saveVisRgn;		// Save window visRgn, used by StartDraw* and FinishDrawing.
	ControlHandle	vScroll,		// Vertical scroll bar
					hScroll;		// Horizontal scroll bar
	short			vPower,hPower;	// Convertion of vMax/hMax to image coordinates
	short			vMax, hMax, 	// Scroll bar maximum value
					vStep, hStep;	// Scroll bar step size
	int				zoomed,			// Non-zero if the window has just been zoomed
					justConfig;		// Non-zero if the window has yet to be drawn once (in current configuration)
	ScrollsPoint	cornShowing;	// Current point at top-left of scrollable area
	ScrollsRect		imageRect;		// Rectangle of image size
	Rect			showRect,		// Rectangle of image showing, window coordinates
					scrollRect,		// Rectangle of scrollable image, window coordinates
					zoomSize;		// Size just before zoom
	RgnHandle		borderRgn;		// Region corresponding to the border
	
	UniversalProcPtr	scrollActionUp,		// Control action for down scroll arrow
						scrollActionDown;	// Control action for up scroll arrow
						
	public:
	Scroll( short windId );
	void Setup( ScrollDesc *desc );
	~Scroll();
	int Event( EventRecord *event );
	void FinishDrawing( void *valueFromStartDraw );
	int MouseInScroll();
	void BringToFront();
	void ResetImageSize( ScrollsRect imageRect, ScrollsPoint topLeft );
	
	protected:
	void WindowPress( Point where, short modifiers );
	void UpdateWindow( RgnHandle visRgn, short newFrame );
	void UpdateScroll( short moveNeeded );
	void HorzScroll( short hStep, Point *where );
	void VertScroll( short vStep, Point *where );
	void Activated();
	void CallDraw( RgnHandle drawRgn, short newFrame );
	void CallDrawBorder( RgnHandle drawRgn, short newFrame );
	void *StartDrawScroll();
	void *StartDrawBorder();

	void SetScrollMax( long height, long width );
	
	virtual void Draw( ScrollsPoint topLeft, RgnHandle drawRgn, RgnHandle visRgn, short newFrame );
	virtual void DrawBorder( RgnHandle drawRgn, short newFrame );
	virtual void Mouse( ScrollsPoint topLeft, Point where, short modifiers );
	virtual void MouseBorder( Point where, short modifiers );
	
	/* See 'WindowPress' for information on the following friends */
	
	friend pascal void sActionUp( ControlHandle control, short part );
	friend pascal void sActionDown( ControlHandle control, short part );
};

#endif
