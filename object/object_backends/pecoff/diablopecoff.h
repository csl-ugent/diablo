/*=============================================================================
  Diablo PE/COFF back-end. 
  Based on Microsoft's PE/COFF Specification v8.2 (September 2010)

  (c) 2011 Stijn Volckaert (svolckae@elis.ugent.be)

  TODO:
  * Check if GCC has a built-in UNALIGNED definition
=============================================================================*/

#ifndef DIABLOPECOFF_H
#define DIABLOPECOFF_H
#include <diabloobject.h>
#pragma pack(push, 4)   // The default packing is 4-byte

/*-----------------------------------------------------------------------------
  Microsoft Datatypes
-----------------------------------------------------------------------------*/
typedef t_int8    SHORT;
typedef t_uint8   BYTE;
typedef t_uint16  WORD;
typedef t_int32   LONG;
typedef t_uint32  DWORD;
typedef t_uint32  ULONG;
typedef t_uint32  LONG_PTR;
typedef t_uint32  ULONG_PTR;
typedef t_uint32  DWORD_PTR;
typedef t_uint64  ULONGLONG;
#define UNALIGNED

/*-----------------------------------------------------------------------------
  VS 2010-2012 Hack
-----------------------------------------------------------------------------*/
#define _(x) x

/*-----------------------------------------------------------------------------
  File Characteristics
-----------------------------------------------------------------------------*/
#define IMAGE_FILE_RELOCS_STRIPPED                      0x0001        // Relocation info stripped from file.
#define IMAGE_FILE_EXECUTABLE_IMAGE                     0x0002        // File is executable  (i.e. no unresolved externel references).
#define IMAGE_FILE_LINE_NUMS_STRIPPED                   0x0004        // Line nunbers stripped from file.
#define IMAGE_FILE_LOCAL_SYMS_STRIPPED                  0x0008        // Local symbols stripped from file.
#define IMAGE_FILE_AGGRESIVE_WS_TRIM                    0x0010        // Agressively trim working set
#define IMAGE_FILE_LARGE_ADDRESS_AWARE                  0x0020        // App can handle >2gb addresses
#define IMAGE_FILE_BYTES_REVERSED_LO                    0x0080        // Bytes of machine word are reversed.
#define IMAGE_FILE_32BIT_MACHINE                        0x0100        // 32 bit word machine.
#define IMAGE_FILE_DEBUG_STRIPPED                       0x0200        // Debugging info stripped from file in .DBG file
#define IMAGE_FILE_REMOVABLE_RUN_FROM_SWAP              0x0400        // If Image is on removable media, copy and run from the swap file.
#define IMAGE_FILE_NET_RUN_FROM_SWAP                    0x0800        // If Image is on Net, copy and run from the swap file.
#define IMAGE_FILE_SYSTEM                               0x1000        // System File.
#define IMAGE_FILE_DLL                                  0x2000        // File is a DLL.
#define IMAGE_FILE_UP_SYSTEM_ONLY                       0x4000        // File should only be run on a UP machine
#define IMAGE_FILE_BYTES_REVERSED_HI                    0x8000        // Bytes of machine word are reversed.

/*-----------------------------------------------------------------------------
  Machine Types
-----------------------------------------------------------------------------*/
#define IMAGE_FILE_MACHINE_UNKNOWN                      0
#define IMAGE_FILE_MACHINE_I386                         0x014c        // Intel 386.
#define IMAGE_FILE_MACHINE_R3000                        0x0162        // MIPS little-endian, 0x160 big-endian
#define IMAGE_FILE_MACHINE_R4000                        0x0166        // MIPS little-endian
#define IMAGE_FILE_MACHINE_R10000                       0x0168        // MIPS little-endian
#define IMAGE_FILE_MACHINE_WCEMIPSV2                    0x0169        // MIPS little-endian WCE v2
#define IMAGE_FILE_MACHINE_ALPHA                        0x0184        // Alpha_AXP
#define IMAGE_FILE_MACHINE_SH3                          0x01a2        // SH3 little-endian
#define IMAGE_FILE_MACHINE_SH3DSP                       0x01a3
#define IMAGE_FILE_MACHINE_SH3E                         0x01a4        // SH3E little-endian
#define IMAGE_FILE_MACHINE_SH4                          0x01a6        // SH4 little-endian
#define IMAGE_FILE_MACHINE_SH5                          0x01a8        // SH5
#define IMAGE_FILE_MACHINE_ARM                          0x01c0        // ARM Little-Endian
#define IMAGE_FILE_MACHINE_THUMB                        0x01c2
#define IMAGE_FILE_MACHINE_AM33                         0x01d3
#define IMAGE_FILE_MACHINE_POWERPC                      0x01F0        // IBM PowerPC Little-Endian
#define IMAGE_FILE_MACHINE_POWERPCFP                    0x01f1
#define IMAGE_FILE_MACHINE_IA64                         0x0200        // Intel 64
#define IMAGE_FILE_MACHINE_MIPS16                       0x0266        // MIPS
#define IMAGE_FILE_MACHINE_ALPHA64                      0x0284        // ALPHA64
#define IMAGE_FILE_MACHINE_MIPSFPU                      0x0366        // MIPS
#define IMAGE_FILE_MACHINE_MIPSFPU16                    0x0466        // MIPS
#define IMAGE_FILE_MACHINE_AXP64                        IMAGE_FILE_MACHINE_ALPHA64
#define IMAGE_FILE_MACHINE_TRICORE                      0x0520        // Infineon
#define IMAGE_FILE_MACHINE_CEF                          0x0CEF
#define IMAGE_FILE_MACHINE_EBC                          0x0EBC        // EFI Byte Code
#define IMAGE_FILE_MACHINE_AMD64                        0x8664        // AMD64 (K8)
#define IMAGE_FILE_MACHINE_M32R                         0x9041        // M32R little-endian
#define IMAGE_FILE_MACHINE_CEE                          0xC0EE

/*-----------------------------------------------------------------------------
  Subsystem Types
-----------------------------------------------------------------------------*/
#define IMAGE_SUBSYSTEM_UNKNOWN                         0             // Unknown subsystem.
#define IMAGE_SUBSYSTEM_NATIVE                          1             // Image doesn't require a subsystem.
#define IMAGE_SUBSYSTEM_WINDOWS_GUI                     2             // Image runs in the Windows GUI subsystem.
#define IMAGE_SUBSYSTEM_WINDOWS_CUI                     3             // Image runs in the Windows character subsystem.
#define IMAGE_SUBSYSTEM_OS2_CUI                         5             // image runs in the OS/2 character subsystem.
#define IMAGE_SUBSYSTEM_POSIX_CUI                       7             // image runs in the Posix character subsystem.
#define IMAGE_SUBSYSTEM_NATIVE_WINDOWS                  8             // image is a native Win9x driver.
#define IMAGE_SUBSYSTEM_WINDOWS_CE_GUI                  9             // Image runs in the Windows CE subsystem.
#define IMAGE_SUBSYSTEM_EFI_APPLICATION                 10            //
#define IMAGE_SUBSYSTEM_EFI_BOOT_SERVICE_DRIVER         11            //
#define IMAGE_SUBSYSTEM_EFI_RUNTIME_DRIVER              12            //
#define IMAGE_SUBSYSTEM_EFI_ROM                         13
#define IMAGE_SUBSYSTEM_XBOX                            14
#define IMAGE_SUBSYSTEM_WINDOWS_BOOT_APPLICATION        16

