/*
 | Implementation of List
 | Double linked list of objects
*/

#include "List.h"

/*
 | ~List   (Destructor)
*/
List::~List()
{
	Item	*thisItem;
	
	thisItem = head;
	while( thisItem ) {
		Item	*next = thisItem->next;
		delete thisItem;
		thisItem = next;
	}
}

/*
 | NewFront
 | Add a new thing, always to the front of the list
*/
int List::NewFront( void *thing )
{
	Item	*newItem;
	
	newItem = new Item;
	if (newItem==0) return(1);
	
	newItem->next = head;
	newItem->prev = 0;
	newItem->thing = thing;
	if (head)  head->prev = newItem;
	head = newItem;
	
	front = thing;
	return(0);
}

/*
 | BringToTop
 | Bring the (existing) thing to the top of the list
*/
int List::BringToTop( void *thing )
{
	Item	*thisItem = (Item *)FindThing( thing );
	if (thisItem==0) return(1);

	Remove( thisItem );
	PutOnTop( thisItem );

	return(0);
}
	
/*
 | Traverse
 | Traverse the documents in the list
*/
int List::Traverse( int (*doMe)( void *thing, void *doMeArg ), 
                           void *doMeArg, int killAsWeGo )
{
	Item	*thisItem, *next;
 	int	result;
	
	thisItem = head;
	while(thisItem) {
		if (result = (*doMe)( thisItem->thing, doMeArg )) return(result);
		next = thisItem->next;
		if (killAsWeGo) {
			delete thisItem;
			head = next;
			if (next) {
				next->prev = 0;
				front = next->thing;
			}
			else 
				front = 0;

			head = next;				
		}
		thisItem = next;
	}
	
	return(0);
}

/*
 | DeleteDoc
 | Delete the document, found anywhere in the list
*/
int List::Delete( void *thing )
{
	Item	*thisItem = (Item *)FindThing( thing );
	if (thisItem==0) return(1);
	
	Remove( thisItem );	
	delete thisItem;
	
	return(0);
}

/*
 | DeleteFront
 | Delete the front document
*/
int List::DeleteFront()
{
	if (head==0) return(1);
	
	Remove( head );
	delete head;
	return(0);
}

/*
 | FindDoc (private)
 | Find document in list
*/
void *List::FindThing( void *thing )
{
	Item *thisItem = head;
	
	while(thisItem) {
		if (thisItem->thing == thing) return(thisItem);
		thisItem = thisItem->next;
	}
	
	return(0);
}

/*
 | Remove (private)
 | Remove a item in the document list, but without deleting it
*/
void List::Remove( Item *thisItem )
{
	Item	*prev = thisItem->prev;
	Item	*next = thisItem->next;
		
	if (prev) 
		prev->next = next;
	else {
		head = next;
		front = head ? head->thing : 0;
	}
			
	if (next) next->prev = prev;
}


/*
 | PutInFront (private)
 | Put an existing list item in the front of the list
*/
void List::PutOnTop( Item *thisItem )
{
	if (head) head->prev = thisItem;
	thisItem->prev = 0;
	thisItem->next = head;
	head = thisItem;
	front = thisItem->thing;
}
