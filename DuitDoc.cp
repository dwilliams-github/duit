/* 
 | Implementation of DuitDoc
*/

#include "DuitDoc.h"
#include "List.h"
#include "WadUtility.h"

#define DOC_DIALOG		129
#define	DOC_DIALOG_EDIT		1
#define DOC_DIALOG_CLOSE	2
#define DOC_DIALOG_LIST		3
#define DOC_DIALOG_REMOVE	4
#define DOC_DIALOG_ADD		5
#define DOC_DIALOG_EDITOUT	6

#define OPEN_ERR_ALERT	130
#define READ_ERR_ALERT	131
#define NOT_PWAD_ALERT	132


static WindListDesc *globalWindowList = 0;

pascal void ListHandler( WindowPtr window, short item )
{
	WindListDesc	*docItem;

	/* See who we are */
	
	docItem = globalWindowList;
	while(docItem) {
		if (docItem->dialog == window) break;
		docItem = docItem->next;
	}
	
	if (docItem) {

		/* Draw the list */
	
		LUpdate( window->visRgn, docItem->list );
	
		/* And the frame around it */
	
		FrameRect( &docItem->outline );
	}

	return;
}



/*
 | DuitDoc (Creater)
 | Open the specified file and display its main window
*/
void MakeTempPrefix( short	docNum, char prefix[8] )
{
	prefix[0] = 'D';
	prefix[1] = 'u';
	prefix[2] = 'i';
	prefix[3] = 't';
	prefix[4] = docNum&0x000F + '0';
	prefix[5] = docNum&0x00F0 + '0';
	prefix[6] = docNum&0x0F00 + '0';
	prefix[7] = docNum&0xF000 + '0';
}


DuitDoc::DuitDoc( unsigned short oneIfNew, unsigned char *name, short vRefNum, short docNum )
{
	long		*CurDirStore = (long *)0x398;
	ThingDesc	*oneThing;
	Rect		listView,
				listGrid = { 0, 0, 1, 1 },
				cRect;
	Point		nullPoint = { 0, 0 },
				cellPlace;
	short		cType;
	Handle		cHandle;
	
	/* I really think c++ stinks. About as robust as a house of cards. */
	
	dialog		= 0;
	fRefNum		= 0;
	nThing		= 0;
	list		= 0;
	
	needsSaving = 0;
	thisDocList = 0;
	openWindows = 0;
	
	listHandler = 0;
	
	// Save global document counter
	
	thisDocNumber = docNum;
	
	// Find directory and construct filename prefix for the temporary files
	// for this document
	
	ioError = FindFolder( vRefNum, kTemporaryFolderType, 
	                      kCreateFolder, &tempInfo.vRefNum, &tempInfo.dirId );
	if (ioError) {
		NoteAlert( OPEN_ERR_ALERT, 0 );
		return;
	}
	MakeTempPrefix( thisDocNumber, tempInfo.prefix );
	
	if (vRefNum) {
	
		/* If this is an old file, try opening it */
	
		ioError = HOpen( vRefNum, *CurDirStore, name, fsRdWrPerm, &fRefNum );
		if (ioError) {
			NoteAlert( OPEN_ERR_ALERT, 0 );
			return;
		}
	
		/* Suck out the juices */
	
		ioError = ReadPWAD();
		if (ioError) return;
	}
	
	/* Open up a dialog window */
	
	dialog = GetNewDialog( DOC_DIALOG, 0, (WindowPtr)-1 );
	if (dialog == 0) {
		ioError = 1;
		return;
	}
	
	GetDItem( dialog, DOC_DIALOG_EDIT, &cType, (Handle *)&editButton, &cRect );

	/* Build open window list */
		
	openWindows = new List;
	openWindows->NewFront( (void *)dialog );

	/* Set title */
	
	SetWTitle( dialog, (unsigned char *)name );
	
	/* Build a list */
	
	GetDItem( dialog, DOC_DIALOG_LIST, &cType, &cHandle, &listRect );
	
	listView.top 	= listRect.top + 1;
	listView.bottom	= listRect.bottom - 1;
	listView.right	= listRect.right - 16;
	listView.left	= listRect.left + 1;
	
	listGrid.bottom = nThing;
	
	list = LNew( &listView, &listGrid, nullPoint, 0, dialog, FALSE, FALSE, FALSE, TRUE );
	
	(*list)->selFlags = lOnlyOne;
	
	/* Fill it */
	
	if (nThing) {
		cellPlace.h = 0;
		cellPlace.v = 0;
		oneThing = things;
		do {
			LSetCell( oneThing->start->name, 8, cellPlace, list );
		} while( oneThing++, ++cellPlace.v < nThing );	

		cellPlace.v = 0;
		LSetSelect( TRUE, cellPlace, list );
		selectedItem = 0;
	}
	else {
		HiliteControl( editButton, 255 );
		selectedItem = -1;
	}
			
	/*
	 | Store a pointer to it in a global structure, for our ListHandler routine.
	 | This unfortunate use of global data is necessary because Apple neglected
	 | to include a user parameter for the user dialog routines. This sucks.
	*/
	
	thisDocList = new WindListDesc;
	if (thisDocList==0) {
		ioError = 1;
		return;
	}
	
	thisDocList->next = globalWindowList;
	if (globalWindowList) globalWindowList->prev = thisDocList;
	thisDocList->prev = 0;
	globalWindowList = thisDocList;
	
	thisDocList->dialog 	= dialog;
	thisDocList->list		= list;
	thisDocList->outline	= listRect;
	
	LDoDraw( TRUE, list );
	
	// Build a universal procedure, so that our list handler (PPC code) can be 
	// called by quickdraw
	
	listHandler = NewControlActionProc( (ProcPtr)ListHandler );
		
	/* And install it */
	
	SetDItem( dialog, DOC_DIALOG_LIST, cType, (Handle)listHandler, &listRect );
	
	/* Draw the window */
	
	ShowWindow( dialog );
	return;
}

