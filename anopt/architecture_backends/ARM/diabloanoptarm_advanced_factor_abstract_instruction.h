#ifndef DIABLOANOPTARM_ADVANCED_FACTOR_ABSTRACT_INSTRUCTION_H
#define DIABLOANOPTARM_ADVANCED_FACTOR_ABSTRACT_INSTRUCTION_H

#define COMPARE_CONDITION
//#define COMPARE_IMMEDIATE

#ifndef AF_FUZZY_REGISTERS
#define COMPARE_REGISTER
#endif

#define COMPARE_DIRUP
#define COMPARE_WRITEBACK
#define COMPARE_SHIFTIMM

enum class CompareInstructionsResult {
  Smaller = -1,
  Equal = 0,
  Greater = 1,
  NotEqual = 2,
};

class AbstractInstructionFormCalculator {
public:
  enum class Type {
    Unsupported = -1,

    /* 0 arguments */
    NoArguments,

    /* 1 register */
    Reg,
    RegImmediate,
    RegImmediatePreindex,
    RegImmediateStatus,
    RegPreindex,

    /* 2 registers */
    RegReg,
    RegRegPreindex,
    RegRegStatus,
    RegRegImmediate,
    RegRegImmediatePreindex,
    RegRegImmediateStatus,
    RegRegShiftByImmediate,
    RegRegShiftByImmediateStatus,
    RegRegShiftByRegister,
    RegRegShiftByRegisterStatus,

    /* 3 registers */
    RegRegReg,
    RegRegRegStatus,
    RegRegRegShiftByImmediate,
    RegRegRegShiftByImmediatePreindex,
    RegRegRegShiftByImmediateStatus,
    RegRegRegShiftByRegister,
    RegRegRegShiftByRegisterStatus,
    RegRegRegPreindex,

    /* 4 registers */
    RegRegRegReg,
    RegRegRegRegStatus,

    BiRegReg,
    BiRegRegImmediate,
    BiRegRegImmediatePreindex,
    BiRegRegReg,
    BiRegRegRegPreindex,

    Constant,
    Address,
    Simd,

    Count
  };

  enum class Field {
    RegA              = 1<<0,
    RegB              = 1<<1,
    RegC              = 1<<2,
    RegAbis           = 1<<3,
    RegS              = 1<<4,
    Immediate         = 1<<5,
    ShiftByRegister   = 1<<6,
    ShiftByImmediate  = 1<<7,
    Status            = 1<<8,
    Constant          = 1<<9,
    Preindex          = 1<<10,
  };

  AbstractInstructionFormCalculator();

  Type Calculate(t_ins *p_ins);
  std::string TypeToString(Type t);

private:
  typedef std::map<t_uint32, Type> t_field2type_map;
  t_field2type_map field2type_map;
};

AbstractInstructionFormCalculator::Type ArmInsGetAbstractInstructionForm(t_ins *p_ins);

bool CompareInstructionsLiteral(t_ins *p_a, t_ins *p_b);
CompareInstructionsResult CompareInstructions(t_ins *a, t_ins *b);
CompareInstructionsResult CompareAbstractInstructions(t_ins *a, t_ins *b);
bool CompareAddressProducers(t_arm_ins *a, t_arm_ins *b, bool extend);

/* dynamic members */
INS_DYNAMIC_MEMBER_GLOBAL(abstract_form, ABSTRACT_FORM, AbstractForm, AbstractInstructionFormCalculator::Type, AbstractInstructionFormCalculator::Type::Unsupported);

#endif /* DIABLOANOPTARM_ADVANCED_FACTOR_ABSTRACT_INSTRUCTION_H */
