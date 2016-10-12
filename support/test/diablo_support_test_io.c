#include <diablosupport.h>

t_string XModifier(t_string modifiers, va_list * ap)
{
   return StringConcat3("<replaced by the modifier x (arg = ",va_arg(*ap,char *),")>");
}

void StatusIo(t_uint32 id, t_string out)
{
   printf(out);
}

int main()
{
   IoHandlerAdd(E_DEBUG,StatusIo);
   IoHandlerAdd(E_FATAL,StatusIo);
   IoModifierAdd('@','x',"bc",XModifier);
   IoModifierAdd('%',0,NULL,IoModifierPrintf);
   DEBUG(("Hallo %s\n","String"));
   DEBUG(("Hallo %d\n",10));
   DEBUG(("Hallo %d %d\n",10,20));
   DEBUG(("Hallo %d %0.2lf %s\n",2,0.3,"BRRRRR"));
   DEBUG(("Long Long 1: %lld, string: %s\n",1LL,"BRRRRR"));
   DEBUG(("Hallo @b\n"));
   DEBUG(("Hallo @@\n"));
   DEBUG(("Hallo @bx @cx\n","a","b"));
   DEBUG(("Hallo @cx @bx\n","a","b"));
   FATAL(("FATAL: Hallo @cx @bx\n","a","b"));
   return 0;
}
