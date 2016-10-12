/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

#ifdef BIT32ADDRSUPPORT
#include <diabloelf.h>

/* ElfRaw32BuildRegionMap {{{ */
t_file_regions * 
ElfRaw32BuildRegionMap(t_elf_raw32 * raw, t_bool include_shdrs, t_bool include_phdrs, t_uint32 exclude_section)
{
  t_uint32 i;
  t_file_regions * regions = FileRegionsNew();

  FileRegionsAddFileRegion(regions, 0, sizeof(Elf32_Ehdr), (char *) raw->hdr, StringDup("File Header"));
  if (include_shdrs)
    FileRegionsAddFileRegion(regions, raw->hdr->e_shoff, sizeof(Elf32_Shdr)*raw->hdr->e_shnum, (char *) raw->shdrs, StringDup("Section Headers"));
 
  if (include_phdrs)
  if (raw->hdr->e_phnum)
  {
    FileRegionsAddFileRegion(regions, raw->hdr->e_phoff, sizeof(Elf32_Phdr)*raw->hdr->e_phnum, (char *) raw->phdrs, StringDup("Program Headers"));
  }

  ASSERT((raw->hdr->e_shstrndx < raw->hdr->e_shnum) && (raw->sdatas[raw->hdr->e_shstrndx]), ("No valid section string table. Elf file is invalid!"));
  
  for (i = 0; i<raw->hdr->e_shnum; i++)
  {
    t_string name;
    
    if (raw->shdrs[i].sh_size == 0) continue;
    if (raw->shdrs[i].sh_type == SHT_NOBITS) continue;
    if (raw->sdatas[i] == NULL) continue;
    if (i == exclude_section) continue;
    
    name = raw->sdatas[raw->hdr->e_shstrndx] + raw->shdrs[i].sh_name;

    FileRegionsAddFileRegion(regions, raw->shdrs[i].sh_offset, raw->shdrs[i].sh_size, (char *) raw->sdatas[i], StringConcat2("Section Data: ", name));
  }

  FileRegionsSort(regions);
  
  return regions;
}
/* }}} */

/* ElfRaw32New {{{ */
t_elf_raw32 *
ElfRaw32New(t_bool le, t_uint32 machine)
{
  t_uint32 tel;
  t_elf_raw32 * ret = Malloc(sizeof(t_elf_raw32));
  ret->hdr = Calloc(1,sizeof(Elf32_Ehdr));

  ret->hdr->e_ident[EI_MAG0] = ELFMAG0;
  ret->hdr->e_ident[EI_MAG1] = ELFMAG1;
  ret->hdr->e_ident[EI_MAG2] = ELFMAG2;
  ret->hdr->e_ident[EI_MAG3] = ELFMAG3;
  ret->hdr->e_ident[EI_VERSION] = EV_CURRENT;

  if (le)
    ret->hdr->e_ident[EI_DATA] = ELFDATA2MSB;
  else
    ret->hdr->e_ident[EI_DATA] = ELFDATA2LSB;


  ret->hdr->e_ident[EI_CLASS] = ELFCLASS32;

  /* Padding : EI_PAD=7 from e_ident[7] to e_ident[15] */
  for (tel = EI_PAD; tel < 16; tel++)
  {
    ret->hdr->e_ident[tel] = 0;
  }

  ret->hdr->e_type = ET_EXEC;
  ret->hdr->e_machine = machine;
  ret->hdr->e_version = EV_CURRENT;

  ret->hdr->e_shentsize = sizeof(Elf32_Shdr);
  ret->hdr->e_ehsize = sizeof(Elf32_Ehdr);
  ret->hdr->e_shoff = sizeof(Elf32_Ehdr);
  ret->hdr->e_shnum = 2;
  ret->hdr->e_shstrndx = 1;
  ret->shdrs = Malloc(sizeof(Elf32_Shdr) * ret->hdr->e_shnum);
  ret->shdrs[0].sh_name = 0;
  ret->shdrs[0].sh_type = SHT_NULL;
  ret->shdrs[0].sh_flags = 0;
  ret->shdrs[0].sh_addr = 0;
  ret->shdrs[0].sh_offset = 0;
  ret->shdrs[0].sh_size = 0;
  ret->shdrs[0].sh_link = SHN_UNDEF;
  ret->shdrs[0].sh_info = 0;
  ret->shdrs[0].sh_addralign = 0;
  ret->shdrs[0].sh_entsize = 0;

  ret->shdrs[1].sh_name = 1;
  ret->shdrs[1].sh_type = SHT_STRTAB;
  ret->shdrs[1].sh_flags = 0;
  ret->shdrs[1].sh_addr = 0;
  ret->shdrs[1].sh_offset = sizeof(Elf32_Ehdr) + sizeof(Elf32_Shdr) * ret->hdr->e_shnum;
  ret->shdrs[1].sh_size = 1 + strlen(".strtab") + 1;
  ret->shdrs[1].sh_link = SHN_UNDEF;
  ret->shdrs[1].sh_info = 0;
  ret->shdrs[1].sh_addralign = 0;
  ret->shdrs[1].sh_entsize = 0;

  ret->sdatas = Calloc(sizeof(char *) , ret->hdr->e_shnum);
  ret->sdatas[1] = Calloc(1,ret->shdrs[1].sh_size);
  strcpy(ret->sdatas[1]+1, ".strtab");

  return ret;
}
/* }}} */

