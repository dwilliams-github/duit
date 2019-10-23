/*
 | General WAD utilties
*/

#include "WadUtility.h"


int CompareStrings( char *s1, char *s2, short length )
{
	char *ss1 = s1 + length;
	char *ss2 = s2 + length;
	
	while( ss1 > s1 ) {
		if (*(--ss1) != *(--ss2)) return(0);
	}
	
	return(1);
}

unsigned long SwapLong( unsigned long toswap )
{
	unsigned long result;
	
	result = ((toswap&0xff000000)>>24) |
	         ((toswap&0x00ff0000)>> 8) |
	         ((toswap&0x0000ff00)<< 8) |
	         ((toswap&0x000000ff)<<24);
	return(result);
}

void SwapShorts( unsigned short *toswap, unsigned long nSwap )
{
	unsigned short	*thisShort;
	
	thisShort = toswap + nSwap;
	while( --thisShort >= toswap ) {
		unsigned short result = *thisShort;
		*thisShort = ((result&0xff00)>>8) | ((result&0x00ff)<<8);
	}
}

void SwapLongs( unsigned long *toswap, unsigned long nSwap )
{
	unsigned long	*thisLong;
	
	thisLong = toswap + nSwap;
	while( --thisLong >= toswap ) {
		unsigned short result = *thisLong;
		*thisLong =	((result&0xff000000)>>24) |
	     	  	   	((result&0x00ff0000)>> 8) |
	     	  		((result&0x0000ff00)<< 8) |
	     	 		((result&0x000000ff)<<24);
	}
}