/*
 | ReadPWAD
 | Read the directory structure of the document file
*/

OSErr DuitDoc::ReadPWAD()
{
	WadHeader	header;
	DirEntry	*lumps,
				*thisLump;
	long		bytes;
	OSErr		ioErr;
	ThingDesc	*oneThing;
	short		nDir;
	
	/* Read header */
	
	ioErr = SetFPos( fRefNum, fsFromStart, 0 );
	if (ioErr) goto fileError;
	
	bytes = sizeof(header);
	ioErr = FSRead( fRefNum, &bytes, &header );
	if (ioErr) goto fileError;
	
	header.numLump   = SwapLong(header.numLump);
	header.dirOffset = SwapLong(header.dirOffset);

	/* Check it out */
	
	if (CompareStrings( header.name, "PWAD", 4 ) == 0) {
		NoteAlert( NOT_PWAD_ALERT, 0 );
		return(1);
	}
	
	/* Allocate memory for the full directory */
	
	lumps = new DirEntry[header.numLump];
	
	/* And read them in */
	
	bytes = sizeof(DirEntry)*header.numLump;
	ioErr = SetFPos( fRefNum, fsFromStart, header.dirOffset );
	if (ioErr) goto fileError;
	
	ioErr = FSRead( fRefNum, &bytes, lumps );
	if (ioErr) goto fileError;
	
	thisLump = lumps + header.numLump;
	while( --thisLump >= lumps ) {
		thisLump->offset = SwapLong( thisLump->offset);
		thisLump->length = SwapLong( thisLump->length );
	}
	
	/* Count catalog entries */
	
	nThing = 0;
	thisLump = lumps + header.numLump;
	while( --thisLump >= lumps ) 
		if (thisLump->length == 0) nThing++;
		
	if (nThing==0) return(0);
	
	/* Allocate level list */
	
	things = new ThingDesc[nThing];
	
	/* And go back and fill it in */
	
	oneThing = 0;
	thisLump = lumps;
	do { 
		if (thisLump->length == 0) {
			if (oneThing) 
				(oneThing++)->nDir = nDir;
			else
				oneThing = things;
				
			oneThing->start = thisLump;
			nDir = 1;
		}
		else
			nDir++;
	} while( ++thisLump < lumps + header.numLump );
	
	if (oneThing) oneThing->nDir = nDir;
	
	// Build things
	
	oneThing = things;
	do {
		oneThing->thing = new DuitLevel( oneThing->start, oneThing->nDir, fRefNum, &tempInfo );
	} while( ++oneThing < things + nThing );
	

	return(0);
	
fileError:
	NoteAlert( READ_ERR_ALERT, 0 );
	return(ioErr);
}

