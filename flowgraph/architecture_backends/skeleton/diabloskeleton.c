#include <diabloskeleton.h>



void
DiabloSkeletonInit (int argc, char **argv)
{
  ArchitectureHandlerAdd("skeleton",&skeleton_description, ADDRSIZEXX);
}

void 
DiabloSkeletonFini()
{
  ArchitectureHandlerRemove("skeleton");
}
