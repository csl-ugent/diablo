template<typename T>
T InstructionFingerprintC<T>::GetFingerprint(t_ins *ins) {
  static t_uint32 n_opcode_bits = 0;
  static t_uint32 n_type_bits = 0;
  T result = 0;

  /* if needed, do a sanity check */
  if (n_opcode_bits == 0)
  {
    /* cache number of bits needed to encode the opcode */
    n_opcode_bits = 0;
    while (1<<n_opcode_bits < ARM_NR_TOTAL_OPCODES)
      n_opcode_bits++;

    n_type_bits = 0;
    while (1<<n_type_bits < (static_cast<int>(AbstractInstructionFormCalculator::Type::Count) + 1))
      n_type_bits++;

    ASSERT(n_opcode_bits + n_type_bits <= sizeof(T)*8, ("%d bits needed for opcode (%d possibilities), %d for type (%d possibilities); can't store this in %d bits (%d needed)!",
           n_opcode_bits, ARM_NR_TOTAL_OPCODES, n_type_bits, static_cast<int>(AbstractInstructionFormCalculator::Type::Count), sizeof(T)*8, n_opcode_bits+n_type_bits));
  }

  /* construct the result value */
  result = ARM_INS_OPCODE(T_ARM_INS(ins)) | ((static_cast<t_uint32>(INS_ABSTRACT_FORM(ins)) & ((1<<n_type_bits) - 1)) << n_opcode_bits);

  return result;
}
