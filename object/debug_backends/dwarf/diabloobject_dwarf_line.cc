/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

#include "diabloobject_dwarf.h"

#include <string>

#define DEBUG_STATE_MACHINE 0

using namespace std;

enum class StandardOpcodes {
  DW_LNS_extended_opcode    = 0x00,
  DW_LNS_copy               = 0x01,
  DW_LNS_advance_pc         = 0x02,
  DW_LNS_advance_line       = 0x03,
  DW_LNS_set_file           = 0x04,
  DW_LNS_set_column         = 0x05,
  DW_LNS_negate_stmt        = 0x06,
  DW_LNS_set_basic_block    = 0x07,
  DW_LNS_const_add_pc       = 0x08,
  DW_LNS_fixed_advance_pc   = 0x09,
  DW_LNS_set_prologue_end   = 0x0a,
  DW_LNS_set_epilogue_begin = 0x0b,
  DW_LNS_set_isa            = 0x0c
};

enum class ExtendedOpcodes {
  DW_LNE_end_sequence       = 0x01,
  DW_LNE_set_address        = 0x02,
  DW_LNE_define_file        = 0x03,
  DW_LNE_set_discriminator  = 0x04,
  DW_LNE_lo_user            = 0x80,
  DW_LNE_hi_user            = 0xff
};

/* state machine needed to create the line information matrix */
struct StateMachine {
  /* instruction address */
  t_address address;

  /* VLIW-instructions only: index of operation */
  t_uint32 op_index;

  /* source file index (-> file name table of owning line information header) */
  t_uint32 file;

  /* source line number;
   * 0 = no associated source line */
  t_uint32 line;

  /* source line column;
   * 0 = begin of line */
  t_uint32 column;

  /* current instruction recommended as breakpoint location? */
  bool is_stmt;

  /* current instruction is beginning of a basic block? */
  bool basic_block;

  /* end of program? */
  bool sequence_end;

  /* current instruction is end of compiler-generated function prologue? */
  bool prologue_end;

  /* current instruction is start of compiler-generated function epilogue? */
  bool epilogue_begin;

  /* instruction set */
  DwarfDecodedULEB128 isa;

  /* in case multiple blocks of instructions are associated with the same
   * source file, line and column, this value distinguishes between those
   * different blocks */
  t_uint32 discriminator;

  /* state machine reset */
  void
  init() {
    address = 0;
    op_index = 0;
    file = 1;
    line = 1;
    column = 0;

    /* is_stmt is determined by the program header */
    basic_block = false;
    sequence_end = false;
    prologue_end = false;
    epilogue_begin = false;

    isa = 0;
    discriminator = 0;
  }
};

static StateMachine m;

void
static
InterpretOpcode(StandardOpcodes opcode, DwarfCompilationUnitHeader *cu, t_section *sec, t_address offset, t_uint32& sz);

/* append one row to the line number information matrix of a compilation unit */
static
void
LineInfoMatrixAppendRow(DwarfCompilationUnitHeader *cu)
{
  DwarfLineInfoMatrixRow new_row;

  /* only copy over the needed state machine information */
  new_row.address = m.address;
  new_row.file = m.file;
  new_row.line = m.line;
  new_row.column = m.column;
  new_row.discriminator = m.discriminator;

  cu->line_info_matrices.back()->push_back(new_row);
}

/* given 'operation_advance', increment the address and op_index values of the state machine
 * accordingly. */
static
void
StateMachineAdvance(DwarfDecodedULEB128 operation_advance, DwarfLineNumberProgramHeader *hdr)
{
  m.address += hdr->minimum_instruction_length * 
                ((m.op_index + operation_advance) / hdr->maximum_operations_per_instruction);
  m.op_index = (m.op_index + operation_advance) % hdr->maximum_operations_per_instruction;
}

