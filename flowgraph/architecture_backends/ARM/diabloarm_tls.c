#include <diabloarm.h>

static
t_symbol *TlsStartSymbol(t_object *obj)
{
  t_symbol *tlsstart_symbol = SymbolTableGetSymbolByName(OBJECT_SUB_SYMBOL_TABLE(obj), "$tls_start");

  /* create the symbol in case it does not exist yet */
  t_section * undef = OBJECT_PARENT(obj) ? OBJECT_UNDEF_SECTION(OBJECT_PARENT(obj)) : OBJECT_UNDEF_SECTION(obj);
  if (!tlsstart_symbol
      /* take care when the $tls_start symbol has been created, but no TLS sections exist! */
      || T_SECTION(SYMBOL_BASE(tlsstart_symbol)) == undef) {
    t_section *tbss_section = SectionGetFromObjectByName(obj, ".tbss");
    ASSERT(tbss_section, ("could not find .tbss section!"));
    tlsstart_symbol = SymbolTableAddSymbol(OBJECT_SUB_SYMBOL_TABLE(obj), "$tls_start", "R00A00+$", 10, FALSE, FALSE, T_RELOCATABLE(SectionGetFromObjectByName(obj, ".tbss")), AddressNew32(0), AddressNew32(0), NULL, AddressNew32(0), SYMBOL_TYPE_NOTYPE);
  }

  return tlsstart_symbol;
}

/* This function adds a new 4-byte TLS variable to the program.
 *
 * Parameters:
 *    obj: the object which TLS variable should be added to.
 *    name: the name of the TLS variable. */
void TlsCreate(t_object *obj, t_const_string name)
{
  t_symbol *sym = SymbolTableGetSymbolByName(OBJECT_SUB_SYMBOL_TABLE(obj), name);
  ASSERT(!sym, ("symbol %s exists! @S", name, sym));

  /* create the .tbss section in case it does not exist yet */
  if (!SectionGetFromObjectByName(obj, ".tbss")) {
    t_section *tbss_section = SectionCreateForObject(obj, TLSBSS_SECTION, NULL, AddressNullForObject(obj), ".tbss");
    SECTION_SET_ALIGNMENT(tbss_section, AddressNew32(4));
  }

  t_section *sec = ObjectNewSubsection(obj, AddressNew32(4), TLSBSS_SECTION);
  SymbolTableAddSymbol(OBJECT_SUB_SYMBOL_TABLE(obj), name, "R00A00+$", -1, NO, YES, T_RELOCATABLE(sec), AddressNew32(0), AddressNew32(0), NULL, AddressNew32(0), 0);
}

/* This function adds the necessary instructions to load the value of a TLS variable.
 *
 * Requirements: one dead register.
 * Parameters:
 *    bbl: the BBL in which to add the instructions.
 *    ins: the instruction AFTER which to add the instructions, NULL if they should be put at the front.
 *    dst: the register in which the TLS variable value will be loaded.
 *    name: the name of the TLS variable to be loaded.
 * Returns: the lastly inserted instruction after which the value is loaded. */
t_ins *TlsLoad(t_bbl *bbl, t_ins *ins, t_reg dst, t_const_string name)
{
  t_object *obj = CFG_OBJECT(BBL_CFG(bbl));

  /* find the name */
  t_symbol *sym = SymbolTableGetSymbolByName(OBJECT_SUB_SYMBOL_TABLE(obj), name);
  ASSERT(sym, ("could not find any symbol with name %s!", name));

  /* thread control block pointer:
   * MRC 15, 0, rX, cr13, cr0, {3} */
  t_arm_ins *mrc = T_ARM_INS(InsNewForBbl(bbl));
  ArmInsMakeMrc(mrc, 15, 0, dst, 13, 0, 3);
  if (ins) InsInsertAfter(T_INS(mrc), ins);
  else     InsPrependToBbl(T_INS(mrc), bbl);

  /* load the TLS variable to the register ('imm' is the offset of the variable within the TCB, and
   * is calculated by means of a relocation):
   * LDR rX, [rX, #imm] */
  t_arm_ins *ldr;
  ArmMakeInsForIns(Ldr, After, ldr, mrc, FALSE, dst, dst, ARM_REG_NONE, 0, ARM_CONDITION_AL, TRUE, TRUE, FALSE);

  /* relocation to fill in the immediate of the LDR (which calculates the offset to the $tls_start symbol) */
  t_reloc *rel = RelocTableAddRelocToRelocatable(OBJECT_RELOC_TABLE(obj), AddressNew32(0),
                                                  T_RELOCATABLE(ldr), AddressNew32(0),
                                                  T_RELOCATABLE(SYMBOL_BASE(sym)), AddressNew32(0),
                                                  FALSE, NULL, NULL, NULL,
                                                  "R00A00+R01-s0008+\\l ifffff000& | w\\s0000$");
  RelocAddRelocatable(rel, T_RELOCATABLE(SYMBOL_BASE(TlsStartSymbol(obj))), AddressNew32(0));

  return T_INS(ldr);
}

/* This function adds the necessary instructions to store the value of a TLS variable.
 *
 * Requirements: two dead registerd.
 * Parameters:
 *    bbl: the BBL in which to add the instructions.
 *    ins: the instruction AFTER which to add the instructions, NULL if they should be put at the front.
 *    src: the register that contains the new TLS variable value.
 *    tmp: a dead, temporary, register to be used by these instructions.
 *    name: the name of the TLS variable to be stored.
 * Returns: the lastly inserted instruction after which the value is stored. */
t_ins *TlsStore(t_bbl *bbl, t_ins *ins, t_reg src, t_reg tmp, t_const_string name)
{
  t_object *obj = CFG_OBJECT(BBL_CFG(bbl));

  /* find the name */
  t_symbol *sym = SymbolTableGetSymbolByName(OBJECT_SUB_SYMBOL_TABLE(obj), name);
  ASSERT(sym, ("could not find any symbol with name %s!", name));

  /* thread control block pointer:
   * MRC 15, 0, rX, cr13, cr0, {3} */
  t_arm_ins *mrc = T_ARM_INS(InsNewForBbl(bbl));
  ArmInsMakeMrc(mrc, 15, 0, tmp, 13, 0, 3);
  if (ins) InsInsertAfter(T_INS(mrc), ins);
  else     InsPrependToBbl(T_INS(mrc), bbl);

  /* store the register value to the TLS variable ('imm' is the offset of the variable within the TCB, and
   * is calculated by means of a relocation):
   * STR rY, [rX, #imm] */
  t_arm_ins *str;
  ArmMakeInsForIns(Str, After, str, mrc, FALSE, src, tmp, ARM_REG_NONE, 0, ARM_CONDITION_AL, TRUE, TRUE, FALSE);

  /* relocation to fill in the immediate of the STR (which calculates the offset to the $tls_start symbol) */
  t_reloc *rel = RelocTableAddRelocToRelocatable(OBJECT_RELOC_TABLE(obj), AddressNew32(0),
                                                  T_RELOCATABLE(str), AddressNew32(0),
                                                  T_RELOCATABLE(SYMBOL_BASE(sym)), AddressNew32(0),
                                                  FALSE, NULL, NULL, NULL,
                                                  "R00A00+R01-s0008+\\l ifffff000& | w\\s0000$");
  RelocAddRelocatable(rel, T_RELOCATABLE(SYMBOL_BASE(TlsStartSymbol(obj))), AddressNew32(0));

  return T_INS(str);
}
