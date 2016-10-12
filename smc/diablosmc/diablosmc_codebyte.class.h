#include <diablosupport_class.h>

#ifndef CODEBYTE
#define CLASS codebyte
#define codebyte_field_select_prefix CODEBYTE
#define codebyte_function_prefix Codebyte
#define MANAGER_TYPE t_cfg * 
#define MANAGER_NAME cfg
#define MANAGER_FIELD codebyte_manager
#endif

/*! \brief A codebyte. 
 *
 * A codebyte represents a byte in the program.
 * A codebyte can be in a number of states. 
 * codebyte_ref refers to the codebyte_ref from the cfg
 * */
DIABLO_CLASS_BEGIN
EXTENDS(t_relocatable)
MEMBER(t_statelist *,statelist,STATELIST)
MEMBER(t_cfg *,cfg,CFG)
MEMBER(t_codebyte_ref *, codebyte_ref, CODEBYTE_REF)
     /*TODO remove next 2 members temporary hacks due to lack of time to do it clean*/
MEMBER(t_uint32, overlap, OVERLAP)
MEMBER(t_bool, screwed, SCREWED)
CONSTRUCTOR({ ; })

DUPLICATOR({  })

DESTRUCTOR({ CodebyteFreeReferedRelocs(to_free); })

FUNCTION1(void, FreeReferedRelocs, t_CLASS *)
FUNCTION1(void, UnlinkFromCfg, t_CLASS *)
FUNCTION1(t_CLASS *, NewForCfg, t_cfg *)

DIABLO_CLASS_END

#undef BASECLASS
#define BASECLASS relocatable
#include  <diabloobject_relocatable.class.h>
#undef BASECLASS
/* vim: set shiftwidth=4 cinoptions={.5s,g0,p5,t0,(0,^-0.5s,n-2,+0.5s foldmethod=marker tw=78 cindent: */
