/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */
#include "i386_obfuscations.h"
#include "i386_architecture_backend.h"
#include "i386_opaque_predicates.h"
#include "i386_opaque_predicates_opt.h"
using namespace std;

/***********************************************************
 * LOCO files
 *
 * Matias Madou
 *
 * mmadou@elis.ugent.be
 **********************************************************/
 
static void InsertBblIntOf6(t_bbl * bbl, t_int32 nr, t_randomnumbergenerator* rng);

/* t_bbl *BblCopy(t_bbl *bbl) {{{ */
/*Make a copy of a BBL and insert it in the function*/
static t_bbl *BblCopy(t_bbl *bbl)
{
  t_ins *ins, *copy;
  t_bbl *newbbl = BblNew(BBL_CFG(bbl));

  BblInsertInFunction(newbbl,BBL_FUNCTION(bbl));

  BBL_FOREACH_INS(bbl,ins)
  {
    copy = InsDup(ins);
    InsAppendToBbl(copy,newbbl);
  }

  return newbbl;
}
/* }}} */
static void change(t_uint32 * array,t_uint32 index1,t_uint32 index2)/* {{{ */
{
  t_uint32 temp=array[index1];
  array[index1]=array[index2];
  array[index2]=temp;
}/* }}} */
typedef struct _opaque_predicate
{
  t_const_string info;
  t_const_string file_name;
  size_t file_length;
  t_uint8* raw_data;
  t_i386_condition_code ccode;
  t_regset * used;
  t_reg based_on[5]; //The registers that are used to compute the true/false opaque predicate
} opaque_predicate;

static const size_t uninitialized_file_length = (size_t) -1;

/* List with !TRUE! opaque predicates {{{ */
static opaque_predicate predicate_table[]={
  /* 0) 7y^2-1!=x^2 {{{ */
  { "7y^2-1!=x^2",
    "opaque_predicates_1.S", uninitialized_file_length, NULL,
    I386_CONDITION_NZ,
    NULL,
    {
      I386_REG_EDX, 
      //I386_REG_EAX, 
      I386_REG_EBX, 
      I386_REG_NONE
    }
  },
  /* }}} */
  /* 1) 2|x+x^2 {{{ */
  { "2|x+x^2",
    "opaque_predicates_2.S", uninitialized_file_length, NULL,
    I386_CONDITION_Z,
    NULL,
    {
      I386_REG_EAX,
      I386_REG_NONE
    }
  },
  /* }}} */
  /* 2) 3|(x^3-x) {{{ */
  { "3|(x^3-x)", 
    "opaque_predicates_3.S", uninitialized_file_length, NULL,
    I386_CONDITION_Z,
    NULL,
    {
      I386_REG_ECX,
      I386_REG_NONE
    }
  },
  /* }}} */
  /* 3) x^2>=0{{{ */
  { "x^2>=0", 
    "opaque_predicates_4.S", uninitialized_file_length, NULL,
    I386_CONDITION_NS,
    NULL,
    {
      I386_REG_EAX,
      I386_REG_NONE
    }
  },
  /* }}} */
  /* 4) 2|x+x {{{ */
  { "2|x+x", 
    "opaque_predicates_5.S", uninitialized_file_length, NULL,
    I386_CONDITION_P,
    NULL,
    {
      I386_REG_EAX,
      I386_REG_NONE
    }
  },
  /* }}} */
  /* 5) 2|x+x {{{ */
  { "2|x+x", 
    "opaque_predicates_6.S", uninitialized_file_length, NULL,
    I386_CONDITION_P,
    NULL,
    {
      I386_REG_EAX,
      I386_REG_NONE
    }
  },
  /* }}} */
  /* 6) 2|x*x/2 Arboit {{{ */
  { "2|x*x/2 Arboit", 
    "opaque_predicates_7.S", uninitialized_file_length, NULL,
    I386_CONDITION_Z,
    NULL,
    {
      I386_REG_EAX,
      I386_REG_NONE
    }
  },
  /* }}} */
  /* dub0) 2|x v 8|(x^2-1) {{{ */
  { "2|x v 8|(x^2-1)", 
    "opaque_predicates_8.S", uninitialized_file_length, NULL,
    I386_CONDITION_Z, //is equal to je
    NULL,
    {
      I386_REG_EAX,
      I386_REG_NONE
    }
  },
  /* }}} */
  /* dub1) 3|(7x-5) => 9|(28x^2-13x-5) {{{ */
  { "3|(7x-5) => 9|(28x^2-13x-5)", 
    "opaque_predicates_9.S", uninitialized_file_length, NULL,
    I386_CONDITION_Z, //is equal to je
    NULL,
    {
      I386_REG_EAX,
      I386_REG_NONE
    }
  },
  /* }}} */
  {NULL,NULL,uninitialized_file_length,NULL,I386_CONDITION_NONE,NULL,{I386_REG_NONE}}
};
/* }}} */

