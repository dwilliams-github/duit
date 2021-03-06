/*
 | Implementation of Scroll class
 |
 | (c) Copyright 1995  David C. Williams
 | All rights reserved
 |
 | History:
 | 		July 4, 1995: Version for MyGraph. Desc now has optional title. Fix bug
 |                    in GetZoomRect. Separate drawing/mouse in border, scroll area,
 |                    which allows setting of origin, visRgn, etc. Now Color!!
 |      May 11, 1996: Now a base class. Drawing is done by inheritance.
 |                    Does not set the quickdraw origin, so that pictures larger
 |                    than short integers can be drawn.
*/

#include "Scroll.h"

/*
 | -------------------------------------------
 | Classes to be replaced by inheritance
 | -------------------------------------------
*/

/*
 | Draw
 | Draw in the scrolled region
 |
 | GrafPort coordinates are set to window coordinates such that quickdraw
 | (0,0) is (0,0) of the scrollable portion of the window. The current scroll 
 | position is only determined by parameter "topLeft". 
*/
void Scroll::Draw( ScrollsPoint topLeft, RgnHandle region, RgnHandle visRgn, short newFrame )
{
	return;
}


/*
 | DrawBorder
 | Draw in the border regions around the scrollable region of the window
*/
void Scroll::DrawBorder( RgnHandle region, short newFrame )
{
	return;
}


/*
 | Mouse
 | This routine called when the mouse is pressed in the scrollable region.
 | It is passed the location in the global coordinates of the object drawn in the
 | scroll region.
*/
void Scroll::Mouse( ScrollsPoint topLeft, Point where, short modifiers )
{
	return;
}


/*
 | MouseBorder
 | This routine called with the mouse is pressed in the border are of the window.
*/
void Scroll::MouseBorder( Point where, short modifiers )
{
	return;
}


/*
 | Activated
 | This routine called when the window receives an activate eventScrollsPoint
*/
void Scroll::Activated()
{
	return;
}


/*
 | -------------------------------------------
 | Classes optionally replaced
 | -------------------------------------------
*/

/*
 | Scroll (constructor)
 | Make our scroll window
*/
Scroll::Scroll( short windId )
{
	static Rect	hRect = { 0,0,15,47 }, vRect = { 0,0, 47,15 };

	/* Get the main window from the resource */

	window = GetNewCWindow( windId, 0, (WindowPtr)(-1) );
	
	/* Get other things we need */
	
	tempRgn1   = NewRgn();
	borderRgn  = NewRgn();
	saveVisRgn = 0;
	
	vScroll = NewControl( window, &vRect, "\pfred", FALSE, 0, 0, vMax, scrollBarProc, 0 ); 
	hScroll = NewControl( window, &hRect, "\pfred", FALSE, 0, 0, hMax, scrollBarProc, 0 ); 
	                             
	cornShowing.h = 0;
	cornShowing.v = 0;
	
	zoomed = 0;

	scrollActionDown = scrollActionUp = 0;
}	

/*
 | Setup
 | Set scroll parameters.
 | This really must be called in the inherited constructor, otherwise strange things
 | might happen, one of which is that the window will never appear.
*/
void Scroll::Setup( ScrollDesc *desc )
{
	short	newWidth, newHeight;
	long	vhMax;
		
	/* Calculate max and min window sizes. Don't forget to account for the scroll bars */
	
	imageRect = desc->imageRect;
	
	leftMargin = desc->leftMargin;
	topMargin  = desc->topMargin;
	
	maxSize.v  = imageRect.bottom - imageRect.top  + 17 + topMargin;
	maxSize.h  = imageRect.right  - imageRect.left + 17 + leftMargin;

	maxWindow.top 	 = desc->minHeight + topMargin;
	maxWindow.left	 = desc->minWidth  + leftMargin;
	maxWindow.bottom = maxSize.h > 32767 ? 32767 : (short)maxSize.v;
	maxWindow.right  = maxSize.v > 32767 ? 32767 : (short)maxSize.h;
	
	/* If requested, set window title */
	
	if (desc->title) SetWTitle( window, desc->title );
	
	/* Is it too big for our image? If so, shrink it. */
	
	newWidth = window->portRect.right;
	if (newWidth > maxWindow.right) newWidth = maxWindow.right;
	
	newHeight = window->portRect.bottom;
	if (newHeight > maxWindow.bottom) newHeight = maxWindow.bottom;
	
	SizeWindow( window, newWidth, newHeight, FALSE );
	
	/* Set initial position of scroll bars */
	
	SetScrollMax( imageRect.bottom - imageRect.top  - newHeight + 16 + topMargin,
	              imageRect.right  - imageRect.left - newWidth  + 16 + leftMargin  );

	switch(desc->startVert) {
		default:
		case scrollTopLeft:		cornShowing.h = 0; 			break;
		case scrollCenter:  	cornShowing.h = hMax>>1; 	break;
		case scrollBottomRight:	cornShowing.h = hMax; 		break;
	}

	switch(desc->startVert) {
		default:
		case scrollTopLeft:		cornShowing.v = 0; 			break;
		case scrollCenter:  	cornShowing.v = vMax>>1; 	break;
		case scrollBottomRight:	cornShowing.v = vMax; 		break;
	}

	/* Set origin to left and right margins */
	
	SetPort( window );
	SetOrigin( -leftMargin, -topMargin ); 
	
	UpdateScroll( 1 );
	ShowControl( vScroll );
	ShowControl( hScroll );
	
	/* Expose the window */
	
	justConfig = 1;
		
	SelectWindow( window );
	ShowWindow( window );
	return;
}