/* ElfRaw32Read {{{ */
t_elf_raw32 * 
ElfRaw32Read(FILE * fp)
{
  t_uint32 i;
  long fpos = ftell(fp);
  size_t size;
  t_elf_raw32 * ret = Malloc(sizeof(t_elf_raw32));
  ret->hdr = Malloc(sizeof(Elf32_Ehdr));
  
  size = fread(ret->hdr, sizeof(Elf32_Ehdr), 1, fp);

  ASSERT(size == 1, ("ElfReadRaw32: Could not read file header"));

  ASSERT((ret->hdr->e_phnum == 0) || (ret->hdr->e_phentsize == sizeof(Elf32_Phdr)), ("ElfReadRaw32: phentsize mismatch"));
  ASSERT(ret->hdr->e_shentsize == sizeof(Elf32_Shdr), ("ElfReadRaw32: shentsize mismatch"));
  ASSERT(ret->hdr->e_ehsize == sizeof(Elf32_Ehdr), ("ElfReadRaw32: ehsize mismatch"));

  /* Read shdrs */
  ASSERT(ret->hdr->e_shnum, ("ElfReadRaw32: ELF file has no sections. This is not implemented!")); 
  ret->shdrs = Malloc(sizeof(Elf32_Shdr) * ret->hdr->e_shnum);
  fseek(fp, fpos + ret->hdr->e_shoff, SEEK_SET);
  size = fread(ret->shdrs, sizeof (Elf32_Shdr), ret->hdr->e_shnum, fp);

  /* Read phdrs */
  if (ret->hdr->e_phnum)
  {
    ret->phdrs = Malloc(sizeof(Elf32_Phdr) * ret->hdr->e_phnum);
    fseek(fp, fpos + ret->hdr->e_phoff, SEEK_SET);
    size = fread(ret->phdrs, sizeof (Elf32_Phdr), ret->hdr->e_phnum, fp);
  }
  else  
    ret->phdrs = NULL;
 
  /* Read section data */
  ret->sdatas = Calloc(sizeof(char *) , ret->hdr->e_shnum);
  for (i = 0; i<ret->hdr->e_shnum; i++)
  {
    if (ret->shdrs[i].sh_size == 0) continue;
    if (ret->shdrs[i].sh_type == SHT_NOBITS) continue;
    ret->sdatas[i] = Malloc(ret->shdrs[i].sh_size);
    fseek(fp, fpos + ret->shdrs[i].sh_offset, SEEK_SET);
    size = fread(ret->sdatas[i], ret->shdrs[i].sh_size, 1, fp);
  }

  fseek(fp, fpos, SEEK_SET);
  return ret;
}
/* }}} */

/* ElfRaw32AddressToSectionName {{{ */
char *
ElfRaw32AddressToSectionName(t_elf_raw32 * raw, t_address address)
{
  t_uint32 i;
  for (i = 0; i<raw->hdr->e_shnum; i++)
  {
    if ((raw->shdrs[i].sh_addr <= G_T_UINT32(address)) && ((raw->shdrs[i].sh_addr + raw->shdrs[i].sh_size) > G_T_UINT32(address))&&(strcmp(raw->sdatas[raw->hdr->e_shstrndx] + raw->shdrs[i].sh_name,".tbss")!=0))
    {
      return StringDup(raw->sdatas[raw->hdr->e_shstrndx] + raw->shdrs[i].sh_name);
    }
  }

  return NULL;
}
/* }}} */