DuitDoc::~DuitDoc()
{
	//
	// Get rid of the dead weight, but make sure you get rid of
	// the list *before* the dialog, otherwise bad things happen
	//

	if (list) LDispose( list );
	if (dialog) DisposeDialog( dialog );
	
	/* Should ask first in case changes need saving */
	
	if (fRefNum) FSClose( fRefNum );
	
	if (nThing) {
		ThingDesc *oneThing = things;
		do {
			delete oneThing->thing;
		} while( ++oneThing < things + nThing );
		
		delete things;
	}
	
	if (openWindows) delete openWindows;
	
	if (thisDocList) {
		WindListDesc	*prev = thisDocList->prev;
		WindListDesc	*next = thisDocList->next;
		
		if (prev) 
			prev->next = next;
		else
			globalWindowList = next;
			
		if (next) next->prev = prev;
		
		delete thisDocList;
	}
			
	if (listHandler) DisposeRoutineDescriptor( listHandler );
			
	return;
}

/*
 | DialogEvent
 | Check the dialog event and process if appropriate
*/
unsigned long DuitDoc::DialogEvent( EventRecord *event, DialogPtr whichDialog, short itemHit )
{
	Point		whereLocal;
	GrafPtr		savePort;
	Cell		selectedCell;
	
	/* Do we care? */
	
	if (whichDialog != dialog) return(DOC_YOU_GET_IT);
	
	/* Process item hit */
	
	switch(itemHit) {
		case DOC_DIALOG_EDIT: 	break;
		case DOC_DIALOG_CLOSE:	return(DOC_I_QUIT);
		case DOC_DIALOG_LIST:
			GetPort( &savePort );
			SetPort( dialog );

	 		whereLocal = event->where;
	 		GlobalToLocal( &whereLocal );
	 		
	 		if (LClick( whereLocal, event->modifiers, list )) {
	 		
	 			/* Double click! But where? */
	 			
	 			Cell cell = LLastClick( list );
	 			if ((cell.h == 0) && (cell.v >= 0) && cell.v < nThing) {
	 				ThingDesc *thisThing = things + cell.v;
	 				
	 				/* Is the selected thing already open? */
	 				
	 				if (thisThing->thing->open) {
	 					thisThing->thing->MakeActive();
	 					openWindows->BringToTop( (void *)thisThing );
	 				}
	 				else {
	 					if (thisThing->thing->Edit() == 0) 
	 						openWindows->NewFront( (void *)thisThing );
	 				}
	 			}
	 		}
	 		
	 		/* Check to make sure at least one cell is selected */
	 		
	 		selectedCell.h = 0;
	 		selectedCell.v = 0;
	 		if (LGetSelect( TRUE, &selectedCell, list )) {
	 			if (selectedItem==-1) HiliteControl( editButton, 0 );
	 			selectedItem = selectedCell.v;
	 		}
	 		else {
	 			if (selectedItem!=-1) HiliteControl( editButton, 255 );
	 			selectedItem = -1;
	 		}
	 			
	 		SetPort( savePort );
	 		break;
	 	case DOC_DIALOG_REMOVE:	break;
	 	case DOC_DIALOG_ADD:	break;
	}
 	
	return(DOC_I_GOT_IT);
}


/*
 | BringToFront
 | Bring this document window to the front
*/
typedef struct {
	DuitDoc		*myDoc;
	WindowPtr	dialogWindow;
	WindowPtr 	last;
} DoNextWinParm;

static int DoNextWindow( void *item, void *arg )
{
	DoNextWinParm	*parm = (DoNextWinParm *)arg;
	
	if (item == (void *)parm->dialogWindow) {
	
		/* Main window */
		
		if (parm->last) 
			parm->myDoc->SetDialogBehind( parm->last );
		else
			SelectWindow( parm->dialogWindow );
		
		parm->last = parm->dialogWindow;
	}
	else {
		ThingDesc	*thisThing = (ThingDesc *)item;
		
		/* Thing window */
		
		if (parm->last)
			thisThing->thing->SetBehind( parm->last );
		else
			thisThing->thing->MakeActive();
			
		parm->last = thisThing->thing->MyBottomWindow();
	}
	
	return(0);
}

void DuitDoc::BringToFront()
{
	DoNextWinParm	parm;
	
	parm.myDoc = this;
	parm.dialogWindow = dialog;
	parm.last = 0;
	
	openWindows->Traverse( DoNextWindow, &parm, 0 );
}


/*
 | CallMeALot
 | A routine to be called routinely (in the main eventloop)
*/
void DuitDoc::CallMeALot()
{
	if (openWindows->front != (void *)dialog) {
		ThingDesc *topThing = (ThingDesc *)openWindows->front;
	
		topThing->thing->CallMeALot();
	}
}