static void OpenBinaryBuffer(opaque_predicate* predicate) {
  ASSERT(predicate->file_length == uninitialized_file_length, ("Opaque predicate %s (%s) already initialized", predicate->info, predicate->file_name));
  stringstream ss;
  if (obfuscation_i386_opaque_predicates_options.opaque_predicate_binary_directory) {
    ss << obfuscation_i386_opaque_predicates_options.opaque_predicate_binary_directory;
  } else {
    ss << OPAQUE_BINARIES_DIR;
  }
  
  ss << "/" << predicate->file_name << ".raw";
  
  ifstream file(ss.str().c_str(), ios::in|ios::binary|ios::ate);
  
  ASSERT(file.is_open(), ("Could not open file '%s'", ss.str().c_str()));
  
  predicate->file_length = file.tellg();
  predicate->raw_data = (t_uint8*)Malloc(predicate->file_length);
  
  file.seekg (0, ios::beg);
  file.read((char*)(predicate->raw_data), predicate->file_length);
  file.close();
}

/* void OpaqueInit(t_cfg * cfg) {{{ */
static void OpaqueInit(t_cfg * cfg)
{
  t_uint32 i,j;
  t_ins * ins=(t_ins*) Calloc(1,CFG_DESCRIPTION(cfg)->decoded_instruction_size);
  t_regset used = RegsetNew();

  /* This only initialises the regsets, and loads the binary code in memory */
  for(j=0;predicate_table[j].info!=0;j++)
  {
    used = RegsetNew();
    OpenBinaryBuffer(&predicate_table[j]);
    
    i = 0;
    
    while(i < predicate_table[j].file_length)
    {
      I386ClearIns(T_I386_INS(ins));
      
      i += I386DisassembleOne(T_I386_INS(ins), predicate_table[j].raw_data + i ,NULL,0,0);

      I386SetGenericInsInfo(T_I386_INS(ins));
      RegsetSetUnion(used,INS_REGS_DEF(ins));
    }
    predicate_table[j].used=(t_regset*) Calloc(1,sizeof(t_regset));
    *(predicate_table[j].used)=NullRegs;
    *(predicate_table[j].used)=used;
  }
}
/* }}} */

static t_regset CheckBbl(t_bbl * bbl, t_ins * after, t_bool recompute)/* {{{ */
{
  t_regset live = RegsetNew();
  t_ins * ins;

  if(recompute)
    CfgComputeLiveness(FUNCTION_CFG(BBL_FUNCTION(bbl)), CONTEXT_SENSITIVE);
  
  BBL_FOREACH_INS(bbl, ins)
  {
    if(ins==after)
      break;
  }

  if(ins==NULL)
    FATAL(("No instruction found!"));

  live=RegsetDup(InsRegsLiveAfter(ins));
  return live;
} /* }}} */

static t_regset CheckOpaqueBblGui(t_bbl * bbl, opaque_predicate * op, t_ins * after, t_bool recompute)/* {{{ */
{
  t_regset live = CheckBbl(bbl,after,recompute);
  return RegsetIntersect(*(op->used),live);
} /* }}} */
  
