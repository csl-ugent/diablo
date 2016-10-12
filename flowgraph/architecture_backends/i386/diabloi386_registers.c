#include <diabloi386.h>

t_string I386RegisterName(t_reg reg) 
{
  return i386_description.register_names[reg];
}

/* vim: set shiftwidth=2: */
