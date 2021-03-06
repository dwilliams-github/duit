/* 
 | Implementation of DuitLevel
*/

#include "DuitLevel.h"
#include "WadUtility.h"

#define DOC_WIND		128
#define OPEN_ERR_ALERT	130
#define READ_ERR_ALERT	131
#define NOT_PWAD_ALERT	132
#define BAD_PWAD_DATA	133

#define CURSOR_MAG		1000

static void GetSize( short nVertex, WadVertex *vertices, ScrollsRect *bounds, short *scale )
{
	WadVertex *thisVertex;
	
	if (vertices == 0) {
		bounds->left 	= 0;
		bounds->right 	= 1000;
		bounds->top 	= 0;
		bounds->bottom 	= 1000;
		return;
	}
	
	thisVertex = vertices;
	bounds->left = bounds->right = thisVertex->x;
	bounds->top = bounds->bottom = thisVertex->y;
	
	while( ++thisVertex < vertices+nVertex ) {
		if (thisVertex->x < bounds->left)
			bounds->left = thisVertex->x;
		else if (thisVertex->x > bounds->right)
			bounds->right = thisVertex->x;
			
		if (thisVertex->y < bounds->bottom)
			bounds->bottom = thisVertex->y;
		else if (thisVertex->y > bounds->top)
			bounds->top = thisVertex->y;
	}
	
	bounds->left 	-= 20;
	bounds->right 	+= 20;
	bounds->top		= -bounds->top - 20;
	bounds->bottom	= -bounds->bottom + 20;
	
	*scale = -1;
	
	bounds->left 	/= 2;
	bounds->right	/= 2;
	bounds->top		/= 2;
	bounds->bottom	/= 2;
	
	return;
}
	

/*
 | DuitLevel (Creater)
 | Open the specified file and display its main window
*/

DuitLevel::DuitLevel( DirEntry *dir, short nEntry, short fRefNum, TempFileInfo *tempInfo ) 
	: DuitThing( dir, nEntry, fRefNum, tempInfo )
{	
	editor = 0;
	
	level.lines   = 0;
	level.sectors = 0;
	level.sides   = 0;
	level.reject  = 0;
	
	debugMe = 0;
	
	return;
}


/*
 | ~DuitLevel (destructor)
*/
DuitLevel::~DuitLevel()
{
	if (editor) delete editor;

	if (inMemory) {
		if (level.lines)	delete level.lines;
		if (level.sectors)	delete level.sectors;
		if (level.sides)	delete level.sides;
		if (level.reject)	delete level.reject;
	}

	if (debugMe) delete debugMe;

	return;
}


/*
 | Edit
 | Edit the level
*/

OSErr DuitLevel::Edit() 
{
	OSErr ioError;
	
	if (editor) return(0);		// Bring to front??

	// Modified?? If not, read this level in from the original file
	
	if (!inMemory) {
		ioError = ReadLevel( original, nOriginal, fRefNumOrig );
		if (ioError) {
			StopAlert( READ_ERR_ALERT, 0 );
			return(ioError);
		}
		inMemory = 1;
	}
	
	// Open up an edit window
	
	editor = new DuitLevelEdit( (unsigned char *)original->name, &level );
	if (editor->ioError) {
		
		// Oh oh. Something really bad happened. Get rid of editor.
		
		delete editor;
		editor = 0;
		StopAlert( BAD_PWAD_DATA, 0 );
		return(1);
	}

	open = 1;
	return(0);
}


/*
 | Revert
 | Destroy all changes to the level
*/
void DuitLevel::Revert()
{
	modified = 0;
	
	if (inMemory) {
		if (level.lines)	delete level.lines;
		if (level.sectors)	delete level.sectors;
		if (level.sides)	delete level.sides;
		if (level.reject)	delete level.reject;
		inMemory = 0;
	}

	return;
}	


OSErr DuitLevel::SaveInto( short fRefNum )
{
	return(0);
}
	
	
/*
 | Event
 | See if this level wants this event
*/
int DuitLevel::Event( EventRecord *event )
{
	if (editor) {
	
		if (event->what == keyDown) {
			if ((event->message&charCodeMask)==0x7A ) {
				editor->NextDebug();
				return( DUIT_I_GOT_IT );
			}
			else if ((event->message&charCodeMask)==0x0D ) {
				editor->NextDebug2();
				return( DUIT_I_GOT_IT );
			}
			else if ((event->message&charCodeMask)==0x20 ) {
				if (debugMe==0) debugMe = new DebugMe();
				debugMe->Next();
				return( DUIT_I_GOT_IT );
			}
			else if ((event->message&charCodeMask)==0x72 ) {
				if (debugMe) debugMe->Recall();
				return( DUIT_I_GOT_IT );
			}
			else if ((event->message&charCodeMask)==0x71 ) {
				if (debugMe) delete debugMe;
				debugMe = 0;
				return( DUIT_I_GOT_IT );
			}
		}
	
		switch( editor->Event( event ) ) {
			case SCROLL_I_QUIT:
				delete editor;
				editor = 0;
				open = 0;
				return( DUIT_I_CLOSED );
			case SCROLL_I_GOT_IT:
				return( DUIT_I_GOT_IT );
			default:
				return( DUIT_YOU_GET_IT );
		}
	}
	else
		return( DUIT_YOU_GET_IT );
}


