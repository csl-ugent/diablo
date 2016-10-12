/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

#include <diablosupport_class.h>

#ifndef CLASS
#define CLASS vmchunk
#define vmchunk_field_select_prefix VMCHUNK
#define vmchunk_function_prefix VmChunk
#endif

DIABLO_CLASS_BEGIN

    /* Members */
    MEMBER(t_ptr_array* , bbls    , BBLS)
    MEMBER(t_ptr_array* , exits   , EXITS)
    MEMBER(t_cfg_edge*  , entry   , ENTRY)
    MEMBER(void *, exit_edge_map, EXIT_EDGE_MAP)
    MEMBER(t_cfg *, cfg, CFG)
    MEMBER(void *, reloc_map, RELOC_MAP)
    MEMBER(t_bool, ip, IP)
    MEMBER(t_bool, integrated, INTEGRATED)
    MEMBER(int, mobile_id, MOBILE_ID)

    /* Functions */
    FUNCTION1(void , Refresh , t_CLASS *)
    FUNCTION2(void , AddBbl  , t_CLASS *  , t_bbl*)
    FUNCTION2(int  , HasBbl  , t_CLASS *  , t_bbl*)

DIABLO_CLASS_END
