#include <diabloobject.h>
#include <diabloticoff.h>


#define Uint32ExtractBits(i, nbits, offset) ((i >> offset) & ((1U << nbits) -1))


t_bool IsTiCoff(FILE * fp)
{
  unsigned char header[22]; 
  long fpos=ftell(fp);
  int ret=fread(header,1,22,fp); 
  t_uint16 file_header_optional_header_size;
  t_uint16 magic;
  fseek(fp,fpos,SEEK_SET);

  if (ret!=22) return FALSE;

  file_header_optional_header_size = *((t_uint16 *) (header+16));
  magic = *((t_uint16 *) (header+20));

  /* Size of optional header in bytes 16 and 17 = 0 or 28, can be switched
   * endian */
  if ((file_header_optional_header_size!=0)&&(file_header_optional_header_size!=0x1c)&&(file_header_optional_header_size!=0x1c00)) return FALSE;

  /* Magic number: 0x0 in byte 21, 0x99 in byte 20 for c6000.... Add more
   * magic for other architectures....:
   *
   * 0x9c = C55x
   * 0x98 = C54x
   * 0x97 = ARM
   * ...
   * */

  if (((magic!=0x99)&&(magic!=0x9900))&&((magic!=0x97)&&(magic!=0x9700))) 
	  return FALSE;
  
  if ((magic==0x9900)&&(file_header_optional_header_size==0x1c)) return FALSE;
  if ((magic==0x99)&&(file_header_optional_header_size==0x1c00)) return FALSE;
  if ((magic==0x9700)&&(file_header_optional_header_size==0x1c)) return FALSE;
  if ((magic==0x97)&&(file_header_optional_header_size==0x1c00)) return FALSE;

  VERBOSE(1,("File is an TICOFF objectfile")); 
  return TRUE;
}

t_bool IsTiCoffARM(FILE * fp)
{
  unsigned char header[22]; 
  long fpos=ftell(fp);
  int ret=fread(header,1,22,fp); 
  t_uint16 file_header_optional_header_size;
  t_uint16 magic;
  fseek(fp,fpos,SEEK_SET);

  if (ret!=22) return FALSE;

  file_header_optional_header_size = *((t_uint16 *) (header+16));
  magic = *((t_uint16 *) (header+20));

  /* Size of optional header in bytes 16 and 17 = 0 or 28, can be switched
   * endian */
  if ((file_header_optional_header_size!=0)&&(file_header_optional_header_size!=0x1c)&&(file_header_optional_header_size!=0x1c00)) return FALSE;

  if ((magic!=0x97)&&(magic!=0x9700)) return FALSE;

  if ((magic==0x9700)&&(file_header_optional_header_size==0x1c)) return FALSE;
  if ((magic==0x97)&&(file_header_optional_header_size==0x1c00)) return FALSE;

  VERBOSE(1,("File is an TICOFF ARM objectfile")); 
  return TRUE;
}

t_bool IsTiCoffC6000(FILE * fp)
{
  unsigned char header[22]; 
  long fpos=ftell(fp);
  int ret=fread(header,1,22,fp); 
  t_uint16 file_header_optional_header_size;
  t_uint16 magic;
  fseek(fp,fpos,SEEK_SET);

  if (ret!=22) return FALSE;

  file_header_optional_header_size = *((t_uint16 *) (header+16));
  magic = *((t_uint16 *) (header+20));

  /* Size of optional header in bytes 16 and 17 = 0 or 28, can be switched
   * endian */
  if ((file_header_optional_header_size!=0)&&(file_header_optional_header_size!=0x1c)&&(file_header_optional_header_size!=0x1c00)) return FALSE;

  if ((magic!=0x99)&&(magic!=0x9900)) return FALSE;

  if ((magic==0x9900)&&(file_header_optional_header_size==0x1c)) return FALSE;
  if ((magic==0x99)&&(file_header_optional_header_size==0x1c00)) return FALSE;

  VERBOSE(1,("File is an TICOFF C6000 objectfile")); 
  return TRUE;
}

typedef struct {
  t_section * sec;
  t_uint32 relptr; 
  t_uint32 nrels; 
} t_section_table_entry;

void TiCoffARMRead(FILE * fp, t_object * obj, t_bool read_debug)
{
	TiCoffReadCommon(fp, obj, read_debug);
}

void TiCoffC6000Read(FILE * fp, t_object * obj, t_bool read_debug)
{
	TiCoffReadCommon(fp, obj, read_debug);
}