/*
 | ThisIsMyWindow
 | Return non-zero if the specified window belongs to this level
*/
int DuitLevel::ThisIsMyWindow( WindowPtr window )
{
	if (editor)
		return( window == editor->window );
	else
		return( 0 );
}


/*
 | MyBottomWindow
 | Return bottommost editor window. Here, we just return our single window.
*/
WindowPtr DuitLevel::MyBottomWindow()
{
	if (editor)
		return( editor->window );
	else
		return( 0 );
}


/*
 | SetBehind
 | Send all associated windows behind
*/
void DuitLevel::SetBehind( WindowPtr frontWindow )
{
	if (editor) editor->SetBehind( frontWindow );
}


/*
 | MakeActive
 | Make this level window active (selected)
*/
void DuitLevel::MakeActive()
{
	if (editor)
		SelectWindow( editor->window );
}


/*
 | CallMeALot
 | A routine to be called routinely (in the main eventloop)
*/
void DuitLevel::CallMeALot()
{
	if (editor) editor->CallMeALot();
}




static void SwapSideBytes( WadSide *sides, short nSide )
{
	WadSide	*thisSide;
	
	thisSide = sides + nSide;
	while( --thisSide >= sides ) {
		SwapShorts( (unsigned short *)&thisSide->xOffset, 2 );
		SwapShorts( (unsigned short *)&thisSide->sector, 1 );
	}
}

static void SwapSectorBytes( WadSector *sectors, short nSector )
{
	WadSector *thisSector;
	
	thisSector = sectors + nSector;
	while( --thisSector >= sectors ) {
		SwapShorts( (unsigned short *)&thisSector->floor, 2 );
		SwapShorts( (unsigned short *)&thisSector->light, 3 );
	}
}


OSErr	DuitLevel::ReadLevel( DirEntry *dir, short nEntry, short fRefNum )
{
	DirEntry	*thisEntry;
	OSErr		ioErr;
	long		bytes;
	
	thisEntry = dir + nEntry;
	while( --thisEntry > dir ) {
		if (CompareStrings( thisEntry->name, "VERTEXES", 8 )) {
			level.nVertex = thisEntry->length/sizeof(WadVertex);
			level.vertices = new WadVertex[level.nVertex];
			
			ioErr = SetFPos( fRefNum, fsFromStart, thisEntry->offset );
			if (ioErr) goto fileError;

			bytes = thisEntry->length;
			ioErr = FSRead( fRefNum, &bytes, level.vertices );
			if (ioErr) goto fileError;
			
			SwapShorts( (unsigned short *)level.vertices, thisEntry->length>>1 );
		}
		else if (CompareStrings( thisEntry->name, "LINEDEFS", 8 )) {
			level.nLine = thisEntry->length/sizeof(WadLine);
			level.lines = new WadLine[level.nLine];
			
			ioErr = SetFPos( fRefNum, fsFromStart, thisEntry->offset );
			if (ioErr) goto fileError;

			bytes = thisEntry->length;
			ioErr = FSRead( fRefNum, &bytes, level.lines );
			if (ioErr) goto fileError;
			
			SwapShorts( (unsigned short *)level.lines, thisEntry->length>>1 );
		}
		else if (CompareStrings( thisEntry->name, "SIDEDEFS", 8 )) {
			level.nSide = thisEntry->length/sizeof(WadSide);
			level.sides = new WadSide[level.nSide];
			
			ioErr = SetFPos( fRefNum, fsFromStart, thisEntry->offset );
			if (ioErr) goto fileError;

			bytes = thisEntry->length;
			ioErr = FSRead( fRefNum, &bytes, level.sides );
			if (ioErr) goto fileError;
			
			SwapSideBytes( level.sides, level.nSide );
		}
		else if (CompareStrings( thisEntry->name, "SECTORS\0", 8 )) {
			level.nSector = thisEntry->length/sizeof(WadSector);
			level.sectors = new WadSector[level.nSector];
			
			ioErr = SetFPos( fRefNum, fsFromStart, thisEntry->offset );
			if (ioErr) goto fileError;

			bytes = thisEntry->length;
			ioErr = FSRead( fRefNum, &bytes, level.sectors );
			if (ioErr) goto fileError;
			
			SwapSectorBytes( level.sectors, level.nSector );
		}
		else if (CompareStrings( thisEntry->name, "REJECT\0\0", 8 )) {
			level.reject = new unsigned char[thisEntry->length];
			
			ioErr = SetFPos( fRefNum, fsFromStart, thisEntry->offset );
			if (ioErr) goto fileError;

			bytes = thisEntry->length;
			ioErr = FSRead( fRefNum, &bytes, level.reject );
			if (ioErr) goto fileError;
		}
	}
	return(0);
	
fileError:
	NoteAlert( READ_ERR_ALERT, 0 );
	return(ioErr);
}


		
		
