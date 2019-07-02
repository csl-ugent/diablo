#include "diabloanoptarm_advanced_factor.hpp"

using namespace std;

INS_DYNAMIC_MEMBER_GLOBAL_ARRAY(abstract_form);

/* NOTE: when adding vector instruction support, also check the ArmUsedRegisters and ArmDefinedRegisters function for correct use/def. */
AbstractInstructionFormCalculator::AbstractInstructionFormCalculator() {
  field2type_map.clear();

#define I(x) static_cast<int>(x)
#define PUT(type, flags) field2type_map[flags] = Type::type;
  PUT(NoArguments,
      0);

  PUT(Reg,
      I(Field::RegB));
  PUT(RegPreindex,
      I(Field::RegB) | I(Field::Preindex));
  PUT(RegImmediate,
      I(Field::RegA) | I(Field::Immediate));
  PUT(RegImmediateStatus,
      I(Field::RegB) | I(Field::Immediate) | I(Field::Status));
  PUT(RegImmediatePreindex,
      I(Field::RegA) | I(Field::Immediate) | I(Field::Preindex));

  PUT(RegReg,
      I(Field::RegA) | I(Field::RegC));
  PUT(RegReg,
      I(Field::RegA) | I(Field::RegB));
  PUT(RegReg,
      I(Field::RegB) | I(Field::RegC));
  PUT(RegRegPreindex,
      I(Field::RegA) | I(Field::RegB) | I(Field::Preindex));
  PUT(RegRegImmediate,
      I(Field::RegA) | I(Field::RegB) | I(Field::Immediate));
  PUT(RegRegImmediatePreindex,
      I(Field::RegA) | I(Field::RegB) | I(Field::Immediate) | I(Field::Preindex));
  PUT(RegRegStatus,
      I(Field::RegB) | I(Field::RegC) | I(Field::Status));
  PUT(RegRegStatus,
      I(Field::RegA) | I(Field::RegC) | I(Field::Status));
  PUT(RegRegImmediateStatus,
      I(Field::RegA) | I(Field::RegB) | I(Field::Immediate) | I(Field::Status));
  PUT(RegRegShiftByImmediate,
      I(Field::RegA) | I(Field::RegC) | I(Field::ShiftByImmediate));
  PUT(RegRegShiftByImmediateStatus,
      I(Field::RegB) | I(Field::RegC) | I(Field::ShiftByImmediate) | I(Field::Status));
  PUT(RegRegShiftByImmediateStatus,
      I(Field::RegA) | I(Field::RegC) | I(Field::ShiftByImmediate) | I(Field::Status));
  PUT(RegRegShiftByRegister,
      I(Field::RegA) | I(Field::RegC) | I(Field::ShiftByRegister));
  PUT(RegRegShiftByRegisterStatus,
      I(Field::RegB) | I(Field::RegC) | I(Field::ShiftByRegister) | I(Field::Status));
  PUT(RegRegShiftByRegisterStatus,
      I(Field::RegA) | I(Field::RegC) | I(Field::ShiftByRegister) | I(Field::Status));

  PUT(RegRegReg,
      I(Field::RegA) | I(Field::RegB) | I(Field::RegC));
  PUT(RegRegRegPreindex,
      I(Field::RegA) | I(Field::RegB) | I(Field::RegC) | I(Field::Preindex));
  PUT(RegRegRegStatus,
      I(Field::RegA) | I(Field::RegB) | I(Field::RegC) | I(Field::Status));
  PUT(RegRegRegShiftByImmediate,
      I(Field::RegA) | I(Field::RegB) | I(Field::RegC) | I(Field::ShiftByImmediate));
  PUT(RegRegRegShiftByImmediatePreindex,
      I(Field::RegA) | I(Field::RegB) | I(Field::RegC) | I(Field::ShiftByImmediate) | I(Field::Preindex));
  PUT(RegRegRegShiftByImmediateStatus,
      I(Field::RegA) | I(Field::RegB) | I(Field::RegC) | I(Field::ShiftByImmediate) | I(Field::Status));
  PUT(RegRegRegShiftByRegister,
      I(Field::RegA) | I(Field::RegB) | I(Field::RegC) | I(Field::ShiftByRegister));
  PUT(RegRegRegShiftByRegisterStatus,
      I(Field::RegA) | I(Field::RegB) | I(Field::RegC) | I(Field::ShiftByRegister) | I(Field::Status));

  PUT(RegRegRegReg,
      I(Field::RegA) | I(Field::RegB) | I(Field::RegC) | I(Field::RegS));
  PUT(RegRegRegRegStatus,
      I(Field::RegA) | I(Field::RegB) | I(Field::RegC) | I(Field::RegS) | I(Field::Status));

  PUT(BiRegReg,
      I(Field::RegA) | I(Field::RegB) | I(Field::RegAbis));
  PUT(BiRegRegImmediate,
      I(Field::RegA) | I(Field::RegB) | I(Field::RegAbis) | I(Field::Immediate));
  PUT(BiRegRegImmediatePreindex,
      I(Field::RegA) | I(Field::RegB) | I(Field::RegAbis) | I(Field::Immediate) | I(Field::Preindex));

  PUT(BiRegRegReg,
      I(Field::RegA) | I(Field::RegB) | I(Field::RegC) | I(Field::RegAbis));
  PUT(BiRegRegRegPreindex,
      I(Field::RegA) | I(Field::RegB) | I(Field::RegC) | I(Field::RegAbis) | I(Field::Preindex));

  PUT(Constant,
      I(Field::Constant));
#undef PUT
#undef I
}