/*-----------------------------------------------------------------------------
  DllCharacteristics
-----------------------------------------------------------------------------*/
#define IMAGE_DLLCHARACTERISTICS_DYNAMIC_BASE           0x0040        // DLL can move.
#define IMAGE_DLLCHARACTERISTICS_FORCE_INTEGRITY        0x0080        // Code Integrity Image
#define IMAGE_DLLCHARACTERISTICS_NX_COMPAT              0x0100        // Image is NX compatible
#define IMAGE_DLLCHARACTERISTICS_NO_ISOLATION           0x0200        // Image understands isolation and doesn't want it
#define IMAGE_DLLCHARACTERISTICS_NO_SEH                 0x0400        // Image does not use SEH.  No SE handler may reside in this image
#define IMAGE_DLLCHARACTERISTICS_NO_BIND                0x0800        // Do not bind this image.
#define IMAGE_DLLCHARACTERISTICS_WDM_DRIVER             0x2000        // Driver uses WDM model
#define IMAGE_DLLCHARACTERISTICS_TERMINAL_SERVER_AWARE  0x8000

/*-----------------------------------------------------------------------------
  Image Directory Types
-----------------------------------------------------------------------------*/
#define IMAGE_DIRECTORY_ENTRY_EXPORT                    0             // Export Directory
#define IMAGE_DIRECTORY_ENTRY_IMPORT                    1             // Import Directory
#define IMAGE_DIRECTORY_ENTRY_RESOURCE                  2             // Resource Directory
#define IMAGE_DIRECTORY_ENTRY_EXCEPTION                 3             // Exception Directory
#define IMAGE_DIRECTORY_ENTRY_SECURITY                  4             // Security Directory
#define IMAGE_DIRECTORY_ENTRY_BASERELOC                 5             // Base Relocation Table
#define IMAGE_DIRECTORY_ENTRY_DEBUG                     6             // Debug Directory
#define IMAGE_DIRECTORY_ENTRY_ARCHITECTURE              7             // Architecture Specific Data
#define IMAGE_DIRECTORY_ENTRY_GLOBALPTR                 8             // RVA of GP
#define IMAGE_DIRECTORY_ENTRY_TLS                       9             // TLS Directory
#define IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG               10            // Load Configuration Directory
#define IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT              11            // Bound Import Directory in headers
#define IMAGE_DIRECTORY_ENTRY_IAT                       12            // Import Address Table
#define IMAGE_DIRECTORY_ENTRY_DELAY_IMPORT              13            // Delay Load Import Descriptors
#define IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR            14            // COM Runtime descriptor

/*-----------------------------------------------------------------------------
  Image Section Characteristics
-----------------------------------------------------------------------------*/
#define IMAGE_SCN_TYPE_NO_PAD                           0x00000008    // Reserved.
#define IMAGE_SCN_CNT_CODE                              0x00000020    // Section contains code.
#define IMAGE_SCN_CNT_INITIALIZED_DATA                  0x00000040    // Section contains initialized data.
#define IMAGE_SCN_CNT_UNINITIALIZED_DATA                0x00000080    // Section contains uninitialized data.
#define IMAGE_SCN_LNK_OTHER                             0x00000100    // Reserved.
#define IMAGE_SCN_LNK_INFO                              0x00000200    // Section contains comments or some other type of information.
//      IMAGE_SCN_TYPE_OVER                             0x00000400    // Reserved.
#define IMAGE_SCN_LNK_REMOVE                            0x00000800    // Section contents will not become part of image.
#define IMAGE_SCN_LNK_COMDAT                            0x00001000    // Section contents comdat.
//                                                      0x00002000    // Reserved.
#define IMAGE_SCN_NO_DEFER_SPEC_EXC                     0x00004000    // Reset speculative exceptions handling bits in the TLB entries for this section.
#define IMAGE_SCN_GPREL                                 0x00008000    // Section content can be accessed relative to GP
#define IMAGE_SCN_MEM_FARDATA                           0x00008000
//      IMAGE_SCN_MEM_SYSHEAP  - Obsolete               0x00010000
#define IMAGE_SCN_MEM_PURGEABLE                         0x00020000
#define IMAGE_SCN_MEM_16BIT                             0x00020000
#define IMAGE_SCN_MEM_LOCKED                            0x00040000
#define IMAGE_SCN_MEM_PRELOAD                           0x00080000
#define IMAGE_SCN_ALIGN_1BYTES                          0x00100000    //
#define IMAGE_SCN_ALIGN_2BYTES                          0x00200000    //
#define IMAGE_SCN_ALIGN_4BYTES                          0x00300000    //
#define IMAGE_SCN_ALIGN_8BYTES                          0x00400000    //
#define IMAGE_SCN_ALIGN_16BYTES                         0x00500000    // Default alignment if no others are specified.
#define IMAGE_SCN_ALIGN_32BYTES                         0x00600000    //
#define IMAGE_SCN_ALIGN_64BYTES                         0x00700000    //
#define IMAGE_SCN_ALIGN_128BYTES                        0x00800000    //
#define IMAGE_SCN_ALIGN_256BYTES                        0x00900000    //
#define IMAGE_SCN_ALIGN_512BYTES                        0x00A00000    //
#define IMAGE_SCN_ALIGN_1024BYTES                       0x00B00000    //
#define IMAGE_SCN_ALIGN_2048BYTES                       0x00C00000    //
#define IMAGE_SCN_ALIGN_4096BYTES                       0x00D00000    //
#define IMAGE_SCN_ALIGN_8192BYTES                       0x00E00000    //
#define IMAGE_SCN_ALIGN_MASK                            0x00F00000
#define IMAGE_SCN_LNK_NRELOC_OVFL                       0x01000000    // Section contains extended relocations.
#define IMAGE_SCN_MEM_DISCARDABLE                       0x02000000    // Section can be discarded.
#define IMAGE_SCN_MEM_NOT_CACHED                        0x04000000    // Section is not cachable.
#define IMAGE_SCN_MEM_NOT_PAGED                         0x08000000    // Section is not pageable.
#define IMAGE_SCN_MEM_SHARED                            0x10000000    // Section is shareable.
#define IMAGE_SCN_MEM_EXECUTE                           0x20000000    // Section is executable.
#define IMAGE_SCN_MEM_READ                              0x40000000    // Section is readable.
#define IMAGE_SCN_MEM_WRITE                             0x80000000    // Section is writeable.
#define IMAGE_SCN_SCALE_INDEX                           0x00000001    // Tls index is scaled

