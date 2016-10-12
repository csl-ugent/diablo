/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

#include "diabloflowgraph_dwarf.h"

#include <map>
#include <string>
#include <iostream>

using namespace std;

INS_DYNAMIC_MEMBER_GLOBAL_ARRAY(lineinfo);

static map<string, char*> file_name_cache;

static void
GetLineInfoForAddress(AddressRangeTableEntry *entry, t_address addr, InsLineInfo *line_info)
{
  /* the DW_AT_stmt_list attribute contains the line number program header */
  DwarfAbstractParsedAttribute * stmt_list_attr =
            LookupAttributeForAbbreviationTableEntry(static_cast<DwarfAbbrevTableEntry *>(entry->cu_header->children[0]), DwarfAttributeCode::DW_AT_stmt_list);
  /* the DW_AT_comp_dir attribute contains the compilation directory */
  DwarfAbstractParsedAttribute *comp_dir_attr =
            LookupAttributeForAbbreviationTableEntry(static_cast<DwarfAbbrevTableEntry *>(entry->cu_header->children[0]), DwarfAttributeCode::DW_AT_comp_dir);
  /* the DW_AT_name attribute contains the full path and name of the compiled unit */
  DwarfAbstractParsedAttribute *name_attr =
            LookupAttributeForAbbreviationTableEntry(static_cast<DwarfAbbrevTableEntry *>(entry->cu_header->children[0]), DwarfAttributeCode::DW_AT_name);

  DwarfLineNumberProgramHeader *hdr;
  t_uint32 file_idx;
  string *file_name;
  string tmp_file_name;

  /* iterate over the line number information table */
  DwarfLineInfoMatrix *matrix = NULL;
  int row_idx = -1;
  for (auto m : entry->cu_header->line_info_matrices)
  {
    row_idx = -1;

    /* is the address covered by this matrix? */
    if (AddressIsLt(addr, m->at(0).address)
        || AddressIsGe(addr, m->at(m->size()-1).address))
      continue;

    /* look for the owning row */
    for (size_t i = 1; i < m->size(); i++)
    {
      if (AddressIsLt(addr, m->at(i).address))
      {
        row_idx = i-1;
        matrix = m;
        break;
      }
    }

    if (matrix)
      break;
  }

  /* don't FATAL here because entries in the non-contiguous address range list
   * attribute don't necessarily have a line info matrix associated with it. */
  /* Linux/ARM, GCC 4.6.4 (no thumb), dynamic, Os, 447.dealII; instruction at 0xef5a8 */
  if (!matrix)
    return;

  /* save line information */
  line_info->line = matrix->at(row_idx).line;

  /* process the file identification */
  hdr = static_cast<DwarfLinePtrAttribute *>(stmt_list_attr)->line_info;

  /* make it zero-based */
  file_idx = matrix->at(row_idx).file - 1;

  /* range check */
  ASSERT(file_idx < hdr->file_names.size(), ("file index exceeded %d/%d", file_idx, hdr->file_names.size()));

  /* save file information */
  file_name = hdr->file_names[file_idx]->name;

  /* relative path? */
  if (file_name->at(0) != '/')
  {
    /* take into account the directory index */
    t_uint32 dir_idx = hdr->file_names[file_idx]->directory_index;
    string *dir_name;

    if (dir_idx == 0)
    {
      /* DW_AT_comp_dir */
      ASSERT(comp_dir_attr, ("could not find DW_AT_comp_dir attribute @G", addr));
      dir_name = static_cast<DwarfStringAttribute *>(comp_dir_attr->decoded)->value;
    }
    else
    {
      /* directory in include_directories list */
      dir_name = hdr->include_directories[dir_idx - 1];
    }

    tmp_file_name = *dir_name + "/" + *file_name;
  }
  else
  {
    /* just use the file name as-is */
    tmp_file_name = *file_name;
  }

  if (file_name_cache.find(tmp_file_name) == file_name_cache.end())
  {
    char *new_str = new char[tmp_file_name.length() + 1];
    sprintf(new_str, "%s", tmp_file_name.c_str());

    file_name_cache[tmp_file_name] = new_str;
  }

  line_info->file = file_name_cache[tmp_file_name];
}

typedef struct {
  t_address low_pc;
  t_address high_pc;
  AddressRangeTableEntry * a_entry;
  DwarfAbbrevTableEntry * d_entry;
} lineinfo_lookup_entry;

#define ENTRY_TABLE_SIZE 1024*1024*16
static lineinfo_lookup_entry list_entries[ENTRY_TABLE_SIZE];
static int nr_list_entries = 0;

int entry_sort_function(const void * a, const void * b)
{
  lineinfo_lookup_entry *entry_a = (lineinfo_lookup_entry*)a;
  lineinfo_lookup_entry *entry_b = (lineinfo_lookup_entry*)b;
  return AddressIsLt(entry_a->low_pc,entry_b->low_pc)?-1:AddressIsEq(entry_a->low_pc,entry_b->low_pc)?0:1;
}

