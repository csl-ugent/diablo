/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

/* The development of portions of the code contained in this file was sponsored by Samsung Electronics UK. */

#ifndef DIABLO_CODE_LAYOUT_TYPEDEFS
#define DIABLO_CODE_LAYOUT_TYPEDEFS
typedef struct _t_chain_holder t_chain_holder;
#endif

#ifdef DIABLOFLOWGRAPH_TYPES
#ifndef DIABLO_CODE_LAYOUT_TYPES
#define DIABLO_CODE_LAYOUT_TYPES
struct _t_chain_holder
{
  t_bbl **chains;
  t_uint32 nchains;
};
#endif
#endif

#ifdef DIABLOFLOWGRAPH_FUNCTIONS
#ifndef DIABLO_CODE_LAYOUT_FUNCTIONS
#define DIABLO_CODE_LAYOUT_FUNCTIONS
extern t_address AssignAddressesInChain (t_bbl * bbl, t_address start);
extern void CreateChains (t_cfg * cfg, t_chain_holder * ch);
extern void DefaultOrderChains (t_chain_holder * ch);
extern void DefaultGiveAddressesToChain (t_address start_address, t_chain_holder * ch);
void MergeAllChains (t_chain_holder * ch);
void MergeChains (t_bbl * ca, t_bbl * cb);

t_bbl * GetHeadOfChain(t_bbl * chain);
t_bbl * GetTailOfChain(t_bbl * chain);
t_bool AlreadyInChain(t_bbl * head, t_bbl * test);
void DetectLoopsInChains(t_cfg * cfg);
void AppendBblToChain(t_bbl * chain, t_bbl * bbl);
#endif
#endif
/* vim: set shiftwidth=2 expandtab cinoptions=p5,t0,(0, foldmethod=marker tw=80 cindent: */