/*-----------------------------------------------------------------------------
  Symbol Section Numbers

  Normally, the PIMAGE_SYMBOL->SectionNumber field points to the COFF section
  in which the symbol is defined. These are the special values the
  SectionNumber can have.
-----------------------------------------------------------------------------*/
#define IMAGE_SYM_UNDEFINED                             (SHORT)0      // Symbol is undefined or is common.
#define IMAGE_SYM_ABSOLUTE                              (SHORT)-1     // Symbol is an absolute value.
#define IMAGE_SYM_DEBUG                                 (SHORT)-2     // Symbol is a special debug item.
#define IMAGE_SYM_SECTION_MAX                           0xFEFF        // Values 0xFF00-0xFFFF are special
#define IMAGE_SYM_SECTION_MAX_EX                        0x7FFFFFFF    // = MAXLONG

/*-----------------------------------------------------------------------------
  Symbol Types
-----------------------------------------------------------------------------*/
#define IMAGE_SYM_TYPE_NULL                             0x0000        // no type.
#define IMAGE_SYM_TYPE_VOID                             0x0001        //
#define IMAGE_SYM_TYPE_CHAR                             0x0002        // type character.
#define IMAGE_SYM_TYPE_SHORT                            0x0003        // type short integer.
#define IMAGE_SYM_TYPE_INT                              0x0004        //
#define IMAGE_SYM_TYPE_LONG                             0x0005        //
#define IMAGE_SYM_TYPE_FLOAT                            0x0006        //
#define IMAGE_SYM_TYPE_DOUBLE                           0x0007        //
#define IMAGE_SYM_TYPE_STRUCT                           0x0008        //
#define IMAGE_SYM_TYPE_UNION                            0x0009        //
#define IMAGE_SYM_TYPE_ENUM                             0x000A        // enumeration.
#define IMAGE_SYM_TYPE_MOE                              0x000B        // member of enumeration.
#define IMAGE_SYM_TYPE_BYTE                             0x000C        //
#define IMAGE_SYM_TYPE_WORD                             0x000D        //
#define IMAGE_SYM_TYPE_UINT                             0x000E        //
#define IMAGE_SYM_TYPE_DWORD                            0x000F        //
#define IMAGE_SYM_TYPE_PCODE                            0x8000        //

/*-----------------------------------------------------------------------------
  Symbol Derived Types
-----------------------------------------------------------------------------*/
#define IMAGE_SYM_DTYPE_NULL                            0             // no derived type.
#define IMAGE_SYM_DTYPE_POINTER                         1             // pointer.
#define IMAGE_SYM_DTYPE_FUNCTION                        2             // function.
#define IMAGE_SYM_DTYPE_ARRAY                           3             // array.

/*-----------------------------------------------------------------------------
  Symbol Storage Classes
-----------------------------------------------------------------------------*/
#define IMAGE_SYM_CLASS_END_OF_FUNCTION                 (BYTE )-1
#define IMAGE_SYM_CLASS_NULL                            0x0000
#define IMAGE_SYM_CLASS_AUTOMATIC                       0x0001
#define IMAGE_SYM_CLASS_EXTERNAL                        0x0002
#define IMAGE_SYM_CLASS_STATIC                          0x0003
#define IMAGE_SYM_CLASS_REGISTER                        0x0004
#define IMAGE_SYM_CLASS_EXTERNAL_DEF                    0x0005
#define IMAGE_SYM_CLASS_LABEL                           0x0006
#define IMAGE_SYM_CLASS_UNDEFINED_LABEL                 0x0007
#define IMAGE_SYM_CLASS_MEMBER_OF_STRUCT                0x0008
#define IMAGE_SYM_CLASS_ARGUMENT                        0x0009
#define IMAGE_SYM_CLASS_STRUCT_TAG                      0x000A
#define IMAGE_SYM_CLASS_MEMBER_OF_UNION                 0x000B
#define IMAGE_SYM_CLASS_UNION_TAG                       0x000C
#define IMAGE_SYM_CLASS_TYPE_DEFINITION                 0x000D
#define IMAGE_SYM_CLASS_UNDEFINED_STATIC                0x000E
#define IMAGE_SYM_CLASS_ENUM_TAG                        0x000F
#define IMAGE_SYM_CLASS_MEMBER_OF_ENUM                  0x0010
#define IMAGE_SYM_CLASS_REGISTER_PARAM                  0x0011
#define IMAGE_SYM_CLASS_BIT_FIELD                       0x0012
#define IMAGE_SYM_CLASS_FAR_EXTERNAL                    0x0044
#define IMAGE_SYM_CLASS_BLOCK                           0x0064
#define IMAGE_SYM_CLASS_FUNCTION                        0x0065
#define IMAGE_SYM_CLASS_END_OF_STRUCT                   0x0066
#define IMAGE_SYM_CLASS_FILE                            0x0067
#define IMAGE_SYM_CLASS_SECTION                         0x0068
#define IMAGE_SYM_CLASS_WEAK_EXTERNAL                   0x0069
#define IMAGE_SYM_CLASS_CLR_TOKEN                       0x006B

/*-----------------------------------------------------------------------------
  Comdat Selection Types
-----------------------------------------------------------------------------*/
#define IMAGE_COMDAT_SELECT_NODUPLICATES                1
#define IMAGE_COMDAT_SELECT_ANY                         2
#define IMAGE_COMDAT_SELECT_SAME_SIZE                   3
#define IMAGE_COMDAT_SELECT_EXACT_MATCH                 4
#define IMAGE_COMDAT_SELECT_ASSOCIATIVE                 5
#define IMAGE_COMDAT_SELECT_LARGEST                     6
#define IMAGE_COMDAT_SELECT_NEWEST                      7

/*-----------------------------------------------------------------------------
  Weak External Linking Methods
-----------------------------------------------------------------------------*/
#define IMAGE_WEAK_EXTERN_SEARCH_NOLIBRARY              1
#define IMAGE_WEAK_EXTERN_SEARCH_LIBRARY                2
#define IMAGE_WEAK_EXTERN_SEARCH_ALIAS                  3

/*-----------------------------------------------------------------------------
  Relocation Types - The types for non-existing or unmaintained Diablo 
  back-ends have been stripped out. These are for object files
-----------------------------------------------------------------------------*/
//
// I386 relocation types.
//
#define IMAGE_REL_I386_ABSOLUTE                         0x0000        // Reference is absolute, no relocation is necessary
#define IMAGE_REL_I386_DIR16                            0x0001        // Direct 16-bit reference to the symbols virtual address
#define IMAGE_REL_I386_REL16                            0x0002        // PC-relative 16-bit reference to the symbols virtual address
#define IMAGE_REL_I386_DIR32                            0x0006        // Direct 32-bit reference to the symbols virtual address
#define IMAGE_REL_I386_DIR32NB                          0x0007        // Direct 32-bit reference to the symbols virtual address, base not included
#define IMAGE_REL_I386_SEG12                            0x0009        // Direct 16-bit reference to the segment-selector bits of a 32-bit virtual address
#define IMAGE_REL_I386_SECTION                          0x000A
#define IMAGE_REL_I386_SECREL                           0x000B
#define IMAGE_REL_I386_TOKEN                            0x000C        // clr token
#define IMAGE_REL_I386_SECREL7                          0x000D        // 7 bit offset from base of section containing target
#define IMAGE_REL_I386_REL32                            0x0014        // PC-relative 32-bit reference to the symbols virtual address

