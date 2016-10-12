#ifndef DIABLOI386_TABLE_TYPEDEFS
#define DIABLOI386_TABLE_TYPEDEFS
typedef struct _t_i386_opcode_entry t_i386_opcode_entry;
typedef struct _t_i386_opcode_he t_i386_opcode_he;
#endif

#include <diabloi386.h>
#ifdef DIABLOI386_TYPES
#ifndef DIABLOI386_TABLE_TYPES
#define DIABLOI386_TABLE_TYPES

typedef t_uint32 (*I386OpDisFunc) (t_i386_ins * ins, t_uint8 * codep, t_uint32 modrm, t_uint32 sib, t_i386_operand * op, t_i386_bytemode bm);
typedef void (*I386OpAsFunc) (t_i386_operand * op, t_i386_bytemode bm, t_uint32 * mod_ret, t_uint32 * reg_ret, t_uint32 * rm_ret, t_uint32 * scale_ret, t_uint32 * index_ret, t_uint32 * base_ret, t_bool * has_sib_ret, t_uint32 *immediate_ret, t_uint32 * immedsz_ret, t_uint32 * displacement_ret, t_uint32 * dispsz_ret, t_uint32 * segsel_ret, t_bool * has_segsel_ret);
typedef t_bool (*I386OpCheckFunc) (t_i386_operand * op, t_i386_bytemode bm);
typedef t_bool (*I386OpNextFunc) (t_i386_ins * ins, t_i386_operand * op, t_i386_bytemode bm, t_immediates * imm);

struct _t_i386_opcode_entry {
  t_i386_opcode opcode;

  I386OpDisFunc op1dis;
  I386OpAsFunc op1as;
  I386OpCheckFunc op1check;
  I386OpNextFunc op1next;
  t_i386_bytemode op1bm;
  
  I386OpDisFunc op2dis;
  I386OpAsFunc op2as;
  I386OpCheckFunc op2check;
  I386OpNextFunc op2next;
  t_i386_bytemode op2bm;
  
  I386OpDisFunc op3dis;
  I386OpAsFunc op3as;
  I386OpCheckFunc op3check;
  I386OpNextFunc op3next;
  t_i386_bytemode op3bm;

  t_bool has_modrm;
  t_uint32 usedefpattern;
};

struct _t_i386_opcode_he {
  t_hash_table_node node;
  t_i386_opcode_entry * entry;
};

extern t_i386_opcode_entry dis386[];
extern t_i386_opcode_entry dis386_twobyte[];
extern t_i386_opcode_entry dis386_3dnow[];
extern t_i386_opcode_entry dis386_grps[][8];
extern t_i386_opcode_entry dis386_fpu_mem[][8];
extern t_i386_opcode_entry dis386_fpu_reg[8][8][8];

/* usedefpattern flags */
/* destination operand used as source */
#define DU	(1 << 0)
/* first source operand defined */
#define S1D	(1 << 1)
/* second source operand defined */
#define S2D	(1 << 2)

extern t_hash_table * i386_opcode_hash;
#endif
#endif

#ifdef DIABLOI386_FUNCTIONS
#ifndef DIABLOI386_TABLE_FUNCTIONS
#define DIABLOI386_TABLE_FUNCTIONS
void I386CreateOpcodeHashTable(void);
void I386DestroyOpcodeHashTable(void);
#endif
#endif
