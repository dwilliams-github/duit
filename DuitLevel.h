/*
 | Declaration of class DuitLevel
*/

#ifndef __DuitLevel__
#define __DuitLevel__

#include "DuitThing.h"
#include "Waddef.h"
#include "Leveldef.h"

#include "DuitThing.h"
#include "DuitLevelEdit.h"

#include "DebugMe.h"

class DuitLevel : public DuitThing {

	private:		
	LevelDesc	level;	
	DuitLevelEdit	*editor;
	DebugMe		*debugMe;

	public:
	DuitLevel( DirEntry *dir, short nEntry, short fRefNum, TempFileInfo *tempInfo );
	~DuitLevel();
	
	OSErr Edit();
	void Revert();
	OSErr SaveInto( short fRefNum );
	
	int Event( EventRecord *event );
	int ThisIsMyWindow( WindowPtr window );
	WindowPtr MyBottomWindow();

	void SetBehind( WindowPtr frontWindow );
	void MakeActive();
	void CallMeALot();
	
	private:
	OSErr ReadLevel( DirEntry *dir, short nEntry, short fRefNum );

};

#endif
