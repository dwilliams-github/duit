/* 
 | Implementation of DuitThing
*/

#include "DuitThing.h"


DuitThing::DuitThing( DirEntry *dir, short nEntry, short fRefNum, TempFileInfo *tInfo )
{
	original	= dir;
	nOriginal	= nEntry;
	fRefNumOrig	= fRefNum;
	tempInfo	= tInfo;
	
	modified = 0;
	inMemory = 0;
	open	 = 0;
	return;
}

OSErr DuitThing::Edit()
{
	// Well, if this base class is being called, then we don't know
	// what to do with this file!
	
	return(1);
}


void DuitThing::Revert()
{
	return;
}

OSErr DuitThing::SaveInto( short fRefNum )
{
	// Simply put what we had back into the file
	
	return(0);
}
	

int DuitThing::Event( EventRecord *event )
{
	return( DUIT_YOU_GET_IT );
}


int DuitThing::ThisIsMyWindow( WindowPtr window )
{
	return(0);
}

void DuitThing::SetBehind( WindowPtr frontWindow ) 
{
	return;
}

void DuitThing::MakeActive( ) 
{
	return;
}

WindowPtr DuitThing::MyBottomWindow( ) 
{
	return(0);		// Hmm... a little caution here by the caller is required
}

void DuitThing::CallMeALot()
{
	return;
}