/* interpret an extended opcode */
static
bool
InterpretExtendedOpcode(DwarfCompilationUnitHeader *cu, t_section *sec, t_address offset, t_uint32& sz)
{
  t_uint32 n_bytes;
  t_uint32 new_sz;
  bool ret = false;

  /* first value = ULEB128 indicating the length of this extended opcode */
  DwarfDecodedULEB128 length = DwarfDecodeULEB128(DwarfReadULEB128FromSection(sec, offset, n_bytes));
  offset = AddressAddUint32(offset, n_bytes);
  sz = n_bytes;

  t_uint32 done = 0;

#if DEBUG_STATE_MACHINE
  DEBUG(("  of length %d", length));
#endif

  /* process all opcodes; break condition is inside the loop */
  while (true)
  {
    /* fetch the next opcode */
    ExtendedOpcodes opcode = static_cast<ExtendedOpcodes>(SectionGetData8(sec, offset));
    offset = AddressAddUint32(offset, 1);
    done++;

    /* additional data is possibly read */
    new_sz = 0;

    /* process this opcode */
    switch(opcode) {
    case ExtendedOpcodes::DW_LNE_set_address:
      m.address = DwarfReadAddress(cu->address_size, sec, offset);
      new_sz = cu->address_size;

#if DEBUG_STATE_MACHINE
      DEBUG(("  address = %x", m.address));
#endif
      break;

    case ExtendedOpcodes::DW_LNE_end_sequence:
      LineInfoMatrixAppendRow(cu);

      /* reset to initial state is done when a new program is read,
       * so don't do it here */
      ret = true;

#if DEBUG_STATE_MACHINE
      DEBUG(("  end of sequence"));
#endif
      break;

    case ExtendedOpcodes::DW_LNE_set_discriminator:
      m.discriminator = static_cast<t_uint32>(DwarfDecodeULEB128(DwarfReadULEB128FromSection(sec, offset, n_bytes)));
      new_sz = n_bytes;

#if DEBUG_STATE_MACHINE
      DEBUG(("  discriminator = %x", m.discriminator));
#endif
      break;

    case ExtendedOpcodes::DW_LNE_define_file:
      FATAL(("implement me"));
    {
      DwarfFileEntry *defined_file = new DwarfFileEntry();

      defined_file->name = new string(ReadStringFromSection(sec, offset));
      offset = AddressAddUint32(offset, defined_file->name->length() + 1);
      new_sz += defined_file->name->length() + 1;

      defined_file->last_modified = DwarfDecodeULEB128(DwarfReadULEB128FromSection(sec, offset, n_bytes));
      offset = AddressAddUint32(offset, n_bytes);
      new_sz += n_bytes;

      defined_file->size = DwarfDecodeULEB128(DwarfReadULEB128FromSection(sec, offset, n_bytes));
      offset = AddressAddUint32(offset, n_bytes);
      new_sz += n_bytes;

      defined_file->directory_index = DwarfDecodeULEB128(DwarfReadULEB128FromSection(sec, offset, n_bytes));
      offset = AddressAddUint32(offset, n_bytes);
      new_sz += n_bytes;

#if DEBUG_STATE_MACHINE
      DEBUG(("  define file %s (directory index %d, modified %d, size %d)", defined_file->name->c_str(), defined_file->directory_index, defined_file->last_modified, defined_file->size));
#endif
    }
      break;

    default:
      FATAL(("unsupported extended opcode %x", opcode));
    }

    /* take into account possible additional data read by the opcode */
    done += new_sz;

    /* sanity check */
    ASSERT(done <= length, ("processed more bytes than allowed %d/%d", done, length));

    /* break condition */
    if (done == length)
      break;
  }

  /* all bytes have been processed */
  sz += length;

  return ret;
}