/*
 | Event.
 | See if the non-dialog event does anything for our document
*/
unsigned long DuitDoc::Event( EventRecord *event )
{
	WindowPtr 		whichWindow;
	RgnHandle		saveRgn;
	int 			whereCode; 
	Rect     		moveBound,
					newChar 	= {35,40,60,70};
	GrafPtr			wPort,
					savePort;	
	short			scrollState;
	
	/* Ask who's on top */
	
	if (openWindows->front == (void *)dialog) {
		
		/* The dialog is on top. Does it want it? */
		
		switch(event->what) {
			case mouseDown:	
		
				/* Mouse was pressed - find out where */
				
				whereCode = FindWindow( event->where, &whichWindow );
				if (whichWindow != dialog) break;
				
				switch (whereCode) {
					case inGoAway:
					
						/* Mouse in "go away" box: go away */
					
						TrackGoAway( whichWindow, event->where );
						return(DOC_I_QUIT);
				 	case inDrag:
				 	
				 		/* Mouse press in title bar: move the window about */
				 	
				 		moveBound = qd.screenBits.bounds;
				 		InsetRect( &moveBound, 4, 4 );
				 		DragWindow( whichWindow, event->where, &moveBound );
				 		return(DOC_I_GOT_IT);
				 	case inContent:
				 		
				 		/* Press in main dialog window. Bring it up front */
				 		
				 		SelectWindow( dialog );
				 		return(DOC_I_GOT_IT);
				 }
				 break;
		}
	}
	else {
		unsigned long answer;
		ThingDesc *topThing = (ThingDesc *)openWindows->front;
	
		/* Top window is a thing. See if it wants this event */
		
		answer = topThing->thing->Event( event );
		if (answer==DUIT_I_GOT_IT) 
			return(DOC_I_GOT_IT);
		else if (answer==DUIT_I_CLOSED) {
			openWindows->Delete( (void *)topThing );
			return(DOC_I_GOT_IT);
		}
	}
	
	/* 
	 | Well, see if it is a mouse press in an inactive window belonging to this 
	 | document. If so, we want to pull up the window, rather than let it process
	 | the mouse down event
	*/
	
	if (event->what == mouseDown) {	
		whereCode = FindWindow( event->where, &whichWindow );
		
		if (whichWindow == dialog) {
			openWindows->BringToTop( (void *)dialog );
			SelectWindow( whichWindow );
			return(DOC_I_GOT_IT);
		}
		else if (nThing) {
			ThingDesc *thisThing = things + nThing;
			
			while( --thisThing >= things ) {
				if (thisThing->thing->ThisIsMyWindow(whichWindow)) {
					openWindows->BringToTop( (void *)thisThing );
					thisThing->thing->MakeActive();
					return(DOC_I_GOT_IT);
				}
			}
		}
	}
	else if (nThing) {
		ThingDesc *thisThing = things + nThing;
	
		/* Well, okay. See if anyone wants this event */
		
		while( --thisThing >= things ) {
			if (thisThing->thing) {
				if (thisThing->thing->Event( event ) != DUIT_YOU_GET_IT) return(DOC_I_GOT_IT);
			}
		}
	}

	
 	/* Well, we don't want this mouse click! */
 	
 	return(DOC_YOU_GET_IT);
}


/*
 | CheckHit
 | Check to see if the specified hit window belongs to the document.
 | If so, the application should then call "BringToFront".
*/
int DuitDoc::CheckHit( WindowPtr hitWindow )
{
	/* Hit our main dialog? */
	
	if (hitWindow == dialog) return(1);
	
	/* How about any open level windows? */

	if (nThing) {
		ThingDesc *thisThing = things + nThing;
		while(--thisThing >= things) {
			if (thisThing->thing) {
				if (thisThing->thing->ThisIsMyWindow(hitWindow)) return(1);
			}
		}
	}
	
	/* Nope. Return zero */
	
	return(0);
}


/*
 | SetDialogBehind
 | Place this level's dialog window behind the specified window
*/
void DuitDoc::SetDialogBehind( WindowPtr frontWindow )
{
	WindowPeek peek = (WindowPeek)dialog;
	
	SendBehind( dialog, frontWindow );
	PaintOne( (GrafPtr)peek, peek->strucRgn );
	CalcVis( (GrafPtr)peek );
}
