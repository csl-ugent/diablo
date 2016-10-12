#include <diabloecoff.h>
#ifndef DIABLOECOFF_MAIN_DEFINES
#define DIABLOECOFF_MAIN_DEFINES

#define ALPHAMAGICBYTE1     0x83
#define ALPHAMAGICBYTE2     0x1

#define ALPHAMAGICZBYTE1     0x88
#define ALPHAMAGICZBYTE2     0x1

#define ECOFF_OMAGIC  0407
#define ECOFF_NMAGIC  0410
#define ECOFF_ZMAGIC  0413

#define F_NO_REORG     040000
#define F_NO_REMOVE    0100000

#define S_NRELOC_OVFL   0x20000000 

#define magicSym    0x1992

/* Symbol storage classes */

#define scNil           0
#define scText          1
#define scData          2
#define scBss           3
#define scRegister      4
#define scAbs           5
#define scUndefined     6
#define scUnallocated   7
#define scBits          8
#define scTlsUndefined  9
#define scRegImage     10
#define scInfo         11
#define scUserStruct   12
#define scSData        13
#define scSBss         14
#define scRData        15
#define scVar          16
#define scCommon       17
#define scSCommon      18
#define scVarRegister  19
#define scVariant      20
#define scFileDesc     20
#define scSUndefined   21
#define scInit         22
#define scBasedVar     23
#define scReportDesc   23
#define scXData        24
#define scPData        25
#define scFini         26
#define scRConst       27
#define scSymRef       28
#define scTlsCommon    29
#define scTlsData      30
#define scTlsBss       31

/* Symbol Types */

#define stNil           0   
#define stGlobal        1   
#define stStatic        2   
#define stParam         3   
#define stLocal         4   
#define stLabel         5   
#define stProc          6   
#define stBlock         7   
#define stEnd           8   
#define stMember        9   
#define stTypedef       10  
#define stfile          11  
#define stregreloc      12  
#define stforward       13  
#define stStaticProc    14
#define stConstant	15
#define stStaParam	16
#define stBase          17
#define stVirtBase      18
#define stTag           19
#define stInter         20
#define stSplit		21
#define stModule	22
#define stNamespace	22
#define stModview	23
#define stUsing		23
#define stAlias		24

#define STYP_REG        0x00000000
#define STYP_DSECT      0x00000001
#define STYP_NOLOAD     0x00000002
#define STYP_GROUP      0x00000004
#define STYP_PAD        0x00000008
#define STYP_COPY       0x00000010
#define STYP_TEXT       0x00000020
#define STYP_DATA       0x00000040
#define STYP_BSS        0x00000080
#define STYP_RDATA      0x00000100
#define STYP_SDATA      0x00000200
#define STYP_SBSS       0x00000400
#define STYP_UCODE      0x00000800
#define STYP_FINI       0x01000000      
#define STYP_INIT       0x80000000
#define STYP_LITA       0x04000000
#define STYP_RCONST     0x02200000  
#define STYP_XDATA      0x02400000
#define STYP_PDATA      0x02800000
#endif

#ifndef DIABLOECOFF_MAIN_TYPEDEFS
#define DIABLOECOFF_MAIN_TYPEDEFS
typedef struct _t_ecoff_file_hdr t_ecoff_file_hdr;
typedef	struct _t_ecoff_aout_hdr t_ecoff_aout_hdr;
typedef struct _t_ecoff_scn_hdr t_ecoff_scn_hdr;
typedef struct _t_ecoff_reloc t_ecoff_reloc;
/*! The ecoff relocation section types */
typedef enum 
{
   R_SN_NULL=0,
   R_SN_TEXT=1,
   R_SN_RDATA=2,
   R_SN_DATA=3,
   R_SN_SDATA=4,
   R_SN_SBSS=5,
   R_SN_BSS=6,
   R_SN_INIT=7,
   R_SN_LIT8=8,
   R_SN_LIT4=9,
   R_SN_XDATA=10,
   R_SN_PDATA=11,
   R_SN_FINI=12,
   R_SN_LITA=13,
   R_SN_ABS=14,
   R_SN_RCONST =15,
   R_SN_TLSDATA=16,
   R_SN_TLSBSS=17,
   R_SN_TLSINIT=18,
   R_SN_GOT=20
} t_ecoff_sectype;

/*! Relocation types */
typedef enum
{
  /* Placeholder used to overwrite relocations that have already been performed
   *    * */
  R_ABS         =       0,
  /* A 32-bit absolute reference to a symbol or a relocatable */
  R_REFLONG     =       1,
  /* A 64-bit absolute reference to a symbol or a relocatable */
  R_REFQUAD     =       2,
  /* A 32-bit gp relative reference to a symbol or a relocatable */
  R_GPREL32     =       3,
  /* A gp relative reference to an entry in the literal address pool */
  R_LITERAL     =       4,
  /* A use of a literal address pool entry, previously loaded with a R_LITERAL
   * relocation. r_symndx is used to identify the subtype of this relocation */
  R_LITUSE      =       5,
  /* Used to relocate the lda/ldah pair of a gp set/reset */
  R_GPDISP      =       6,
  /* A 21-bit pc relative reference to a symbol or a relocatable (as used in
   * branches/ direct calls) */
  R_BRADDR      =       7,
  /* A 14-bit hint used in jsr instructions to predict the target of indirect
   * calls */
  R_HINT        =       8,
  /* A 16-bit self relative reference to a symbol or a relocatable */
  R_SREL16      =       9,
  /* A 32-bit self relative reference to a symbol or a relocatable */
  R_SREL32      =       10,
  /* A 64-bit self relative reference to a symbol or a relocatable */
  R_SREL64      =       11,
  R_OP_PUSH     =       12,
  R_OP_STORE    =       13,
  R_OP_PSUB     =       14,
  R_OP_PRSHIFT  =       15,
  R_GPVALUE     =       16,
  R_GPRELHIGH   =       17,
  R_GPRELLOW    =       18,
  R_IMMED       =       19,
  R_TLS_LITERAL =       20,
  R_TLS_HIGH    =       21,
  R_TLS_LOW     =       22
} t_ecoff_reltypes;