/* void InsertCondition(t_bbl * bbl, t_bool true_opaque_predicate, t_bbl * wrong_target, t_i386_condition_code true_condition_code) {{{ */
static void InsertCondition(t_bbl * bbl, t_bool true_opaque_predicate, t_bbl * wrong_target, t_i386_condition_code true_condition_code)
{
  t_ins * ins;
  t_bbl * split_off, * fallthrough_bbl=NULL;
  t_cfg_edge * edge;

  if(true_opaque_predicate)
  {
    t_cfg_edge * next_edge;
    /* code for a true opaque predicate */
    ins = InsNewForBbl(bbl);
    I386InstructionMakeCondJump(T_I386_INS(ins), true_condition_code);
    InsAppendToBbl(ins, bbl);

    BBL_FOREACH_SUCC_EDGE_SAFE(bbl,edge,next_edge)
    {
      if(fallthrough_bbl!=NULL)
        FATAL(("ERR"));
      fallthrough_bbl=(t_bbl*)CFG_EDGE_TAIL(edge);
      CfgEdgeKill(edge);

    }

    split_off=BblSplitBlock(bbl, BBL_INS_LAST(bbl), FALSE);
    ins = InsNewForBbl(split_off);
    I386InstructionMakeJump(T_I386_INS(ins));
    InsAppendToBbl(ins, split_off);

    CfgEdgeCreate(BBL_CFG(bbl),bbl,fallthrough_bbl,ET_JUMP);
    CfgEdgeCreate(BBL_CFG(bbl),split_off,wrong_target,ET_JUMP);

  }
  else
  {
    /* code for a false opaque predicate */
    ins = InsNewForBbl(bbl);
    if(!I386InvertConditionExist(true_condition_code))
      FATAL(("No invert condition found"));

    I386InstructionMakeCondJump(T_I386_INS(ins), I386InvertCondition(true_condition_code));
    InsAppendToBbl(ins, bbl);

    CfgEdgeCreate(BBL_CFG(bbl),bbl,wrong_target,ET_JUMP);

  }
}
/* }}} */

static t_ins * InsertPiece(t_bbl * bbl, t_const_string file)/* {{{ */
{
  t_ins * ins ;
  t_uint32 i;
  
  opaque_predicate predicate;
  predicate.file_name = file;
  predicate.raw_data = NULL;
  predicate.file_length = -1;
  
  OpenBinaryBuffer(&predicate);

  i = 0;
  while(i < predicate.file_length)
  {
    ins = InsNewForBbl(bbl);
    i += I386DisassembleOne(T_I386_INS(ins), predicate.raw_data + i ,NULL,0,0);
    InsAppendToBbl(ins, bbl);
  }

  Free(predicate.raw_data);
  
  return ins;
}/* }}} */

static void AddArboitPredicate(t_bbl * bbl, t_bbl *fake_bbl)/* {{{ */
{
  t_ins * ins = NULL;
  /* input values; %eax, %edx */
  t_cfg * cfg=BBL_CFG(bbl);
  /* Arboit Implementation; compiled: -O4 {{{*/

  t_bbl * split_off_1, *split_off_2;
  t_section* section;

  ins=InsertPiece(bbl,"arboit_predicate_piece_1.S");
  section = ObjectNewSubsection(CFG_OBJECT(cfg),5*sizeof(t_uint32),RODATA_SECTION);
  ((t_uint32 *) SECTION_DATA(section))[0]=0;
  ((t_uint32 *) SECTION_DATA(section))[1]=1;
  ((t_uint32 *) SECTION_DATA(section))[2]=2;
  ((t_uint32 *) SECTION_DATA(section))[3]=4;
  ((t_uint32 *) SECTION_DATA(section))[4]=5;

  /* The last instruction of bbl is actually pc-relative on PIE code, enforce that here:
   * movl OFFSET(,edx,4), esi [edx is offset in section -> section + edx*4]
   * =>
   * call get_pc_thunk.ebx
   * add  ebx, OFFSET
   * movl (ebx,edx,4), esi
   *
   * We want to address section + edx*4 => OFFSET = section-addr_of_mov => (section-addr_of_mov)+ebx[=addr_of_mov] + edx*4
   */
  t_bbl* split_off_0 = BblSplitBlock(bbl, ins, TRUE /* before last ins */);

  t_function* eip_stub = GetReturnAddressStubForRegister(cfg, I386_REG_EBX);
  t_i386_ins* ins_2;
  I386MakeInsForBbl(Call, Append, ins_2, bbl);
  CfgEdgeCreateCall(cfg, bbl, FUNCTION_BBL_FIRST(eip_stub), split_off_0, FunctionGetExitBlock(eip_stub));

  I386MakeInsForIns(Arithmetic, Before, ins_2, T_I386_INS(ins), I386_ADD, I386_REG_EBX, I386_REG_NONE, 0);
  RelocTableAddRelocToRelocatable(OBJECT_RELOC_TABLE(CFG_OBJECT(cfg)),
                                  0x0,
                                  T_RELOCATABLE(ins_2),
                                  0x0,
                                  T_RELOCATABLE(section),
                                  0x0,
                                  FALSE,
                                  NULL, /* edge */
                                  NULL, /* corresp */
                                  T_RELOCATABLE(split_off_0) , /* sec */
                                  "R00A00+R01-" "\\" WRITE_32);
  I386_OP_FLAGS(I386_INS_SOURCE1(T_I386_INS(ins_2))) = I386_OPFLAG_ISRELOCATED;
  bbl = split_off_0;
  ins=InsertPiece(bbl,"arboit_predicate_piece_2.S");
  split_off_1=BblSplitBlock(bbl, ins, FALSE);
  ins=InsertPiece(split_off_1,"arboit_predicate_piece_3.S");
  split_off_2=BblSplitBlock(split_off_1, ins, FALSE);
  CfgEdgeCreate(cfg,bbl,split_off_2,ET_JUMP);
  CfgEdgeCreate(cfg,split_off_1,split_off_1,ET_JUMP);
  InsertPiece(split_off_2,"arboit_predicate_piece_4.S");
  CfgEdgeCreate(cfg,split_off_2,fake_bbl,ET_JUMP);/*fake edge */

}/* }}} */