//
// ARM relocation types.
//
#define IMAGE_REL_ARM_ABSOLUTE                          0x0000        // No relocation required
#define IMAGE_REL_ARM_ADDR32                            0x0001        // 32 bit address
#define IMAGE_REL_ARM_ADDR32NB                          0x0002        // 32 bit address w/o image base
#define IMAGE_REL_ARM_BRANCH24                          0x0003        // 24 bit offset << 2 & sign ext.
#define IMAGE_REL_ARM_BRANCH11                          0x0004        // Thumb: 2 11 bit offsets
#define IMAGE_REL_ARM_TOKEN                             0x0005        // clr token
#define IMAGE_REL_ARM_GPREL12                           0x0006        // GP-relative addressing (ARM)
#define IMAGE_REL_ARM_GPREL7                            0x0007        // GP-relative addressing (Thumb)
#define IMAGE_REL_ARM_BLX24                             0x0008
#define IMAGE_REL_ARM_BLX11                             0x0009
#define IMAGE_REL_ARM_SECTION                           0x000E        // Section table index
#define IMAGE_REL_ARM_SECREL                            0x000F        // Offset within section

//
// x64 relocations
//
#define IMAGE_REL_AMD64_ABSOLUTE                        0x0000        // Reference is absolute, no relocation is necessary
#define IMAGE_REL_AMD64_ADDR64                          0x0001        // 64-bit address (VA).
#define IMAGE_REL_AMD64_ADDR32                          0x0002        // 32-bit address (VA).
#define IMAGE_REL_AMD64_ADDR32NB                        0x0003        // 32-bit address w/o image base (RVA).
#define IMAGE_REL_AMD64_REL32                           0x0004        // 32-bit relative address from byte following reloc
#define IMAGE_REL_AMD64_REL32_1                         0x0005        // 32-bit relative address from byte distance 1 from reloc
#define IMAGE_REL_AMD64_REL32_2                         0x0006        // 32-bit relative address from byte distance 2 from reloc
#define IMAGE_REL_AMD64_REL32_3                         0x0007        // 32-bit relative address from byte distance 3 from reloc
#define IMAGE_REL_AMD64_REL32_4                         0x0008        // 32-bit relative address from byte distance 4 from reloc
#define IMAGE_REL_AMD64_REL32_5                         0x0009        // 32-bit relative address from byte distance 5 from reloc
#define IMAGE_REL_AMD64_SECTION                         0x000A        // Section index
#define IMAGE_REL_AMD64_SECREL                          0x000B        // 32 bit offset from base of section containing target
#define IMAGE_REL_AMD64_SECREL7                         0x000C        // 7 bit unsigned offset from base of section containing target
#define IMAGE_REL_AMD64_TOKEN                           0x000D        // 32 bit metadata token
#define IMAGE_REL_AMD64_SREL32                          0x000E        // 32 bit signed span-dependent value emitted into object
#define IMAGE_REL_AMD64_PAIR                            0x000F
#define IMAGE_REL_AMD64_SSPAN32                         0x0010        // 32 bit signed span-dependent value applied at link time

/*-----------------------------------------------------------------------------
  Base Relocation Types - These are for image files.
-----------------------------------------------------------------------------*/
#define IMAGE_REL_BASED_ABSOLUTE                        0
#define IMAGE_REL_BASED_HIGH                            1
#define IMAGE_REL_BASED_LOW                             2
#define IMAGE_REL_BASED_HIGHLOW                         3
#define IMAGE_REL_BASED_HIGHADJ                         4
#define IMAGE_REL_BASED_MIPS_JMPADDR                    5
#define IMAGE_REL_BASED_MIPS_JMPADDR16                  9
#define IMAGE_REL_BASED_IA64_IMM64                      9
#define IMAGE_REL_BASED_DIR64                           10

/*-----------------------------------------------------------------------------
  Miscellaneous Constants
-----------------------------------------------------------------------------*/
#define IMAGE_SIZEOF_FILE_HEADER                        20
#define IMAGE_SIZEOF_SYMBOL                             18
#define IMAGE_SIZEOF_SHORT_NAME                         8
#define IMAGE_NUMBEROF_DIRECTORY_ENTRIES                16

/*-----------------------------------------------------------------------------
  PE Signatures
-----------------------------------------------------------------------------*/
#define IMAGE_DOS_SIGNATURE                             0x5A4D        // MZ
#define IMAGE_OS2_SIGNATURE                             0x454E        // NE
#define IMAGE_OS2_SIGNATURE_LE                          0x454C        // LE
#define IMAGE_VXD_SIGNATURE                             0x454C        // LE
#define IMAGE_NT_SIGNATURE                              0x00004550    // PE00
#define IMAGE_NT_OPTIONAL_HDR32_MAGIC                   0x10b         // => PE32
#define IMAGE_NT_OPTIONAL_HDR64_MAGIC                   0x20b         // => PE32+

/*-----------------------------------------------------------------------------
  Symbol Type Packing
-----------------------------------------------------------------------------*/
#define N_BTMASK                                        0x000F
#define N_TMASK                                         0x0030
#define N_TMASK1                                        0x00C0
#define N_TMASK2                                        0x00F0
#define N_BTSHFT                                        4
#define N_TSHIFT                                        2

/*-----------------------------------------------------------------------------
  Macros
-----------------------------------------------------------------------------*/
//
// Section Browsing Macros
//
/* 
GCC might complain about this macro so here's a lame workaround:
#define FIELD_OFFSET(type, field) ((LONG)(LONG_PTR)&(((type *)1)->field)-1)
*/
#define FIELD_OFFSET(type, field) ((LONG)(LONG_PTR)&(((type *)0)->field))
#define IMAGE_FIRST_SECTION( ntheader ) ((PIMAGE_SECTION_HEADER)        \
    ((ULONG_PTR)(ntheader) +                                            \
     FIELD_OFFSET( IMAGE_NT_HEADERS, OptionalHeader ) +                 \
     ((ntheader))->FileHeader.SizeOfOptionalHeader                      \
    ))

//
// Bit Selection Macros
//
#define LOWORD(l)           ((WORD)(((DWORD_PTR)(l)) & 0xffff))
#define HIWORD(l)           ((WORD)((((DWORD_PTR)(l)) >> 16) & 0xffff))
#define LOBYTE(w)           ((BYTE)(((DWORD_PTR)(w)) & 0xff))
#define HIBYTE(w)           ((BYTE)((((DWORD_PTR)(w)) >> 8) & 0xff))

