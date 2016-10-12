#include <diablosmc.h>
#include <diabloi386.h>

void SetSectionFlags(Elf32_Word * flags)
{
  (*flags)|=SHF_WRITE;
}

/* vim: set shiftwidth=2 foldmethod=marker:*/
