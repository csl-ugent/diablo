#include <diablosmc.h>
#ifndef SMCFLOWGRAPH_H
#define SMCFLOWGRAPH_H

void SmcAddCodebytes(unsigned char * codep, t_uint32 len, t_ins * ins, t_bool relocatable);
void SmcInitInstruction(t_ins * ins);
//void SmcDeflowgraphSection(t_section * code);
void SmcCreateDataIns(t_ins * ins);
void SmcAssembleSection(t_section * sec);
void SmcCalcReloc(t_reloc * rel, t_object * obj);
void SmcInstallStuff(t_cfg * cfg);

#define INS_STATE_REF_FIRST(ins) (STATELIST_FIRST(INS_STATELIST(ins)))
#define INS_STATE_REF_LAST(ins) (STATELIST_LAST(INS_STATELIST(ins)))
#define INS_STATE_FIRST(ins) (STATE_REF_STATE(STATELIST_FIRST(INS_STATELIST(ins))))
#define INS_STATE_LAST(ins)  (STATE_REF_STATE(STATELIST_LAST(INS_STATELIST(ins))))
#define INS_CODEBYTE_FIRST(ins) (STATE_CODEBYTE(STATE_REF_STATE(INS_STATE_REF_FIRST(ins))))
#define INS_CODEBYTE_LAST(ins) (STATE_CODEBYTE(STATE_REF_STATE(INS_STATE_REF_LAST(ins))))

#define CODEBYTE_STATE_FIRST(codebyte) (STATE_REF_STATE(STATELIST_FIRST(CODEBYTE_STATELIST(codebyte))))
#define CODEBYTE_STATE_LAST(codebyte) (STATE_REF_STATE(STATELIST_LAST(CODEBYTE_STATELIST(codebyte))))

#define INS_FOREACH_STATE_REF(ins,state_ref) \
  for (state_ref = INS_STATE_REF_FIRST(ins); \
      state_ref!=NULL; \
      state_ref = state_ref->next)

#define INS_FOREACH_STATE(ins,state,state_ref) \
  for (state_ref = INS_STATE_REF_FIRST(ins), state = (state_ref)?STATE_REF_STATE(state_ref):NULL; \
      state_ref!=NULL;\
      state_ref = STATE_REF_NEXT(state_ref), state = (state_ref)?STATE_REF_STATE(state_ref):NULL)

#define INS_FOREACH_CODEBYTE(ins,codebyte,state_ref)  \
for (state_ref = STATELIST_FIRST(INS_STATELIST(ins)),codebyte = (state_ref)?STATE_CODEBYTE(STATE_REF_STATE(state_ref)):NULL; \
    state_ref!=NULL; \
    state_ref = state_ref->next, codebyte = (state_ref)?STATE_CODEBYTE(STATE_REF_STATE(state_ref)):NULL)
																											
#define CODEBYTE_FOREACH_STATE(codebyte, state, state_ref) \
for(state_ref = STATELIST_FIRST(CODEBYTE_STATELIST(codebyte)), state = (state_ref)?STATE_REF_STATE(state_ref):NULL; \
    state_ref!=NULL; \
    state_ref = state_ref->next, state=(state_ref)?STATE_REF_STATE(state_ref):NULL)

#endif