AbstractInstructionFormCalculator::Type AbstractInstructionFormCalculator::Calculate(t_ins *p_ins) {
  t_arm_ins *ins = T_ARM_INS(p_ins);

  auto log_unsupported = [] (string reason, t_arm_ins *ins) {
    t_string insn = StringIo("UNSUPPORTED:%s:@I", reason.c_str(), ins);
    FactoringLogComment(std::string(insn));
    Free(insn);
  };

  if (ARM_INS_OPCODE(ins) == ARM_ADDRESS_PRODUCER) {
    if (diabloanoptarm_options.advanced_factoring_include_addressproducers)
      return Type::Address;
    else {
      log_unsupported("address-producer", ins);
      return Type::Unsupported;
    }
  }

  /* instructions that have relocations associated with them can't be factored */
  if (INS_REFERS_TO(p_ins)) {
    log_unsupported("refers-to", ins);
    return Type::Unsupported;
  }

  /* no support for branch-immediate (offset) instructions */
  if (ARM_INS_TYPE(ins) == IT_BRANCH
      && ARM_INS_FLAGS(ins) & FL_IMMED) {
    log_unsupported("branch-immediate", ins);
    return Type::Unsupported;
  }

  /* no support for implicit branch instructions */
  if (RegsetIn(ARM_INS_REGS_DEF(ins), ARM_REG_R15)) {
    log_unsupported("control-flow", ins);
    return Type::Unsupported;
  }

  /* as our analyses don't support propagation of SIMD registers and constants */
  if (ARM_INS_TYPE(ins) == IT_SIMD) {
    log_unsupported("simd", ins);
    return Type::Simd;
  }

  /* no need to factor data */
  if (ARM_INS_TYPE(ins) == IT_DATA) {
    log_unsupported("data", ins);
    return Type::Unsupported;
  }

  /* DMB, DSB, ISB */
  if (ARM_INS_TYPE(ins) == IT_SYNC) {
    log_unsupported("sync", ins);
    return Type::Unsupported;
  }

  /* SWI calls */
  if (ARM_INS_TYPE(ins) == IT_SWI) {
    log_unsupported("swi", ins);
    return Type::Unsupported;
  }

  /* PLD, PLDW, PLI, CLREX, CPS, LDC, STC, ... */
  if (ARM_INS_TYPE(ins) == IT_UNKNOWN) {
    log_unsupported("unknown", ins);
    return Type::Unsupported;
  }

  if (ARM_INS_TYPE(ins) == IT_STATUS) {
    log_unsupported("status", ins);
    return Type::Unsupported;
  }

  if (ARM_INS_OPCODE(ins) == ARM_MRC) {
    log_unsupported("mrc", ins);
    return Type::Unsupported;
  }

  /* SIMD arithmetic instructions */
  if (ARM_INS_TYPE(ins) == IT_FLT_ALU) {
    log_unsupported("simd-alu", ins);
    return Type::Simd;
  }

  /* SIMD store instructions */
  if (ARM_INS_TYPE(ins) == IT_FLT_STORE) {
    log_unsupported("simd-store", ins);
    return Type::Simd;
  }

  /* SIMD load instructions */
  if (ARM_INS_TYPE(ins) == IT_FLT_LOAD) {
    log_unsupported("simd-load", ins);
    return Type::Simd;
  }

  /* this instruction uses an array of data bytes */
  if (ARM_INS_OPCODE(ins) == ARM_VFPFLOAT_PRODUCER) {
    log_unsupported("float-producer", ins);
    return Type::Unsupported;
  }

  if (ARM_INS_OPCODE(ins) == ARM_CONSTANT_PRODUCER)
    return Type::Constant;

  if (ARM_INS_TYPE(ins) == IT_NOP)
    return Type::NoArguments;

  /* special case: we can't know in any way whether or not this instruction will be executed or not.
   * Moreover, if the would-be-defined register of 'ins' is also in the USE-set, compensation instructions
   * will be inserted before the conditional instruction, and it is assumed that this instruction is executed. */
  if (ArmInsIsConditional(ins)
      && !RegsetIsEmpty(RegsetIntersect(ARM_INS_REGS_DEF(ins), ARM_INS_REGS_USE(ins)))) {
    log_unsupported("conditional-usedef", ins);
    return Type::Unsupported;
  }

  t_uint32 fields = 0;
  if (ARM_INS_REGA(ins) != ARM_REG_NONE) fields |= static_cast<t_uint32>(Field::RegA);
  if (ARM_INS_REGB(ins) != ARM_REG_NONE) fields |= static_cast<t_uint32>(Field::RegB);
  if (ARM_INS_REGC(ins) != ARM_REG_NONE) fields |= static_cast<t_uint32>(Field::RegC);
  if (ARM_INS_REGS(ins) != ARM_REG_NONE) fields |= static_cast<t_uint32>(Field::RegS);
  if (ARM_INS_REGABIS(ins) != ARM_REG_NONE) fields |= static_cast<t_uint32>(Field::RegAbis);
  if (ARM_INS_FLAGS(ins) & FL_S) fields |= static_cast<t_uint32>(Field::Status);

  if (ArmInsHasImmediateOp(ins))
    fields |= static_cast<t_uint32>(Field::Immediate);

  if (ARM_INS_FLAGS(ins) & FL_PREINDEX)
    fields |= static_cast<t_uint32>(Field::Preindex);

  if (ARM_INS_SHIFTTYPE(ins) != ARM_SHIFT_TYPE_NONE)
  {
    if (ARM_INS_SHIFTTYPE(ins) <= ARM_SHIFT_TYPE_ROR_IMM) fields |= static_cast<t_uint32>(Field::ShiftByImmediate);
    else if (ARM_INS_SHIFTTYPE(ins) <= ARM_SHIFT_TYPE_ROR_REG
              || ARM_INS_SHIFTTYPE(ins) == ARM_SHIFT_TYPE_RRX)
    {
      /* unmask RegS as this is the shift register */
      fields &= ~static_cast<t_uint32>(Field::RegS);
      fields |= static_cast<t_uint32>(Field::ShiftByRegister);
    }
    else FATAL(("unsupported @I", ins));
  }

  auto result = field2type_map.find(fields);
  ASSERT(result != field2type_map.end(), ("could not find type 0x%08x in field to type map (instruction @I)", fields, ins));
  return result->second;
}

