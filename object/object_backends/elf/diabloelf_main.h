/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

/* The development of portions of the code contained in this file was sponsored by Samsung Electronics UK. */

#include <diabloelf.h> 
#ifndef DIABLOELF_MAIN_DEFINES
#define DIABLOELF_MAIN_DEFINES

#define EI_MAG0    0
#define EI_MAG1    1
#define EI_MAG2    2
#define EI_MAG3    3
#define EI_CLASS   4
#define EI_DATA    5
#define EI_VERSION 6
#define EI_PAD     7
#define EI_NIDENT 16

#define ET_NONE   0
#define ET_REL    1
#define ET_EXEC   2
#define ET_DYN    3
#define ET_CORE   4
#define ET_LOPROC 0xff00
#define ET_HIPROC 0xffff

#define EM_NONE            0x0	
#define EM_M32             0x0001 /* AT&T WE 32100 */
#define EM_SWITCHED_M32    0x0100
#define EM_SPARC           0x0002
#define EM_SWITCHED_SPARC  0x0200
#define EM_386             0x0003
#define EM_SWITCHED_386    0x0300
#define EM_MIPS            0x0008
#define EM_SWITCHED_MIPS   0x0800
#define EM_ARM             0x0028 /* ARM-ELF if the endianness is the same */ 
#define EM_SWITCHED_ARM    0x2800 /* ARM-ELF if the endianness is switched */ 
#define EM_SH              0x002a /* SH-ELF if the endianness is the same */ 
#define EM_SWITCHED_SH     0x2a00 /* SH-ELF if the endianness is switched */ 
#define EM_ARC             0x002d /* ARC-ELF if the endianness is the same */
#define EM_SWITCHED_ARC    0x2d00 /* ARC-ELF if the endianness is switched */
#define EM_ARCOMPACT       0x005d /* ARCompact-ELF if the endianness is the same. */ 
#define EM_SWITCHED_ARCOMPACT    0x5d00 /* ARCompact-ELF if the endianness is switched */ 
#define EM_IA_64           0x0032 /* Itanium if the endianness is the same */
#define EM_SWITCHED_IA_64  0x3200 /* Itanium if the endianness is switched */
#define EM_ALPHA           0x9026 /* Alpha if the endianness is the same */
#define EM_SWITCHED_ALPHA  0x2690 /* Alpha if the endianness is switched */
#define EM_PPC             0x0014 /* PowerPC */
#define EM_SWITCHED_PPC    0x1400 /* PowerPC */
#define EM_AMD64           0x003e
#define EM_SWITCHED_AMD64  0x3e00
#define EM_PPC64           0x0015 /* PowerPC 64bit */
#define EM_SWITCHED_PPC64  0x1500 /* PowerPc 64bit */
#define EM_SPE             0x0017 /* Cell SPE */
#define EM_SWITCHED_SPE    0x1700 /* Cell SPE */

#define EV_NONE    0
#define EV_CURRENT 1

/*! \name Elf-Magic */ 
/*@{*/
#define ELFMAG0      0x7f
#define ELFMAG1      'E' 
#define ELFMAG2      'L'
#define ELFMAG3      'F'
/*@}*/

/*! \name Elf-Classes */ 
/*@{*/
#define ELFCLASSNONE 0
#define ELFCLASS32   1
#define ELFCLASS64   2
/*@}*/

/*! \name Elf-Datatypes */ 
/*@{*/
#define ELFDATANONE  0
#define ELFDATA2LSB  1
#define ELFDATA2MSB  2
/*@}*/

/* PROGRAM HEADER CONSTS */

#define PT_NULL         0
#define PT_LOAD         1
#define PT_DYNAMIC      2
#define PT_INTERP       3
#define PT_NOTE         4
#define PT_SHLIB        5
#define PT_PHDR         6
#define PT_TLS          7
#define PT_GNU_STACK    0x6474e551
#define PT_RELRO        0x6474e552

#define PT_LOPROC       0x70000000
#define PT_HIPROC       0x7fffffff
#define PT_ARM_EXIDX    0x70000001

#define PF_X           0x1
#define PF_W           0x2
#define PF_R           0x4
#define PF_ARM_ENTRY   0x80000000 

/* SECTION HEADER CONSTS */
/** \name Section types */
/*@{*/
/* 1.1. These are specified by  System V Application Binary Interface */

