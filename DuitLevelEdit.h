/*
 | Declaration of class DuitLevelEdit
 | A window for editing a level
*/

#ifndef __DuitLevelEdit__
#define __DuitLevelEdit__

#include "Waddef.h"
#include "Leveldef.h"
#include "Reject.h"
#include "Palette.h"

#include "Scroll.h"

typedef struct sSectVerts {
	WadVertex	*vertex;			// Vertex index
	struct sSectVerts	*next;		// Next in list
} SectVerts;

typedef struct sSectVList {
	SectVerts	*head;				// First vectex in list
	SectVerts	*tail;				// Last vertex in list, head==tail for a closed list
	short		nVert;				// Number vertices
	struct sSectVList *next;		// Next vertex list
} SectVList;

typedef enum {
	off=0,				// Sector not drawn for current screen
	on,					// Sector drawn in total for current screen
	onPart				// Sector drawn in concatonated form for current screen
} SectStat;

typedef struct {
	RgnHandle		region;			// Quickdraw region corresponding to the sector
	SectStat		onScreen;		// Non-zero if region is currently on screen
	ScrollsPoint	origin;			// Origin of region
	SectVList		*vLists;		// List of vertex lists
	Rect			bounds;			// Smallest rectangle that contains all vertices
	short			error;			// Error number:   0=okay  1=unclosed  2=empty
} SectDesc;

class DuitLevelEdit : public Scroll {

	public:
	OSErr		ioError;			// Set non-zero in case something bad happened

	private:	
	unsigned short	needsSaving;	// Non-zero if changes made that need saving
	Palette			*toolPal;		// Tool selection palette
	short			scale;			// Image scale, power of two
	long			scaleFact;		// Divisor or multiplier corresponding to scale
	ScrollsRect		bounds;			// Image bounds
	ScrollsPoint	topLeftSect;	// TopLeft of screen corresponding to current regions
	SectDesc		*sectDesc;		// Sector descriptors
	
	RgnHandle		saveRgn,
					toxor1, toxor2;	// Scratch regions
	
	PixPatHandle	pPatBack,
					pPatFill,
					pPatOutline,
					pPatControl,
					pPatSelect,
					pPatMetaSelect;
					
	int 			mouseWasIn;
	CursHandle		magCursor;
	SectDesc		*selected;		// Currently selected region
	
	LevelDesc		*level;
	
	short			debugLindex;	// Reject package target debug line
	RejectDebugLine *debugLines;	// Reject package debug lines, 0=no debug
	short			nDebugLine;		// Number debug lines to draw
	PixPatHandle	pPatDebug[3];	// Debug colors
	short			debugFlash;		// Debug flash state
	RejectDebugLine2 *debugLines2;	// Reject package debug lines, 0=no debug
	short			nDebugLine2;	// Number debug lines to draw
	RejectDebugLine2 *debugNext2;	// Next debug option 2
	
	public:
	DuitLevelEdit( unsigned char *title, LevelDesc *descrip );
	~DuitLevelEdit();
	void SetBehind( WindowPtr frontWindow );
	void CallMeALot();
	
	void NextDebug();
	void NextDebug2();
	
	private:
	void Draw( ScrollsPoint topLeft, RgnHandle updateRgn, RgnHandle visRgn, short newFrame );
	void DrawBorder( RgnHandle updateRgn, short newFrame );
    void Mouse( ScrollsPoint topLeft, Point where, short modifiers );
    void MouseBorder( Point where, short modifiers );
	int ReadLevel( DirEntry *dir, short nEntry, short fRefNum );
	void RebuildRgn( SectDesc *sect );
	void MakeSectRegion( SectVList *list, RgnHandle region );
	
	int BuildSectDesc();
	
	void DrawDebug( int erase, int wait );
	void DrawDebug2( int erase, RejectDebugLine2 *drawMe );
	void FlashDebug();
	
	short	XtoScreen( short x );
	short	YtoScreen( short y );
	short	XtoWad( short x );
	short	YtoWad( short y );
};

#endif
