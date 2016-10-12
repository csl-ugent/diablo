/*=============================================================================
	Diablo PE/COFF back-end. 
  Based on Microsoft's PE/COFF Specification v8.2 (September 2010)

  (c) 2011 Stijn Volckaert (svolckae@elis.ugent.be)  
=============================================================================*/

#include <diabloobject.h>
#include "diablopecoff.h"

/*-----------------------------------------------------------------------------
	PeRvaToOffset - Converts a Relative Virtual Address to a File Offset
  @return -1 on error
-----------------------------------------------------------------------------*/
DWORD PeRvaToOffset(void* pImage, DWORD dwRva)
{
  PIMAGE_DOS_HEADER     pDOSHeader      = NULL;
  PIMAGE_NT_HEADERS     pNTHeader       = NULL;		
  PIMAGE_SECTION_HEADER pSectionHeader  = NULL;
  DWORD                 dwSectionNum    = NULL;
  DWORD                 i;
  
  pDOSHeader = (PIMAGE_DOS_HEADER)pImage;  
  if (pDOSHeader->e_magic != IMAGE_DOS_SIGNATURE)
    return -1;

  pNTHeader = (PIMAGE_NT_HEADERS)((LONG)pDOSHeader + pDOSHeader->e_lfanew);
  if (pNTHeader->Signature != LOWORD(IMAGE_NT_SIGNATURE))
    return -1;		

  pSectionHeader = IMAGE_FIRST_SECTION(pNTHeader);
  dwSectionNum = pNTHeader->FileHeader.NumberOfSections;

  for (i = 0; i < dwSectionNum; ++i)
  {			
    /* Check if rva is within this section */
    if (pSectionHeader->VirtualAddress <= dwRva && pSectionHeader->Misc.VirtualSize + pSectionHeader->VirtualAddress > dwRva)
    {				
      /* Calculate offset relative to RVA of section */
      dwRva -= pSectionHeader->VirtualAddress;
      /* Now add the RVA of the section */
      dwRva += pSectionHeader->PointerToRawData;
      /* And return */
      return dwRva;
    }

    pSectionHeader++;
  }  

  /* Invalid offset => -1 */
  return -1;
}

/*-----------------------------------------------------------------------------
	PeOffsetToRva - Converts a File Offset to a Relative Virtual Address
  @return -1 on error
-----------------------------------------------------------------------------*/
DWORD PeOffsetToRva(void* pImage, DWORD dwOffset)
{
  PIMAGE_DOS_HEADER     pDOSHeader      = NULL;
  PIMAGE_NT_HEADERS     pNTHeader       = NULL;		
  PIMAGE_SECTION_HEADER pSectionHeader  = NULL;
  DWORD                 dwSectionNum    = NULL;
  DWORD                 i;
				
  pDOSHeader = (PIMAGE_DOS_HEADER)pImage;  
  if (pDOSHeader->e_magic != IMAGE_DOS_SIGNATURE)
    return -1;

  pNTHeader = (PIMAGE_NT_HEADERS)((LONG)pDOSHeader + pDOSHeader->e_lfanew);
  if (pNTHeader->Signature != LOWORD(IMAGE_NT_SIGNATURE))
    return -1;		

  pSectionHeader = IMAGE_FIRST_SECTION(pNTHeader);
  dwSectionNum = pNTHeader->FileHeader.NumberOfSections;

  for (i = 0; i < dwSectionNum; ++i)
  {
    /* Check if offset is within this section */
    if (pSectionHeader->PointerToRawData <= dwOffset && pSectionHeader->SizeOfRawData + pSectionHeader->PointerToRawData > dwOffset)
    {
      /* Calculate offset relative to the start of the section */
      dwOffset -= pSectionHeader->PointerToRawData;
      /* Now add the RVA of the section */
      dwOffset += pSectionHeader->VirtualAddress;
      /* And return */
      return dwOffset;
    }

    pSectionHeader++;
  }

  /* Invalid offset => -1 */
  return -1;
}