#define SHT_NULL          0
#define SHT_PROGBITS      1
#define SHT_SYMTAB        2
#define SHT_STRTAB        3
#define SHT_RELA          4
#define SHT_HASH          5
#define SHT_DYNAMIC       6
#define SHT_NOTE          7
#define SHT_NOBITS        8
#define SHT_REL           9
#define SHT_SHLIB         10
#define SHT_DYNSYM        11

#define SHT_INIT_ARRAY    14
#define SHT_FINI_ARRAY    15
#define SHT_PREINIT_ARRAY 16

#define SHT_GROUP         17
#define SHT_LOPROC        0x70000000
#define SHT_HIPROC        0x7fffffff
#define SHT_LOUSER        0x80000000
#define SHT_HIUSER        0x8fffffff

/* 1.2. Linux And Sun add OS specific relocations */

#define SHT_LOOS         0x60000000     /* Lower bound of OS-specific section-types */
#define SHT_HIOS         0x6fffffff     /* Upper bound of OS-specific section-types */

/* 1.3. Special Linux Section types (Linux Standard Base Specification) */

#define SHT_GNU_verdef    0x6ffffffd    /* This section contains the symbol versions that are provided. */
#define SHT_GNU_verneed   0x6ffffffe    /* This section contains the symbol versions that are required. */
#define SHT_GNU_versym    0x6fffffff    /* This section contains the Symbol Version Table. */


/* 1.4. Special IA64 types */ 
#define SHT_IA_64_UNWIND	(SHT_LOPROC + 1) /* unwind bits */

/* 1.5. Special MIPS types */
#define SHT_MIPS_REGINFO  0x70000006	/* Needed to read in the .reginfo section, to recover the original gp-value */
/*@}*/

/** \name Section flags */
/*@{*/
#define SHF_WRITE         0x1 
#define SHF_ALLOC         0x2 
#define SHF_EXECINSTR     0x4 
#define SHF_LINK_ORDER    0x80
#define SHF_GROUP         0x200
#define SHF_TLS           0x400

#define SHF_ARM_ENTRYSECT 0x10000000 
#define SHF_ARM_COMDEF    0x80000000
#define SHF_MASKPROC      0xf0000000
/*@}*/

/** \name Section groups */
/*@{*/
#define GRP_COMDAT       0x1
/*@}*/

/** \name Special section */
/*@{*/
#define SHN_UNDEF  0
#define SHN_LORESERVE    0xff00
#define SHN_ABS          0xfff1
#define SHN_COMMON       0xfff2
#define SHN_HIRESERVE    0xffff
/*@}*/

/** \name SYMBOL CONSTANTS */
/*@{*/
/* utils */
#define ELF32_ST_BIND(i) ((i)>>4)
#define ELF32_ST_TYPE(i) ((i)&0xf)
#define ELF32_ST_VIS(i) ((i)&0x3)
#define ELF32_ST_INFO(b,t) (((b)<<4)+((t)&0xf))

#define ELF64_ST_BIND(val)              ELF32_ST_BIND (val)
#define ELF64_ST_TYPE(val)              ELF32_ST_TYPE (val)
#define ELF64_ST_VIS(val) 	 	ELF32_ST_VIS(val) 
#define ELF64_ST_INFO(bind, type)       ELF32_ST_INFO ((bind), (type))

/* Symbol binding */
#define STB_LOCAL  0
#define STB_GLOBAL 1
#define STB_WEAK   2
#define STB_LOPROC 13
#define STB_HIPROC 15

/* Symbol types */
#define STT_NOTYPE  0
#define STT_OBJECT  1
#define STT_FUNC    2
#define STT_SECTION 3
#define STT_FILE    4
#define STT_TLS     6
#define STT_IFUNC   10
#define STT_LOPROC  13
#define STT_HIPROC  15

/* Additional symbol types for Thumb */
#define STT_ARM_TFUNC  STT_LOPROC



/* Symbol visibility */
#define STV_DEFAULT	0
#define STV_INTERNAL	1
#define STV_HIDDEN	2
#define STV_PROTECTED	3

/*@}*/

/** \name Relocation entry utils*/
/*@{*/
#define ELF32_R_SYM(i)     ((i) >> 8)
#define ELF32_R_TYPE(i)    ((unsigned char)  i)
#define ELF32_R_INFO(s, t) (((s) << 8) + ((unsigned char) t))

#define ELF64_R_SYM(i)			((Elf32_Word)((i) >> 32))
#define ELF64_R_TYPE(i)			((Elf32_Word)((i) & 0xffffffff))
#define ELF64_R_INFO(sym,type)		((((Elf64_Xword) (sym)) << 32) + (type))
/*@}*/

