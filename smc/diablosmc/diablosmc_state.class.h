#include <diablosupport_class.h>

#ifndef CLASS
#define CLASS state
#define state_field_select_prefix STATE
#define state_function_prefix State
#endif

/*! \brief A state. 
 *
 * A state represents a particular state a memState can be in. 
 * A state is always within a linked list of its codebyte.
 * If this state corresponds to a byte in an instruction (parent_ins), it points to it and the offset(parent_offset) is set
 * When KNOWN, VALUE is set to the correct value (it might not be known because it is relocatable)*/
DIABLO_CLASS_BEGIN
MEMBER(t_bool,known,KNOWN)
MEMBER(t_uint8,value,VALUE)
MEMBER(t_codebyte *,codebyte,CODEBYTE)
MEMBER(t_ins *,parent_ins,PARENT_INS)
MEMBER(t_uint8,parent_offset,PARENT_OFFSET)
  
PFUNCTION0(t_CLASS *, New)
PFUNCTION1(t_CLASS *, NewForCodebyte, t_codebyte *)
PFUNCTION2(t_uint8, AddToIns, t_state *, t_ins *)
PFUNCTION2(void, RemoveFromStatelist, t_state *, t_statelist *)
PFUNCTION2(void, AddToStatelist, t_state *, t_statelist *)
PFUNCTION3(t_CLASS *, NewForCodebyteIns, t_codebyte *, t_ins *, t_uint8)
DIABLO_CLASS_END