static void AddArboitPredicateSum(t_bbl * bbl, t_bbl * fake_bbl)/* {{{ */
{
  t_ins * ins = NULL;
  /* input values; %eax, %edx */
  t_cfg * cfg=BBL_CFG(bbl);
  /* Arboit Implementation; compiled: -Os, -O4 (combination...) {{{*/
  t_bbl * split_off_1, *split_off_2;

  ins=InsertPiece(bbl,"arboit_predicate_sum_piece_1.S");
  split_off_1=BblSplitBlock(bbl, ins, FALSE);
  ins=InsertPiece(split_off_1,"arboit_predicate_sum_piece_2.S");
  split_off_2=BblSplitBlock(split_off_1, ins, FALSE);
  InsertPiece(split_off_2,"arboit_predicate_sum_piece_3.S");

  CfgEdgeCreate(cfg,bbl,split_off_2,ET_JUMP);
  CfgEdgeCreate(cfg,split_off_1,split_off_1,ET_JUMP);
  CfgEdgeCreate(cfg,split_off_2,fake_bbl,ET_JUMP);/* Fake edge! */

}/* }}} */

static void InsertBblIntOf6(t_bbl * bbl, t_int32 nr, t_randomnumbergenerator* rng)
{
  t_int32 i=nr;
  t_bbl * split_off, * bbl_with_last_ins, * buggy_bbl;
  t_ins * h_ins; t_i386_ins * i386_ins;
  t_cfg * cfg;

  t_uint32 split_point;
  t_uint32 tel=1;
  t_ins * ins;

  if(bbl==NULL)
    FATAL(("ERR: Can't be NULL!"));
  if( BBL_NINS(bbl)<=2)
    FATAL(("ERR: Can't be below 2"));
  
  cfg=BBL_CFG(bbl);

  split_point=BBL_NINS(bbl)/2;
  //printf("Insertion of the opaque predicate\n");

  BBL_FOREACH_INS(bbl, ins)
  {
    if(tel==split_point)
      break;
    tel++;
  }

  split_off=BblSplitBlock(bbl, ins, FALSE);
  bbl_with_last_ins=BblSplitBlock(split_off, BBL_INS_LAST(split_off), TRUE);
  buggy_bbl=BblCopy(split_off);
  /* TODO Make the duplicated block buggy {{{ */
  /* First one, make the */
  BBL_FOREACH_INS_R(buggy_bbl,h_ins)
  {
    if(I386_INS_OPCODE(T_I386_INS(h_ins))==I386_MOV)
    {
      t_i386_operand * op=I386_INS_SOURCE1(T_I386_INS(h_ins));
      /* Don't set random immediate operands for relocated operands */
      if(I386_OP_TYPE(op) == i386_optype_imm && !(I386_OP_FLAGS(op) & I386_OPFLAG_ISRELOCATED) )
      {
        I386_OP_IMMEDIATE(op) = RNGGenerateWithRange(rng, 0, UINT32_MAX);
        //printf("size/4:%d Random nr:%d\n",n_size/4,I386_OP_IMMEDIATE(op));
        if(I386_OP_SCALE(op)==I386_SCALE_1)
          I386_OP_IMMEDIATE(op)&=0xff;
        else if(I386_OP_SCALE(op)==I386_SCALE_2)
          I386_OP_IMMEDIATE(op)&=0xffff;
        else if(I386_OP_SCALE(op)==I386_SCALE_4)
          I386_OP_IMMEDIATE(op)&=0xffffff;

      }
    }
    //      else if(I386_INS_OPCODE(T_I386_INS(h_ins))==I386_POP)
    //        I386_INS_SET_OPCODE(T_I386_INS(h_ins),I386_PUSH);
  }/* }}} */
  I386MakeInsForBbl(Push,Append,i386_ins,buggy_bbl, RNGGenerateWithRange(rng, 0, 7),0);
  I386MakeInsForBbl(Jump,Append,i386_ins,buggy_bbl);
  CfgEdgeCreate(cfg,buggy_bbl,bbl_with_last_ins,ET_JUMP);

  if(i<4)
  {
    opaque_predicate* predicate = NULL;
    t_i386_condition_code ccode = I386_CONDITION_NONE;
    t_uint32 l;

    switch(i)
    {
      case 0:
        predicate=&predicate_table[0];
        ccode=predicate_table[0].ccode;
        break;
      case 1:
        predicate=&predicate_table[2];
        ccode=predicate_table[2].ccode;
        break;
      case 2:
        predicate=&predicate_table[6];
        ccode=predicate_table[6].ccode;
        break;
      case 3:
        predicate=&predicate_table[7];
        ccode=predicate_table[7].ccode;
        break;
    }

    l = 0;
    
    while(l < predicate->file_length)
    {
      ins = InsNewForBbl(bbl);
      l += I386DisassembleOne(T_I386_INS(ins), predicate->raw_data + l ,NULL,0,0);
      InsAppendToBbl(ins, bbl);
      if(I386_INS_OPCODE(T_I386_INS(ins))==I386_Jcc)
      {
        t_bbl * h_split_off;
        h_split_off=BblSplitBlock(bbl, ins, FALSE);
        CfgEdgeCreate(BBL_CFG(bbl),bbl,split_off,ET_JUMP);
        bbl=h_split_off;
      }
    }    
    InsertCondition(bbl, TRUE,  buggy_bbl, ccode);
  }
  else if(i==4)
  {
    AddArboitPredicateSum(bbl,buggy_bbl);
  }
  else if(i==5)
  {
    AddArboitPredicate(bbl, buggy_bbl);
  }
}
/* }}} */