#define DT_NULL     0
#define DT_NEEDED   1
#define DT_PLTRELSZ 2
#define DT_PLTGOT   3
#define DT_HASH     4
#define DT_STRTAB   5
#define DT_SYMTAB   6
#define DT_RELA     7
#define DT_RELASZ   8
#define DT_RELAENT  9 
#define DT_STRSZ    10
#define DT_SYMENT   11
#define DT_INIT     12
#define DT_FINI     13
#define DT_SONAME   14
#define DT_RPATH    15
#define DT_SYMBOLIC 16
#define DT_REL      17
#define DT_RELSZ    18
#define DT_RELENT   19
#define DT_PLTREL   20 
#define DT_DEBUG    21 
#define DT_TEXTREL  22
#define DT_JMPREL   23 
#define DT_BIND_NOW      24
#define DT_INIT_ARRAY    25
#define DT_FINI_ARRAY    26
#define DT_INIT_ARRAYSZ  27
#define DT_FINI_ARRAYSZ  28
#define DT_RUNPATH  29  
#define DT_FLAGS    30
#define DT_PREINIT_ARRAY   32
#define DT_PREINIT_ARRAYSZ 33
#define DT_LOOS     0x60000000
#define DT_HIOS     0x6fffffff 
#define DT_LOPROC   0x70000000
#define DT_HIPROC   0x7fffffff
#define DT_PPC64_GLINK 0x70000000

/* These were chosen by Sun.  */
#define DT_FLAGS_1 0x6ffffffb

/* gnu symbol versioning extensions */
#define DT_VERSYM 0x6ffffff0
#define DT_VERDEF 0x6ffffffc
#define DT_VERDEFNUM 0x6ffffffd
#define DT_VERNEED 0x6ffffffe
#define DT_VERNEEDNUM 0x6fffffff

/* Legal values for vd_version (version revision).  */
#define VER_DEF_NONE    0               /* No version */
#define VER_DEF_CURRENT 1               /* Current version */
#define VER_DEF_NUM     2               /* Given version number */

/* Legal values for vd_flags (version information flags).  */
#define VER_FLG_BASE    0x1             /* Version definition of file itself */
#define VER_FLG_WEAK    0x2             /* Weak version identifier */

/* Versym symbol index values.  */
#define VER_NDX_LOCAL           0       /* Symbol is local.  */
#define VER_NDX_GLOBAL          1       /* Symbol is global.  */
#define VER_NDX_LORESERVE       0xff00  /* Beginning of reserved entries.  */
#define VER_NDX_ELIMINATE       0xff01  /* Symbol is to be eliminated.  */

/* Legal values for vn_version (version revision).  */
#define VER_NEED_NONE    0              /* No version */
#define VER_NEED_CURRENT 1              /* Current version */
#define VER_NEED_NUM     2              /* Given version number */

/* Legal values for vna_flags.  */
#define VER_FLG_WEAK    0x2             /* Weak version identifier */

#endif



#ifdef DIABLOELF_TYPES
#ifndef DIABLOELF_MAIN_TYPES
#define DIABLOELF_MAIN_TYPES

/** Elf 32 Header */
typedef struct 
{
   Elf32_Byte e_ident[EI_NIDENT];
   Elf32_Half e_type;
   Elf32_Half e_machine;
   Elf32_Word e_version;
   Elf32_Addr e_entry; 
   Elf32_Off  e_phoff;
   Elf32_Off  e_shoff;
   Elf32_Word e_flags;
   Elf32_Half e_ehsize;
   Elf32_Half e_phentsize;
   Elf32_Half e_phnum;  
   Elf32_Half e_shentsize;
   Elf32_Half e_shnum;   
   Elf32_Half e_shstrndx; 
} Elf32_Ehdr; 

/** Elf 64 Header */
typedef struct
{
   unsigned char e_ident[EI_NIDENT];
   Elf64_Half    e_type;
   Elf64_Half    e_machine;
   Elf64_Word    e_version;
   Elf64_Addr    e_entry;
   Elf64_Off     e_phoff;
   Elf64_Off     e_shoff;
   Elf64_Word    e_flags;
   Elf64_Half    e_ehsize;
   Elf64_Half    e_phentsize;
   Elf64_Half    e_phnum;
   Elf64_Half    e_shentsize;
   Elf64_Half    e_shnum;
   Elf64_Half    e_shstrndx;
} Elf64_Ehdr;