// 
// Symbol Identification Macros
//
// Basic Type of  x
#define BTYPE(x) ((x) & N_BTMASK)

// Is x a pointer?
#ifndef ISPTR
#define ISPTR(x) (((x) & N_TMASK) == (IMAGE_SYM_DTYPE_POINTER << N_BTSHFT))
#endif

// Is x a function?
#ifndef ISFCN
#define ISFCN(x) (((x) & N_TMASK) == (IMAGE_SYM_DTYPE_FUNCTION << N_BTSHFT))
#endif

// Is x an array?
#ifndef ISARY
#define ISARY(x) (((x) & N_TMASK) == (IMAGE_SYM_DTYPE_ARRAY << N_BTSHFT))
#endif

// Is x a structure, union, or enumeration TAG?
#ifndef ISTAG
#define ISTAG(x) ((x)==IMAGE_SYM_CLASS_STRUCT_TAG || (x)==IMAGE_SYM_CLASS_UNION_TAG || (x)==IMAGE_SYM_CLASS_ENUM_TAG)
#endif

#ifndef INCREF
#define INCREF(x) ((((x)&~N_BTMASK)<<N_TSHIFT)|(IMAGE_SYM_DTYPE_POINTER<<N_BTSHFT)|((x)&N_BTMASK))
#endif
#ifndef DECREF
#define DECREF(x) ((((x)>>N_TSHIFT)&~N_BTMASK)|((x)&N_BTMASK))
#endif

/*-----------------------------------------------------------------------------
  _IMAGE_DOS_HEADER - This is placed in front of the COFF-header in every 
  executable file (.exe, .dll, .vxd, ...). This header is just here to maintain 
  compatibility with the almighty MS-DOS operating system.

  Present in:
  * Image files only
-----------------------------------------------------------------------------*/
#pragma pack(push, 2)                 // 2-byte packing for 16-bit structures
typedef struct _IMAGE_DOS_HEADER 
{
  WORD   e_magic;                     // Magic number
  WORD   e_cblp;                      // Bytes on last page of file
  WORD   e_cp;                        // Pages in file
  WORD   e_crlc;                      // Relocations
  WORD   e_cparhdr;                   // Size of header in paragraphs
  WORD   e_minalloc;                  // Minimum extra paragraphs needed
  WORD   e_maxalloc;                  // Maximum extra paragraphs needed
  WORD   e_ss;                        // Initial (relative) SS value
  WORD   e_sp;                        // Initial SP value
  WORD   e_csum;                      // Checksum
  WORD   e_ip;                        // Initial IP value
  WORD   e_cs;                        // Initial (relative) CS value
  WORD   e_lfarlc;                    // File address of relocation table
  WORD   e_ovno;                      // Overlay number
  WORD   e_res[4];                    // Reserved words
  WORD   e_oemid;                     // OEM identifier (for e_oeminfo)
  WORD   e_oeminfo;                   // OEM information; e_oemid specific
  WORD   e_res2[10];                  // Reserved words
  LONG   e_lfanew;                    // File address of new exe header
} IMAGE_DOS_HEADER, *PIMAGE_DOS_HEADER;
#pragma pack(pop)                     // disable 2-byte packing

/*-----------------------------------------------------------------------------
  _IMAGE_FILE_HEADER - Also known as the COFF header

  Present in:
  * Image and Object files  
-----------------------------------------------------------------------------*/
typedef struct _IMAGE_FILE_HEADER 
{
  WORD    Machine;
  WORD    NumberOfSections;
  DWORD   TimeDateStamp;
  DWORD   PointerToSymbolTable;
  DWORD   NumberOfSymbols;
  WORD    SizeOfOptionalHeader;
  WORD    Characteristics;
} IMAGE_FILE_HEADER, *PIMAGE_FILE_HEADER;

/*-----------------------------------------------------------------------------
  _IMAGE_OPTIONAL_HEADER - This header was present in the COFF standard and
  was later extended by Microsoft

  Present in:
  * Image files
  * Object files (COFF Fields only. Object files do not contain Microsoft
  specific fields. This obviously means that Object files don't have data
  directories either.)
-----------------------------------------------------------------------------*/
typedef struct _IMAGE_DATA_DIRECTORY 
{
  DWORD   VirtualAddress;
  DWORD   Size;
} IMAGE_DATA_DIRECTORY, *PIMAGE_DATA_DIRECTORY;

typedef struct _IMAGE_OPTIONAL_HEADER 
{
  //
  // COFF Fields
  //
  WORD    Magic;
  BYTE    MajorLinkerVersion;
  BYTE    MinorLinkerVersion;
  DWORD   SizeOfCode;
  DWORD   SizeOfInitializedData;
  DWORD   SizeOfUninitializedData;
  DWORD   AddressOfEntryPoint;
  DWORD   BaseOfCode;
  DWORD   BaseOfData;

  // 
  // Microsoft COFF extensions
  //
  DWORD   ImageBase;
  DWORD   SectionAlignment;
  DWORD   FileAlignment;
  WORD    MajorOperatingSystemVersion;
  WORD    MinorOperatingSystemVersion;
  WORD    MajorImageVersion;
  WORD    MinorImageVersion;
  WORD    MajorSubsystemVersion;
  WORD    MinorSubsystemVersion;
  DWORD   Win32VersionValue;
  DWORD   SizeOfImage;
  DWORD   SizeOfHeaders;
  DWORD   CheckSum;
  WORD    Subsystem;
  WORD    DllCharacteristics;
  DWORD   SizeOfStackReserve;
  DWORD   SizeOfStackCommit;
  DWORD   SizeOfHeapReserve;
  DWORD   SizeOfHeapCommit;
  DWORD   LoaderFlags;
  DWORD   NumberOfRvaAndSizes;
  IMAGE_DATA_DIRECTORY DataDirectory[IMAGE_NUMBEROF_DIRECTORY_ENTRIES];
} IMAGE_OPTIONAL_HEADER32, *PIMAGE_OPTIONAL_HEADER32;

