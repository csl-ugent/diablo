#define DIABLOOBJECT_INTERNAL
#include <diabloobject.h>
void ObjectHashElementFree (void *tf, void *data);

t_object *obj2;
int
main (int argc, char **argv)
{
  t_object *obj;
  t_symbol * sym;
  t_reloc * rel;
  
  DiabloObjectInit (argc, argv);
  DiabloBinutilsInit (argc, argv);
  DiabloTiLinkerInit (argc, argv);

#ifdef DIABLOOBJECT_ELFSUPPORT
  DiabloElfInit (argc, argv);
#endif

#ifdef DIABLOOBJECT_ECOFFSUPPORT
  DiabloEcoffInit (argc, argv);
#endif

#ifdef DIABLOOBJECT_TICOFFSUPPORT
  DiabloTiCoffInit (argc, argv);
#endif
  
  DiabloArInit (argc, argv);
#if 0
  obj = ObjectRead (SRCDIR "/test/inputs/hw", NULL, FALSE);
  OBJECT_SET_SUBOBJECT_CACHE(obj,  HashTableNew (3001, 0, (t_uint32 (*)(void *, t_hash_table *)) StringHash, (t_int32 (*)(void *, void *)) StringCmp, ObjectHashElementFree));
  
  obj2 = ObjectGet ("main.o", obj, FALSE);

  OBJECT_FOREACH_SYMBOL(obj2,sym)
  {
	  VERBOSE(0,("DIFF: @S\n",sym));
  }
  OBJECT_FOREACH_RELOC(obj2,rel)
  {
	  VERBOSE(0,("DIFF: @R\n",rel));
  }
  ObjectWrite (obj, "b.out");

  obj = LinkEmulate ("hw",FALSE);
  ObjectWrite (obj, "c.out");

  
  obj = ObjectRead ("/home/bdebus/c6x/example/echo.out", NULL, FALSE);
  obj = LinkEmulate ("/home/bdebus/c6x/example/echo.out",FALSE);
#endif
  
  
  DiabloBinutilsFini ();
#ifdef DIABLOOBJECT_ELFSUPPORT
  DiabloElfFini ();
#endif

#ifdef DIABLOOBJECT_ECOFFSUPPORT
  DiabloEcoffFini ();
#endif

#ifdef DIABLOOBJECT_TICOFFSUPPORT
  DiabloTiCoffFini ();
#endif
  DiabloArFini ();
  DiabloObjectFini();
  return 0;
}
