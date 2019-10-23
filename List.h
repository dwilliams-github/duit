/*
 | Declaration of class List
*/

#ifndef __List__
#define __List__

class List {
	private:
	typedef struct sItem {
		struct sItem	*next;
		struct sItem	*prev;
		void			*thing;
	} Item;

	Item		*head;
	
	public:
	void		*front;
	
	List() {front = 0; head = 0;}
	~List();
	
	int NewFront( void *thing );
	int	BringToTop( void *thing );
	int Traverse( int (*doMe)( void *thing, void *doMeArg ), 
	              void *doMeArg, int killAsWeGo );
	int Delete( void *thing );
	int DeleteFront();
	
	private:
	void *FindThing( void *thing );			// Actually returns *Item
	void Remove( Item *thisDoc );
	void PutOnTop( Item *thisDoc );
};

#endif