/* ElfRaw32AddSection {{{ */
char *
ElfRaw32AddSection(t_elf_raw32 * raw, t_const_string name, t_address virt, t_address asize, t_uint32 flags)
{
  t_uint32 size = G_T_UINT32(asize);
  t_file_regions * regions = ElfRaw32BuildRegionMap(raw, FALSE, TRUE, raw->hdr->e_shstrndx);
  t_uint32 new_section_data_start = FileRegionsFindGap(regions, size);
  char * data = Malloc(size);
  FileRegionsAddFileRegion(regions, new_section_data_start, size, data, StringDup("NEW SECTION"));

  FileRegionsSort(regions);
  printf("New section at %x size %x\n", new_section_data_start, size);
  printf("Placing shdrs\n");

  raw->hdr->e_shoff = FileRegionsFindGap(regions, sizeof(Elf32_Shdr)*(raw->hdr->e_shnum+1));
  printf("At %x\n", raw->hdr->e_shoff); 
  raw->shdrs = Realloc(raw->shdrs, sizeof(Elf32_Shdr)*(raw->hdr->e_shnum+1));
  raw->shdrs[raw->hdr->e_shnum].sh_offset = new_section_data_start;
  raw->shdrs[raw->hdr->e_shnum].sh_type = SHT_PROGBITS;
  raw->shdrs[raw->hdr->e_shnum].sh_size = size;
  raw->shdrs[raw->hdr->e_shnum].sh_name = 0;
  raw->shdrs[raw->hdr->e_shnum].sh_link = 0;
  raw->shdrs[raw->hdr->e_shnum].sh_info = 0;
  raw->shdrs[raw->hdr->e_shnum].sh_flags = flags;
  raw->shdrs[raw->hdr->e_shnum].sh_entsize = 0;
  raw->shdrs[raw->hdr->e_shnum].sh_addralign = 0;
  raw->shdrs[raw->hdr->e_shnum].sh_addr = G_T_UINT32(virt);
  raw->sdatas = Realloc(raw->sdatas, sizeof(char*)*(raw->hdr->e_shnum+1));
  raw->sdatas[raw->hdr->e_shnum]=data;
  raw->hdr->e_shnum++;
  FileRegionsFree(regions);

  printf("Placing strtab\n");
  regions = ElfRaw32BuildRegionMap(raw, TRUE, TRUE, raw->hdr->e_shstrndx);
  raw->shdrs[raw->hdr->e_shnum-1].sh_name = raw->shdrs[raw->hdr->e_shstrndx].sh_size;
  raw->shdrs[raw->hdr->e_shstrndx].sh_size += strlen(name) + 1;
  raw->sdatas[raw->hdr->e_shstrndx] = Realloc(raw->sdatas[raw->hdr->e_shstrndx], raw->shdrs[raw->hdr->e_shstrndx].sh_size);

  strcpy(raw->sdatas[raw->hdr->e_shstrndx] +  raw->shdrs[raw->hdr->e_shstrndx].sh_size - (strlen(name) + 1), name);
  raw->shdrs[raw->hdr->e_shstrndx].sh_offset = FileRegionsFindGap(regions, raw->shdrs[raw->hdr->e_shstrndx].sh_size);
  FileRegionsFree(regions);
  return data;
}
/* }}} */

/* ElfRaw32AddMarkerSection {{{ */
char *
ElfRaw32AddMarkerSection(t_elf_raw32 * raw, t_const_string name, t_address asize)
{
  return ElfRaw32AddSection(raw, name, AddressSub(asize,asize), asize, 0);
}
/* }}} */

/* ElfRaw32AddressToFileOffset {{{ */
t_address
ElfRaw32AddressToFileOffset(t_elf_raw32 * raw, t_address address)
{
  t_uint32 i;
  for (i = 0; i<raw->hdr->e_phnum; i++)
  {
    if (raw->phdrs[i].p_type != PT_LOAD) continue;
    if ((raw->phdrs[i].p_vaddr <= G_T_UINT32(address)) && (raw->phdrs[i].p_vaddr + raw->phdrs[i].p_memsz > G_T_UINT32(address)))
    {
      if ((G_T_UINT32(address) - raw->phdrs[i].p_vaddr)>raw->phdrs[i].p_filesz)
        return AddressNew32(0xffffffff);
      else
        return AddressNew32(raw->phdrs[i].p_offset + (G_T_UINT32(address) - raw->phdrs[i].p_vaddr));
    }
  }

  return AddressNew32(0xffffffff);
}
/* }}} */

/* ElfRaw32FileOffsetToAddress {{{ */
t_address
ElfRaw32FileOffsetToAddress(t_elf_raw32 * raw, t_address offset)
{
  t_uint32 i;
  for (i = 0; i<raw->hdr->e_phnum; i++)
  {
    if (raw->phdrs[i].p_type != PT_LOAD) continue;
    if ((raw->phdrs[i].p_offset <= G_T_UINT32(offset)) && (raw->phdrs[i].p_offset + raw->phdrs[i].p_filesz > G_T_UINT32(offset)))
    {
        return AddressNew32(raw->phdrs[i].p_vaddr + (G_T_UINT32(offset) - raw->phdrs[i].p_offset));
    }
  }

  return AddressNew32(0xffffffff);
}
/* }}} */

/* ElfRaw32Write {{{ */
void 
ElfRaw32Write(t_elf_raw32 * raw, FILE * fp)
{
  t_file_regions * regions = ElfRaw32BuildRegionMap(raw, TRUE, TRUE, 0xffffffff);
  FileRegionsWrite(regions, fp);
}
/* }}} */
#endif
/* vim: set shiftwidth=2 expandtab cinoptions=p5,t0,(0, foldmethod=marker: */
