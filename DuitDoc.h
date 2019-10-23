/*
 | Declaration of class DuitDoc
*/

#ifndef __DuitDoc__
#define __DuitDoc__

#include "Palette.h"
#include "Waddef.h"
#include "DuitLevel.h"
#include "List.h"

#define DOC_YOU_GET_IT	1
#define DOC_I_GOT_IT	2
#define DOC_I_QUIT		0

typedef struct {
	DirEntry	*start;			// Starting index into directory, including marker entry
	short		nDir;			// Number entries in directory
	DuitThing	*thing;			// Associated edit window, or zero if window not open
} ThingDesc;

typedef struct sWindListDesc {
	struct sWindListDesc	*prev;			// Previous item in list
	struct sWindListDesc	*next;			// Next item in list
	DialogPtr				dialog;			// This dialog (document window)
	ListHandle				list;			// This list
	Rect					outline;		// The outline to the list
} WindListDesc;

void MakeTempPrefix( short num, char prefix[8] );

class DuitDoc {
	public:
	OSErr		ioError;			// Set non-zero in case something bad happened
	DialogPtr	dialog;				// For the main window for this file

	private:	
	unsigned short	needsSaving;	/* Non-zero if changes made that need saving */
	
	ControlHandle editButton;		// Handle for the edit button of the window
	ListHandle	list;				// The scroll list for level selection
	Rect		listRect;			//    and the rectangle that contains it
	short		selectedItem;		//    currently selected cell
	short		fRefNum;			// File reference number
	WindListDesc	*thisDocList;	// This documents entry in the global list
	
	UniversalProcPtr	listHandler;	// List handling procedure
	
	List		*openWindows;		// A list of open windows, in order on the screen

	short		nThing;				// Number things in file
	ThingDesc	*things;			// Description of these things
	
	short			thisDocNumber;	// Unique document number
	
	TempFileInfo	tempInfo;		// Temporary file information for this document
	
	public:
	DuitDoc( unsigned short oneIfNew, unsigned char *name, short vRefNum, short docNum );
	~DuitDoc();
	unsigned long DialogEvent( EventRecord *event, DialogPtr whichDialog, short itemHit );
	unsigned long Event( EventRecord *event );
	unsigned long WindowHit( EventRecord *event );
	void BringToFront();
	unsigned short Close();
	int CheckHit( WindowPtr hitWindow );
	void SetDialogBehind( WindowPtr frontWindow );
	
	void CallMeALot();
	
	private:
	OSErr ReadPWAD();
	
    friend pascal void ListHandler( WindowPtr window, short item );
};

#endif
