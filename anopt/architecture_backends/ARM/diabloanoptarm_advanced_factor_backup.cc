#include "diabloanoptarm_advanced_factor.hpp"

using namespace std;

#define REGISTER_BACKUP_SECTION_NAME "$registers"

#define BACKUP_VERBOSITY 1

/* functions to manage register backups */
static int backup_slot_index = 0;
static int nr_allocated_backup_slots = 0;
static t_section *register_backup_section = nullptr;

struct BackupData {
  t_reg reg;
  TransformedSliceInformation *info;
};
static map<t_bbl *, vector<BackupData>> backup_queue;

static
t_arm_ins *BackupRegister(t_bbl *bbl, t_arm_ins *ins, bool after, t_reg reg, t_reg tmp, AddedInstructionInfo& added_ins_info) {
  if (register_backup_section == nullptr) {
    t_object *obj = CFG_OBJECT(BBL_CFG(bbl));
    register_backup_section = SectionCreateForObject(ObjectGetLinkerSubObject(obj), DATA_SECTION, SectionGetFromObjectByName(obj, ".data"), AddressNew32(0), REGISTER_BACKUP_SECTION_NAME);
    SECTION_SET_ALIGNMENT(register_backup_section, AddressNewForSection(obj, 4));
  }

  /* do we have enough room? */
  VERBOSE(BACKUP_VERBOSITY, ("backup r%d (tmp r%d) in slot %d", reg, tmp, backup_slot_index));

  if (nr_allocated_backup_slots <= backup_slot_index) {
    /* allocate more space if needed */
    VERBOSE(BACKUP_VERBOSITY, ("add slot! %d slots in total", backup_slot_index+1));

    SECTION_SET_CSIZE(register_backup_section, SECTION_CSIZE(register_backup_section) + 4);
    SECTION_SET_DATA(register_backup_section, Realloc(SECTION_DATA(register_backup_section), SECTION_CSIZE(register_backup_section)));
    VERBOSE(BACKUP_VERBOSITY, ("section now @T", register_backup_section));

    nr_allocated_backup_slots = backup_slot_index;
  }

  /* ADDR <tmp>, <table address> */
  t_arm_ins *addrp_ins = AddInstructionToBblI(bbl, ins, after);
  ArmInsMakeMov(addrp_ins, tmp, ARM_REG_NONE, 0, ARM_CONDITION_AL);
  t_reloc *reloc = RelocTableAddRelocToRelocatable(OBJECT_RELOC_TABLE(CFG_OBJECT(BBL_CFG(bbl))), AddressNew32(0),
                                                    T_RELOCATABLE(addrp_ins), AddressNew32(0),
                                                    T_RELOCATABLE(register_backup_section), AddressNew32(backup_slot_index * 4),
                                                    FALSE, NULL, NULL, NULL,
                                                    "R00A00+" "\\" WRITE_32);
  ArmInsMakeAddressProducer(addrp_ins, 0, reloc);
  AFFactoringLogInstruction(addrp_ins, "BACKUP");
  added_ins_info.AddInstruction(T_INS(addrp_ins));

  /* STR <reg>, [<tmp>, #<slot offset>] */
  t_arm_ins *str_ins = AddInstructionToBblI(bbl, addrp_ins, true);
  ArmInsMakeStr(str_ins, reg, tmp, ARM_REG_NONE, 0, ARM_CONDITION_AL, true, true, false);
  AFFactoringLogInstruction(str_ins, "BACKUP");
  added_ins_info.AddInstruction(T_INS(str_ins));

  return str_ins;
}

static
t_arm_ins *RestoreRegister(t_bbl *bbl, t_arm_ins *ins, bool after, t_reg reg, int slot_index, AddedInstructionInfo& added_ins_info) {
  ASSERT(register_backup_section, ("what? no backup done yet!"));

  /* ADDR <reg>, <table address> */
  t_arm_ins *addrp_ins = AddInstructionToBblI(bbl, ins, after);
  ArmInsMakeMov(addrp_ins, reg, ARM_REG_NONE, 0, ARM_CONDITION_AL);
  t_reloc *reloc = RelocTableAddRelocToRelocatable(OBJECT_RELOC_TABLE(CFG_OBJECT(BBL_CFG(bbl))), AddressNew32(0),
                                                    T_RELOCATABLE(addrp_ins), AddressNew32(0),
                                                    T_RELOCATABLE(register_backup_section), AddressNew32(slot_index * 4),
                                                    FALSE, NULL, NULL, NULL,
                                                    "R00A00+" "\\" WRITE_32);
  ArmInsMakeAddressProducer(addrp_ins, 0, reloc);
  AFFactoringLogInstruction(addrp_ins, "BACKUP");
  added_ins_info.AddInstruction(T_INS(addrp_ins));

  /* LDR <reg>, [<reg>, #<slot offset>] */
  t_arm_ins *ldr_ins = AddInstructionToBblI(bbl, addrp_ins, true);
  ArmInsMakeLdr(ldr_ins, reg, reg, ARM_REG_NONE, 0, ARM_CONDITION_AL, true, true, false);
  AFFactoringLogInstruction(ldr_ins, "BACKUP");
  added_ins_info.AddInstruction(T_INS(ldr_ins));

  return ldr_ins;
}