/* interpret a special opcode */
static
void
InterpretSpecialOpcode(t_uint8 opcode, DwarfCompilationUnitHeader *cu, DwarfLineNumberProgramHeader *hdr, t_section *sec, t_address offset, t_uint32& sz, bool advance_line)
{
  t_uint8 adjusted_opcode = opcode - hdr->opcode_base;
  t_uint8 operation_advance = adjusted_opcode / hdr->line_range;

  /* increment the 'address' and 'op_index' of the state machine, given the 'operation_advance' value */
  StateMachineAdvance(operation_advance, hdr);

  /* sometimes the line number does not need to be advanced (see lower, for special opcode 0xff) */
  if (advance_line)
    m.line += static_cast<t_uint32>(hdr->line_base) + (adjusted_opcode % hdr->line_range);

  /* every special opcode triggers the addition of a row to the line information matrix */
  LineInfoMatrixAppendRow(cu);

  /* reset some values of the state machine */
  m.basic_block = false;
  m.prologue_end = false;
  m.epilogue_begin = false;
  m.discriminator = 0;

#if DEBUG_STATE_MACHINE
  DEBUG(("  address %x, line %d", m.address, m.line));
#endif
}

/* interpret a regular opcode */
static
void
InterpretOpcode(StandardOpcodes opcode, DwarfCompilationUnitHeader *cu, DwarfLineNumberProgramHeader *hdr, t_section *sec, t_address offset, t_uint32& sz, bool& do_next)
{
  t_uint32 new_sz = 0;
  DwarfDecodedULEB128 decoded;
  t_uint32 n_bytes;

  /* opcode handling */
  switch(opcode) {
  case StandardOpcodes::DW_LNS_extended_opcode:
#if DEBUG_STATE_MACHINE
    DEBUG(("extended opcode"));
#endif

    /* 'do_next' is TRUE when an end-of-sequence is encountered */
    do_next = !InterpretExtendedOpcode(cu, sec, offset, new_sz);
    break;

  case StandardOpcodes::DW_LNS_advance_line:
    m.line += DwarfDecodeSLEB128(DwarfReadSLEB128FromSection(sec, offset, n_bytes));
    sz += n_bytes;

#if DEBUG_STATE_MACHINE
    DEBUG(("advance line %d", m.line));
#endif
    break;

  case StandardOpcodes::DW_LNS_copy:
    LineInfoMatrixAppendRow(cu);

    /* state machine reset */
    m.discriminator = 0;
    m.basic_block = false;
    m.prologue_end = false;
    m.epilogue_begin = false;

#if DEBUG_STATE_MACHINE
    DEBUG(("copy"));
#endif
    break;

  case StandardOpcodes::DW_LNS_const_add_pc:
#if DEBUG_STATE_MACHINE
    DEBUG(("const add PC"));
#endif

    InterpretSpecialOpcode(0xff, cu, hdr, sec, offset, new_sz, false);
    break;

  case StandardOpcodes::DW_LNS_advance_pc:
    decoded = DwarfDecodeULEB128(DwarfReadULEB128FromSection(sec, offset, n_bytes));
    sz += n_bytes;
    StateMachineAdvance(decoded, hdr);

#if DEBUG_STATE_MACHINE
    DEBUG(("advance PC: address %x", m.address, m.line));
#endif
    break;

  case StandardOpcodes::DW_LNS_set_file:
    m.file = static_cast<t_uint32>(DwarfDecodeULEB128(DwarfReadULEB128FromSection(sec, offset, n_bytes)));
    sz += n_bytes;

#if DEBUG_STATE_MACHINE
    DEBUG(("set file %x", m.file));
#endif
    break;

  case StandardOpcodes::DW_LNS_negate_stmt:
    m.is_stmt = !m.is_stmt;

#if DEBUG_STATE_MACHINE
    DEBUG(("negate stmt %d", m.is_stmt));
#endif
    break;

  case StandardOpcodes::DW_LNS_set_column:
    m.column = static_cast<t_uint32>(DwarfDecodeULEB128(DwarfReadULEB128FromSection(sec, offset, n_bytes)));
    sz += n_bytes;

#if DEBUG_STATE_MACHINE
    DEBUG(("set column %x", m.column));
#endif
    break;

  case StandardOpcodes::DW_LNS_set_basic_block:
    m.basic_block = true;

#if DEBUG_STATE_MACHINE
    DEBUG(("set basic block"));
#endif
    break;

  case StandardOpcodes::DW_LNS_fixed_advance_pc:
  {
    t_uint16 value = SectionGetData16(sec, offset);
    sz += 2;

    m.address += value;
    m.op_index = 0;
  }

#if DEBUG_STATE_MACHINE
    DEBUG(("fix advance PC: %x", m.address));
#endif
    break;

  case StandardOpcodes::DW_LNS_set_prologue_end:
    m.prologue_end = true;

#if DEBUG_STATE_MACHINE
    DEBUG(("set prologue end"));
#endif
    break;

  case StandardOpcodes::DW_LNS_set_epilogue_begin:
    m.epilogue_begin = true;

#if DEBUG_STATE_MACHINE
    DEBUG(("set epilogue_begin begin"));
#endif
    break;

  case StandardOpcodes::DW_LNS_set_isa:
    m.isa = DwarfDecodeULEB128(DwarfReadULEB128FromSection(sec, offset, n_bytes));
    sz += n_bytes;

#if DEBUG_STATE_MACHINE
    DEBUG(("set isa %x", m.isa));
#endif
    break;

  default:
    /* treat as special opcode */
#if DEBUG_STATE_MACHINE
    DEBUG(("special opcode"));
#endif

    InterpretSpecialOpcode(static_cast<t_uint8>(opcode), cu, hdr, sec, offset, new_sz, true);
    break;
  }

  sz += new_sz;
}

