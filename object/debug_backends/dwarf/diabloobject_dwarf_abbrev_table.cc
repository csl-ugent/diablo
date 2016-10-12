/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

#include "diabloobject_dwarf.h"

#include <iostream>
#include <map>
#include <vector>

using namespace std;

static map<t_address, DwarfAbbrevDeclarationList *> dwarf_abbrev_table_cache;

/* Constructs an abbreviation table, given the section  */
DwarfAbbrevDeclarationList *
ReadAbbreviationDeclarationList(t_section *sec, t_address offset)
{
  DwarfAbbrevDeclarationList *new_table;

  /* cache lookup */
  if (dwarf_abbrev_table_cache.find(offset) != dwarf_abbrev_table_cache.end())
    return dwarf_abbrev_table_cache[offset];

  /* create a new table instance */
  new_table = new DwarfAbbrevDeclarationList();
  dwarf_abbrev_table_cache[offset] = new_table;

  /* list of declarations */
  while (true)
  {
    DwarfAbbrevDeclaration *new_declaration;
    t_uint32 n_bytes = 0;
    DwarfDecodedULEB128 code = 0;

    code = DwarfDecodeULEB128(DwarfReadULEB128FromSection(sec, offset, n_bytes));
    offset = AddressAddUint32(offset, n_bytes);

    /* end of abbreviation list */
    if (code == 0)
      break;

    /* create a new declaration entry */
    new_declaration = new DwarfAbbrevDeclaration();
    new_table->push_back(new_declaration);

    /* code was already read beforehand, to determine the presence of an entry */
    new_declaration->code = code;

    /* tag */
    new_declaration->tag = DwarfDecodeULEB128(DwarfReadULEB128FromSection(sec, offset, n_bytes));
    offset = AddressAddUint32(offset, n_bytes);

    /* children */
    new_declaration->has_children = (static_cast<DwarfChildEncoding>(SectionGetData8(sec, offset)) == DwarfChildEncoding::DW_CHILDREN_yes);
    offset = AddressAddUint32(offset, 1);

    /* list of attributes */
    while (true)
    {
      DwarfAttributeSpec *new_attribute;

      DwarfDecodedULEB128 name = 0;
      DwarfDecodedULEB128 form = 0;

      /* name */
      name = DwarfDecodeULEB128(DwarfReadULEB128FromSection(sec, offset, n_bytes));
      offset = AddressAddUint32(offset, n_bytes);

      /* form */
      form = DwarfDecodeULEB128(DwarfReadULEB128FromSection(sec, offset, n_bytes));
      offset = AddressAddUint32(offset, n_bytes);

      /* end of attribute list */
      if (name == 0
          && form == 0)
        break;

      /* create a new attribute for this declaration entry */
      new_attribute = new DwarfAttributeSpec();
      new_declaration->attributes.push_back(new_attribute);

      new_attribute->name = static_cast<DwarfAttributeCode>(name);
      new_attribute->form = static_cast<DwarfFormCode>(form);
    }
  }

  return new_table;
}