string AbstractInstructionFormCalculator::TypeToString(Type t) {
  switch(t) {

/* macro definition for cleaner source code */
#define CASE(t) \
case Type::t: \
  return #t;\
  break;
/********************************************/

  CASE(Unsupported);
  CASE(RegImmediate);
  CASE(RegRegImmediate);
  CASE(RegImmediateStatus);
  CASE(RegRegStatus);
  CASE(RegRegReg);
  CASE(RegRegRegStatus);
  CASE(RegReg);
  CASE(Reg);
  CASE(RegRegShiftByImmediate);
  CASE(RegRegImmediateStatus);
  CASE(RegRegRegShiftByImmediate);
  CASE(RegRegRegReg);
  CASE(RegRegRegShiftByRegister);
  CASE(RegRegRegShiftByRegisterStatus);
  CASE(RegRegShiftByRegisterStatus);
  CASE(RegRegShiftByRegister);
  CASE(RegRegShiftByImmediateStatus);
  CASE(NoArguments);
  CASE(RegRegRegRegStatus);
  CASE(BiRegRegImmediate);
  CASE(BiRegRegReg);
  CASE(Constant);
  CASE(RegPreindex);
  CASE(RegRegImmediatePreindex);
  CASE(RegRegRegPreindex);
  CASE(RegImmediatePreindex);
  CASE(BiRegRegImmediatePreindex);
  CASE(BiRegRegRegPreindex);
  CASE(RegRegRegShiftByImmediatePreindex);
  CASE(RegRegPreindex);

#undef CASE

  default:
    FATAL(("could not convert type to string: %d", static_cast<int>(t)));
  }
}