/** Program header table entry */
typedef struct
{
   Elf32_Word p_type;
   Elf32_Off  p_offset;
   Elf32_Addr p_vaddr;
   Elf32_Addr p_paddr;
   Elf32_Word p_filesz;
   Elf32_Word p_memsz;
   Elf32_Word p_flags;
   Elf32_Word p_align;
} Elf32_Phdr;


/** Program header table entry */
typedef struct
{
   Elf64_Word p_type;
   Elf64_Word p_flags;
   Elf64_Off  p_offset;
   Elf64_Addr p_vaddr;
   Elf64_Addr p_paddr;
   Elf64_Xword p_filesz;
   Elf64_Xword p_memsz;
   Elf64_Xword p_align;
} Elf64_Phdr;




/** Section header table entry */
typedef struct
{
   Elf32_Word sh_name;
   Elf32_Word sh_type;
   Elf32_Word sh_flags;
   Elf32_Addr sh_addr;
   Elf32_Off  sh_offset;
   Elf32_Word sh_size;
   Elf32_Word sh_link;
   Elf32_Word sh_info;
   Elf32_Word sh_addralign;
   Elf32_Word sh_entsize;
} Elf32_Shdr;


/** Section header table entry (64 bit) */
typedef struct
{
   Elf64_Word    sh_name;
   Elf64_Word    sh_type;
   Elf64_Xword   sh_flags;
   Elf64_Addr    sh_addr;
   Elf64_Off     sh_offset;
   Elf64_Xword   sh_size;
   Elf64_Word    sh_link;
   Elf64_Word    sh_info;
   Elf64_Xword   sh_addralign;
   Elf64_Xword   sh_entsize;
} Elf64_Shdr;



/** Symbol table entry (32 bit)*/
typedef struct
{
   Elf32_Word st_name;
   Elf32_Addr st_value;
   Elf32_Word st_size;
   Elf32_Byte st_info;
   Elf32_Byte st_other;
   Elf32_Half st_shndx; 
} Elf32_Sym;


/** Symbol table entry (64 bit)*/
typedef struct
{
   Elf64_Word    st_name;	/* 4 bytes */
   Elf64_Byte    st_info;	/* 1 byte  */
   Elf64_Byte    st_other;      /* 1 byte  */
   Elf64_Section st_shndx;	/* 2 bytes */
   Elf64_Addr    st_value;	/* 8 bytes */
   Elf64_Xword   st_size;
} Elf64_Sym;

/** Relocation table entry */
typedef struct
{
   Elf32_Addr r_offset;
   Elf32_Word r_info;
} Elf32_Rel;

  
typedef struct  
{ 
   Elf64_Addr    r_offset;
   Elf64_Xword   r_info;
} Elf64_Rel;


typedef struct
{
  Elf32_Addr	r_offset;
  Elf32_Word	r_info;	
  Elf32_Sword	r_addend;
} Elf32_Rela;

typedef struct
{
  Elf64_Addr	r_offset;
  Elf64_Xword	r_info;	
  Elf64_Sxword	r_addend;
} Elf64_Rela;



typedef struct
{
  Elf32_Sword d_tag;
  union {
    Elf32_Word d_val;
    Elf32_Addr d_ptr;
  } d_un;
} Elf32_Dyn; 


typedef struct
{
   Elf64_Sxword  d_tag;
   union
   {
      Elf64_Xword d_val;
      Elf64_Addr d_ptr;
   } d_un;
} Elf64_Dyn;

typedef struct
{
  Elf32_Word	ri_gprmask;
  Elf32_Word	ri_cprmask[4];
  Elf32_Sword ri_gp_value;
} Elf_RegInfo;

typedef struct {
  Elf32_Word  ri_gprmask;
  Elf32_Word  ri_cprmask[4];
  Elf32_Sword ri_gp_value;
} Elf32_RegInfo;

/** ELF header information **/
typedef struct {
  Elf32_Word pagesize;
  Elf32_Word pltshtype;
  Elf32_Word pltentsize;
  Elf32_Word ptload_align;
  Elf32_Word ptdynamic_align;
  Elf32_Word ptinterp_align;
  Elf32_Word ptnote_align;
  Elf32_Word ptphdr_align;
  Elf32_Word pttls_align;
} Elf32_HeaderInfo;

typedef struct {
  Elf64_Xword pagesize;
  Elf64_Xword pltshtype;
  Elf64_Xword pltentsize;
  Elf64_Xword ptload_align;
  Elf64_Xword ptdynamic_align;
  Elf64_Xword ptinterp_align;
  Elf64_Xword ptnote_align;
  Elf64_Xword ptphdr_align;
  Elf64_Xword pttls_align;
} Elf64_HeaderInfo;

