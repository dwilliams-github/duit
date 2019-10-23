/*
 | Duit
 |
 | Doom WAD editor
 |
 | (c) Copyright 1996  David C. Williams
 | All rights reserved
*/

#include "DuitDoc.h"
#include "List.h"

#define		MAIN_WIND	128
#define		ABOUT_WIND	129

#define		INFO_STRS	128

#define		APPLE_MENU	128
#define		APPLE_ABOUT	1

#define		FILE_MENU	129
#define		FILE_NEW	1
#define		FILE_OPEN	2
#define		FILE_SAVE	3
#define		FILE_SAVEAS 4
#define		FILE_CLOSE	5
#define		FILE_QUIT	7

#define		EDIT_MENU	130


typedef struct {
	MenuHandle	appleMenu,
				fileMenu, 
				editMenu;
	List		*docs;
	short		docNum;
} ProgInfo;

void KillDocument( ProgInfo *prog, DuitDoc *doc );
void NewDocument( ProgInfo *prog, DuitDoc *doc );
void NewFrontDocument( ProgInfo *prog, DuitDoc *doc );

void NewFile( ProgInfo *prog );
void OldFile( ProgInfo *prog );

int	DoMenu( Point where, ProgInfo *prog );
void ShowAbout();

void MakeMenus( ProgInfo *prog );
void EventLoop( ProgInfo *prog );

void main()
{	
	ProgInfo prog;
		
	/* Initialize all the system stuff */

	InitGraf( &qd.thePort );
	InitFonts();
	InitWindows();
	InitMenus();
	TEInit();
	InitDialogs( 0L );
	
	SetCursor( &qd.arrow );
	
	/* Initialize program info */
	
	prog.docs = new List;
	
	/* Make our menus */

	MakeMenus( &prog );						
	
	/* Wait for event */

	EventLoop( &prog ); 
	
	/* Finish up */
		
	ExitToShell();
}

void KillDocument( ProgInfo *prog, DuitDoc *doc )
{
	prog->docs->Delete( doc );
	delete doc;

	if (prog->docs->front == 0) {
		DisableItem( prog->fileMenu, FILE_SAVE );
		DisableItem( prog->fileMenu, FILE_SAVEAS );
		DisableItem( prog->fileMenu, FILE_CLOSE );
	}
}


void NewDocument( ProgInfo *prog, DuitDoc *doc )
{
	prog->docs->NewFront( doc );
	
	EnableItem( prog->fileMenu, FILE_SAVE );
	EnableItem( prog->fileMenu, FILE_SAVEAS );
	EnableItem( prog->fileMenu, FILE_CLOSE );
	
	return;
}

void NewFrontDocument( ProgInfo *prog, DuitDoc *doc )
{
	prog->docs->BringToTop( doc );
	doc->BringToFront();
} 

typedef struct {
	WindowPtr	hitWindow;
	DuitDoc		*docHit;
	List		*list;
	EventRecord *event;
} AskThisDocParm;

static int AskThisDoc( void *thing, void *arg )
{
	AskThisDocParm	*myParm = (AskThisDocParm *)arg;
	DuitDoc			*thisDoc = (DuitDoc *)thing;
	
	if (thisDoc==myParm->list->front) return(0);
	
	if (thisDoc->CheckHit( myParm->hitWindow )) {
		myParm->docHit = thisDoc;
		return(1);
	}
	return(0);
}
	
static int AskThisDoc2( void *thing, void *arg )
{
	AskThisDocParm	*myParm = (AskThisDocParm *)arg;
	DuitDoc			*thisDoc = (DuitDoc *)thing;
	
	if (thisDoc==myParm->list->front) return(0);
	
	if (thisDoc->Event( myParm->event ) != DOC_YOU_GET_IT) {
		myParm->docHit = thisDoc;
		return(1);
	}
	return(0);
}
	