/*
 | ~Scroll
 | (Destructor)
 |
 | Note that DisposeWindow automatically erase all associated controls.
*/
Scroll::~Scroll()
{
	DisposeWindow( window );
	return;
}


/*
 | ResetImageSize
 | Change size of image
*/
void Scroll::ResetImageSize( ScrollsRect newImageRect, ScrollsPoint topLeft )
{
	imageRect = newImageRect;
	maxSize.v = imageRect.bottom - imageRect.top  + 17 + topMargin;
	maxSize.h = imageRect.right  - imageRect.left + 17 + leftMargin;
	
	// Validate and update the image origin	
	
	cornShowing.h = topLeft.h - imageRect.left;
	if (cornShowing.h < 0) cornShowing.h = 0;
	cornShowing.v = topLeft.v - imageRect.top;
	if (cornShowing.v < 0) cornShowing.v = 0;
	
	// Recalculate scroll bars and associated variables

	UpdateScroll( 0 );
	
	// Force update of window contents
	
	justConfig = 1;
	SetPort( window );
	InvalRect( &scrollRect );
	return;
}


/*
 | MouseInScroll
 | Return true if the mouse is currently in the scroll region
*/
int Scroll::MouseInScroll()
{
	Point	mouse;
	GrafPtr savePort;
	
	GetPort( &savePort );
	SetPort( window );
	GetMouse( &mouse );
	SetPort( savePort );
		
	return(PtInRect( mouse, &scrollRect ));
}

/*
 | Calculate zoom window bounds
*/
static void GetZoomRect( short hSize, short vSize, Rect *currRect, Rect *newRect )
{
	/* Start with screen size (minus four bits AND MENU BAR AND TITLE BAR!!!) */

 	*newRect = qd.screenBits.bounds;
 	InsetRect( newRect, 4, 4 );
 	newRect->top += GetMBarHeight() + 18;
 	
 	/* Adjust the bounds so that it does not exceed the image size */

	if (newRect->right - newRect->left > hSize) {	
	
		/* Too large: shrink it to size, and preserve left side if possible */
		
		if (currRect->left + hSize < newRect->right) {
			newRect->left = currRect->left;
			newRect->right = currRect->left + hSize - 1;
		}
		else 
			newRect->left = newRect->right - hSize + 1;
	}				

	if (newRect->bottom - newRect->top > vSize) {	
	
		/* Too large: shrink it to size, and preserve left side if possible */
		
		if (currRect->top + vSize < newRect->bottom) {
			newRect->top = currRect->top;
			newRect->bottom = currRect->top + vSize - 1;
		}
		else 
			newRect->top = newRect->bottom - vSize + 1;
	}				
}

