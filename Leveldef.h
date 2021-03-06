/*
 | Leveldef.h
 | Description of structures for a level
*/

#ifndef __LEVELDEF__
#define __LEVELDEF__



typedef struct {
	short	nVertex;
	WadVertex	*vertices;
	short	nLine;
	WadLine		*lines;
	short	nSide;
	WadSide		*sides;
	short	nSector;
	WadSector	*sectors;
	unsigned char	*reject;
} LevelDesc;

#endif