/*-----------------------------------------------------------------------------
  _IMAGE_OPTIONAL_HEADER64 - This is the Optional header for PE32+ files.
  Unlike the regular PE32 executables, PE32+ executables are large address
  space aware (which means they can run in native AMD64 mode). 
  Besides that, they're very similar.

  Present in:
  * See _IMAGE_OPTIONAL_HEADER
-----------------------------------------------------------------------------*/
typedef struct _IMAGE_OPTIONAL_HEADER64 
{
  WORD        Magic;
  BYTE        MajorLinkerVersion;
  BYTE        MinorLinkerVersion;
  DWORD       SizeOfCode;
  DWORD       SizeOfInitializedData;
  DWORD       SizeOfUninitializedData;
  DWORD       AddressOfEntryPoint;
  DWORD       BaseOfCode;
  ULONGLONG   ImageBase;
  DWORD       SectionAlignment;
  DWORD       FileAlignment;
  WORD        MajorOperatingSystemVersion;
  WORD        MinorOperatingSystemVersion;
  WORD        MajorImageVersion;
  WORD        MinorImageVersion;
  WORD        MajorSubsystemVersion;
  WORD        MinorSubsystemVersion;
  DWORD       Win32VersionValue;
  DWORD       SizeOfImage;
  DWORD       SizeOfHeaders;
  DWORD       CheckSum;
  WORD        Subsystem;
  WORD        DllCharacteristics;
  ULONGLONG   SizeOfStackReserve;
  ULONGLONG   SizeOfStackCommit;
  ULONGLONG   SizeOfHeapReserve;
  ULONGLONG   SizeOfHeapCommit;
  DWORD       LoaderFlags;
  DWORD       NumberOfRvaAndSizes;
  IMAGE_DATA_DIRECTORY DataDirectory[IMAGE_NUMBEROF_DIRECTORY_ENTRIES];
} IMAGE_OPTIONAL_HEADER64, *PIMAGE_OPTIONAL_HEADER64;

/*-----------------------------------------------------------------------------
  Optional Header Typedefs

  TODO: Add DIABLOOBJECT_PECOFF_WIN64 to diabloobject_config.h
-----------------------------------------------------------------------------*/
#ifdef DIABLOOBJECT_PECOFF_WIN64
typedef IMAGE_OPTIONAL_HEADER64             IMAGE_OPTIONAL_HEADER;
typedef PIMAGE_OPTIONAL_HEADER64            PIMAGE_OPTIONAL_HEADER;
#define IMAGE_NT_OPTIONAL_HDR_MAGIC         IMAGE_NT_OPTIONAL_HDR64_MAGIC
#else
typedef IMAGE_OPTIONAL_HEADER32             IMAGE_OPTIONAL_HEADER;
typedef PIMAGE_OPTIONAL_HEADER32            PIMAGE_OPTIONAL_HEADER;
#define IMAGE_NT_OPTIONAL_HDR_MAGIC         IMAGE_NT_OPTIONAL_HDR32_MAGIC
#endif

/*-----------------------------------------------------------------------------
  _IMAGE_NT_HEADERS - These headers immediately follow the DOS header in image
  files. For obvious reasons, these headers look slightly different in PE32+
  files (different optional header lay-out).

  Present in:
  * Image files only
-----------------------------------------------------------------------------*/
typedef struct _IMAGE_NT_HEADERS64 
{
  DWORD Signature;
  IMAGE_FILE_HEADER FileHeader;
  IMAGE_OPTIONAL_HEADER64 OptionalHeader;
} IMAGE_NT_HEADERS64, *PIMAGE_NT_HEADERS64;

typedef struct _IMAGE_NT_HEADERS 
{
  DWORD Signature;
  IMAGE_FILE_HEADER FileHeader;
  IMAGE_OPTIONAL_HEADER32 OptionalHeader;
} IMAGE_NT_HEADERS32, *PIMAGE_NT_HEADERS32;

#ifdef DIABLOOBJECT_PECOFF_WIN64
typedef IMAGE_NT_HEADERS64                  IMAGE_NT_HEADERS;
typedef PIMAGE_NT_HEADERS64                 PIMAGE_NT_HEADERS;
#else
typedef IMAGE_NT_HEADERS32                  IMAGE_NT_HEADERS;
typedef PIMAGE_NT_HEADERS32                 PIMAGE_NT_HEADERS;
#endif

/*-----------------------------------------------------------------------------
  _IMAGE_SECTION_HEADER - An array of _IMAGE_FILE_HEADER->NumberOfSections
  immediately follows the optional header. This is a standard COFF-header
  (no Microsoft extensions).

  Present in:
  * Image and Object files
-----------------------------------------------------------------------------*/
typedef struct _IMAGE_SECTION_HEADER 
{
  /* 
  From Section 3, "Section Table (Section Headers)", in the PE/COFF Spec v8:

  An 8-byte, null-padded UTF-8 encoded string. If the string is exactly 8 
  characters long, there is no terminating null. For longer names, this field 
  contains a slash (/) that is followed by an ASCII representation of a decimal 
  number that is an offset into the string table. Executable images do not use a 
  string table and do not support section names longer than 8 characters. 
  Long names in object files are truncated if they are emitted to an 
  executable file.
  */
  BYTE    Name[IMAGE_SIZEOF_SHORT_NAME];
  union 
  {
    DWORD   PhysicalAddress;
    DWORD   VirtualSize;
  } Misc;
  DWORD   VirtualAddress;
  DWORD   SizeOfRawData;
  DWORD   PointerToRawData;
  DWORD   PointerToRelocations;
  DWORD   PointerToLinenumbers;    // COFF line numbers are deprecated
  WORD    NumberOfRelocations;
  WORD    NumberOfLinenumbers;
  DWORD   Characteristics;
} IMAGE_SECTION_HEADER, *PIMAGE_SECTION_HEADER;

/*-----------------------------------------------------------------------------
  _IMAGE_SYMBOL - Object files may contain an array of COFF Symbols. This array
  is pointed to by the _IMAGE_FILE_HEADER->PointerToSymbolTable field and has
  _IMAGE_FILE_HEADER->NumberOfSymbols entries. There are no restrictions on 
  the exact location of this table within the object.

  Do note that there is a distinction between normal and auxiliary COFF Symbol
  records. Certain normal symbols may be followed by auxiliary symbol records.
  These auxiliary symbol records contain extra information about the symbol.

  Refer to Section 4.5, "Auxiliary Symbol Records", in the PE/COFF Spec v8 for
  additional information on auxiliary symbols.

  Present in:
  * Possible in Image files but uncommon
  * Object files
-----------------------------------------------------------------------------*/
#pragma pack(push, 2)              // Symbols, relocs and linenumbers are 2-byte packed
typedef struct _IMAGE_SYMBOL 
{
  // N can be either a name by itself or an offset into the string table
  union 
  {
    BYTE    ShortName[8];
    struct 
    {
      DWORD   Short;               // if 0, use LongName
      DWORD   Long;                // offset into string table
    } Name;
    DWORD   LongName[2];           // PBYTE [2]
  } N;
  DWORD   Value;
  SHORT   SectionNumber;
  WORD    Type;
  BYTE    StorageClass;
  BYTE    NumberOfAuxSymbols;
} IMAGE_SYMBOL;
typedef IMAGE_SYMBOL UNALIGNED *PIMAGE_SYMBOL;
#pragma pack(pop)

