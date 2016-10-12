#include <diablosupport.h>

int main()
{
   t_string_array_elem * tmp;
   t_string_array * divided=StringDivide("Hello:World:Dit:Is:Een:Test",":",FALSE,FALSE);
   STRING_ARRAY_FOREACH_ELEM(divided,tmp)
   {
      printf("%s\n",tmp->string);
   }
   return 0;
}