/*
 | ScrollEvent.
 | See if the next event should do something to our scrolling window.
 |
 | Some warning: the usual action when a mouse is pressed in an inactive window
 | is to make that window active. That's not what's done in this routine. Instead,
 | it is up to the application to know if the scroll window is active and to intercept
 | (i.e. don't call this routine) the mouse down events, and call BringToFront if a mouse
 | press is in a inactive Scroll window.
*/
int Scroll::Event( EventRecord *event )
{
	WindowPtr 		whichWindow;
	RgnHandle		saveRgn;
	int 			whereCode; 
	Rect     		moveBound,
					newChar 	= {35,40,60,70};
	GrafPtr			wPort,
					savePort;	
	short			scrollState;
	
	union {
		long	l;
		short	w[2];
	} hw;
	
	switch(event->what) {
		case mouseDown:	
	
			/* Mouse was pressed - find out where */
			
			whereCode = FindWindow( event->where, &whichWindow );
			if (whichWindow != window) return(SCROLL_YOU_GET_IT);
			
			switch (whereCode) {
				case inGoAway:
				
					/* Mouse in "go away" box: go away */
				
					return( TrackGoAway( whichWindow, event->where ) ? SCROLL_I_QUIT : SCROLL_I_GOT_IT );
			 	case inDrag:
			 	
			 		/* Mouse press in title bar: move the window about */
			 	
			 		moveBound = qd.screenBits.bounds;
			 		InsetRect( &moveBound, 4, 4 );
			 		DragWindow( whichWindow, event->where, &moveBound );
			 		return(SCROLL_I_GOT_IT);
			 	case inGrow:
			 	
			 		/* 
			 		 | Mouse press in size box: resize the window, adjusting
			 		 | the scroll bars as necessary.
			 		*/
			 	
					GetPort( &savePort );
					SetPort( window );
			 		hw.l = GrowWindow( whichWindow, event->where, &maxWindow );
			 		SizeWindow( whichWindow, hw.w[1], hw.w[0], FALSE );
					UpdateScroll( 1 );
			 		UpdateWindow( window->visRgn, 1 );
			 		ValidRgn( ((WindowRecord *)window)->updateRgn );
					SetPort( savePort );
					zoomed = 0;
					return(SCROLL_I_GOT_IT);
				case inZoomIn:
				case inZoomOut:
					GetPort( &savePort );
					SetPort( window );
					
					ShowHide( window, FALSE );

					if (zoomed) {
						zoomed = 0;
					
						/* Zoom in: restore window size and position to saved values */
						
						SizeWindow( window, zoomSize.right-zoomSize.left,
										    zoomSize.bottom-zoomSize.top, FALSE );
						MoveWindow( window, zoomSize.left, zoomSize.top, TRUE );
					}
					else {
						zoomed = 1;
						
						/*
						 | In ZoomOut box: zoom away!
						 | We could use the toolbox routine ZoomWindow, but it doesn't limit
						 | the grown window to the image size. Instead, do things manually.
						*/
						
						zoomSize = window->portRect;
						LocalToGlobal( &topLeft(zoomSize) );
						LocalToGlobal( &botRight(zoomSize) );
						
						GetZoomRect( maxWindow.right, maxWindow.bottom, &zoomSize, &moveBound );
						
						SizeWindow( window, moveBound.right-moveBound.left,
										    moveBound.bottom-moveBound.top, FALSE );
						MoveWindow( window, moveBound.left, moveBound.top, TRUE );
					}
					
					UpdateScroll( 1 );
					ShowHide( window, TRUE );
			 		UpdateWindow( window->visRgn, 1 );
					SetPort( savePort );
					return(SCROLL_I_GOT_IT);
			 	case inContent:
			 	
			 		/*
			 		 | Mouse press in window content: deal with it
			 		*/
			 	
					GetPort( &savePort );
					SetPort( window );
					GlobalToLocal( &event->where );
			 		WindowPress( event->where, event->modifiers );
					SetPort( savePort );
					return(SCROLL_I_GOT_IT);
			 }
	 		break;
	 	case updateEvt:
	 	
	 		/* Window update: is it for our window? */
	 		
	 		if ((WindowPtr)event->message != window ) return(SCROLL_YOU_GET_IT);
	 		
	 		/* Yup: update the controls and the image */
	 	
			GetPort( &savePort );
			SetPort( window );
			
			saveRgn = NewRgn();
			CopyRgn( window->visRgn, saveRgn );
			
			BeginUpdate( window );
			UpdtControl( window, window->visRgn );
			DrawGrowIcon( window );
			UpdateWindow( saveRgn, justConfig );
			justConfig = 0;
			EndUpdate( window );	
			
			DisposeRgn( saveRgn );
			SetPort( savePort );
			return(SCROLL_I_GOT_IT);
		case activateEvt:
		
			/* Activate/deactive: adjust controls as required */
		
			scrollState = (event->modifiers&0x001) ? 0 : 255;
			
			HiliteControl( vScroll, scrollState );
			HiliteControl( hScroll, scrollState );
			DrawGrowIcon( window );
			
			/* Tell someone that the visRgn may have changed */
			
			Activated();

			return(SCROLL_YOU_GET_IT);
		default:;
 	}
 	
 	/* Well, we don't want this mouse click! */
 	
 	return(SCROLL_YOU_GET_IT);
}


