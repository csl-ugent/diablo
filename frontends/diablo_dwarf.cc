#include <diabloflowgraph_dwarf.hpp>

extern "C" {
#include <diabloflowgraph.h>
#include <diabloelf.h>
#include <diablo_options.h>
#include <diabloarm.h>

#include <sys/stat.h>

#include <libdwarf/dwarf.h>
#include <libdwarf/libdwarf.h>
#include <libdwarf/dwarf_base_types.h>
#include <libdwarf/dwarf_opaque.h>
}

#include <frontends/common.h>

#ifdef CPREGSET
#undef CPREGSET
#endif
#define CPREGSET(cfg, regset) &arm_description, regset

using namespace std;

static int result;
static Dwarf_Error error;

static DwarfSections *dwarf_sections;

static LogFile *L_OUT;

static
void read_die_info(Dwarf_Die die, DwarfAbbrevTableEntry *entry) {
  Dwarf_Half tag = 0;
  dwarf_tag(die, &tag, &error);

  entry->declaration->code = static_cast<DwarfDecodedULEB128>(-1);
  entry->declaration->tag = static_cast<DwarfTagCode>(tag);

  Dwarf_Half has_children = 0;
  dwarf_die_abbrev_children_flag(die, &has_children);
  entry->declaration->has_children = has_children;
}

static
void read_attributes(Dwarf_Debug dbg, Dwarf_Die die, DwarfAbbrevTableEntry *entry, t_address offset) {
  Dwarf_Attribute *atlist;
  Dwarf_Signed atcount;
  dwarf_attrlist(die, &atlist, &atcount, &error);

  for (Dwarf_Signed i = 0; i < atcount; ++i) {
    Dwarf_Attribute attr = atlist[i];

    Dwarf_Half attrnum;
    dwarf_whatattr(attr, &attrnum, &error);

    Dwarf_Byte_Ptr info_ptr = 0;
    Dwarf_Half form = 0;
    _dwarf_get_value_ptr(die, attrnum, &form, &info_ptr, &error);

    DwarfAttributeSpec *a = new DwarfAttributeSpec();
    a->name = static_cast<DwarfAttributeCode>(attrnum);
    a->form = static_cast<DwarfFormCode>(form);
    a->use_fixed_offset = true;
    a->fixed_offset = AddressNew32(static_cast<t_uint8 *>(info_ptr) - static_cast<t_uint8*>(SECTION_DATA(dwarf_sections->info_section))) - offset;

    // DEBUG(("got attribute at @G", a->fixed_offset));

    entry->declaration->attributes.push_back(a);
  }

  dwarf_dealloc(dbg, atlist, DW_DLA_LIST);
}

static
void get_die_and_siblings(DwarfCompilationUnitHeader *cu_header, DwarfAbbrevTableEntry *parent, Dwarf_Debug dbg, Dwarf_Die in_die, int in_level, vector<DwarfAbstractAttribute *>& to_process, map<t_address, DwarfAbbrevTableEntry *>& table_entries, t_address offset) {
  Dwarf_Die cur_die = in_die;
  Dwarf_Die child = 0;

  for (;;) {
    DwarfAbbrevTableEntry *entry = new DwarfAbbrevTableEntry();
    entry->declaration = new DwarfAbbrevDeclaration();
    entry->offset = AddressNew32(static_cast<t_uint8 *>(cur_die->di_debug_ptr) - static_cast<t_uint8*>(SECTION_DATA(dwarf_sections->info_section)));

    if (parent)
      parent->children.push_back(entry);
    else {
      cu_header->children.push_back(entry);

      /* don't know why this is needed,
       * but for binaries that have their dwarf sections linked in after linking (i.e., Debian *-dbg packages linked to original binaries),
       * there seems to be a constant offset. Here we calculate that offset, and use it to compensate.
       * The offset is 0 for normal binaries and libraries (e.g., libc.so). */
      offset = entry->offset - offset;
    }

    entry->offset -= offset;
    table_entries[entry->offset] = entry;

    // DEBUG(("reading die at @G (%x)", entry->offset, offset));

    read_die_info(cur_die, entry);
    read_attributes(dbg, cur_die, entry, offset);

    ParseAttributes(entry, 0, to_process, cu_header, dwarf_sections);

    result = dwarf_child(cur_die, &child, &error);
    ASSERT(result != DW_DLV_ERROR, ("error in dwarf_child, level %d", in_level));

    if (result == DW_DLV_OK) {
      /* new instance */
      get_die_and_siblings(cu_header, entry, dbg, child, in_level+1, to_process, table_entries, offset);

      /* free up memory */
      dwarf_dealloc(dbg, child, DW_DLA_DIE);
      child = 0;
    }

    Dwarf_Die sib_die = 0;
    result = dwarf_siblingof_b(dbg, cur_die, true, &sib_die, &error);
    ASSERT(result != DW_DLV_ERROR, ("error in dwarf_siblingof_b, level %d", in_level));

    if (result == DW_DLV_NO_ENTRY) {
      /* no more entries, we're done here */
      break;
    }

    if (cur_die != in_die) {
      dwarf_dealloc(dbg, cur_die, DW_DLA_DIE);
      cur_die = 0;
    }

    cur_die = sib_die;
  }
}

