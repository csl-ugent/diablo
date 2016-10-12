#ifndef DIABLOAMD64_TABLE_TYPEDEFS
#define DIABLOAMD64_TABLE_TYPEDEFS
typedef struct _t_amd64_opcode_entry t_amd64_opcode_entry;
typedef struct _t_amd64_opcode_he t_amd64_opcode_he;
#endif

#include <diabloamd64.h>
#ifdef DIABLOAMD64_TYPES
#ifndef DIABLOAMD64_TABLE_TYPES
#define DIABLOAMD64_TABLE_TYPES

typedef t_uint32 (*Amd64OpDisFunc) (t_amd64_ins * ins, t_uint8 * codep, t_uint32 modrm, t_uint32 sib, t_amd64_operand * op, t_amd64_bytemode bm,char rex);
typedef void (*Amd64OpAsFunc) (t_amd64_ins * ins, t_amd64_operand * op, t_amd64_bytemode bm, t_uint32 * mod_ret, t_uint32 * reg_ret, t_uint32 * rm_ret, t_uint32 * scale_ret, t_uint32 * index_ret, t_uint32 * base_ret, t_bool * has_sib_ret, t_uint64 *immediate_ret, t_uint32 * immedsz_ret, t_uint64 * displacement_ret, t_uint32 * dispsz_ret, t_uint32 * segsel_ret, t_bool * has_segsel_ret,char *rex);
typedef t_bool (*Amd64OpCheckFunc) (t_amd64_operand * op, t_amd64_bytemode bm);
typedef t_bool (*Amd64OpNextFunc) (t_amd64_ins * ins, t_amd64_operand * op, t_amd64_bytemode bm, t_amd64_immediates * imm);

struct _t_amd64_opcode_entry {
  t_amd64_opcode opcode;

  Amd64OpDisFunc op1dis;
  Amd64OpAsFunc op1as;
  Amd64OpCheckFunc op1check;
  Amd64OpNextFunc op1next;
  t_amd64_bytemode op1bm;
  
  Amd64OpDisFunc op2dis;
  Amd64OpAsFunc op2as;
  Amd64OpCheckFunc op2check;
  Amd64OpNextFunc op2next;
  t_amd64_bytemode op2bm;
  
  Amd64OpDisFunc op3dis;
  Amd64OpAsFunc op3as;
  Amd64OpCheckFunc op3check;
  Amd64OpNextFunc op3next;
  t_amd64_bytemode op3bm;

  t_bool has_modrm;
  t_uint32 usedefpattern;
};

struct _t_amd64_opcode_he {
  t_hash_table_node node;
  t_amd64_opcode_entry * entry;
};

extern t_amd64_opcode_entry nopinstr;
extern t_amd64_opcode_entry disamd64[];
extern t_amd64_opcode_entry disamd64_twobyte[];
extern t_amd64_opcode_entry disamd64_3dnow[];
extern t_amd64_opcode_entry disamd64_grps[][8];
extern t_amd64_opcode_entry disamd64_fpu_mem[][8];
extern t_amd64_opcode_entry disamd64_fpu_reg[8][8][8];

/* usedefpattern flags */
/* destination operand used as source */
#define DU	(1 << 0)
/* first source operand defined */
#define S1D	(1 << 1)
/* second source operand defined */
#define S2D	(1 << 2)

extern t_hash_table * amd64_opcode_hash;
#endif
#endif

#ifdef DIABLOAMD64_FUNCTIONS
#ifndef DIABLOAMD64_TABLE_FUNCTIONS
#define DIABLOAMD64_TABLE_FUNCTIONS
void Amd64CreateOpcodeHashTable(void);
#endif
#endif
