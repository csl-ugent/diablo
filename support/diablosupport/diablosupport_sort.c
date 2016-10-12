/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

#include <diablosupport.h>

#define ELEMENT_AT(b, o, s) ((void*)((char*)(b) + (s)*(o)))

//#define USE_DIABLO_STABLE_SORT

// This code is based on Wikipedia's description of insertion sort
void diablo_stable_sort(
	void *base, unsigned long nmemb, unsigned long size,
    int(*compar)(const void *, const void *))
{
  if (nmemb == 0)
    return;
#ifdef USE_DIABLO_STABLE_SORT
	int i;
	void* item = malloc(size);
	for (i = 1; i < nmemb; i++) 
    {
     // A[ i ] is added in the sorted sequence A[0, .. i-1]
     int hole = i;
	 // Save A[ i ] temporarily
     memcpy(item, ELEMENT_AT(base, i, size), size);
     // keep moving the hole to the next smaller index until the sequence is again sorted when adding the saved item
     while (hole > 0 && compar(ELEMENT_AT(base, hole - 1, size), item) > 0)
     {
		 memcpy(ELEMENT_AT(base, hole, size), ELEMENT_AT(base, hole - 1, size), size);
         hole--;
     }
	 // Put the saved element back in the hole
	 memcpy(ELEMENT_AT(base, hole, size), item, size);
   }
	free(item);
#else
	qsort(base, nmemb, size, compar);
#endif
}