static AbstractInstructionFormCalculator calculator;
AbstractInstructionFormCalculator::Type ArmInsGetAbstractInstructionForm(t_ins *p_ins)
{
  return calculator.Calculate(p_ins);
}

bool CompareAddressProducers(t_arm_ins *a, t_arm_ins *b, bool extend) {
  if (!extend
      && diabloanoptarm_options.advanced_factoring_addressproducers_updown)
    return true;

  /* return TRUE if considered equivalent */
  ASSERT(ARM_INS_REFERS_TO(a), ("@I has no refers-to", a));
  ASSERT(ARM_INS_REFERS_TO(b), ("@I has no refers-to", b));

  t_reloc *rel_a = RELOC_REF_RELOC(ARM_INS_REFERS_TO(a));
  t_reloc *rel_b = RELOC_REF_RELOC(ARM_INS_REFERS_TO(b));

  /* returns 0 if equal */
  if (RelocCmp(rel_a, rel_b, FALSE) != 0)
    return false;

  return true;
}

bool CompareInstructionsLiteral(t_ins *p_a, t_ins *p_b)
{
  t_arm_ins *a = T_ARM_INS(p_a);
  t_arm_ins *b = T_ARM_INS(p_b);

  if (!RegsetEquals(ARM_INS_REGS_DEF(a), ARM_INS_REGS_DEF(b)))
    return false;

  if (!RegsetEquals(ARM_INS_REGS_USE(a), ARM_INS_REGS_USE(b)))
    return false;

  if (ARM_INS_IMMEDIATE(a) != ARM_INS_IMMEDIATE(b))
    return false;

  if (ARM_INS_FLAGS(a) != ARM_INS_FLAGS(b))
    return false;

  if (ARM_INS_ATTRIB(a) != ARM_INS_ATTRIB(b))
    return false;

  if (ARM_INS_SHIFTTYPE(a) != ARM_INS_SHIFTTYPE(b))
    return false;

  if (ARM_INS_SHIFTLENGTH(a) != ARM_INS_SHIFTLENGTH(b))
    return false;

  /* identical to abstract compare */

  if (ARM_INS_TYPE(a) != ARM_INS_TYPE(b))
    return false;

  if (ARM_INS_OPCODE(a) != ARM_INS_OPCODE(b))
    return false;

  if (ARM_INS_CONDITION(a) != ARM_INS_CONDITION(b))
    return false;

  /* SIMD arithmetic instructions */
  if (ARM_INS_TYPE(a) == IT_FLT_ALU)
    return false;

  /* SIMD store instructions */
  if (ARM_INS_TYPE(a) == IT_FLT_STORE)
    return false;

  /* SIMD load instructions */
  if (ARM_INS_TYPE(a) == IT_FLT_LOAD)
    return false;

  if (ARM_INS_OPCODE(a) == ARM_ADDRESS_PRODUCER)
    if (!CompareAddressProducers(a, b, FALSE))
      return false;

  return true;
}