/* symbol versioning{{{*/

typedef struct
{
  Elf32_Half    vd_version;             /* Version revision */
  Elf32_Half    vd_flags;               /* Version information */
  Elf32_Half    vd_ndx;                 /* Version Index */
  Elf32_Half    vd_cnt;                 /* Number of associated aux entries */
  Elf32_Word    vd_hash;                /* Version name hash value */
  Elf32_Word    vd_aux;                 /* Offset in bytes to verdaux array */
  Elf32_Word    vd_next;                /* Offset in bytes to next verdef
                                           entry */
} Elf32_Verdef;

typedef struct
{
  Elf64_Half    vd_version;             /* Version revision */
  Elf64_Half    vd_flags;               /* Version information */
  Elf64_Half    vd_ndx;                 /* Version Index */
  Elf64_Half    vd_cnt;                 /* Number of associated aux entries */
  Elf64_Word    vd_hash;                /* Version name hash value */
  Elf64_Word    vd_aux;                 /* Offset in bytes to verdaux array */
  Elf64_Word    vd_next;                /* Offset in bytes to next verdef
                                           entry */
} Elf64_Verdef;


/* Auxialiary version information.  */

typedef struct
{
  Elf32_Word    vda_name;               /* Version or dependency names */
  Elf32_Word    vda_next;               /* Offset in bytes to next verdaux
                                           entry from the start of this entry.
                                           0 if last. */
} Elf32_Verdaux;

typedef struct
{
  Elf64_Word    vda_name;               /* Version or dependency names */
  Elf64_Word    vda_next;               /* Offset in bytes to next verdaux
                                           entry from the start of this entry.
                                           0 if last. */
} Elf64_Verdaux;

/* Version dependency section.  */

typedef struct
{
  Elf32_Half    vn_version;             /* Version of structure */
  Elf32_Half    vn_cnt;                 /* Number of associated aux entries */
  Elf32_Word    vn_file;                /* Offset of filename for this
                                           dependency */
  Elf32_Word    vn_aux;                 /* Offset in bytes to vernaux array */
  Elf32_Word    vn_next;                /* Offset in bytes to next verneed
                                           entry from the start of this entry.
                                           0 if last. */
} Elf32_Verneed;

typedef struct
{
  Elf64_Half    vn_version;             /* Version of structure */
  Elf64_Half    vn_cnt;                 /* Number of associated aux entries */
  Elf64_Word    vn_file;                /* Offset of filename for this
                                           dependency */
  Elf64_Word    vn_aux;                 /* Offset in bytes to vernaux array */
  Elf64_Word    vn_next;                /* Offset in bytes to next verneed
                                           entry from the start of this entry.
                                           0 if last. */
} Elf64_Verneed;


/* Auxiliary needed version information.  */

typedef struct
{
  Elf32_Word    vna_hash;               /* Hash value of dependency name */
  Elf32_Half    vna_flags;              /* Dependency specific information */
  Elf32_Half    vna_other;              /* Unused */
  Elf32_Word    vna_name;               /* Dependency name string offset */
  Elf32_Word    vna_next;               /* Offset in bytes to next vernaux
                                           entry from the start of this entry.
                                           0 if last. */
} Elf32_Vernaux;

typedef struct
{
  Elf64_Word    vna_hash;               /* Hash value of dependency name */
  Elf64_Half    vna_flags;              /* Dependency specific information */
  Elf64_Half    vna_other;              /* Unused */
  Elf64_Word    vna_name;               /* Dependency name string offset */
  Elf64_Word    vna_next;               /* Offset in bytes to next vernaux
                                           entry from the start of this entry.
                                           0 if last. */
} Elf64_Vernaux;


/*}}}*/
#endif
#endif

#ifdef DIABLOELF_FUNCTIONS
#ifndef DIABLOELF_MAIN_FUNCTIONS
#define DIABLOELF_MAIN_FUNCTIONS
t_bool IsElf(FILE * fp);
t_bool IsElf32(FILE * fp);
void ReadElf(FILE * fp,t_object * obj);
t_uint64 ElfGetSizeofHeaders(t_object * obj, const t_layout_script * layoutscript);
t_uint32 ElfComputeBucketCount(t_object * obj, t_uint32 symsize, t_uint32 pagesize, t_uint32 optimize);
t_uint32 ElfHash(char *name);
#endif
#endif