typedef enum 
{
  R_IMMED_GP_16=1,
  R_IMMED_GP_HI32=2,
  R_IMMED_SCN_HI32=3,
  R_IMMED_BR_HI32=4,
  R_IMMED_LO32=5
} t_ecoff_immed_rel_subtypes;
#endif

#ifdef DIABLOECOFF_TYPES
#ifndef DIABLOECOFF_MAIN_TYPES
#define DIABLOECOFF_MAIN_TYPES
struct _t_ecoff_file_hdr {
   t_uint16 	f_magic;
   t_uint16 	f_nscns;
   t_uint32	f_timdat;
   t_uint64    f_symptr;
   t_uint32    f_nsyms;
   t_uint16    f_opthdr;
   t_uint16    f_flags;
};

struct _t_ecoff_aout_hdr {
  t_uint16	magic;
  t_uint16	vstamp;
  t_uint16	bldrev;
  t_uint16	padcell;
  t_uint64	tsize;
  t_uint64	dsize;
  t_uint64	bsize;
  t_uint64	entry;
  t_uint64	text_start;
  t_uint64	data_start;
  t_uint64	bss_start;
  t_uint32	gprmask;
  t_uint32	fprmask;
  t_uint64	gp_value;
}; 

struct _t_ecoff_scn_hdr {
   char	s_name[8];
   t_uint64 s_paddr;
   t_uint64 s_vaddr;
   t_uint64 s_size;
   t_uint64 s_scnptr;
   t_uint64 s_relptr;
   t_uint64 s_lnnoptr;
   t_uint16 s_nreloc;
   t_uint16 s_nlnno_or_align;
   t_uint32 s_flags;
};

struct _t_ecoff_reloc {
   t_uint64	r_vaddr;
   t_uint32	r_symndx;
   t_uint32	r_type 	: 8;
   t_uint32	r_extern: 1;
   t_uint32	r_offset:6;
   t_uint32	r_reserved:11;
   t_uint32	r_size:6;
};

typedef struct {
  t_uint16	magic;
  t_uint16	vstamp;
  t_uint32	ilineMax;
  t_uint32	idnMax;
  t_uint32	ipdMax;
  t_uint32	isymMax;
  t_uint32	ioptMax;
  t_uint32	iauxMax;
  t_uint32	issMax;	
  t_uint32	issExtMax;
  t_uint32	ifdMax;	
  t_uint32	crfd;
  t_uint32	iextMax;
  t_uint64	cbLine;	
  t_uint64	cbLineOffset;
  t_uint64	cbDnOffset;
  t_uint64	cbPdOffset;
  t_uint64	cbSymOffset;
  t_uint64	cbOptOffset;
  t_uint64	cbAuxOffset;
  t_uint64	cbSsOffset;
  t_uint64	cbSsExtOffset;
  t_uint64	cbFdOffset;
  t_uint64	cbRfdOffset;
  t_uint64	cbExtOffset;
} ecoff_symbol_table_hdr;

typedef struct {
  t_uint64	value;
  t_uint32	iss;
  t_uint32	st : 6;
  t_uint32	sc : 5;
  t_uint32	reserved : 1;
  t_uint32	index : 20;
} ecoff_loc_sym; 

typedef struct {
   ecoff_loc_sym asym;
   t_uint32	    jmptbl:1;
   t_uint32	    cobol_main:1;
   t_uint32	    weakext:1;
   t_uint32        alignment:4;
   t_uint32	    reserved:25;
   t_uint32	    ifd;
} ecoff_ext_sym; 


typedef struct {
  t_uint64      adr;		
  t_uint64	cbLineOffset;	
  t_uint32	isym;		
  t_uint32	iline;		
  t_uint32	regmask;	
  t_uint32	regoffset;	
				
				
  t_uint32	iopt;		
  t_uint32	fregmask;	
  t_uint32	fregoffset;	
  t_uint32	frameoffset;	
  t_uint32	lnLow;		
  t_uint32	lnHigh;		
  t_uint32	gp_prologue:8; 	
  t_uint32	gp_used : 1;	
  t_uint32	reg_frame : 1;	
  t_uint32	prof : 1;	
  t_uint32	gp_tailcall:1;	
  t_uint32      reserved : 12;  
  t_uint32      localoff : 8;   
  t_uint16      framereg;       
  t_uint16      pcreg;          
} ecoff_pdr;
#endif
#endif

#ifdef DIABLOECOFF_FUNCTIONS
#ifndef DIABLOECOFF_MAIN_FUNCTIONS
#define DIABLOECOFF_MAIN_FUNCTIONS
t_bool IsEcoff(FILE * fp);
void EcoffRead(FILE *, t_object *, t_bool);
void EcoffWrite(FILE * fp, t_object * obj);
t_uint64 EcoffGetSizeofHeaders(t_object * obj, t_layout_script * layoutscript);
#endif
#endif