/*
 | HorzScroll.
 | Scroll the image in the window by a step size.
 | 
 | It is assumed that the graf port has already been set.
*/
void Scroll::HorzScroll( short hStep, Point *where )
{
	short value, newValue, actualStep;
	
	/* Get new control value */
	
	value = GetCtlValue( hScroll );
	
	if (where) {
	
		/* 'where' specified: track the control */
		
		TrackControl( hScroll, *where, 0 );
		newValue = GetCtlValue( hScroll );
	}
	else {
	
		/* Jump the requested amount */	
		
		newValue = value + hStep;
		if (newValue < 0)
			newValue = 0;
		else if (newValue > hMax)
			newValue = hMax;
			
		/* Reset the control */

		SetCtlValue( hScroll, newValue );
	}
	
	actualStep = (newValue - value) << hPower;
	if (actualStep == 0) return;

	/* Scroll over the existing image */
	
	ScrollRect( &scrollRect, -actualStep, 0, tempRgn1 );

	/* Redefine cornShowing */
	
	cornShowing.h += actualStep;
	
	/* Draw exposed area */
	
	CallDraw( tempRgn1, 1 );
}


/*
 | VertScroll.
 | Scroll the image in the window by a step size.
 | 
 | It is assumed that the graf port has already been set.
*/
void Scroll::VertScroll( short vStep, Point *where )
{
	short value, newValue, actualStep;
	
	/* Get new control value */
	
	value = GetCtlValue( vScroll );
	
	if (where) {
	
		/* 'where' specified: track the control */
		
		TrackControl( vScroll, *where, 0 );
		newValue = GetCtlValue( vScroll );
	}
	else {
		
		/* Jump the requested amount */	
		
		newValue = value + vStep;
		if (newValue < 0)
			newValue = 0;
		else if (newValue > vMax)
			newValue = vMax;
			
		/* Reset the control */

		SetCtlValue( vScroll, newValue );
	}

	actualStep = (newValue - value) << vPower;
	if (actualStep == 0) return;

	/* Scroll over the existing image */
	
	ScrollRect( &scrollRect, 0, -(short)actualStep, tempRgn1 );

	/* Redefine cornShowing */
	
	cornShowing.v += actualStep;
	
	/* Draw exposed area */
	
	CallDraw( tempRgn1, 1 );
}


/*
 | WindowPress
 | Deal with a mouse press in the window
 |
 | It is assumed that the graf port has already been set.
 |
 | Notes:
 |    * Alas, member functions can only be used as Mac Toolbox call back routines
 |      if they're declared as 'static.' Unfortunately, static member functions violate
 |      ANSI standards. However, if one tries to write non-member functions as
 |      scroll bar tracking routines, you cannot call the private member functions
 |      to redraw the image as it is scrolling. What to do?? Use 'friend' functions.
 |
 |      What are 'friend's? One of the screwy c++ tricks you thought you would
 |      never need to use. Hah!!
*/
Scroll *aActionScroll;

pascal void sActionUp( ControlHandle control, short part )
{
	if (part == inUpButton) {
		if (*control == *aActionScroll->hScroll) 
			aActionScroll->HorzScroll( -16, 0 );
		else
			aActionScroll->VertScroll( -16, 0 );
	}
}
		
pascal void sActionDown( ControlHandle control, short part )
{
	if (part == inDownButton) {
		if (*control == *aActionScroll->hScroll) 
			aActionScroll->HorzScroll( +16, 0 );
		else
			aActionScroll->VertScroll( +16, 0 );
	}
}
		