void TiCoffReadCommon(FILE * fp, t_object * obj, t_bool read_debug)
{
  t_bool switched_endian = FALSE;
  t_bool have_optional_header=FALSE;
  t_uint32 stringtab_size;

  unsigned char file_header[22]; 

  t_uint16 file_header_version;
  t_uint16 file_header_nsecs;
  t_uint32 file_header_symtab_ptr;
  t_uint32 file_header_nsyms;
  t_uint16 file_header_optional_header_size;
  t_uint16 file_header_flags;
  t_uint16 file_header_magic;
  
  unsigned char optional_header[28];

  /* t_uint16 optional_header_magic; */
  /* t_uint16 optional_header_version; */
  /* t_uint32 optional_header_code_size; */
  /* t_uint32 optional_header_data_size; */
  /* t_uint32 optional_header_bss_size; */
  /* t_uint32 optional_header_entry; */
  /* t_uint32 optional_header_code_start; */
  /* t_uint32 optional_header_data_start; */
  
  unsigned char section_header[48];
 
  t_uint32 section_header_flags;
  t_uint32 section_header_vaddr;
  t_uint32 section_header_paddr;
  t_uint32 section_header_size;
  t_uint32 section_header_pos;
  t_uint32 section_header_relptr;
  t_uint32 section_header_nrels;
  
  long fpos=ftell(fp);
  long cpos=fpos;
  int ret=fread(file_header,1,22,fp); 
  t_uint32 i;
  char * stringtab=NULL;
  t_string section_name=NULL;
  t_section_table_entry * sections=NULL;

  t_symbol ** symbol_table=NULL;
  
  t_address section_vaddr, section_paddr, section_size,section_align;
  t_section * undef;
  t_section * abs;

  ASSERT(ret == 22, ("Truncated TICOFF objectfile for C6000 series named %s (reading file header)", OBJECT_NAME(obj)));
  ASSERT(obj, ("Object = NULL"));

  if (!OBJECT_PARENT(obj))
  {
	  /* create global dummy sections (abs and undef) */
	  OBJECT_SET_UNDEF_SECTION(obj, SectionCreateForObject (obj, SPECIAL_SECTION, NULL, AddressNullForObject (obj), "*UNDEF*"));
	  OBJECT_SET_ABS_SECTION(obj, SectionCreateForObject (obj, SPECIAL_SECTION, NULL, AddressNullForObject (obj), "*ABS*"));
	  undef = OBJECT_UNDEF_SECTION(obj);
	  abs = OBJECT_ABS_SECTION(obj);
  }
  else
  {
	  undef = OBJECT_UNDEF_SECTION(OBJECT_PARENT(obj));
	  abs = OBJECT_ABS_SECTION(OBJECT_PARENT(obj));
  }


  
  OBJECT_SET_ADDRESS_SIZE (obj, ADDRSIZE32);

  file_header_version=*((t_uint16 *) (file_header));
  file_header_nsecs=*((t_uint16 *) (file_header+2));
  file_header_symtab_ptr=*((t_uint32 *) (file_header+8));
  file_header_nsyms=*((t_uint32 *) (file_header+12));
  file_header_optional_header_size = *((t_uint16 *) (file_header+16));
  file_header_flags = *((t_uint16 *) (file_header+18));
  file_header_magic = *((t_uint16 *) (file_header+20));

  if ((file_header_magic == 0x9900) 
  || (file_header_magic == 0x9700))
  {
    switched_endian = TRUE; 
    file_header_version = Uint16SwapEndian(file_header_version);
    file_header_nsecs = Uint16SwapEndian(file_header_nsecs);
    file_header_symtab_ptr = Uint32SwapEndian(file_header_symtab_ptr);
    file_header_nsyms = Uint32SwapEndian(file_header_nsyms);
    file_header_optional_header_size = Uint16SwapEndian(file_header_optional_header_size);
    file_header_flags = Uint16SwapEndian(file_header_flags);
    file_header_magic = Uint16SwapEndian(file_header_magic);
  }

  if (file_header_version == 0xc1)
  {
    VERBOSE(0, ("TI COFF v1"));
  }
  else if (file_header_version == 0xc2)
  {
    VERBOSE(0, ("TI COFF v2"));
  }
  else if (file_header_version == 0x93)
  {
    FATAL(("look like TI COFF c3x/c4x"));
  }
  else 
    FATAL(("Unknown ticoff version"));

  printf("FLAGS: %x\n",file_header_flags);

  if (file_header_flags & 0x1) VERBOSE(0, ("Reloc information is stripped"));
  if (file_header_flags & 0x2) VERBOSE(0, ("File is executable"));
  if (file_header_flags & 0x8) VERBOSE(0, ("Stripped"));
  if (file_header_flags & 0x20) VERBOSE(0, ("C64"));        
  else if (file_header_flags & 0x40) VERBOSE(0, ("C67"));        
  else VERBOSE(0, ("C62"));        
		  
  if (file_header_flags & 0x100) VERBOSE(0, ("Little endian"));
  if (file_header_flags & 0x200) VERBOSE(0, ("Little endian"));
 
  file_header_flags &= ~(0x1 | 0x2 | 0x8 | 0x20 | 0x40 | 0x100 | 0x200);
  
  printf("NOT DECODE FLAGS: %x\n",file_header_flags);
  
  if (file_header_optional_header_size==0)
  {
    have_optional_header=FALSE;
  }
  else if (file_header_optional_header_size == 28)
  {
    ret=fread(optional_header,1,28,fp); 
    ASSERT(ret == 28, ("Truncated TICOFF objectfile for C6000 series named %s (reading optional header)", OBJECT_NAME(obj)));
    have_optional_header=TRUE;

    /* TODO: Decode optional header */


  }
  else 
  {
    FATAL(("Strange size for optional header (expected 28) in TICOFF objectfile for C6000 series named %s", OBJECT_NAME(obj)));
  }

  
  /* Read the string table (located after the symbol table) */

  cpos=ftell(fp);
  fseek(fp,file_header_symtab_ptr + (file_header_nsyms)*18 + fpos, SEEK_SET);
  ret=fread(&stringtab_size,1,4,fp);
  ASSERT(ret == 4, ("Truncated TICOFF objectfile for C6000 series (reading string table size)"));
  if (switched_endian)
  {
    stringtab_size = Uint32SwapEndian(stringtab_size);
  }
  printf("String tab size: %d\n",stringtab_size);
  fseek(fp,file_header_symtab_ptr + (file_header_nsyms)*18 + fpos, SEEK_SET);
  stringtab=Malloc(stringtab_size);
  ret=fread(stringtab,1,stringtab_size,fp);
  ASSERT(ret == stringtab_size, ("Truncated TICOFF objectfile for C6000 series (reading string table)"));

  
  
  
  fseek(fp,cpos,SEEK_SET);

  sections = (t_section_table_entry *) Calloc (sizeof (t_section_table_entry), file_header_nsecs);
  printf("Have %d section headers\n", file_header_nsecs);

  for (i=0; i<file_header_nsecs; i++)
  {
    ret=fread(section_header,1,48,fp);
    ASSERT(ret == 48, ("Truncated TICOFF objectfile for C6000 series"));
    
    section_header_paddr  = *((t_uint32 *) (section_header+ 8));
    section_header_vaddr  = *((t_uint32 *) (section_header+12));
    section_header_size   = *((t_uint32 *) (section_header+16));
    section_header_pos    = *((t_uint32 *) (section_header+20));
    section_header_relptr = *((t_uint32 *) (section_header+24));
    section_header_nrels  = *((t_uint32 *) (section_header+32));
    section_header_flags  = *((t_uint32 *) (section_header+40));
    

    if (switched_endian)
    {
      section_header_paddr  = Uint32SwapEndian (section_header_paddr);
      section_header_vaddr  = Uint32SwapEndian (section_header_vaddr);
      section_header_size   = Uint32SwapEndian (section_header_size);
      section_header_pos    = Uint32SwapEndian (section_header_pos);
      section_header_relptr = Uint32SwapEndian (section_header_relptr);
      section_header_nrels  = Uint32SwapEndian (section_header_nrels);
      section_header_flags  = Uint32SwapEndian (section_header_flags);
    }

    section_vaddr = AddressNew32 (section_header_vaddr);
    section_paddr = AddressNew32 (section_header_paddr);
    section_size  = AddressNew32 (section_header_size);
    section_align = AddressNew32 (1<<((section_header_flags & 0xf00) >> 8));

    if (section_header[0]!=0) 
    { 
      section_name=Malloc(9); 
      strncpy(section_name,(char *) section_header,8); 
      section_name[8]='\0'; 
    }
    else
    {
      t_uint32 section_header_name_offset=(*((t_uint32 *) (section_header+4)));
      
      if (switched_endian)
        section_header_name_offset = Uint32SwapEndian (section_header_name_offset);

      section_name=StringDup(stringtab+section_header_name_offset);
    }

    if ((section_header_flags & STYP_DSECT) || (section_header_flags & STYP_COPY))  { printf("Skipping section %d (dsect or copy): %s\n",i,section_name); continue; }

    /*if (AddressIsNull(section_size)) { printf("Skipping section %d: %s (empty)\n",i,section_name); continue; }
*/
    ASSERT(AddressIsEq(section_vaddr,section_paddr),("Physical and virtual addressess of section %s differ in TICOFF objectfile for C6000",section_name));


    sections[i].relptr=section_header_relptr;
    sections[i].nrels=section_header_nrels;
    
    if (section_header_flags & STYP_TEXT) 
    {
      ASSERT( !(section_header_flags & STYP_DATA), ("TICOFF objectfile for C6000 series has data text section called %s",section_name));
      ASSERT( !(section_header_flags & STYP_BSS), ("TICOFF objectfile for C6000 series has bss text section called %s",section_name));
      ASSERT( AddressIsNull(section_size) ||section_header_pos, ("TICOFF objectfile for C6000 series has text section with no raw data called %s",section_name));

      sections[i].sec=ObjectAddSectionFromFile (obj, CODE_SECTION, TRUE, fp, OBJECT_STREAMPOS (obj) + section_header_pos, section_vaddr, section_size, section_align, section_name, i);
      printf("%3d. text %s\n",i+1,section_name);
    }
    else if (section_header_flags & STYP_DATA)
    {
      ASSERT( !(section_header_flags & STYP_BSS), ("TICOFF objectfile for C6000 series has bss data section called %s",section_name));
      ASSERT( AddressIsNull(section_size) || section_header_pos, ("TICOFF objectfile for C6000 series has data section with no raw data called %s",section_name));

      sections[i].sec=ObjectAddSectionFromFile (obj, DATA_SECTION, FALSE, fp, OBJECT_STREAMPOS (obj) + section_header_pos, section_vaddr, section_size, section_align, section_name,i);
      printf("%3d. data %s\n",i+1,section_name);
    }
    else if (section_header_flags & STYP_BSS)
    {
      ASSERT( !section_header_pos, ("TICOFF objectfile for C6000 series has data section with raw data called %s",section_name));
      sections[i].sec=ObjectAddSectionFromFile (obj, BSS_SECTION, FALSE, fp, OBJECT_STREAMPOS (obj) + section_header_pos, section_vaddr, section_size, section_align, section_name,i);
      printf("%3d. bss %s\n",i+1,section_name);
    }
    else 
    {
      printf("Hummmm..........%s\n",section_name);
      sections[i].sec=ObjectAddSectionFromFile (obj, NOTE_SECTION, FALSE, fp, OBJECT_STREAMPOS (obj) + section_header_pos, section_vaddr, section_size, section_align, section_name,i);

      printf("%3d. %s\n",i+1,section_name);
      printf("     Data: %x\n",*((t_uint32 *) (section_header+20)));
      printf("     Rels: %x\n",*((t_uint32 *) (section_header+24)));
      printf("     Res1: %x\n",*((t_uint32 *) (section_header+28)));
      printf("     nRel: %x\n",*((t_uint32 *) (section_header+32)));
      printf("     Res2: %x\n",*((t_uint32 *) (section_header+36)));
      if (section_header_flags & STYP_NOLOAD) printf("No load ");
      else if (section_header_flags & STYP_COPY) printf("Copy ");
      else printf("Regular ");
      if (section_header_flags & STYP_BLOCK) printf("(block)"); 
      if (section_header_flags & STYP_PASS) printf("(pass)"); 
      if (section_header_flags & STYP_CLINK) printf("(clink)");
      if (section_header_flags & STYP_VECTOR) printf("(vector)");
      if (section_header_flags & STYP_PADDED) printf("(padded)");
      printf("\n");
      printf("     Flag: %x\n",*((t_uint32 *) (section_header+40)));
      printf("     Res3: %x\n",*((t_uint16 *) (section_header+44)));
      printf("     Page: %x\n",*((t_uint16 *) (section_header+46)));
    }
  }


  OBJECT_SET_SYMBOL_TABLE (obj, SymbolTableNew (obj));
  
  fseek(fp, fpos + file_header_symtab_ptr, SEEK_SET);

  
  symbol_table = Calloc (sizeof(t_symbol *) , file_header_nsyms );
  for (i=0; i<file_header_nsyms; i++)
  {
    t_string symbol_name;
    unsigned char symbol[18];
    unsigned char aux_symbol[18];
    t_address value;
    t_uint32 ivalue;
    t_int16 sectionno;

    ret=fread(symbol,1,18,fp);
    ASSERT(ret == 18, ("Truncated TICOFF objectfile for C6000 series"));
   
    /* The first eight bytes of a symbol table entry (bytes 0-7) indicate a
     * symbol's name:
     * - If the symbol name is eight characters or less, this field has type
     *   character. The name is padded with nulls (if necessary) and stored in
     *   bytes 0-7.
     * - If the symbol name is greater than eight characters, this field is
     *   treated as two integers. The entire symbol name is stored in the
     *   string table. Bytes 0-3 contain 0, and bytes 4-7 are an offset into
     *   the string table. */

    if (symbol[0]!=0) 
    { 
      symbol_name=Malloc(9); 
      strncpy(symbol_name,(char *) symbol,8); 
      symbol_name[8]='\0'; 
    }
    else
    {
      t_uint32 name_offset=(*((t_uint32 *) (symbol+4)));
      
      if (switched_endian)
        name_offset = Uint32SwapEndian (name_offset);

      symbol_name=StringDup(stringtab+name_offset);
    }

    /* Bytes 8-11 of a symbol table entry indicate a symbol's value. The C_EXT,
     * C_STAT, and C_LABEL storage classes hold relocatable addresses. */

    ivalue =  *((t_uint32 *) (symbol+8));
    sectionno = *((t_int16 *) (symbol+12));

    if (switched_endian)
    {
      ivalue = Uint32SwapEndian(ivalue);
      sectionno = Uint16SwapEndian(sectionno);
    }

    value=AddressNew32(ivalue);
   
    /* Bytes 12-13 of a symbol table entry contain a number that indicates in
     * which section the symbol was defined. If a symbol has a section number
     * of 0, -1, or -2, it is not defined in a section. A section number of -1
     * indicates that the symbol has a value but is not relocatable. A section
     * number of 0 indicates a relocatable external symbol that is not defined
     * in the current file. */


    

    switch(sectionno)
    {
      case -2:
    printf("%3d: ",i); 
        printf("Debug symbol %s: val %x %x\n",symbol_name,G_T_UINT32(value),*((t_int16 *) (symbol+12)));
        symbol_table[i]=SymbolTableAddSymbol (OBJECT_SYMBOL_TABLE (obj), symbol_name, "R00A00+$", -1, TRUE, FALSE,  T_RELOCATABLE (abs), value, AddressNullForObject(obj), NULL, AddressNullForObject(obj), 0);
        break;
      case -1:
    printf("%3d: ",i); 
          switch(symbol[16])
          {
            case C_NULL:
              symbol_table[i]=SymbolTableAddSymbol (OBJECT_SYMBOL_TABLE (obj), symbol_name, "R00A00+$", -1, TRUE, FALSE,  T_RELOCATABLE (abs), value, AddressNullForObject(obj), NULL, AddressNullForObject(obj), 0);
              break;
            case C_EXT:
              printf("External ");
	      symbol_table[i]=SymbolTableAddSymbol (OBJECT_SYMBOL_TABLE (obj), symbol_name, "R00A00+$", 10, PERHAPS, FALSE,  T_RELOCATABLE (abs), value, AddressNullForObject(obj), NULL, AddressNullForObject(obj), 0);
	      break;
            case C_STAT:
              printf("Static ");
        symbol_table[i]=SymbolTableAddSymbol (OBJECT_SYMBOL_TABLE (obj), symbol_name, "R00A00+$", -1, TRUE, FALSE,  T_RELOCATABLE (abs), value, AddressNullForObject(obj), NULL, AddressNullForObject(obj), 0);
	      break;
            case C_LABEL:
              printf("Label ");
        symbol_table[i]=SymbolTableAddSymbol (OBJECT_SYMBOL_TABLE (obj), symbol_name, "R00A00+$", -1, TRUE, FALSE,  T_RELOCATABLE (abs), value, AddressNullForObject(obj), NULL, AddressNullForObject(obj), 0);
              break;
            default:
              FATAL(("SYMBOL CLASS %d\n",symbol[16]));
	  }

        printf("Absolute symbol %s: val %x %x\n",symbol_name,*((t_uint32 *) (symbol+8)),*((t_int16 *) (symbol+12)));
        break;
      case 0:
    printf("%3d: ",i); 
        printf("Undefined symbol %s: val %x %x\n",symbol_name,*((t_uint32 *) (symbol+8)),*((t_int16 *) (symbol+12)));
        symbol_table[i]=SymbolTableAddSymbol (OBJECT_SYMBOL_TABLE (obj), symbol_name, "R00A00+$", 0, PERHAPS, TRUE,  T_RELOCATABLE (undef), AddressNullForObject(obj), AddressNullForObject(obj), NULL, AddressNullForObject(obj), 0);
	printf("(%d)",symbol[16]);
        break;
      default:
        if (sections[sectionno-1].sec) 
        {
    printf("%3d: ",i); 
          switch(symbol[16])
          {
            case C_EXT:
              printf("External ");
              symbol_table[i]=SymbolTableAddSymbol (OBJECT_SYMBOL_TABLE (obj), symbol_name, "R00A00+$", 10, FALSE, FALSE,  T_RELOCATABLE (sections[sectionno-1].sec), AddressSub(value,SECTION_CADDRESS(sections[sectionno-1].sec)), AddressNullForObject(obj), NULL, AddressNullForObject(obj), 0);
              break;
            case C_STAT:
              printf("Static ");
              symbol_table[i]=SymbolTableAddSymbol (OBJECT_SYMBOL_TABLE (obj), symbol_name, "R00A00+$", -1, TRUE, FALSE,  T_RELOCATABLE (sections[sectionno-1].sec), AddressSub(value,SECTION_CADDRESS(sections[sectionno-1].sec)), AddressNullForObject(obj), NULL, AddressNullForObject(obj), 0);
              break;
            case C_LABEL:
              printf("Label ");
              symbol_table[i]=SymbolTableAddSymbol (OBJECT_SYMBOL_TABLE (obj), symbol_name, "R00A00+$", -1, TRUE, FALSE,  T_RELOCATABLE (sections[sectionno-1].sec), AddressSub(value,SECTION_CADDRESS(sections[sectionno-1].sec)), AddressNullForObject(obj), NULL, AddressNullForObject(obj), 0);
              break;
            default:
              FATAL(("SYMBOL CLASS %d\n",symbol[16]));
          }
          VERBOSE(0,("Symbol %s in section %s offset @G",symbol_name,SECTION_NAME(sections[sectionno-1].sec), AddressSub(value,SECTION_CADDRESS(sections[sectionno-1].sec))));
        }
        else
        {
          printf("%3d: Skipped symbol %d to ignored section %d\n", i, i ,sectionno-1);
        }
    }
    
    /* Each symbol table entry can have an auxiliary entry. An auxiliary symbol
     * table entry contains the same number of bytes as a symbol table entry (18). 
     * 	    Section Format for Auxiliary Table Entries 
     * 	    Bytes 0-3: Section length 
     * 	    Bytes 4-5: Number of relocation entries 
     * 	    Bytes 6-7: Number of line number entries 
     * 	    Bytes 8-17: Not used (zero filled) */

    if (symbol[17])
    {
       ret=fread(aux_symbol,1,18,fp);
       ASSERT(ret == 18, ("Truncated TICOFF objectfile for C6000 series"));
       i++;
    }
  }
  
  OBJECT_SET_RELOC_TABLE (obj, RelocTableNew (obj));


  for (i=0; i < file_header_nsecs; i++)
  {
    if ((sections[i].sec)&&(sections[i].nrels))
    {
      t_uint32 j;
      unsigned char reloc[12];
      t_uint32 reloc_address;
      t_uint32 reloc_symbol_index;
      t_uint16 reloc_type;
      VERBOSE(0,("Reading %d relocs for section %s at %x",sections[i].nrels,SECTION_NAME(sections[i].sec), sections[i].relptr));
      fseek(fp, fpos + sections[i].relptr, SEEK_SET);
      for (j=0; j<sections[i].nrels; j++)
      {
        ret=fread(reloc,1,12,fp);
       
        ASSERT(ret == 12, ("Truncated TICOFF objectfile for C6000 series"));
        reloc_address=*((t_uint32 *) reloc);
        reloc_symbol_index=*((t_uint32 *) (reloc+4));
        reloc_type=*((t_uint16 *) (reloc+10));

        if (switched_endian)
        {
          reloc_address = Uint32SwapEndian (reloc_address);
          reloc_symbol_index = Uint32SwapEndian (reloc_symbol_index);
          reloc_type = Uint16SwapEndian (reloc_type);
        }

        printf("Reloc from %s+%x to symbol %d type\n",SECTION_NAME(sections[i].sec), reloc_address,reloc_symbol_index);

        switch (reloc_type)
        {
          /* R_RELLONG: 32bit reference to symbol value */
          case 0x11:
            if (((t_int32) reloc_symbol_index)==-1)
            {
              t_address generic = AddressNew32 (reloc_address);
              t_uint32 data = SectionGetData32 (sections[i].sec, generic);
              t_address addend = AddressNew32 (data);

              RelocTableAddRelocToRelocatable (OBJECT_RELOC_TABLE (obj), AddressNullForObject(obj), T_RELOCATABLE(sections[i].sec) , generic, T_RELOCATABLE(sections[i].sec), addend, FALSE, NULL, NULL, NULL, "S00A00+" "\\" WRITE_32);
            }
            else if (symbol_table[reloc_symbol_index])
            {
              t_address generic = AddressNew32 (reloc_address);
              t_uint32 data = SectionGetData32 (sections[i].sec, generic);
              t_address addend = AddressNew32 (data);
              printf("RELLONG to %s\n",SYMBOL_NAME(symbol_table[reloc_symbol_index]));
              RelocTableAddRelocToSymbol (OBJECT_RELOC_TABLE (obj), addend, T_RELOCATABLE(sections[i].sec), generic, symbol_table[reloc_symbol_index], FALSE, NULL, NULL, NULL, "S00A00+" "\\" WRITE_32);
            }
            else
              FATAL(("No symbol for RELLONG reloc"));
            break;
	  /* PCR23H */  
	  case 0x16:
	    if (((t_int32) reloc_symbol_index)==-1)
            {
              t_address generic = AddressNew32 (reloc_address);
              t_uint32 data = SectionGetData32 (sections[i].sec, generic);
              t_address addend = AddressNew32 (data);

              RelocTableAddRelocToRelocatable (OBJECT_RELOC_TABLE (obj), AddressNullForObject(obj), T_RELOCATABLE(sections[i].sec) , generic, T_RELOCATABLE(sections[i].sec), addend, FALSE, NULL, NULL, NULL, "s0000" "\\" WRITE_32);
            }
            else if (symbol_table[reloc_symbol_index])
            {
              t_uint32 tmp;
              t_address generic = AddressNew32 (reloc_address);
              t_uint32 data = SectionGetData32 (sections[i].sec, generic);
              t_address addend;

	            tmp = Uint32SelectBits (data, 10, 0);
	            tmp <<= 11;
	            tmp = Uint32SignExtend (tmp, 21);
	            data = Uint32SelectBits (data, 26, 16);
	            data |= tmp;
	            data <<= 1;

              addend = AddressNew32 (0);
              printf("RELLONG to %s\n",SYMBOL_NAME(symbol_table[reloc_symbol_index]));
              RelocTableAddRelocToSymbol (OBJECT_RELOC_TABLE (obj), addend, T_RELOCATABLE(sections[i].sec), generic, symbol_table[reloc_symbol_index], FALSE, NULL, NULL, NULL,  "u00?s0004P+:S00! u00?s0001:=M! ?P:Pifffffffc&!-A00+\\s0001 > = i003ff800 & s000b > % i000007ff & s0010 < | l if800f800 & | u00?s0001:S00M! ?:iefffffff&! w\\s0000$");
            }
            else
              FATAL(("No symbol for RELLONG reloc"));
            break;

          /* PCR24W */
          case 0x17:
            if (((t_int32) reloc_symbol_index)==-1)
            {
              t_address generic = AddressNew32 (reloc_address);
              t_uint32 data = SectionGetData32 (sections[i].sec, generic);
              t_address addend = AddressNew32 (data);

              RelocTableAddRelocToRelocatable (OBJECT_RELOC_TABLE (obj), AddressNullForObject(obj), T_RELOCATABLE(sections[i].sec) , generic, T_RELOCATABLE(sections[i].sec), addend, FALSE, NULL, NULL, NULL, "S00A00+" "\\" WRITE_32);
            }
            else if (symbol_table[reloc_symbol_index])
            {
              t_address generic = AddressNew32 (reloc_address);
              t_uint32 data = SectionGetData32 (sections[i].sec, generic);
              t_address addend = AddressNew32 (data);
              printf("RELLONG to %s\n",SYMBOL_NAME(symbol_table[reloc_symbol_index]));
              RelocTableAddRelocToSymbol (OBJECT_RELOC_TABLE (obj), addend, T_RELOCATABLE(sections[i].sec), generic, symbol_table[reloc_symbol_index], FALSE, NULL, NULL, NULL, "S00A00+" "\\" WRITE_32);
            }
            else
              FATAL(("No symbol for RELLONG reloc"));
            break;

          /* R_C60BASE: Data page pointer-based offset */
          case 0x50:
            if (((t_int32) reloc_symbol_index)==-1)
            {
              FATAL(("Implement"));
            }
            else if (symbol_table[reloc_symbol_index])
            {
	      t_symbol * bss=SymbolTableAddSymbol (OBJECT_SYMBOL_TABLE (obj), "$bss", "R00A00+$", 0, PERHAPS, TRUE,  T_RELOCATABLE (undef), AddressNullForObject(obj), AddressNullForObject(obj), NULL, AddressNullForObject(obj), 0);
              t_address generic = AddressNew32 (reloc_address);
              t_uint32 data = SectionGetData32 (sections[i].sec, generic);


	      t_uint32 ldst=Uint32ExtractBits(data, 3, 4);


	      switch (ldst)
	      {
                /* LDBU */
                case 1:
                /* LDB */
                case 2:
                /* STB */
                case 3:
                  {
                  t_address addend = AddressNew32 (Uint32SignExtend(Uint32ExtractBits(data, 15, 8),14));
                  RelocTableAddRelocToSymbol (OBJECT_RELOC_TABLE (obj), addend, T_RELOCATABLE(sections[i].sec), generic, symbol_table[reloc_symbol_index], FALSE, NULL, NULL, bss, "S00A00+S01-" "\\" "i00007fff & s0008<liffc0007f&|w" "\\" "s0000$");
                  }

                  break;
                /* LDHU */
                case 0:
                /* LDH */
                case 4:
                /* STH */
                case 5:
                  {
                  t_address addend = AddressNew32 (2*Uint32SignExtend(Uint32ExtractBits(data, 15, 8),14));
                  RelocTableAddRelocToSymbol (OBJECT_RELOC_TABLE (obj), addend, T_RELOCATABLE(sections[i].sec), generic, symbol_table[reloc_symbol_index], FALSE, NULL, NULL, bss, "S00A00+S01-" "\\" "s0001> i00007fff & s0008<liffc0007f&|w" "\\" "s0000$");
                  }

                  break;
                /* LDW */
                case 6:
                /* STW */
                case 7:
                  {
                  t_address addend = AddressNew32 (4*Uint32SignExtend(Uint32ExtractBits(data, 15, 8),14));
                  RelocTableAddRelocToSymbol (OBJECT_RELOC_TABLE (obj), addend, T_RELOCATABLE(sections[i].sec), generic, symbol_table[reloc_symbol_index], FALSE, NULL, NULL, bss, "S00A00+S01-" "\\" "s0002> i00007fff & s0008<liffc0007f&|w" "\\" "s0000$");
                  }
                  break;
	      }

	      
              printf("C60BASE to %s\n",SYMBOL_NAME(symbol_table[reloc_symbol_index]));
            }
            else
              FATAL(("No symbol for C60BASE reloc"));
            break;
          /* R_C60PCR21: 21-bit packet, PC relative */
          case 0x52:
            if (((t_int32) reloc_symbol_index)==-1)
            {
              FATAL(("Implement"));
            }
            else if (symbol_table[reloc_symbol_index])
            {
              t_address generic = AddressNew32 (reloc_address);
              t_uint32 data = SectionGetData32 (sections[i].sec, generic);
              t_address addend = AddressNullForObject(obj); //AddressNew32 (Uint32SignExtend(Uint32ExtractBits(data, 21, 7),20));
              printf("C60PCR21 to %s (addend=%d) %x %x\n",SYMBOL_NAME(symbol_table[reloc_symbol_index]),Uint32SignExtend(Uint32ExtractBits(data, 21, 7),20), data, Uint32SwapEndian(data));

              RelocTableAddRelocToSymbol (OBJECT_RELOC_TABLE (obj), addend, T_RELOCATABLE(sections[i].sec), generic, symbol_table[reloc_symbol_index], FALSE, NULL, NULL, NULL, "S00Piffffffe0& -A00+" "\\" "s0002> i001fffff & s0007<lif000007f&|w" "\\" "s0000$");
            }
            else
              FATAL(("No symbol for C60PCR21 reloc"));
            break;
          /* R_C60LO16 0054h MVK instruction low half register */
          case 0x54:
            if (((t_int32) reloc_symbol_index)==-1)
            {
              //FATAL(("Implement"));
            }
            else if (symbol_table[reloc_symbol_index])
            {
              t_address generic = AddressNew32 (reloc_address);
              t_uint32 data = SectionGetData32 (sections[i].sec, generic);
              t_address addend = AddressNew32 (Uint32SignExtend(Uint32ExtractBits(data, 16, 7),15));
              printf("C60LO16 to %s\n",SYMBOL_NAME(symbol_table[reloc_symbol_index]));
              RelocTableAddRelocToSymbol (OBJECT_RELOC_TABLE (obj), addend, T_RELOCATABLE(sections[i].sec), generic, symbol_table[reloc_symbol_index], FALSE, NULL, NULL, NULL, "S00A00+i0000ffff & " "\\" "s0007<liff80007f&|w" "\\" "s0000$");
            }
            else
              FATAL(("No symbol for C60LO16 reloc"));
            break;
          /* R_C60HI16 0055h MVKH or MVKLH high half register */
          case 0x55:
            if (((t_int32) reloc_symbol_index)==-1)
            {
              //FATAL(("Implement"));
            }
            else if (symbol_table[reloc_symbol_index])
            {
              t_address generic = AddressNew32 (reloc_address);
              t_uint32 data = SectionGetData32 (sections[i].sec, generic);
              t_address addend = AddressNew32 (Uint32SignExtend(Uint32ExtractBits(data, 16, 7),15));

              printf("C60HI16 to %s\n",SYMBOL_NAME(symbol_table[reloc_symbol_index]));
              RelocTableAddRelocToSymbol (OBJECT_RELOC_TABLE (obj), addend, T_RELOCATABLE(sections[i].sec), generic, symbol_table[reloc_symbol_index], FALSE, NULL, NULL, NULL, "S00A00+ iffff0000 &" "s0009>\\" "liff80007f&|w" "\\" "s0000$");
            }
            else
              FATAL(("No symbol for R_C60HI16 reloc"));
            break;
          /* RE_ADD 4000h Operator instruction + */
          case 0x4000:
            printf("RE_ADD\n");
            break;
          /* RE_SUB 4001h Operator instruction ? */
          case 0x4001:
            printf("RE_SUB\n");
            break;
          /* RE_DIV 4004h Operator instruction / */
          case 0x4004:
            printf("RE_DIV\n");
            break;
          /* RE_SSTFLD 4010h Signed relocation field store */
          case 0x4010:
            printf("RE_SSTFLD\n");
            break;
          /* RE_PUSH 4011h Push symbol on the stack */
          case 0x4011:
            printf("RE_PUSH\n");
            break;
          /* RE_PUSHUK 4013h Push unsigned constant on the stack */
          case 0x4013:
            printf("RE_PUSHUK\n");
            break;
          /* RE_XSTFLD 4016h Signed state is not relevant */
          case 0x4016:
            printf("RE_XSTFLD\n");
            break;
          default:
            FATAL(("Reloc type %d\n",reloc_type));
        }
      }
    }
  }

}

void DiabloTiCoffInit(int argc, char ** argv)
{
  ObjectHandlerAdd("TICOFF", NULL,   IsTiCoff, NULL, NULL, NULL, NULL, NULL, NULL,NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
#ifdef BIT32ADDRSUPPORT
  ObjectHandlerAdd("TICOFF", "C6000",   IsTiCoffC6000, TiCoffC6000Read, NULL, NULL, NULL, NULL, NULL,NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
  ObjectHandlerAdd("TICOFF", "ARM",   IsTiCoffARM, TiCoffARMRead, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
#endif
}

void DiabloTiCoffFini()
{
}

