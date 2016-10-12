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
   IoHandlerAdd(E_STATUS,StatusIo);
   IoHandlerAdd(E_FATAL,StatusIo);
   IoModifierAdd('@','x',"bc",XModifier);
   IoModifierAdd('%',0,NULL,IoModifierPrintf);
   DirMake("./a",FALSE);
   DirMake("./a/b",FALSE);
   DirMake("./a/b/c",FALSE);
   DirDel("./a");
   return 0;
}