void Scroll::WindowPress( Point where, short modifiers )
{
	ControlHandle 	control;
	int				part,
					value,
					horz;

	/* Was this mouse press in a control (i.e. one of the scroll bars) */

	part = FindControl( where, window, &control );
	if (part == 0) {
	
		/* Not in a control: is it in the scrollable region? */
		
		if (PtInRect( where, &scrollRect )) {
		
			/* Yup. Offset accordingly and call mouse routine */
			
			ScrollsPoint	topLeft;
				
			topLeft.h = cornShowing.h + imageRect.left;
			topLeft.v = cornShowing.v + imageRect.top;

			Mouse( topLeft, where, modifiers );
		}
		else {
		
			/* Nope, call border mouse routine, if there is one */
			
			MouseBorder( where, modifiers );
		}
		return;
	}
	
	/* Which scroll bar? */
	
	horz = (*control == *hScroll) ? 1 : 0; 
	
	switch (part) {
		case inThumb:
			if (horz) 
				HorzScroll( 0, &where );
			else
				VertScroll( 0, &where );
			break;
		case inUpButton:
			aActionScroll = this;
			if (scrollActionUp == 0) 
				scrollActionUp = NewControlActionProc( (ProcPtr)sActionUp );
			TrackControl( control, where, scrollActionUp );	
			break;
		case inDownButton:
			aActionScroll = this;
			if (scrollActionDown == 0) 
				scrollActionDown = NewControlActionProc( (ProcPtr)sActionDown );
			TrackControl( control, where, scrollActionDown );
			break;
		case inPageUp:
			if (horz) 
				HorzScroll( -hStep, 0 );
			else
				VertScroll( -vStep, 0 );
			break;
		case inPageDown:
			if (horz) 
				HorzScroll( +hStep, 0 );
			else
				VertScroll( +vStep, 0 );
			break;
	}
}

/*
 | UpdateWindow.
 | Redraw the window according to the current visRgn, cutting appropriately before calling
 | the two user draw routines.
 |
 | If the visRgn is in only the scroll region, it is more efficient to call CallDrawer directly.
*/
void Scroll::UpdateWindow( RgnHandle visRgn, short newFrame )
{	
	/* Does the visRgn include the scroll region? */
	
	if (RectInRgn( &scrollRect, visRgn )) {
	
		/* 
		 | Yup. Define temporary region that includes the intersection of visRgn and
		 | of the scroll region.
		*/
	
		RectRgn( tempRgn1, &scrollRect );
		SectRgn( tempRgn1, visRgn, tempRgn1 );
		
		/* Call drawer */
		
		CallDraw( tempRgn1, newFrame );
	}
	
	/* Build intersection of border and visRgn */
	
	SectRgn( visRgn, borderRgn, tempRgn1 );
	
	/* If anything is left, call user border drawing routine */
	
	if (!EmptyRgn(tempRgn1)) CallDrawBorder( tempRgn1, newFrame );
}


/*
 | BringToFront
 | Bring the scroll window to the front of the desktop
*/
void Scroll::BringToFront()
{
	SelectWindow( window );
}

/*
 | CallDraw
 | Set origin and call user draw routine. The draw region remains unchanged and is assumed
 | to be set accordingly (for example, to NOT include the border).
*/
void Scroll::CallDraw( RgnHandle drawRgn, short newFrame )
{	
	RgnHandle	visRgn;
	ScrollsPoint topLeft;
	
	/* Temporarily replace windows visRgn with that belonging to scrollable area */
	
	visRgn = window->visRgn;
	window->visRgn = drawRgn;
		
	/* Set port */
	
	SetPort( window );
		
	/* Call user draw routine */
	
	topLeft.h = cornShowing.h + imageRect.left;
	topLeft.v = cornShowing.v + imageRect.top;
	
	Draw( topLeft, drawRgn, visRgn, newFrame );
	
	/* Replace visRgn */
	
	window->visRgn = visRgn;
}


/*
 | CallDrawBorder
 | Set origin and call user border draw routine. The draw region remains unchanged and is assumed
 | to be set accordingly (for example, to NOT include the scrollable region).
*/
void Scroll::CallDrawBorder( RgnHandle drawRgn, short newFrame )
{	
	RgnHandle saveRgn;
	
	/* Temporarily replace window's visRgn with real visRgn */
	
	saveRgn = window->visRgn;
	window->visRgn = drawRgn;
	
	/* Set port and call user draw routine */
	
	SetPort( window );
	DrawBorder( drawRgn, newFrame );
	
	/* Put things back like they were */
	
	window->visRgn = saveRgn;
}

/*
 | StartDrawScroll
 | Call by application to prepare drawing in scrolled region when doing so outside of Scroll's
 | control (i.e. not in user drawing routine). Sets the Graf port, origin, and visRgn.
 | Do not call this routine when you drawing routine is called by Scroll.
 |
 | Returns a handle that must be passed to FinishDrawing when the application is finished drawing.
*/
void *Scroll::StartDrawScroll()
{
	GrafPtr	savePort;
	
	/* Get current port, whatever it is */
	
	GetPort( &savePort );
	
	/* Save current visRgn, if it hasn't been already */
	
	if (saveVisRgn == 0) saveVisRgn = window->visRgn;
	
	/* Build clip region that includes just the scrollable area */
	
	RectRgn( tempRgn1, &scrollRect );
	SectRgn( window->visRgn, tempRgn1, tempRgn1 );
	window->visRgn = tempRgn1;
	
	/* Set port */
		
	SetPort( window );
	return( savePort );	
}

