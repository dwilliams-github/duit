/*
 | Declaration of class DuitThing: a base class for a generic PWAD item
 |
 | DuitThing     - Declare a new thing to edit, as when opening a PWAD file
 | Edit          - Edit this item (if you can). The edited version is kept either
 |                 in memory or in a temporary file, separate from the original.
 | Revert        - Abandom changes and go back to original
 | SaveInto      - Save into a PWAD, updating directory table
*/

#ifndef __DuitThing__
#define __DuitThing__

#define DUIT_YOU_GET_IT	0
#define DUIT_I_GOT_IT	1
#define DUIT_I_CLOSED	2

#include "Waddef.h"

typedef struct {
	short	vRefNum;		// Volume reference number
	long	dirId;			// Directory id
	char	prefix[8];		// Filename prefix
} TempFileInfo;

class DuitThing {
	public:
	OSErr		ioError;			// Set non-zero in case something bad happened
	short		inMemory;			// Non-zero if the item has been read in
	short		modified;			// Non-zero if the item has been modified
	short		open;				// Non-zero if the item is open (i.e. editor showing)
	
	DirEntry	*original;			// Pointer to original directory entry
	short		nOriginal;			// Number of entries in original directory
	short		fRefNumOrig;		// Original file
	TempFileInfo	*tempInfo;		// Temporary file information for associated document

	public:
	DuitThing( DirEntry *original, short nOriginal, short fRefNum, TempFileInfo *tempInfo );
	virtual ~DuitThing() {};
	
	virtual OSErr Edit();
	virtual void Revert();
	virtual OSErr SaveInto( short fRefNum );
	
	virtual int Event( EventRecord *event );
	virtual int ThisIsMyWindow( WindowPtr window );
	virtual WindowPtr MyBottomWindow();

	virtual void SetBehind( WindowPtr frontWindow );
	virtual void MakeActive();
	virtual void CallMeALot();
};

#endif
