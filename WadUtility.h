/*
 | Definition of WadUtility routines
*/

#ifndef __WADUTILITY__
#define __WADUTILITY___

int CompareStrings( char *s1, char *s2, short length );

unsigned long SwapLong( unsigned long toswap );
void SwapShorts( unsigned short *toswap, unsigned long nSwap );
void SwapLongs( unsigned long *toswap, unsigned long nSwap );

#endif