/*
 | StartDrawScroll
 | Call by application to prepare drawing in border region when doing so outside of Scroll's
 | control (i.e. not in routine Draw/DrawControl). Sets the Graf port, origin, and visRgn.
 | Do not call this routine when you drawing routine is called by Scroll.
 |
 | Returns a handle that must be passed to FinishDrawing when the application is finished drawing.
*/
void *Scroll::StartDrawBorder()
{
	GrafPtr	savePort;
	
	/* Get current port, whatever it is */
	
	GetPort( &savePort );
	
	/* Save current visRgn, if it hasn't been already */
	
	if (saveVisRgn == 0) saveVisRgn = window->visRgn;
	
	/* Build clip region that includes just the border area */
	
	UnionRgn( window->visRgn, borderRgn, tempRgn1 );
	window->visRgn = tempRgn1;
	
	/* Set port */
		
	SetPort( window );
	
	return( savePort );	
}

/*
 | FinishDrawing.
 | Called by the application when drawing is completed. 
*/
void Scroll::FinishDrawing( void *startHandle )
{
	GrafPtr savePort = (GrafPtr)startHandle;
	
	/* Replace visRgn */
	
	window->visRgn = saveVisRgn;
	saveVisRgn = 0;
	
	/* Return port */
	
	SetPort( savePort );
}	
	
	
	
/*
 | SetScrollMax
 | Set maximum scroll values, and the scroll control powers
*/
void Scroll::SetScrollMax( long height, long width )
{
	vPower = 0;
	while( height > 32767 ) {
		vPower++;
		height >>= 1;
	}
	vMax = height;
	
	hPower = 0;
	while( width > 32767 ) {
		hPower++;
		width >>= 1;
	}
	hMax = width;
}


/*
 | UpdateScroll
 | Readjust and redraw the scroll bars, fill showRect and v/hMax
*/
void Scroll::UpdateScroll( short moveNeeded )
{
	Rect	borderRect;

	/* Rebuild showRect to new drawable region */
	
	showRect.top	= -topMargin;
	showRect.left	= -leftMargin;
	showRect.right	= window->portRect.right  - 16;
	showRect.bottom	= window->portRect.bottom - 16;

	scrollRect.top 		= 0;
	scrollRect.left 	= 0;
	scrollRect.bottom 	= showRect.bottom;
	scrollRect.right 	= showRect.right;
	
	/* Build border region */
	
	borderRect = showRect;
	borderRect.right = 0;
	RectRgn( borderRgn, &borderRect );
	
	borderRect.right = showRect.right;
	borderRect.bottom = 0;
	RectRgn( tempRgn1, &borderRect );
	
	UnionRgn( tempRgn1, borderRgn, borderRgn );
	
	/* What are our new control max values? */
	
	SetScrollMax( imageRect.bottom - imageRect.top  - showRect.bottom,
	              imageRect.right  - imageRect.left - showRect.right   );
	
	SetCtlMax( vScroll, vMax );
	SetCtlMax( hScroll, hMax );
	
	/* Reset cornShowing if necessary */
	
	if (cornShowing.h > (hMax<<hPower)) cornShowing.h = (hMax<<hPower);
	if (cornShowing.v > (vMax<<hPower)) cornShowing.v = (vMax<<vPower);
			
	/* Reset control values to new max values and new cornShowing */
	
	SetCtlValue( vScroll, cornShowing.v>>vPower );
	SetCtlValue( hScroll, cornShowing.h>>hPower );
	
	/* Redefine new page step sizes */
	
	vStep = window->portRect.bottom - 32;
	hStep = window->portRect.right - 32;
	
	if (moveNeeded) {
	
		/* Reposition and resize controls to fit the new window size */
		
		MoveControl( vScroll, window->portRect.right-15, -1 );
		MoveControl( hScroll, -1, window->portRect.bottom-15 );
	
		SizeControl( vScroll, 16, window->portRect.bottom-13 );
		SizeControl( hScroll, window->portRect.right-13, 16 );
			
		/* Redraw the grow icon */
	
		DrawGrowIcon( window );	
	}
}
	


