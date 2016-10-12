#include <diablosmc.h>
#include <diabloi386.h>

void CodebyteUnlinkFromCfg(t_codebyte * codebyte)/*{{{*/
{
  t_codebyte_ref * codebyte_ref = CODEBYTE_CODEBYTE_REF(codebyte);

  /*Cfg no contains list of codebytes*/
  if(codebyte_ref == NULL)
    return;

  if(CODEBYTE_REF_PREV(codebyte_ref))
    CODEBYTE_REF_SET_NEXT(CODEBYTE_REF_PREV(codebyte_ref),CODEBYTE_REF_NEXT(codebyte_ref));
  else CODEBYTELIST_FIRST(CFG_CODEBYTELIST(CODEBYTE_CFG(codebyte))) = CODEBYTE_REF_NEXT(codebyte_ref);
  if(CODEBYTE_REF_NEXT(codebyte_ref))
    CODEBYTE_REF_SET_PREV(CODEBYTE_REF_NEXT(codebyte_ref),CODEBYTE_REF_PREV(codebyte_ref));
  else CODEBYTELIST_LAST(CFG_CODEBYTELIST(CODEBYTE_CFG(codebyte))) = CODEBYTE_REF_PREV(codebyte_ref);
  Free(codebyte_ref);
}
/*}}}*/

void CodebyteFreeReferedRelocs (t_codebyte * codebyte)/*{{{*/
{
  t_reloc_ref *iter = CODEBYTE_REFERS_TO(codebyte);
  t_reloc_ref *iter_next;

  while (iter)
  {
    iter_next = RELOC_REF_NEXT(iter);
    if (RELOCATABLE_RELOCATABLE_TYPE(RELOC_FROM(RELOC_REF_RELOC(CODEBYTE_REFERS_TO(codebyte)))) == RT_CODEBYTE)
    {
      if(RELOC_SWITCH_EDGE(RELOC_REF_RELOC(CODEBYTE_REFERS_TO(codebyte))))
	CFG_EDGE_SET_REL(RELOC_SWITCH_EDGE(RELOC_REF_RELOC(CODEBYTE_REFERS_TO(codebyte))),NULL);
      RelocTableRemoveReloc (OBJECT_RELOC_TABLE(CFG_OBJECT(CODEBYTE_CFG(codebyte))), RELOC_REF_RELOC(CODEBYTE_REFERS_TO(codebyte)));
    }
    else
      FATAL(("Illegal relocation for instruction!"));
    iter = iter_next;
  }
}
/*}}}*/

t_codebyte * CodebyteNewForCfg(t_cfg * cfg)/*{{{*/
{
  t_codebyte_ref * codebyte_ref = Calloc(1,sizeof(t_codebyte_ref));
  t_codebyte * codebyte = CodebyteNew(cfg);
  t_codebytelist * codebytelist = CFG_CODEBYTELIST(cfg);

  CODEBYTE_SET_CODEBYTE_REF(codebyte,codebyte_ref);
  CODEBYTE_SET_RELOCATABLE_TYPE(codebyte, RT_CODEBYTE);
    
  if(codebytelist==0)
    FATAL(("Trouble"));
  
  if(CODEBYTELIST_FIRST(codebytelist) == NULL){
    //codebytelist = Calloc(1,sizeof(t_codebytelist));
    //CFG_SET_CODEBYTELIST(cfg,codebytelist);
    CODEBYTELIST_SET_FIRST(codebytelist,codebyte_ref);
    CODEBYTELIST_SET_COUNT(codebytelist,0);
  }
  else
    CODEBYTE_REF_SET_NEXT(CODEBYTELIST_LAST(codebytelist),codebyte_ref);
  
  CODEBYTE_REF_SET_CODEBYTE(codebyte_ref, codebyte);

  CODEBYTE_REF_SET_PREV(codebyte_ref, CODEBYTELIST_LAST(codebytelist));
  CODEBYTELIST_SET_LAST(codebytelist,codebyte_ref);
  CODEBYTELIST_SET_COUNT(codebytelist,CODEBYTELIST_COUNT(codebytelist)+1);

  CODEBYTE_SET_CFG(codebyte,cfg);

  return codebyte;
}
/* }}} */