/* {{{ For the iterative framework */

static void BasedOn(t_reg based_on[5], int i) {
  switch(i) {
    case 0:
            memcpy(based_on, predicate_table[0].based_on, 5*sizeof(t_reg));
      break;
    case 1:
      memcpy(based_on, predicate_table[2].based_on, 5*sizeof(t_reg));
      break;
    case 2:
      memcpy(based_on, predicate_table[6].based_on, 5*sizeof(t_reg));
      break;
    case 3:
      memcpy(based_on, predicate_table[7].based_on, 5*sizeof(t_reg));
      break;
    case 4:
      based_on[0]=I386_REG_EAX;
      based_on[1]=I386_REG_NONE;
      break; // Missing break of Bertrand???? TODO
    case 5:
      based_on[0]=I386_REG_ECX;
      based_on[1]=I386_REG_EDX;
      based_on[2]=I386_REG_NONE;
    default:
      break;
  }
}

static t_bool CanBblBeTransformedWithPredicateNr(t_cfg* cfg, const t_bbl* bbl, int i) {
  t_uint32 split_point=BBL_NINS(bbl)/2;
  t_uint32 tel=1;
  t_ins* ins;
  t_regset live = RegsetNew();
  t_regset used;
  t_reg based_on[5]; //The registers that are used to compute the true/false opaque predicate
  t_i386_condition_code ccode;


  BBL_FOREACH_INS(bbl, ins)
  {
    if(tel==split_point)
      break;
    tel++;
  }

  if(tel!=split_point)
    FATAL(("Error: tel != split_point"));

  live=RegsetDup(InsRegsLiveAfter(ins));

  /* Register changed */
  used=RegsetNew();
  RegsetSetAddReg(used,I386_CONDREG_AF);
  RegsetSetAddReg(used,I386_CONDREG_OF);
  RegsetSetAddReg(used,I386_CONDREG_SF);
  RegsetSetAddReg(used,I386_CONDREG_PF);
  RegsetSetAddReg(used,I386_CONDREG_ZF);
  RegsetSetAddReg(used,I386_CONDREG_CF);

  BasedOn(based_on, i);

  if(RegsetIsEmpty(RegsetIntersect(used,live)))
  {
    t_uint32 k=0;
    t_ins * i_ins;
    t_bool variable;

    t_procstate * next_state= ProcStateNew(CFG_DESCRIPTION(cfg));
    ProcStateDup(next_state,BBL_PROCSTATE_IN(bbl),CFG_DESCRIPTION(cfg));

    /* Check Variability */
    BBL_FOREACH_INS(bbl,i_ins)
    {   
      if(k==split_point)
        break;
      //VERBOSE(0,("@I\n",i_ins));
      I386InstructionEmulator(T_I386_INS(i_ins),next_state,TRUE);
      k++;
    }

    variable=TRUE;

    for(k=0;based_on[k]!=I386_REG_NONE;k++)
    {
      t_register_content content;
      t_lattice_level lev;

      lev=ProcStateGetReg(next_state,based_on[k],&content);

      if (lev==CP_VALUE)
        variable=FALSE;
    }

    ProcStateFree(next_state);

    if(variable)
    {
      return TRUE;
    }
  }

  return FALSE;
}

