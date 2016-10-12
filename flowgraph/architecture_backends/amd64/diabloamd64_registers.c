#include <diabloamd64.h>

t_string Amd64RegisterName(t_reg reg) 
{
  return amd64_description.register_names[reg];
}
/* vim: set shiftwidth=2: */