/*-----------------------------------------------------------------------------
  IMAGE_AUX_SYMBOL_FUNCTION_DEF - See Section 4.5.1:
  "Auxiliary Format 1: Function Definitions"

  Present in:
  * Possible in Image files but uncommon
  * Object files
-----------------------------------------------------------------------------*/
typedef struct IMAGE_AUX_SYMBOL_FUNCTION_DEF
{
  DWORD TagIndex;
  DWORD TotalSize;
  DWORD PointerToLineNumber;
  DWORD PointerToNextFunction;
  BYTE  Padding[2];                // Unused
} IMAGE_AUX_SYMBOL_FUNCTION_DEF;
typedef IMAGE_AUX_SYMBOL_FUNCTION_DEF UNALIGNED *PIMAGE_AUX_SYMBOL_FUNCTION_DEF;

/*-----------------------------------------------------------------------------
  IMAGE_AUX_SYMBOL_BF_EF_SYM - See Section 4.5.2:
  "Auxiliary Format 2: .bf and .ef Symbols"

  Present in:
  * Possible in Image files but uncommon
  * Object files
-----------------------------------------------------------------------------*/
typedef struct IMAGE_AUX_SYMBOL_BF_EF_SYM
{
  BYTE  Padding1[4];               // Unused
  WORD  LineNumber;
  BYTE  Padding2[6];               // Unused
  DWORD PointerToNextFunction;
  BYTE  Padding3[2];               // Unused
} IMAGE_AUX_SYMBOL_BF_EF_SYM;
typedef IMAGE_AUX_SYMBOL_BF_EF_SYM UNALIGNED *PIMAGE_AUX_SYMBOL_BF_EF_SYM;

/*-----------------------------------------------------------------------------
  IMAGE_AUX_SYMBOL_WEAK_EXTERNAL - See Section 4.5.3:
  "Auxiliary Format 3: Weak Externals"

  Present in:
  * Object files
-----------------------------------------------------------------------------*/
typedef struct IMAGE_AUX_SYMBOL_WEAK_EXTERNAL
{
  DWORD TagIndex;
  DWORD Characteristics;
  BYTE  Padding[10];               // Unused
} IMAGE_AUX_SYMBOL_WEAK_EXTERNAL;
typedef IMAGE_AUX_SYMBOL_WEAK_EXTERNAL UNALIGNED *PIMAGE_AUX_SYMBOL_WEAK_EXTERNAL;

/*-----------------------------------------------------------------------------
  IMAGE_AUX_SYMBOL_FILE - See Section 4.5.4:
  "Auxiliary Format 4: Files"

  Present in:
  * Possible in Image files but uncommon
  * Object files
-----------------------------------------------------------------------------*/
typedef struct IMAGE_AUX_SYMBOL_FILE
{
  BYTE  FileName[IMAGE_SIZEOF_SYMBOL];
} IMAGE_AUX_SYMBOL_FILE;
typedef IMAGE_AUX_SYMBOL_FILE UNALIGNED *PIMAGE_AUX_SYMBOL_FILE;

/*-----------------------------------------------------------------------------
  IMAGE_AUX_SYMBOL_SECTION_DEF - See Section 4.5.5:
  "Auxiliary Format 5: Section Definitions"

  Present in:
  * Possible in Image files but uncommon
  * Object files
-----------------------------------------------------------------------------*/
typedef struct IMAGE_AUX_SYMBOL_SECTION_DEF
{
  DWORD   Length;                  // section length
  WORD    NumberOfRelocations;     // number of relocation entries
  WORD    NumberOfLinenumbers;     // number of line numbers
  DWORD   CheckSum;                // checksum for communal
  SHORT   Number;                  // section number to associate with
  BYTE    Selection;               // communal selection type
  BYTE    bReserved;
  SHORT   HighNumber;              // high bits of the section number
} IMAGE_AUX_SYMBOL_SECTION_DEF;
typedef IMAGE_AUX_SYMBOL_SECTION_DEF UNALIGNED *PIMAGE_AUX_SYMBOL_SECTION_DEF;

/*-----------------------------------------------------------------------------
  IMAGE_AUX_SYMBOL_TOKEN_DEF - See Section 4.5.7:
  "CLR Token Definition (Object Only)"

  Present in:
  * Object files
-----------------------------------------------------------------------------*/
#pragma pack(push, 2)
typedef struct IMAGE_AUX_SYMBOL_TOKEN_DEF 
{
  BYTE  bAuxType;                  // Must be 1
  BYTE  bReserved;                 // Must be 0
  DWORD SymbolTableIndex;
  BYTE  rgbReserved[12];           // Must be 0
} IMAGE_AUX_SYMBOL_TOKEN_DEF;
typedef IMAGE_AUX_SYMBOL_TOKEN_DEF UNALIGNED *PIMAGE_AUX_SYMBOL_TOKEN_DEF;
#pragma pack(pop)

/*-----------------------------------------------------------------------------
  _IMAGE_AUX_SYMBOL - Auxiliary Symbol Record. Auxiliary Symbol Records contain
  extra information about a certain symbol. They immediately follow and apply
  to a symbol in the symbol table.

  The contents of the record depend on the symbol the record applies to, but
  in any case, the size of the record is 18 (= sizeof(_IMAGE_SYMBOL)).

  I redefined this structure because Microsoft's definition made my eyes bleed.

  Present in:
  * Possible in Image files but uncommon
  * Object files
-----------------------------------------------------------------------------*/
typedef union _IMAGE_AUX_SYMBOL 
{
  IMAGE_AUX_SYMBOL_FUNCTION_DEF   Function;
  IMAGE_AUX_SYMBOL_BF_EF_SYM      BfEfSymbol;
  IMAGE_AUX_SYMBOL_WEAK_EXTERNAL  WeakExternal;
  IMAGE_AUX_SYMBOL_FILE           File;
  IMAGE_AUX_SYMBOL_SECTION_DEF    Section;
  IMAGE_AUX_SYMBOL_SECTION_DEF    ComdatSection;
  IMAGE_AUX_SYMBOL_TOKEN_DEF      CLRToken;
} IMAGE_AUX_SYMBOL;
typedef IMAGE_AUX_SYMBOL UNALIGNED *PIMAGE_AUX_SYMBOL;

/*-----------------------------------------------------------------------------
  _IMAGE_RELOCATION - Each section in a COFF file may have a number of 
  relocations associated with it. The array of relocations is pointed to by
  the _IMAGE_SECTION_HEADER->PointerToRelocations field and contains
  _IMAGE_SECTION_HEADER->NumberOfRelocations entries.

  Note that Image files do not contain COFF relocations. If Image files contain
  position dependent code or data, the dynamic linker will adjust the code/data
  using the fixups (aka dynamic relocations) in the PE base relocation table.

  Present in:
  * Object files only
-----------------------------------------------------------------------------*/
#pragma pack(push, 2)
typedef struct _IMAGE_RELOCATION 
{
  union 
  {
    DWORD   VirtualAddress;
    DWORD   RelocCount;            // Set to the real count when IMAGE_SCN_LNK_NRELOC_OVFL is set
  } DUMMYUNIONNAME;
  DWORD   SymbolTableIndex;
  WORD    Type;
} IMAGE_RELOCATION;
#pragma pack(pop)
typedef IMAGE_RELOCATION UNALIGNED *PIMAGE_RELOCATION;

