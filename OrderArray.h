/*
 | OrderArray
 | Ordered array of items
*/

#ifndef __ORDERARRAY__
#define __ORDERARRAY__

class OrderArray {
	public:
	short	nItem;

	private:
	short	*list;
	short	maxItem;
	
	public:
	OrderArray( short nMax );
	~OrderArray();
	
	int NewTop( short item );
	int MoveToTop( short item );
	int Delete( short item );
}
