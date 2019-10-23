/*
 | Waddef.h
 | Definition of Doom structures
*/

#ifndef __WADDEF__
#define __WADDEF__


typedef struct {
	char			name[4];
	unsigned long	numLump;
	unsigned long	dirOffset;
} WadHeader;

typedef struct {
	unsigned long	offset;
	unsigned long	length;
	char			name[8];
} DirEntry;

typedef struct {
	unsigned char	*data; 		/* Pointer to reject data 	*/
	unsigned long	size;		/* Size of data				*/
	unsigned long	nSect;		/* Number of sectors		*/
	unsigned long	filePos;	/* Location of reject data in file	*/
} RejectDesc;

typedef struct {
	short	xPos, yPos;			// Location of the thing
	short	angle;				// Angle it faces
	short	type;				// Type of thing
	short	options;			// Options (bitmask, see below)
} WadThing;

#define WAD_THING_SKILL12		0
#define WAD_THING_SKILL3		1
#define WAD_THING_SKILL45		2
#define WAD_THING_DEAF			3
#define WAD_THING_MULTIPLAYER	4

typedef struct {
	short	from, to;			// Vertices
	short	flags;
	short	types;
	short	tag;
	short	right;				// Right sidedef
	short	left;				// Left sidef
} WadLine;

#define WAD_LINE_IMPASSIBLE		0
#define WAD_LINE_BLOCKMONSTER	1
#define WAD_LINE_TWOSIDE		2
#define WAD_LINE_UPPEG			3
#define WAD_LINE_LOPEG			4
#define WAD_LINE_SECRET			5
#define WAD_LINE_BLOCKSOUND		6
#define WAD_LINE_NOMAP			7
#define WAD_LINE_ONMAP			8

typedef struct {
	short	xOffset, yOffset;		// Texture offset
	char	upper[8];				// Upper texture name
	char	lower[8];				// Lower texture name
	char	middle[8];				// Middle texture name
	short	sector;					// Sector the sidedef faces
} WadSide;

typedef struct {
	short	x, y;
} WadVertex;

typedef struct {
	short	floor;				// Floor height
	short	ceiling;			// Ceiling height
	char	floorFlat[8];		// Name of texture for floor
	char	ceilingFlat[8];		// Name of texture for ceiling
	short	light;				// Light level
	short	special;
	short	tag;
} WadSector;

typedef struct {
	short	start, end;				// Associated vertices
	short	angle;					// Angle, in BAM
	short	line;					// Line belonging to this seg
	short	direction;
	short	offset;
} WadSeg;

typedef struct {
	short	numSegs;
	short	firstSeg;
} WadSSector;

typedef struct {
	short 	yUpper, 
			yLower,
			xLower,
			xUpper;
} WadNodeBound;
	
typedef struct {
	short	x, y;
	short	dx, dy;
	WadNodeBound	left, right;
	short	rightNode;
	short	leftNode;
} WadNode;


#endif