static
void read_cu_list(Dwarf_Debug dbg) {
  for (int cu_number = 0; ; ++cu_number) {
    Dwarf_Unsigned cu_header_length = 0;
    Dwarf_Half version_stamp = 0;
    Dwarf_Unsigned abbrev_offset = 0;
    Dwarf_Half address_size = 0;
    Dwarf_Half offset_size = 0;
    Dwarf_Half extension_size = 0;
    Dwarf_Sig8 signature;
    Dwarf_Unsigned typeoffset = 0;
    Dwarf_Unsigned next_cu_header = 0;
    Dwarf_Half header_cu_type = DW_UT_compile;
    result =  dwarf_next_cu_header_d(dbg, true, &cu_header_length, &version_stamp, &abbrev_offset, &address_size, &offset_size, &extension_size, &signature, &typeoffset, &next_cu_header, &header_cu_type, &error);
    ASSERT(result != DW_DLV_ERROR, ("error reading cu header"));

    if (result == DW_DLV_NO_ENTRY) {
      /* no more entries, we're done */
      break;
    }

    /* first sibling */
    Dwarf_Die cu_die = 0;
    result = dwarf_siblingof_b(dbg, NULL, true, &cu_die, &error);
    ASSERT(result != DW_DLV_ERROR, ("error reading sibling of CU die"));
    ASSERT(result != DW_DLV_NO_ENTRY, ("this should not happen"));

    /* construct CU header */
    DwarfCompilationUnitHeader *cu_header = new DwarfCompilationUnitHeader();
    cu_header->offset = cu_die->di_cu_context->cc_debug_offset;
    cu_header->unit_length = cu_header_length;
    cu_header->is_64bit = false;
    cu_header->version = version_stamp;
    cu_header->debug_abbrev_offset = abbrev_offset;
    cu_header->address_size = address_size;
    //cu_header->header_size is not set

    // DEBUG(("CU @G (0x%x)", cu_header->offset, cu_die->di_cu_context->cc_cu_die_global_sec_offset));

    vector<DwarfAbstractAttribute *> to_process;
    map<t_address, DwarfAbbrevTableEntry *> table_entries;
    get_die_and_siblings(cu_header, NULL, dbg, cu_die, 0, to_process, table_entries, cu_die->di_cu_context->cc_cu_die_global_sec_offset);

    PostProcessParsedAttributes(to_process, table_entries);

    /* find functions */
    for (auto entry : cu_header->children)
      FindFunctionDeclarations(static_cast<DwarfAbbrevTableEntry *>(entry));
  }
}

t_section *get_section_info(Dwarf_Debug dbg, t_object *obj, Dwarf_Section_s section, t_const_string name) {
  t_section *sec = SectionNew(obj, RODATA_SECTION, name);
  _dwarf_load_section(dbg, &section, &error);

  SECTION_SET_DATA(sec, reinterpret_cast<void *>(section.dss_data));
  SECTION_SET_CSIZE(sec, AddressNew32(section.dss_size));
  return sec;
}