static
int ResetRegisterBackups() {
  backup_slot_index = 0;

  return backup_slot_index;
}

static
int IncrementBackupSlotIndex() {
  backup_slot_index++;

  return backup_slot_index;
}

void QueueForBackup(t_bbl *landing_site, t_reg reg, TransformedSliceInformation *info) {
  backup_queue[landing_site].push_back(BackupData{reg, info});
}

AddedInstructionInfo BackupRegisters(t_bbl *factored_bbl, t_regset new_live, bool added_trampoline, BblSet& modified) {
  AddedInstructionInfo result = AddedInstructionInfo();
  t_cfg *cfg = BBL_CFG(factored_bbl);

  /* backup */
  int current_backup_slot_index = ResetRegisterBackups();
  for (auto p : backup_queue) {
    t_bbl *landing_site = p.first;
    vector<BackupData> backup_todo = p.second;
    VERBOSE(1, ("landing site @eiB", landing_site));

    /* dead registers after slice */
    t_regset possible_backup_registers = AFBblRegsLiveBeforeTail(BBL_SUCC_FIRST(landing_site));
    RegsetSetInvers(possible_backup_registers);

    /* remove registers overwritten by the slice */
    RegsetSetDiff(possible_backup_registers, AFFunctionDefines(BBL_FUNCTION(factored_bbl)));

    /* remove registers defined by the slice landing site */
    RegsetSetDiff(possible_backup_registers, BblRegsMaybeDef(landing_site));

    /* remove new live registers used by the dispatcher */
    RegsetSetDiff(possible_backup_registers, new_live);

    TransformedSliceInformation *slice_information = backup_todo[0].info;
    t_gpregisters overwritten_registers = slice_information->overwritten_registers;
    t_all_register_info all_registers = slice_information->register_info;

    t_cfg_edge *af_out = BBL_PRED_FIRST(landing_site);
    if (added_trampoline) {
      af_out = BBL_PRED_FIRST(CFG_EDGE_HEAD(af_out));
    }
    ASSERT(CfgEdgeIsAF(af_out), ("not an AF edge! @eiB @E", landing_site, af_out));

    /* construct list of predecessors */
    BblSet predecessors;
    t_cfg_edge *af_in;
    BBL_FOREACH_PRED_EDGE(factored_bbl, af_in) {
      t_cfg_edge *could_be = af_in;
      ASSERT(CfgEdgeIsAF(af_in), ("expected AF edge @E @eiB", af_in, factored_bbl));
      ASSERT(CFG_EDGE_AF_CORR(could_be), ("no corresponding AF edge!"));

      if (CFG_EDGE_AF_CORR(could_be) != af_out)
        continue;

      t_bbl *pred = CFG_EDGE_HEAD(could_be);
      if (modified.find(pred) == modified.end()) {
        function<void(t_bbl*)> add_preds;
        add_preds = [&modified, &add_preds, &possible_backup_registers, &predecessors] (t_bbl *bbl) {
          t_cfg_edge *e;
          BBL_FOREACH_PRED_EDGE(bbl, e) {
            t_bbl *pred = CFG_EDGE_HEAD(e);

            if (modified.find(pred) == modified.end())
              add_preds(pred);
            else {
              RegsetSetDiff(possible_backup_registers, BblRegsLiveBefore(pred));
              predecessors.insert(pred);
            }
          }
        };

        add_preds(pred);
      }
      else {
        /* just add this predecessor */
        RegsetSetDiff(possible_backup_registers, BblRegsLiveBefore(pred));
        RegsetSetDiff(possible_backup_registers, BblRegsMaybeDef(pred));
        predecessors.insert(pred);
      }
    }

    t_regset backup_registers = RegsetNew();
    for (auto backup : backup_todo)
      RegsetSetAddReg(backup_registers, backup.reg);

    struct TemporaryBackupData {
      t_reg tmp;
      t_arm_ins *restore_ins;
    };
    map<t_bbl *, TemporaryBackupData> temp_backup_data;

    for (auto backup : backup_todo) {
      t_reg reg = backup.reg;
      VERBOSE(0, ("BACKUP r%d", reg));

      /* sanity check */
      if (predecessors.size() == 0) {
        static int x = 0;
        DEBUG(("no predecessors! dumping dots to preds%d @eiB @E corr @E @eiB", x, landing_site, af_out, CFG_EDGE_AF_CORR(af_out), factored_bbl));
        DumpDots(cfg, "preds", x);
        x++;
      }

      /* find a usable register */
      t_reg usable = ARM_REG_NONE;
      REGSET_FOREACH_REG(possible_backup_registers, usable) break;

      if (ARM_REG_R0 <= usable && usable < ARM_REG_R15) {
        RegsetSetSubReg(possible_backup_registers, usable);

        /* save the register */
        for (t_bbl *pred : predecessors) {
          VERBOSE(0, ("using dead register r%d", usable));
          t_arm_ins *ins = ProduceRegisterMoveInBbl(pred, usable, reg, true, false);
          result.AddInstruction(T_INS(ins));
        }

        /* restore the register */
        t_arm_ins *ins = ProduceRegisterMoveInBbl(landing_site, reg, usable, false, true);
        result.AddInstruction(T_INS(ins));
      }
      else {
        /* try to backup the register */

        /* save the register */
        for (t_bbl *pred : predecessors) {
          /* find a constant producer */
          t_arm_ins *cprod_ins = NULL;
          t_arm_ins *mov_ins = NULL;

          t_arm_ins *ins;
          BBL_FOREACH_ARM_INS(pred, ins) {
            if (!cprod_ins
                && ARM_INS_OPCODE(ins) == ARM_CONSTANT_PRODUCER)
              cprod_ins = ins;

            if (!mov_ins
                && ARM_INS_OPCODE(ins) == ARM_MOV
                && !(ARM_INS_FLAGS(ins) & FL_IMMED))
              mov_ins = ins;

            /* we need to have the original register! */
            if (RegsetIn(ARM_INS_REGS_DEF(ins), reg))
              break;
          }

          t_arm_ins *before = (cprod_ins) ? cprod_ins : mov_ins;
          if (before
              /* the temporary register and the subject register can't be equal */
              && (reg != ARM_INS_REGA(before))) {
            /* constant producer found */
            VERBOSE(0, ("backup before @I", before));

            BackupRegister(pred, before, false, reg, ARM_INS_REGA(before), result);
          }
          else {
            /* need to find another tmp register */
            VERBOSE(0, ("need to find another way! @iB", pred));

            /* back up the register in a data slot immediately after the predecessor */

            if (temp_backup_data.find(pred) == temp_backup_data.end()) {
              /* look for a temporary register */
              t_regset possible_temp_registers = CFG_DESCRIPTION(cfg)->int_registers;
              RegsetSetDiff(possible_temp_registers, backup_registers);

              t_reg tmp_backup_register = ARM_REG_NONE;
              REGSET_FOREACH_REG(possible_temp_registers, tmp_backup_register) break;
              ASSERT(tmp_backup_register < ARM_REG_R15
                      && tmp_backup_register != ARM_REG_NONE, ("could not find temporary register for backup r%d! @iB", pred, reg));
              VERBOSE(0, ("using temporary register r%d", tmp_backup_register));

              t_object *obj = CFG_OBJECT(cfg);

              t_arm_ins *push_ins = AddInstructionToBblI(pred, NULL, false);
              ArmInsMakePush(push_ins, 1<<tmp_backup_register, ARM_CONDITION_AL, false);
              AFFactoringLogInstruction(push_ins), "BACKUP";
              result.AddInstruction(T_INS(push_ins));

              t_arm_ins *pop_ins = AddInstructionToBblI(pred, push_ins, true);
              ArmInsMakePop(pop_ins, 1<<tmp_backup_register, ARM_CONDITION_AL, false);
              AFFactoringLogInstruction(pop_ins, "BACKUP");
              result.AddInstruction(T_INS(pop_ins));

              temp_backup_data[pred] = TemporaryBackupData{tmp_backup_register, pop_ins};

              /* fix the block: add a branch if needed */
              t_cfg_edge *succ = BBL_SUCC_FIRST(pred);
              ASSERT(CFG_EDGE_CAT(succ) == ET_IPFALLTHRU
                      || CFG_EDGE_CAT(succ) == ET_IPJUMP
                      || CFG_EDGE_CAT(succ) == ET_JUMP, ("unexpected edge type! @E @eiB", succ, pred));

              if (CFG_EDGE_CAT(succ) == ET_IPFALLTHRU) {
                /* need to add branch */

                /* create an unconditional branch, jumping to the factored BBL */
                t_arm_ins *ins;
                ArmMakeInsForBbl(UncondBranch, Append, ins, pred, false);
                result.AddInstruction(T_INS(ins));

                /* make this edge a jump edge */
                CFG_EDGE_SET_CAT(succ, ET_IPJUMP);
              }
            }

            /* back up the register before the restoration instruction */
            BackupRegister(pred, temp_backup_data[pred].restore_ins, false, reg, temp_backup_data[pred].tmp, result);
          }
        }

        /* restore the register */
        RestoreRegister(landing_site, NULL, true, reg, current_backup_slot_index, result);

        current_backup_slot_index = IncrementBackupSlotIndex();
      }

      for (auto pred: predecessors)
        VERBOSE(1, ("predecessor @iB", pred));
      VERBOSE(1, ("factored @iB", factored_bbl));
      VERBOSE(1, ("landing site @iB", landing_site));
    }
  }

  backup_queue.clear();

  return result;
}