void EventLoop( ProgInfo *prog )
{
	EventRecord		doneEvent;
	WindowPtr 		whichWindow;
	int 			whereCode; 
	Rect     		newChar 	= {35,40,60,70};
	GrafPtr			wPort,
					savePort;	
	char			theChar;
	int				scrollAnswer;
	int				mouseIsIn;
	DialogPtr		whichDialog;
	short			itemHit;
	unsigned long	docAnswer;
	AskThisDocParm	askParm;
	
	union {
		long	l;
		short	w[2];
	} hw;
	
	int DoMenu( Point where, ProgInfo *prog );
	
	/* Start fresh */

	FlushEvents( everyEvent, 0 );
		
	/* Wait for "goaway" event or quit selection in menu */
	
	for (;;) {
	
		while (!GetNextEvent( everyEvent, &doneEvent )) {
			SystemTask();
			if (prog->docs->front) {
				DuitDoc	*front = (DuitDoc *)prog->docs->front;
				front->CallMeALot();
			}
		}			
	
		/* Something happened: is it (Modeless) dialog event? */
		
		if (IsDialogEvent( &doneEvent )) {
		
			/* Yeah... but first take care of command key presses */
			
			// if ((doneEvent->what == keyDown) && (doneEvent->modifiers & cmdKey) ) 
			
			/* Decide if we need to do anything and if so, which dialog is involved */
		
			if (DialogSelect( &doneEvent, &whichDialog, &itemHit ) == 0) continue;
		
			if (prog->docs->front) {
				DuitDoc	*front = (DuitDoc *)prog->docs->front;
				docAnswer = front->DialogEvent( &doneEvent, whichDialog, itemHit );
				if (docAnswer==DOC_I_QUIT) 
					KillDocument( prog, front );
			}
			continue;
		}
		
		/* Nope: ask our front document if it is interested */
		
		if (prog->docs->front) {
			DuitDoc	*front = (DuitDoc *)prog->docs->front;
			int neverMind;
			
			switch(front->Event( &doneEvent )) {
				case DOC_I_QUIT:
					KillDocument( prog, front );
				case DOC_I_GOT_IT:
					neverMind = 1;
					break;
				case DOC_YOU_GET_IT:
					neverMind = 0;
			}
			if (neverMind) continue;
		}
			
		/* Still not? Check out global (application level) events */
	
		switch(doneEvent.what) {
			case mouseDown:	
		
				/* Mouse was pressed - find out where */
				
				whereCode = FindWindow( doneEvent.where, &whichWindow );
				switch (whereCode) {
				 	case inSysWindow:
				 		SystemClick( &doneEvent, whichWindow );
				 		break;
			 		case inMenuBar:
				 		if (!DoMenu( doneEvent.where, prog )) return;
				 		break;
				 	default:
				 	
				 		/* Check other (non-active) documents */
				 	
						askParm.hitWindow	= whichWindow;
						askParm.list 		= prog->docs;
						askParm.event 		= &doneEvent;
						if (prog->docs->Traverse( AskThisDoc, (void *)&askParm, 0 )) {
							askParm.docHit->BringToFront();
							prog->docs->BringToTop( askParm.docHit );
						}
				}
		 		break;
		 	case keyDown:
		 		
		 		/* Key pressed: which one? */
		 		
		 		theChar = doneEvent.message & charCodeMask;
		 		
		 		break;
			case activateEvt:
			
				/* Activate: set cursor */
				
				SetCursor( &qd.arrow );
			default:;
			
				/* 
				 | Well, something happened!! Ask around (could be update event
				 | for an inactive window 
				*/
				
				askParm.list 		= prog->docs;
				askParm.event 		= &doneEvent;
				prog->docs->Traverse( AskThisDoc2, (void *)&askParm, 0 );
	 	}
	}
}


void NewFile( ProgInfo *prog )
{
	DuitDoc *newDoc = new DuitDoc( 1, "\pUntitled", 0, ++prog->docNum );
	if (newDoc) NewDocument( prog, newDoc );
		
	return;
}


void OldFile( ProgInfo *prog )
{
	static Point	where = { 100, 100 };
//	static OSType	typeList[] = { 'CODE', 'APPL' };
	SFReply	reply;
	DuitDoc			*newDoc;
	
	SFGetFile( where, 0, 0, -1, 0, 0, &reply );
	if (reply.good) {
		newDoc = new DuitDoc( 0, reply.fName, reply.vRefNum, ++prog->docNum );
		if (newDoc->ioError) {
			delete newDoc;
			newDoc = 0;
		}
	}
	else
		newDoc = 0;
		
	if (newDoc) NewDocument( prog, newDoc );
	return;
}