/* MAIN */
int
main (int argc, char **argv)
{
  t_object *obj;
  double  secs;
  double  CPUsecs;
  double  CPUutilisation;

  pid_t child_process_id = 0;
  int fd_fork_stdout;
  bool forked = false;

  start_time();
  start_CPU_time();

  PrintFullCommandline(argc, argv);

  DiabloArmInit(argc, argv);

  /* The final option parsing should have TRUE as its last argument */
  GlobalInit ();
  OptionParseCommandLine (global_list, argc, argv, TRUE);
  OptionGetEnvironment (global_list);
  GlobalVerify ();
  OptionDefaults (global_list);

  PrintVersionInformationIfRequested();

  if (global_options.saveState)
  {
    ASSERT(global_options.objectfilename, ("Input objectfile/executable name not set!"));
    SaveState(global_options.objectfilename, const_cast<char*>("b.out"));
    exit(0);
  }

  /* Restore a dumped program, it will be loaded by ObjectGet */
  if (diabloobject_options.restore || (diabloobject_options.restore_multi != -1))
  {
    if (global_options.objectfilename) Free(global_options.objectfilename);
    global_options.objectfilename = RestoreDumpedProgram ();
  }

  RNGInitialise();
  if (global_options.random_overrides_file)
    RNGReadOverrides(global_options.random_overrides_file);

  if (!global_options.objectfilename) {
    VERBOSE(0, ("Diablo needs at least an input objectfile/executable as argument to do something."));
    return -1;
  }

  diablosupport_options.enable_transformation_log = TRUE;

  /* The real work: Link Emulate, Dissasemble, Flowgraph, Optimize, Deflowgraph, Assemble or just Dump {{{ */
  if (global_options.read)
  {
    DwarfInit();

    int fd = open(global_options.objectfilename, O_RDONLY);
    ASSERT(fd >= 0, ("could not open input file %s", global_options.objectfilename));

    Dwarf_Debug dbg = NULL;
    result = dwarf_init(fd, DW_DLC_READ, NULL, NULL, &dbg, &error);
    ASSERT(result == DW_DLV_OK, ("can't do DWARF processing"));

    /* create a dummy object instance (needed to work with sections) */
    t_object *obj = ObjectNew();
    OBJECT_SET_NAME(obj, StringDup("DUMMY_OBJECT"));

    /* set up sections object */
    dwarf_sections = new DwarfSections();
    dwarf_sections->aranges_section = get_section_info(dbg, obj, dbg->de_debug_aranges, ".debug_aranges");
    dwarf_sections->frame_section = get_section_info(dbg, obj, dbg->de_debug_frame, ".debug_frame");
    dwarf_sections->abbrev_section = get_section_info(dbg, obj, dbg->de_debug_abbrev, ".debug_abbrev");
    dwarf_sections->str_section = get_section_info(dbg, obj, dbg->de_debug_str, ".debug_str");
    dwarf_sections->loc_section = get_section_info(dbg, obj, dbg->de_debug_loc, ".debug_loc");
    dwarf_sections->ranges_section = get_section_info(dbg, obj, dbg->de_debug_ranges, ".debug_ranges");
    dwarf_sections->macinfo_section = get_section_info(dbg, obj, dbg->de_debug_macinfo, ".debug_macinfo");
    dwarf_sections->line_section = get_section_info(dbg, obj, dbg->de_debug_line, ".debug_line");
    dwarf_sections->pubnames_section = get_section_info(dbg, obj, dbg->de_debug_pubnames, ".debug_pubnames");
    dwarf_sections->pubtypes_section = get_section_info(dbg, obj, dbg->de_debug_pubtypes, ".debug_pubtypes");
    dwarf_sections->info_section = get_section_info(dbg, obj, dbg->de_debug_info, ".debug_info");
    dwarf_sections->types_section = get_section_info(dbg, obj, dbg->de_debug_types, ".debug_types");

    read_cu_list(dbg);

    auto addr_to_def = DwarfParseInformation(NULL);

    /* emit information to a log */
    string filename = string(global_options.output_name) + ".liveness";
    INIT_LOGGING(L_OUT, filename.c_str());

    for (auto p : addr_to_def) {
      DwarfFunctionDefinition *def = p.second;

      t_regset args, rets;
      DiabloBrokerCall("DwarfArchitectureSpecificFunctionFloatArgRetRegsets", def, &args, &rets);

      bool a = false;
      if (def->linkage_name.size() > 0) {
        a = true;
        LOG_MESSAGE(L_OUT, "%s:%s:%s\n", def->linkage_name.c_str(), RegsetSerialize(args), RegsetSerialize(rets));
      }

      bool b = false;
      if (def->name.size() > 0) {
        b = true;
        LOG_MESSAGE(L_OUT, "%s:%s:%s\n", def->name.c_str(), RegsetSerialize(args), RegsetSerialize(rets));
      }

      ASSERT(a || b, ("no information printed for '%s'", def->to_string().c_str()));
    }

    FINI_LOGGING(L_OUT);
  }

  RNGFinalise();

  /* Free used libraries and debug print {{{ */
  /* remove directory that held the restored dump */
  /*if (global_options.restore) RmdirR("./DUMP"); */
  GlobalFini ();

  /* If DEBUG MALLOC is defined print a list of all memory
   * leaks. Needs to increase the verbose-level to make
   * sure these get printed. This is a hack but it may be
   * removed when a real version of diablo is made */
  diablosupport_options.verbose++;
  PrintRemainingBlocks ();
  /*}}} */

  CloseAllLibraries();

  DiabloFlowgraphFini();

  secs = end_time();
  CPUsecs = end_CPU_time();

  printf(" TIME: %7.4fs clock wall time\n", secs);
  printf(" TIME: %7.4fs cpu time\n", CPUsecs);

  return 0;
}
