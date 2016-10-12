#include <diablosupport.h>

int main()
{
   void * x=Malloc(10);
   void * y=Malloc(10);

   y=Realloc(y,100);
#ifdef MALLOC_DEBUG
   if (GetMemUse()!=110) FATAL(("Malloc Info Corrupt!"));
#endif

   Free(y);

#ifdef MALLOC_DEBUG
   if (GetMemUse()!=10) FATAL(("Malloc Info Corrupt!"));
#endif
   Free(x);
   return 0;
}
