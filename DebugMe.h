//
// Debug window declaration
//

#include "Scroll.h"
#include "Reject.h"
#include "RejectPrivate.h"

class DebugMe : public Scroll {

	private:
	Block     block;
	BlockLine line, seen;
	short	  nTest;
	int		  anyBlock;
	float	  bStart, bEnd;
	ScrollsPoint	topLeftSect;	// TopLeft of screen corresponding to current regions
	
	PixPatHandle	pPat1,
					pPat2,
					pPat3;


	public:
	DebugMe();
	~DebugMe();
	void Next();
	void Recall();

	private:
	void Draw( ScrollsPoint topLeft, RgnHandle updateRgn, RgnHandle visRgn, short newFrame );
	void DrawDebug();
};