typedef struct _t_opaque_predicates_transform_info t_opaque_predicates_transform_info;

struct _t_opaque_predicates_transform_info {
  t_int32 possible_transforms[6];
  t_int32 max_transform;
};

bool I386OpaquePredicateTransformation::canTransform(const t_bbl* bbl) const {
  static t_bool initialized = FALSE; // TODO; genericize
  t_cfg* cfg = BBL_CFG(bbl);

  if (!OpaquePredicateTransformation::canTransform(bbl))
    return false;

  if (!initialized) {
    OpaqueInit(cfg);
    initialized = TRUE;
  }

  // TODO genericize this all! Init of function, and iterate over function instead of bbs
  {
    t_int32 i,j;
    t_procstate * state = BBL_PROCSTATE_IN(bbl);
    t_bool all_top=TRUE;
    t_register_content value;
    t_opaque_predicates_transform_info info;

    if(state==NULL) {
      return false;
    }

    for(j=I386_REG_EAX;j<I386_REG_ESP;j++)
      if(ProcStateGetReg(state,j,&value)!=CP_TOP)
              all_top=FALSE;

    if(all_top) {
      return false;
    }

    if(!bbl || BBL_NINS(bbl)<=2) {
      return false;
    }
            
    info.max_transform = 0;
    for(j=0;j<6;j++) {
      if (CanBblBeTransformedWithPredicateNr(cfg, bbl, j)) {
        info.possible_transforms[info.max_transform++] = j;
      }
    }

    if (info.max_transform > 0) {
      return true;
    }
  }

  return false;
}

bool I386OpaquePredicateTransformation::doTransform(t_bbl* bbl, t_randomnumbergenerator * rng) {
  t_int32 chosen;
  t_opaque_predicates_transform_info info;
  t_cfg* cfg = BBL_CFG(bbl);

  /* TODO: reuse from above */
  {
    t_int32 i,j;
    t_procstate * state = BBL_PROCSTATE_IN(bbl);
    t_bool all_top=TRUE;
    t_register_content value;

    for(j=I386_REG_EAX;j<I386_REG_ESP;j++)
      if(ProcStateGetReg(state,j,&value)!=CP_TOP)
              all_top=FALSE;

    info.max_transform = 0;
    for(j=0;j<6;j++) {
      if (CanBblBeTransformedWithPredicateNr(cfg, bbl, j)) {
        info.possible_transforms[info.max_transform++] = j;
      }
    }
  }
  
  ASSERT(info.max_transform > 0, ("Inconsistent state in @eiB for opaque predicates: %i", bbl, info.max_transform));

  chosen = RNGGenerateWithRange(rng, 0, info.max_transform - 1);

  InsertBblIntOf6(bbl,info.possible_transforms[chosen], rng);

  bblsTransformed++;

  return true;
}

void I386OpaquePredicateTransformation::dumpStats(const std::string& prefix) {
  VERBOSE(0, ("%sI386Opaque_Stats,bbls_transformed,%i", prefix.c_str(), bblsTransformed));
}

I386OpaquePredicateTransformation::I386OpaquePredicateTransformation()
  : bblsTransformed(0)
{
  AddOptionsListInitializer(obfuscation_i386_opaque_predicates_option_list); I386OpaquePredicatesOptInit();
}

/* }}} */

/* vim: set shiftwidth=2 foldmethod=marker:*/
