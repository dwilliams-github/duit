/*
 | Reject.h
 | Definitions for reject map building package
*/

#ifndef __REJECT__
#define __REJECT__

#include "Waddef.h"
#include "LevelDef.h"

typedef struct {
	long	x1, y1, 		// line first point
			x2, y2;			// line second point
	short	color;			// color code, 0=don't draw
	short	spare;			// not used
} RejectDebugLine;

typedef struct {
	long	x1, y1,
			x2, y2,
			x3, y3,
			x4, y4;
	short 	color;
	short	spare;
} RejectDebugLine2;

int RejectDebug( LevelDesc *level, short iLine, 
				 RejectDebugLine *debugLines, short *nDebug,
				 RejectDebugLine2 *debugLines2, short *nDebug2 );

#endif