CompareInstructionsResult CompareInstructions(t_ins *p_a, t_ins *p_b)
{
  t_arm_ins *a = T_ARM_INS(p_a);
  t_arm_ins *b = T_ARM_INS(p_b);

#define f(x) \
  { \
    if (x(a) > x(b)) return CompareInstructionsResult::Greater; \
    else if (x(a) < x(b)) return CompareInstructionsResult::Smaller; \
  }
#define g(x) \
  { \
    if ((ARM_INS_FLAGS(a) & (x)) ^ (ARM_INS_FLAGS(b) & (x))) return CompareInstructionsResult::NotEqual; \
  }

  if (ARM_INS_OPCODE(a) == ARM_ADDRESS_PRODUCER)
    /* only support identical address producers in real slices */
    if (!CompareAddressProducers(a, b, TRUE))
      return CompareInstructionsResult::NotEqual;

  if (ARM_INS_TYPE(a) != ARM_INS_TYPE(b))
    return CompareInstructionsResult::NotEqual;

  if (ARM_INS_TYPE(a) == IT_LOAD_MULTIPLE
      || ARM_INS_TYPE(b) == IT_STORE_MULTIPLE)
  {
    return CompareInstructionsResult::NotEqual;
  }

#ifdef COMPARE_CONDITION
  f(ARM_INS_CONDITION);
#endif

  if (ArmInsIsConditional(a)) {
    if (!RegsetEquals(ARM_INS_REGS_DEF(a), ARM_INS_REGS_DEF(b)))
      return CompareInstructionsResult::NotEqual;
  }

  if (RegsetIn(ARM_INS_REGS_USE(a), ARM_REG_R15)
      || RegsetIn(ARM_INS_REGS_USE(b), ARM_REG_R15)) {
    return CompareInstructionsResult::NotEqual;
  }

  if ((ARM_INS_OPCODE(a) == ARM_UBFX
       || ARM_INS_OPCODE(a) == ARM_BFC
       || ARM_INS_OPCODE(a) == ARM_BFI
       || ARM_INS_OPCODE(a) == ARM_SBFX)
      && (ARM_INS_IMMEDIATE(a) != ARM_INS_IMMEDIATE(b)))
    return CompareInstructionsResult::NotEqual;

#ifdef COMPARE_IMMEDIATE
  /* we only have to test one instruction flag because it is already included in the fingerprint */
  if (ARM_INS_FLAGS(a) & FL_IMMED)
    f(ARM_INS_IMMEDIATE);
#endif

  /* constant producers should have their immediate values checked. Always. */
  if (INS_ABSTRACT_FORM(p_a) == AbstractInstructionFormCalculator::Type::Constant)
    f(ARM_INS_IMMEDIATE);

#ifdef COMPARE_REGISTER
  f(ARM_INS_REGA);
  f(ARM_INS_REGB);
  f(ARM_INS_REGC);
  f(ARM_INS_REGS);
  f(ARM_INS_REGABIS);
#endif

#ifdef COMPARE_DIRUP
  g(FL_DIRUP)
#endif

#ifdef COMPARE_WRITEBACK
  g(FL_WRITEBACK)
#endif

  f(ARM_INS_SHIFTTYPE);

#ifdef COMPARE_SHIFTIMM
  if (ARM_SHIFT_TYPE_LSL_IMM <= ARM_INS_SHIFTTYPE(a)
      && ARM_INS_SHIFTTYPE(a) <= ARM_SHIFT_TYPE_ROR_IMM)
    f(ARM_INS_SHIFTLENGTH);
#endif

  if (ARM_INS_TYPE(a) == IT_LOAD_MULTIPLE
      || ARM_INS_TYPE(a) == IT_STORE_MULTIPLE)
    f(ARM_INS_IMMEDIATE);

  /* some instructions can't have their immediate value changed to a register.
   * TODO: this should be moved to the immediate equalization algorithm. */
  if (ARM_INS_OPCODE(a) == ARM_MOVT
      || ARM_INS_OPCODE(a) == ARM_MOVW)
    f(ARM_INS_IMMEDIATE);

  return CompareInstructionsResult::Equal;

#undef f
#undef g
}

CompareInstructionsResult CompareAbstractInstructions(t_ins *a, t_ins *b)
{
  if (INS_ABSTRACT_FORM(a) == AbstractInstructionFormCalculator::Type::Unsupported
      || INS_ABSTRACT_FORM(b) == AbstractInstructionFormCalculator::Type::Unsupported)
    return CompareInstructionsResult::Greater;
  else if (INS_ABSTRACT_FORM(a) == AbstractInstructionFormCalculator::Type::Simd
      || INS_ABSTRACT_FORM(b) == AbstractInstructionFormCalculator::Type::Simd) {
    /* TODO: maybe support literal compare in the future. For now, not many instructions are matched here.
     * bool x = ArmCompareInstructions(T_ARM_INS(a), T_ARM_INS(b)); */
    return CompareInstructionsResult::Greater;
  }

  if (INS_FINGERPRINT(a) != INS_FINGERPRINT(b))
  {
    if (INS_FINGERPRINT(a) > INS_FINGERPRINT(b))
      return CompareInstructionsResult::Greater;
    else
      return CompareInstructionsResult::Smaller;
  }

  return CompareInstructions(a, b);
}