/* interpret the bytecode to generate the line information matrix */
static
void
InterpretProgram(DwarfCompilationUnitHeader *cu, DwarfLineNumberProgramHeader *hdr, t_section *sec, t_address offset, t_uint32& size)
{
  bool do_next = true;
  t_uint32 sz;

  /* state machine initialisation */
  m.init();

  /* initial value */
  m.is_stmt = hdr->default_is_stmt;

  while (do_next) {
    /* fetch the next opcode */
    StandardOpcodes opcode = static_cast<StandardOpcodes>(SectionGetData8(sec, offset));
    offset = AddressAddUint32(offset, 1);
    size += 1;

    sz = 0;

    /* interpret this opcode */
    InterpretOpcode(opcode, cu, hdr, sec, offset, sz, do_next);
    offset = AddressAddUint32(offset, sz);
    size += sz;
  }
}

/* read in a line number program header */
DwarfLineNumberProgramHeader *
ReadLineNumberProgram(DwarfCompilationUnitHeader *cu, t_section *sec, t_address offset)
{
  DwarfLineNumberProgramHeader *ret = new DwarfLineNumberProgramHeader();
  t_uint32 sz;
  t_address program_start;
  t_address end_offset;

  /* unit length */
  ret->unit_length = DwarfReadInitialLength(sec, offset, sz);
  ret->is_64bit = (sz == 12);
  offset = AddressAddUint32(offset, sz);
  end_offset = AddressAddUint32(offset, ret->unit_length);

  /* version */
  ret->version = SectionGetData16(sec, offset);
  offset = AddressAddUint32(offset, 2);

  /* header length */
  if (ret->is_64bit)
  {
    ret->header_length = SectionGetData64(sec, offset);
    offset = AddressAddUint32(offset, 8);
  }
  else
  {
    ret->header_length = static_cast<t_uint64>(SectionGetData32(sec, offset));
    offset = AddressAddUint32(offset, 4);
  }
  program_start = AddressAddUint32(offset, ret->header_length);

  ret->minimum_instruction_length = SectionGetData8(sec, offset);
  offset = AddressAddUint32(offset, 1);

  /* apparently this is not defined in the header for ARM */
#if 0
  ret->maximum_operations_per_instruction = SectionGetData8(sec, offset);
  offset = AddressAddUint32(offset, 1);
  DEBUG(("max ops per instr %d", ret->maximum_operations_per_instruction));
#else
  ret->maximum_operations_per_instruction = 1;
#endif

  ret->default_is_stmt = SectionGetData8(sec, offset);
  offset = AddressAddUint32(offset, 1);

  ret->line_base = static_cast<t_int8>(SectionGetData8(sec, offset));
  offset = AddressAddUint32(offset, 1);

  ret->line_range = SectionGetData8(sec, offset);
  offset = AddressAddUint32(offset, 1);

  ret->opcode_base = SectionGetData8(sec, offset);
  offset = AddressAddUint32(offset, 1);
  ASSERT(ret->opcode_base > 0, ("opcode base should be positive"));

  /* standard opcode lengths */
  ret->standard_opcode_lengths = new t_uint8[ret->opcode_base - 1];
  for (int i = 0; i < ret->opcode_base - 1; i++)
  {
    ret->standard_opcode_lengths[i] = SectionGetData8(sec, AddressAddUint32(offset, i));
  }
  /* skip over array of opcode lengths */
  offset = AddressAddUint32(offset, ret->opcode_base - 1);

  /* include directories */
  while (true) {
    string *dir = new string(ReadStringFromSection(sec, offset));
    offset = AddressAddUint32(offset, dir->length() + 1);

    /* list is terminated by a NULL-byte */
    if (dir->length() == 0) {
      delete dir;
      break;
    }

    ret->include_directories.push_back(dir);
  }

  /* file entries */
  while (true) {
    DwarfFileEntry *new_file;
    t_uint32 n_bytes = 0;

    string *file_name = new string(ReadStringFromSection(sec, offset));
    offset = AddressAddUint32(offset, file_name->length() + 1);

    /* list is terminated by a NULL-byte */
    if (file_name->length() == 0) {
      delete file_name;
      break;
    }

    new_file = new DwarfFileEntry();
    ret->file_names.push_back(new_file);

    new_file->name = file_name;

    new_file->directory_index = DwarfDecodeULEB128(DwarfReadULEB128FromSection(sec, offset, n_bytes));
    offset = AddressAddUint32(offset, n_bytes);

    new_file->last_modified = DwarfDecodeULEB128(DwarfReadULEB128FromSection(sec, offset, n_bytes));
    offset = AddressAddUint32(offset, n_bytes);

    new_file->size = DwarfDecodeULEB128(DwarfReadULEB128FromSection(sec, offset, n_bytes));
    offset = AddressAddUint32(offset, n_bytes);
  }

  /* only read program data when a program exists */
  while (!AddressIsEq(offset, end_offset))
  {
    cu->line_info_matrices.push_back(new DwarfLineInfoMatrix());

    sz = 0;
    InterpretProgram(cu, ret, sec, offset, sz);
    offset = AddressAddUint32(offset, sz);
  }

  /* debug information */
#if DEBUG_DWARF
  VERBOSE(DWARF_VERBOSITY, (" Length 0x%x (%d)", ret->unit_length, ret->is_64bit));
  VERBOSE(DWARF_VERBOSITY, (" Version %d", ret->version));
  VERBOSE(DWARF_VERBOSITY, (" Header length 0x%x", ret->header_length));
  VERBOSE(DWARF_VERBOSITY, (" Minimum instruction length %d", ret->minimum_instruction_length));
  VERBOSE(DWARF_VERBOSITY, (" Maximum operations per instruction %d", ret->maximum_operations_per_instruction));
  VERBOSE(DWARF_VERBOSITY, (" Default is_stmt %d", ret->default_is_stmt));
  VERBOSE(DWARF_VERBOSITY, (" Line base %d", ret->line_base));
  VERBOSE(DWARF_VERBOSITY, (" Line range %d", ret->line_range));
  VERBOSE(DWARF_VERBOSITY, (" Opcode base 0x%x", ret->opcode_base));

  string opcode_lengths_str = "";
  for (int i = 0; i < ret->opcode_base - 1; i++)
    opcode_lengths_str += to_string(ret->standard_opcode_lengths[i]) + " ";
  VERBOSE(DWARF_VERBOSITY, (" Opcode length [%s]", opcode_lengths_str.c_str()));

  for (auto i : ret->include_directories)
    VERBOSE(DWARF_VERBOSITY, (" Include directory: %s", i->c_str()));
  for (auto i : ret->file_names)
    VERBOSE(DWARF_VERBOSITY, (" File %s (directory %d, modified %d, size %d)", i->name->c_str(), i->directory_index, i->last_modified, i->size));
#endif

  return ret;
}
