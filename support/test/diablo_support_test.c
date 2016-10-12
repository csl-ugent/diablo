#include <diablosupport.h>

int main()
{
   printf("-------- Diablo Support Tests ---------\n");
   printf("OPTIONS: MALLOC_DEBUG is %s\n",
#ifdef MALLOC_DEBUG
	 "on"
#else
	 "off"
#endif
	 );
   return 0;
}
