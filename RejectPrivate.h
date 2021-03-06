/*
 | RejectPrivate.h
 | Private declaration for reject map building package
*/


// Block
// A structure that defines how one line blocks the view from another

typedef struct {
	WadVertex	*vert;
	long 		x0, y0, tx, ty;
} BlockLine;

typedef struct {
	long 		tx, ty;
} BlockBase;

typedef struct sBlock {
	BlockLine 	bl, br;
	BlockBase	bbl, bbr;
	WadLine		*line;
	short		covered;		// Bitmask: 0 set = left covered, 1 set = right covered
} Block;

// Seen
// A segment along the length of a line that is visible, as stored
// in a linked list. Values are in fractional coordinates from the start of the line.

typedef struct sSeen {
	struct sSeen *next;
 	float start, end;
} Seen;



static int	InView( WadVertex *vertA, WadVertex *vertB, WadVertex *vertC, WadVertex *vertD );
static void GetSeen( Block *blocks, short nBlock, WadLine *skipMe,
                     WadVertex *start, WadVertex *end, Seen *seens, Seen **head );
int	AmIBlocked( BlockLine *line, Block *block, float *bStart, float *bEnd );
static int	GetIntersect( BlockLine *line1, BlockLine *line2, float *s1, float *s2 );
static long	CrossOut( BlockLine *line1, BlockLine *line2 );
static long	CrossOver( BlockLine *line1, BlockLine *line2 );
static int	BlockBlock( Block *block1, Block *block2 );