void DwarfTableForLineInfoAddEntry(AddressRangeTableEntry * a_entry, DwarfAbbrevTableEntry *d_entry)
{
  DwarfAbstractParsedAttribute *low_pc_attr = LookupAttributeForAbbreviationTableEntry(d_entry, DwarfAttributeCode::DW_AT_low_pc);
  t_address low_pc;
  
  DwarfAbstractParsedAttribute *high_pc_attr = LookupAttributeForAbbreviationTableEntry(d_entry, DwarfAttributeCode::DW_AT_high_pc);
  t_address high_pc;
  
  DwarfAbstractParsedAttribute *ranges_attr = LookupAttributeForAbbreviationTableEntry(d_entry, DwarfAttributeCode::DW_AT_ranges);
  
  /* first possible pattern: DW_AT_low_pc and DW_AT_high_pc specified */
  if (low_pc_attr && high_pc_attr)
    {
      low_pc = static_cast<t_address>(static_cast<DwarfAddressAttribute *>(low_pc_attr->decoded)->value);
      
      /* high_pc_attr can be of multiple types */
      if (high_pc_attr->decoded->form == DwarfAttributeForm::Address)
	high_pc = static_cast<t_address>(static_cast<DwarfAddressAttribute *>(high_pc_attr->decoded)->value);
      else if (high_pc_attr->decoded->form == DwarfAttributeForm::Constant)
	high_pc = AddressAdd(low_pc, static_cast<t_address>(static_cast<DwarfConstantAttribute *>(high_pc_attr->decoded)->value));
      else
	FATAL(("unsupported DW_AT_high_pc form"));
      
      list_entries[nr_list_entries].low_pc = low_pc;
      list_entries[nr_list_entries].high_pc = high_pc;
      list_entries[nr_list_entries].d_entry = d_entry;
      list_entries[nr_list_entries].a_entry = a_entry;
      nr_list_entries++;
      ASSERT(nr_list_entries<ENTRY_TABLE_SIZE-1,("Tried to allocate too many entries in dwarf line info entry table"));
    }
  else if (ranges_attr)
    {
      /* non-contiguous address range(s) specified */
      DwarfRangeListPtrAttribute *ranges_attr_ = static_cast<DwarfRangeListPtrAttribute *>(ranges_attr);
      for (auto range_entry : *ranges_attr_->range_list)
	{
	  list_entries[nr_list_entries].low_pc = range_entry->first;
	  list_entries[nr_list_entries].high_pc = range_entry->second;
	  list_entries[nr_list_entries].d_entry = d_entry;
	  list_entries[nr_list_entries].a_entry = a_entry;
	  nr_list_entries++;
	  ASSERT(nr_list_entries<ENTRY_TABLE_SIZE-1,("Tried to allocate too many entries in dwarf line info entry table"));
	}
    }
  else if (low_pc_attr && !high_pc_attr)
    {
      /* look into the abbreviation table for subprograms with specified ranges */
      for (auto e : d_entry->children)
	{
	  auto et = static_cast<DwarfAbbrevTableEntry *>(e);
	  
	  /* not a subprogram tag --> continue */
	  if (et->declaration->tag != 0x2e) continue;
	  
	  /* found subprogram */
	  DwarfTableForLineInfoAddEntry(a_entry,et);
	}
    }
}



void DwarfBuildTableForLineInfo(DwarfSections *dwarf_sections, DwarfInfo dwarf_info)
{
  AddressRangeTable *arange_table = static_cast<AddressRangeTable *>(dwarf_info.arange_table);
  AddressRangeTableEntry *entry = NULL;

  for (auto entry_it : *arange_table)
    DwarfTableForLineInfoAddEntry(entry_it, static_cast<DwarfAbbrevTableEntry *>(entry_it->cu_header->children[0]));
  
  qsort(list_entries,nr_list_entries,sizeof(lineinfo_lookup_entry),entry_sort_function);
}

InsLineInfo *
DwarfGetLineInfoForIns (t_ins *ins, DwarfSections *dwarf_sections, DwarfInfo dwarf_info)
{
  InsLineInfo *info = NULL;
  t_address addr = INS_OLD_ADDRESS(ins);
  /* iterate of the Dwarf address range table, looking for a compilation unit */
  AddressRangeTable *arange_table = static_cast<AddressRangeTable *>(dwarf_info.arange_table);
  AddressRangeTableEntry *entry = NULL;

  // binary search in array with entries
  
  int first = 0;
  int last = nr_list_entries - 1;
  int middle = (first+last)/2;
  
  while (first <= last) {
    if (AddressIsGe(addr,list_entries[middle].high_pc))
      first = middle + 1;    
    else if (AddressIsGe(addr,list_entries[middle].low_pc) && AddressIsLt(addr,list_entries[middle].high_pc))
      {
	entry = list_entries[middle].a_entry;
	break;
      }
    else
      last = middle - 1;
    middle = (first + last)/2;
  } 
  
  if (!entry)
    // no debugging information found for this address 
    return NULL; 
  
  /* If we reach this point, debugging information was found.
   * The address range table entry is selected. */
  info = new InsLineInfo();
  GetLineInfoForAddress(entry, addr, info);
  
  return info;
}

void
CfgAssociateLineInfoWithIns(t_cfg *cfg, DwarfSections *dwarf_sections, DwarfInfo dwarf_info)
{
  t_ins * ins;
  t_bbl * bbl;

  DwarfBuildTableForLineInfo(dwarf_sections, dwarf_info);

  CFG_FOREACH_BBL(cfg, bbl)
    BBL_FOREACH_INS(bbl, ins)
    {
      /* line information data structure */
      INS_SET_LINEINFO(ins, DwarfGetLineInfoForIns (ins, dwarf_sections, dwarf_info));

      /* fast accessors */
      if (INS_LINEINFO(ins))
      {
        INS_SET_SRC_FILE(ins, INS_LINEINFO(ins)->file);
        INS_SET_SRC_LINE(ins, INS_LINEINFO(ins)->line);
      }
    }
}

void
CfgFileNameCacheFree()
{
  for (auto e : file_name_cache)
    delete[] e.second;
}
