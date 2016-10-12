#ifdef BIT64ADDRSUPPORT
#include <diabloobject.h>
#include <diabloecoff.h>

static int ecoff_module_usecount=0;

void DiabloEcoffInit(int argc, char ** argv)
{
  if (!ecoff_module_usecount)
  {
    ObjectHandlerAdd("ECOFF", NULL, IsEcoff, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
    ObjectHandlerAdd("ECOFF", "alpha", IsEcoff, EcoffRead, EcoffWrite, EcoffGetSizeofHeaders, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
  }
  ecoff_module_usecount++;
}

void DiabloEcoffFini()
{
  ecoff_module_usecount--;
}
#endif
/* vim: set shiftwidth=2 expandtab cinoptions=p5,t0,(0, foldmethod=marker: */