/* parses the abbreviation table associated with 'cu_header' */
void
ParseAbbreviationTable(DwarfCompilationUnitHeader *cu_header, DwarfSections *dwarf_sections, t_address offset)
{
  DwarfDecodedULEB128 declaration_id;
  bool iterate = false;
  int current_level = 0;

  vector<DwarfDebugInformationEntry *> nested_entries;
  nested_entries.push_back(cu_header);

  do
  {
    t_uint32 n_bytes = 0;
    DwarfAbbrevDeclaration *declaration;
    DwarfAbbrevTableEntry *entry;
    vector<DwarfAbstractAttribute *> attributes;

    /* read in the declaration id for this declaration */
    declaration_id = DwarfDecodeULEB128(DwarfReadULEB128FromSection(dwarf_sections->info_section, offset, n_bytes));
    offset = AddressAddUint32(offset, n_bytes);

    /* possible early exit */
    if (declaration_id == 0)
    {
      if (current_level == 1)
      {
        /* we already are in the upper-most level,
         * a NULL-entry indicates the end of this table */
        break;
      }
      else
      {
        /* a NULL-entry indicates the end of this child list,
         * go up one level */
        current_level--;
        nested_entries.pop_back();
        continue;
      }
    }

    /* make it zero-based */
    declaration_id--;

    ASSERT(declaration_id < cu_header->abbrev_table->size(),
           ("declaration ID %d out of range (max %d)", declaration_id, cu_header->abbrev_table->size()));

    /* look up the declaration */
    declaration = cu_header->abbrev_table->at(declaration_id);

    /* create a new entry of the abbrev table */
    entry = new DwarfAbbrevTableEntry();
    nested_entries.back()->children.push_back(entry);

    entry->declaration = declaration;

    /* Decode all attributes for this declaration and add them to a temporary list.
     * The reason we need to do this, is because some basic attributes are needed by
     * the more complex attributes which need additional parsing. */
    attributes.clear();
    for (DwarfAttributeSpec *attribute : declaration->attributes)
    {
      t_uint32 sz = 0;

      attributes.push_back(DwarfDecodeAttribute(attribute, cu_header, dwarf_sections, offset, sz));

      /* some attributes need to be parsed up front, to enable parsing more
       * complex attributes later on */
      if (!AttributeNeedsParsing(attributes.back()))
        entry->attributes.push_back(DwarfParseAttribute(cu_header, dwarf_sections, attributes.back()));

      offset = AddressAddUint32(offset, sz);
    }

    /* parse the decoded attributes (i.e. attributes of type rangelistptr, stmtlist, ...);
     * only attributes which load data from other sections need additional parsing. */
    for (DwarfAbstractAttribute *decoded : attributes)
      if (AttributeNeedsParsing(decoded))
        entry->attributes.push_back(DwarfParseAttribute(cu_header, dwarf_sections, decoded));

    /* children only exist for the upper-most DIE if the has_children property is TRUE. */
    if (declaration_id == 0
        && declaration->has_children)
      iterate = true;

    /* if this DIE has children, the next entries are owned by that DIE */
    if (declaration->has_children)
    {
      nested_entries.push_back(entry);
      current_level++;
    }
  } while (iterate);
}

/* Given an entry in the abbreviation table (i.e., a declaration), iterate over its list of
 * attributes and look for the first attribute with code 'attr_code'. */
DwarfAbstractParsedAttribute *
LookupAttributeForAbbreviationTableEntry(DwarfAbbrevTableEntry *entry, DwarfAttributeCode attr_code)
{
  for (DwarfAbstractParsedAttribute *attr : entry->attributes)
    if (attr->decoded->attr->name == attr_code)
      return attr;

  return NULL;
}

vector<DwarfAbstractParsedAttribute *>
LookupAllAttributeForAbbreviationTableEntryArray(DwarfAbbrevTableEntry *entry, DwarfAttributeCode attr_code)
{
  vector<DwarfAbstractParsedAttribute *> ret;

  for (DwarfAbstractParsedAttribute *attr : entry->attributes)
    if (attr->decoded->attr->name == attr_code)
      ret.push_back(attr);

  return ret;
}

void
AbbreviationTableFreeDeclarationLists()
{
  for (auto& map_entry : dwarf_abbrev_table_cache)
  {
    for (auto e : *(map_entry.second))
      delete e;

    delete map_entry.second;
  }
}

/* For debugging purposes: print some indentation whitespace */
static
void
_print_indent(int depth)
{
  for (int i = 0; i < depth; i++)
    cout << " ";
}

/* For debugging purposes: recursively print an abbreviation table */
static
void
PrintAbbreviationTableRecursive(DwarfAbbrevTableEntry *entry, int depth)
{
  /* tag */
  _print_indent(depth);
  cout << "T: " << TagToString(entry->declaration->tag) << endl;

  /* attributes */
  for (auto i : entry->attributes)
  {
    _print_indent(depth+1);
    cout << "A: " << AttributeToString(i->decoded->attr->name) << " (" << i->decoded->ToString() << ")" << endl;
  }

  /* children */
  for (auto i : entry->children)
    PrintAbbreviationTableRecursive(static_cast<DwarfAbbrevTableEntry *>(i), depth+2);
}

/* For debugging purposes: print an abbreviation table owned by a compilation unit */
void
PrintAbbreviationTable(DwarfCompilationUnitHeader *cu_header)
{
  for (auto i : cu_header->children)
    PrintAbbreviationTableRecursive(static_cast<DwarfAbbrevTableEntry *>(i), 0);
}