int	DoMenu( Point where, ProgInfo *prog )
/*
 | Handle a menu event. 
 |   Arguments: None
 |   Return value: 0 if "quit" selected, 1 otherwise
*/
{
	union {
		short		byWord[2];
		long		byLong;
	} menuId;
	
	Str255			appleName;
	StringHandle	menuStr;
	DuitDoc			*front = (DuitDoc *)prog->docs->front;
	
	void ShowAbout();
		
	/* Get menu selection */
	
	menuId.byLong = MenuSelect( where );
	
	if (menuId.byWord[1] != 0) {
	
	 	/* Valid menu selection */
	 	
	 	switch(menuId.byWord[0]) {
	 	
	 		case APPLE_MENU:					/* In Apple Menu: display 'about' info */
	 			switch(menuId.byWord[1]) {
	 				case APPLE_ABOUT:
		 			 	ShowAbout(); 	
		 				break;
		 			default:
		 				GetItem( prog->appleMenu, menuId.byWord[1], appleName );
		 				OpenDeskAcc( appleName );
		 		}
		 		break;
			case FILE_MENU:					    /* In File Menu */
	 			switch(menuId.byWord[1]) {
	 				case FILE_NEW:
	 					NewFile( prog );
	 					break;
	 				case FILE_OPEN:
	 					OldFile( prog );
	 					break;
	 				case FILE_CLOSE:
	 					KillDocument( prog, front );
	 					break;
	 				case FILE_QUIT:
	 					HiliteMenu(0);
	 					return(0);
	 					break;
	 			}
	 			break;
	 	}
	}
	
	HiliteMenu(0);
	return(1);
}

void MakeMenus( ProgInfo *prog )
{
	/* Make Apple menu, add standard DA menu items */
	
	prog->appleMenu = GetMenu( APPLE_MENU );
	AddResMenu( prog->appleMenu, 'DRVR' );
	InsertMenu( prog->appleMenu, 0 );
	
	/* Add our file menu */
	
	prog->fileMenu = GetMenu( FILE_MENU );
	InsertMenu( prog->fileMenu, 0 );
	
	/* Add the default edit menu */
	
	prog->editMenu = GetMenu( EDIT_MENU );
	InsertMenu( prog->editMenu, 0 );
			
	/* Draw the menus */
	
	DrawMenuBar();
	
	return;
}


void ShowAbout()
/*
 | Make a modal window that shows information about the program (to be called
 | in the "about..." entry in the apple menu). Window is released with a 
 | mouse click.
*/
{
	WindowPtr		aboutWindow,
					whichWindow;
	EventRecord 	aboutEvent;
	int				where, hGlobal, width;
	
	Str255			infoLine;
	int				line,
					lineh[5] = { 15, 35, 45, 65, 77 };
					
	WindowPtr 		mainWindow;

	/* Get screen size from window manager port */
	
	GetWMgrPort( &mainWindow );

	/* Make info Window */
	
	aboutWindow = GetNewWindow( ABOUT_WIND, 0L, (WindowPtr)(-1) );
	
	/* Move it into the center of the main window */
	
	hGlobal = ( (mainWindow->portRect.right - mainWindow->portRect.left)  -
	            (aboutWindow->portRect.right - aboutWindow->portRect.left)    ) >> 1;
	MoveWindow( aboutWindow, hGlobal, 100, TRUE );
	
	/* Draw information */

	SetPort( aboutWindow );	
	TextSize( 9 );
	
	width = aboutWindow->portRect.right - aboutWindow->portRect.left;
	
	for(line=0;line<5;line++) {
		GetIndString( infoLine, INFO_STRS, line+1 );
		MoveTo( (width-StringWidth( infoLine )) >> 1, lineh[line] );
		DrawString( infoLine );
	}
	
	/* Wait for a mouse down event inside the window */
	
	for(;;) {
		while( !GetNextEvent( mDownMask, &aboutEvent )) SystemTask();
		
		where = FindWindow( aboutEvent.where, &whichWindow );
		if ( (where==inContent) && (whichWindow==aboutWindow) ) break;
		
		SysBeep( 30 );
	}

	DisposeWindow( aboutWindow );
	
	return;
}