/*-----------------------------------------------------------------------------
  IMPORT_OBJECT_HEADER - 

  Present in:
  * Object files only
-----------------------------------------------------------------------------*/
#define IMPORT_OBJECT_HDR_SIG2  0xffff
typedef struct IMPORT_OBJECT_HEADER 
{
  WORD    Sig1;                    // Must be IMAGE_FILE_MACHINE_UNKNOWN
  WORD    Sig2;                    // Must be IMPORT_OBJECT_HDR_SIG2.
  WORD    Version;
  WORD    Machine;
  DWORD   TimeDateStamp;           // Time/date stamp
  DWORD   SizeOfData;              // particularly useful for incremental links
  union 
  {
      WORD    Ordinal;             // if grf & IMPORT_OBJECT_ORDINAL
      WORD    Hint;
  } DUMMYUNIONNAME;
  WORD    Type : 2;                // IMPORT_TYPE
  WORD    NameType : 3;            // IMPORT_NAME_TYPE
  WORD    Reserved : 11;           // Reserved. Must be zero.
} IMPORT_OBJECT_HEADER;

typedef enum IMPORT_OBJECT_TYPE
{
  IMPORT_OBJECT_CODE    = 0,
  IMPORT_OBJECT_DATA    = 1,
  IMPORT_OBJECT_CONST   = 2,
} IMPORT_OBJECT_TYPE;

typedef enum IMPORT_OBJECT_NAME_TYPE
{
  IMPORT_OBJECT_ORDINAL         = 0,  // Import by ordinal
  IMPORT_OBJECT_NAME            = 1,  // Import name == public symbol name.
  IMPORT_OBJECT_NAME_NO_PREFIX  = 2,  // Import name == public symbol name skipping leading ?, @, or optionally _.
  IMPORT_OBJECT_NAME_UNDECORATE = 3,  // Import name == public symbol name skipping leading ?, @, or optionally _
                                      // and truncating at first @
} IMPORT_OBJECT_NAME_TYPE;

/*-----------------------------------------------------------------------------
  _IMAGE_BASE_RELOCATION - The .reloc section in an image file is nothing more
  than an array of fixups. Fixups are applied if the image was loaded at a
  base address that differs from its preferred base.

  The base relocations table is pointed to by the
  _IMAGE_OPTIONAL_HEADER->DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC] field.

  The lay-out of the fixup table is as follows:

    DWORD PIMAGE_BASE_RELOCATION.VirtualAddress -> VA of the block the relocs belong to
    DWORD PIMAGE_BASE_RELOCATION.SizeOfBlock    -> Size of this block of relocations (including the base_relocation - which is 2 DWORDS - itself)
    WORD  reloc0                                -> Offset of the instruction that needs to be fixed (relative to the VA specified by the block)
    WORD  reloc1
    ...
    DWORD PIMAGE_BASE_RELOCATION.VA
    DWORD PIMAGE_BASE_RELOCATION.SizeOfBlock
    WORD
    WORD
    ...
	
  Example:
  Suppose the VA = 0x1000 and reloc1 = 0x037 
  ==> address that is at VA 0x1037 needs relocation (convert to offset to apply on disk image)

  Present in:
  * Image files only
-----------------------------------------------------------------------------*/
typedef struct _IMAGE_BASE_RELOCATION 
{
  DWORD   VirtualAddress;
  DWORD   SizeOfBlock;
} IMAGE_BASE_RELOCATION;
typedef IMAGE_BASE_RELOCATION UNALIGNED * PIMAGE_BASE_RELOCATION;

/*-----------------------------------------------------------------------------
  IMAGE_EXPORT_DIRECTORY - Documented in Section 5.3 ("The .edata Section")

  The PE export tables are similar to the ELF dynsym table!

  Present in:
  * Image files only
-----------------------------------------------------------------------------*/
typedef struct _IMAGE_EXPORT_DIRECTORY 
{
  DWORD   Characteristics;
  DWORD   TimeDateStamp;
  WORD    MajorVersion;
  WORD    MinorVersion;
  DWORD   Name;
  DWORD   Base;
  DWORD   NumberOfFunctions;
  DWORD   NumberOfNames;
  DWORD   AddressOfFunctions;     // RVA from base of image
  DWORD   AddressOfNames;         // RVA from base of image
  DWORD   AddressOfNameOrdinals;  // RVA from base of image
} IMAGE_EXPORT_DIRECTORY, *PIMAGE_EXPORT_DIRECTORY;

/*-----------------------------------------------------------------------------
  IMAGE_IMPORT_DESCRIPTOR - Described in Section 5.4: "The .idata section"
-----------------------------------------------------------------------------*/
typedef struct _IMAGE_IMPORT_DESCRIPTOR 
{
  union 
  {
    DWORD   Characteristics;              // 0 for terminating null import descriptor
    DWORD   OriginalFirstThunk;           // RVA to original unbound IAT (PIMAGE_THUNK_DATA)
  } DUMMYUNIONNAME;
  DWORD   TimeDateStamp;                  // 0 if not bound,
  // -1 if bound, and real date\time stamp
  //     in IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT (new BIND)
  // O.W. date/time stamp of DLL bound to (Old BIND)
  DWORD   ForwarderChain;                 // -1 if no forwarders
  DWORD   Name;
  DWORD   FirstThunk;                     // RVA to IAT (if bound this IAT has actual addresses)
  } IMAGE_IMPORT_DESCRIPTOR;
typedef IMAGE_IMPORT_DESCRIPTOR UNALIGNED *PIMAGE_IMPORT_DESCRIPTOR;

typedef struct _IMAGE_IMPORT_BY_NAME 
{
  WORD    Hint;
  BYTE    Name[1];
} IMAGE_IMPORT_BY_NAME, *PIMAGE_IMPORT_BY_NAME;

typedef struct _IMAGE_THUNK_DATA32 
{
  union 
  {
    DWORD ForwarderString;      // PBYTE 
    DWORD Function;             // PDWORD
    DWORD Ordinal;
    DWORD AddressOfData;        // PIMAGE_IMPORT_BY_NAME
  } u1;
} IMAGE_THUNK_DATA32;
typedef IMAGE_THUNK_DATA32 * PIMAGE_THUNK_DATA32;

#define IMAGE_ORDINAL_FLAG32 0x80000000
#define IMAGE_ORDINAL32(Ordinal) (Ordinal & 0xffff)

/*-----------------------------------------------------------------------------
  Public Function Prototypes
-----------------------------------------------------------------------------*/
void  DiabloPeCoffInit(int, char **);
void  DiabloPeCoffFini();
void  PeRead(FILE * fp, t_object * obj, t_bool read_debug);
DWORD PeRvaToOffset(void* pImage, DWORD dwRva);
DWORD PeOffsetToRva(void* pImage, DWORD dwOffset);

#pragma pack(pop)
#endif