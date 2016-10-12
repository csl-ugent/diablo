/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

#include <diablosmc.h>
#include <diabloi386.h>

/* void SmcAssembleSection(t_section * sec) {{{ */
void SmcAssembleSection(t_section * sec)
{
  t_uint32 total;
  int nins = 0;
  t_codebyte * codebyte;

  t_uint8 * data = SECTION_TMP_BUF(sec);
  t_uint8 * resized;
  t_cfg * cfg;

  /* if we've only disassembled and reassembled without flowgraphing, we have
   * to give all instructions their old length back (because otherwise we'd
   * need to relocate all jump offsets etc, which is impossible without a flow
   * graph). however, this is not always possible (the diablo instruction
   * representation doesn't allow an exact description of the instruction
   * encoding), so if necessary we pad the instruction with noops until it
   * reaches the desired length.
   */
  total = 0;
  for (codebyte = ((t_codebyte *)SECTION_DATA(sec)); codebyte; codebyte = CODEBYTE_NEXT_IN_CHAIN(codebyte))
  {
    *data = STATE_VALUE(STATE_REF_STATE(STATELIST_FIRST(CODEBYTE_STATELIST(codebyte))));
    data++;
    total+=1;
  }

  {
    resized=Malloc(sizeof(t_uint8)*total);
    data = SECTION_TMP_BUF(sec);
    memcpy(resized,data,(size_t) total);

    Free(data);

    VERBOSE(0,("section prev size @G new size 0x%x\n", SECTION_CSIZE(sec),total));
    SECTION_SET_TMP_BUF(sec, resized);
    SECTION_SET_CSIZE(sec, AddressNew32(total));
  }
  ObjectPlaceSections(SECTION_OBJECT(sec), FALSE, FALSE, TRUE);

  cfg = CODEBYTE_CFG(((t_codebyte *)SECTION_DATA(sec)));

  printf("assembled %d instructions, for a total of 0x%x bytes\n",nins,total);
  SECTION_SET_DATA(sec,NULL);

  /*Remove Relocations {{{*/
  {
    t_codebyte_ref * codebyte_ref;
    CFG_FOREACH_CODEBYTE(cfg,codebyte)
    {
      while(CODEBYTE_REFED_BY(codebyte))
      {
        if(RELOC_SWITCH_EDGE(RELOC_REF_RELOC(CODEBYTE_REFED_BY(codebyte))))
          CFG_EDGE_SET_REL(RELOC_SWITCH_EDGE(RELOC_REF_RELOC(CODEBYTE_REFED_BY(codebyte))),NULL);
        RelocTableRemoveReloc(OBJECT_RELOC_TABLE(CFG_OBJECT(CODEBYTE_CFG(codebyte))),RELOC_REF_RELOC(CODEBYTE_REFED_BY(codebyte)));
      }
      while(CODEBYTE_REFERS_TO(codebyte))
      {
        if(RELOC_SWITCH_EDGE(RELOC_REF_RELOC(CODEBYTE_REFERS_TO(codebyte))))
          CFG_EDGE_SET_REL(RELOC_SWITCH_EDGE(RELOC_REF_RELOC(CODEBYTE_REFERS_TO(codebyte))),NULL);
        RelocTableRemoveReloc(OBJECT_RELOC_TABLE(CFG_OBJECT(CODEBYTE_CFG(codebyte))),RELOC_REF_RELOC(CODEBYTE_REFERS_TO(codebyte)));
      }
    }
  }
  /*}}}*/

  /* clean up the opcode hash table (no longer needed if all sections are assembled) */
  if (sec == OBJECT_CODE(SECTION_OBJECT(sec))[OBJECT_NCODES(SECTION_OBJECT(sec))-1]){
    CodebyteStopNextInChain(cfg);
    CodebyteStopPrevInChain(cfg);
    CfgStopCodebytelist(SECTION_OBJECT(sec));
    InsStopStatelist(cfg);
  }
} 
/* }}} */

/* vim: set shiftwidth=2 expandtab cinoptions=p5,t0,(0, foldmethod=marker tw=80 cindent: */
