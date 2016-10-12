/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

#include <strings.h>
extern "C" {
#include <diabloobject.h>
#include <diabloarm.h>
#include <diabloflowgraph.h>
}

#include <diablosoftvm.h>
#include <jansson.h>

#include <algorithm>
#include <vector>

#include "diablosoftvm_vmchunk.h"

extern int frontend_id;

#define DEBUG_JSON_OUTPUT 0

typedef std::vector<t_bbl *> BblVector;
typedef std::vector<t_arm_ins *> InsVector;

json_t *Regset2Json(t_cfg *cfg, t_regset regs)
{
  json_t *ret = json_array();

  t_reg reg;

  REGSET_FOREACH_REG(regs, reg)
    json_array_append_new(ret, json_string(CFG_DESCRIPTION(cfg)->register_names[reg]));

  return ret;
}

json_t *Address2Json(t_address addr)
{
  t_string s = StringIo("@G", addr);
  json_t *ret = json_string(s);

  Free(s);

  return ret;
}

json_t *Instruction2Json(t_arm_ins *ins)
{
  t_string s = StringIo("@I", ins);
  json_t *ret = json_string(s);

  Free(s);

  return ret;
}

json_t *Ins2Json(t_cfg *cfg, t_arm_ins *ins, InsVector *ivect, InsVector *cvect, int ins_symbol_offset, int const_symbol_offset)
{
  json_t * ret = json_object();

  json_t *type;
  char *ins_assembled;
  char encoding_arr[9];

  /* type */
  switch (ARM_INS_OPCODE(ins))
  {
  case ARM_ADDRESS_PRODUCER:
    /* address producers */
  {
    type = json_string("address_producer");
    json_object_set_new(ret, "addrregister", json_string(CFG_DESCRIPTION(cfg)->register_names[ARM_INS_REGA(ins)]));

    /* look up the instruction in the ivect array */
    int looked_up = -1;
    for (auto i = 0U; i < ivect->size(); i++)
      if (ivect->at(i) == ins) {
        looked_up = i;
        break;
      }
    ASSERT(looked_up != -1, ("could not find address producer @I in array", ins));

    json_object_set_new(ret, "addrsymbol", json_integer(looked_up + ins_symbol_offset));
  }
    break;

  case ARM_VFPFLOAT_PRODUCER:
  case ARM_CONSTANT_PRODUCER:
  {
    /* constant producers */
    type = json_string("constant_producer");
	json_object_set_new(ret, "targetregister", json_string(CFG_DESCRIPTION(cfg)->register_names[ARM_INS_REGA(ins)]));
	
	int looked_up = -1;
	for (size_t i = 0; i < cvect->size(); i++)
	  if (cvect->at(i) == ins) {
		looked_up = i;
		break;
	  }
	ASSERT(looked_up != -1, ("could not find constant producer @I in array", ins));
	
	json_object_set_new(ret, "valuesymbol", json_integer(looked_up + const_symbol_offset));
  }
    break;

  default:
    /* regular instructions */
    type = json_string("normal");

    /* assemble the instruction to generate the encoded version */
    ins_assembled = (char*)Malloc(ARM_INS_CSIZE(ins) * sizeof(char));
    ArmAssembleOne(ins, ins_assembled);
    snprintf (encoding_arr, 9, "%08x", *((int*)ins_assembled));
    Free(ins_assembled);
    json_object_set_new(ret, "encoding", json_string(encoding_arr));
  }
  json_object_set_new(ret, "type", type);

  /* registers read/written */
  json_object_set_new(ret, "regswritten", Regset2Json(cfg, ARM_INS_REGS_DEF(ins)));
  json_object_set_new(ret, "regsread", Regset2Json(cfg, ARM_INS_REGS_USE(ins)));

#if DEBUG_JSON_OUTPUT
  json_object_set_new(ret, "diablo-instruction", Instruction2Json(ins));
#endif

  return ret;
}

json_t *Bbl2Json(t_cfg *cfg, t_bbl *bbl, InsVector *ivect, InsVector *cvect, int ins_symbol_offset, int const_symbol_offset)
{
  json_t *ret = json_object();

  json_t *ins_array;
  t_arm_ins *ins;
  t_function *func;
  t_address addr;

  /* instruction list */
  ins_array = json_array();
  BBL_FOREACH_ARM_INS(bbl, ins)
    if (!ArmIsControlflow(ins))
      json_array_append_new(ins_array, Ins2Json(cfg, ins, ivect, cvect, ins_symbol_offset, const_symbol_offset));
  json_object_set_new(ret, "instructions", ins_array);

  /* regs live out */
  json_object_set_new(ret, "regsliveout", Regset2Json(cfg, BBL_REGS_LIVE_OUT(bbl)));

  /* function name */
  func = BBL_FUNCTION(bbl);
  json_object_set_new(ret, "function name", json_string(FUNCTION_NAME(func)));

  /* function offset */
  json_object_set_new(ret, "function offset", Address2Json(BBL_OLD_ADDRESS(bbl)));

  return ret;
}

static int BblIndexInCfg(t_cfg *cfg, t_bbl *bbl)
{
  int ret = 0;
  t_bool found = FALSE;
  t_bbl *bbl_it;

  CFG_FOREACH_BBL(cfg, bbl_it)
  {
    /* skip HELL nodes */
    if (BBL_IS_HELL(bbl_it))
      continue;

    /* skip empty BBLs */
    if (BBL_NINS(bbl_it) == 0)
      continue;

    if (bbl_it == bbl)
    {
      found = TRUE;
      break;
    }
    else
      ret++;
  }

  return (found) ? ret : -1;
}

static int EdgeDestSymbolIndexInMap(t_edge_to_edge_map *e2a_map, BblVector *vect, t_cfg_edge *edge)
{
  int ret;
  t_edge_to_edge_map::iterator it, itt;

  /* first look up the edge in the map */
  for (it = e2a_map->begin(); it != e2a_map->end(); it++)
    if (it->first == edge)
      break;

  /* If this did not succeed, the edge is not present in the map.
   * Return early. */
  if (it == e2a_map->end())
    return -1;

  ret = -1;
  int idx = 0;
  for (auto i : *vect)
  {
    if (CFG_EDGE_TAIL(it->second) == i)
    {
      ret = idx;
      break;
    }

    idx++;
  }
  ASSERT(ret != -1, ("something went bananas"));

  return ret;
}

json_t *Edge2Json(t_vmchunk *chunk, t_cfg_edge *edge, t_edge_to_edge_map *total_exit_edge_map, BblVector *vect, int bbl_symbol_offset)
{
  json_t *ret= json_object();

  json_t *type;

  int idxHead = BblIndexInCfg(VMCHUNK_CFG(chunk), CFG_EDGE_HEAD(edge)) + bbl_symbol_offset;
  ASSERT(idxHead != -1, ("expected to find head of @E in chunk, but did not find it!", edge));

  /* the source BBL index is easy to look up */
  json_object_set_new(ret, "sourcebbl", json_integer(idxHead));

  /* If this edge goes to HELL, it is an exit edge.
   * If it doesn't it is an internal edge of the chunk. */
  if (BBL_IS_HELL(CFG_EDGE_TAIL(edge)))
  {
    /* yes, it is an exit edge */

    /* look up the index of the destination BBL symbol */
    int idxTail = EdgeDestSymbolIndexInMap(total_exit_edge_map, vect, edge) + bbl_symbol_offset;
    ASSERT(idxTail != -1, ("expected to find external edge @E in the exit edge map, but did not find it!", edge));

    json_object_set_new(ret, "destsymbol", json_integer(idxTail));

  #if DEBUG_JSON_OUTPUT
    json_object_set_new(ret, "destbblsymbol-addr", Address2Json(total_exit_edge_map->at(edge)));
  #endif
  }
  else
  {
    /* no, it is not an exit edge */

    /* look up the index of the destination BBL in the chunk CFG */
    int idxTail = BblIndexInCfg(VMCHUNK_CFG(chunk), CFG_EDGE_TAIL(edge)) + bbl_symbol_offset;
    ASSERT(idxTail != -1, ("expected to find tail of @E in chunk, but did not find it!", edge));

    json_object_set_new(ret, "destbbl", json_integer(idxTail));
  }

  /* type */
  switch (CFG_EDGE_CAT(edge))
  {
  case ET_JUMP:
  case ET_IPJUMP:
  {
    t_arm_ins *last_ins = T_ARM_INS(BBL_INS_LAST(CFG_EDGE_HEAD(edge)));
    std::string str("jump");

    if (ARM_INS_IS_CONDITIONAL(last_ins))
    {
      switch(ARM_INS_CONDITION(last_ins))
      {
      case ARM_CONDITION_EQ: str += "-eq"; break;
      case ARM_CONDITION_NE: str += "-ne"; break;
      case ARM_CONDITION_CS: str += "-cs"; break;
      case ARM_CONDITION_CC: str += "-cc"; break;
      case ARM_CONDITION_MI: str += "-mi"; break;
      case ARM_CONDITION_PL: str += "-pl"; break;
      case ARM_CONDITION_VS: str += "-vs"; break;
      case ARM_CONDITION_VC: str += "-vc"; break;
      case ARM_CONDITION_HI: str += "-hi"; break;
      case ARM_CONDITION_LS: str += "-ls"; break;
      case ARM_CONDITION_GE: str += "-ge"; break;
      case ARM_CONDITION_LT: str += "-lt"; break;
      case ARM_CONDITION_GT: str += "-gt"; break;
      case ARM_CONDITION_LE: str += "-le"; break;
      case ARM_CONDITION_AL: break;

      default:
        FATAL(("Unsupported condition code in @I", last_ins));
      }
    }

    type = json_string(str.c_str());
  }
    break;

  case ET_FALLTHROUGH:
    type = json_string("fallthrough");
    break;

  case ET_CALL:
    type = json_string("call");
    break;

  case ET_RETURN:
    type = json_string("return");
    break;

  case ET_SWITCH:
    type = json_string("switch");
    break;

  default:
    FATAL(("Unsupported edge type: @E", edge));
  }
  json_object_set_new(ret, "type", type);

  return ret;
}

json_t *Chunk2Json(t_cfg *cfg, t_vmchunk *chunk, t_edge_to_edge_map *total_exit_edge_map, BblVector *vect, InsVector *ivect, InsVector *cvect, int bbl_symbol_offset, int ins_symbol_offset, int const_symbol_offset)
{
  json_t *ret = json_object();

  /* mobility ID, if the chunk is made mobile */
  if (VMCHUNK_MOBILE_ID(chunk) != -1)
  {
    t_string mobile_id_str = StringIo("%d", VMCHUNK_MOBILE_ID(chunk));
    json_object_set_new(ret, "mobile_id", json_string(mobile_id_str));
    Free(mobile_id_str);
  }

  json_t *bbl_array, *edge_array;
  t_bbl *entry_bbl;
  t_bbl *bbl_it;

  /* BBLs/edges */
  bbl_array = json_array();
  edge_array = json_array();

  CFG_FOREACH_BBL(VMCHUNK_CFG(chunk), bbl_it)
  {
    t_cfg_edge *edge;

    /* skip HELL nodes */
    if (BBL_IS_HELL(bbl_it))
      continue;

    /* skip empty BBLs */
    if (BBL_NINS(bbl_it) == 0)
      continue;

    json_array_append_new(bbl_array, Bbl2Json(cfg, bbl_it, ivect, cvect, ins_symbol_offset, const_symbol_offset));

    BBL_FOREACH_SUCC_EDGE(bbl_it, edge)
      json_array_append_new(edge_array, Edge2Json(chunk, edge, total_exit_edge_map, vect, bbl_symbol_offset));
  }

  json_object_set_new(ret, "bbls", bbl_array);
  json_object_set_new(ret, "edges", edge_array);

  /* regs live in */
  entry_bbl = CFG_EDGE_TAIL(BBL_SUCC_FIRST(CFG_UNIQUE_ENTRY_NODE(VMCHUNK_CFG(chunk))));
  json_object_set_new(ret, "regslivein", Regset2Json(cfg, BblRegsLiveBefore(entry_bbl)));

  return ret;
}

json_t *Symbol2Json(t_string name, t_address addr, t_bool emit_address_information)
{
  json_t *ret = json_object();

  /* unique name for this symbol (e.g., bbl_XXX, addr_XXX or const_XXX) */
  json_object_set_new(ret, "name", json_string(name));

  /* in case of a constant producer, the value it should produce */
  /* in case of an address producer, the address it should produce */
  /* in case of a BBL, its address in the final layout */
  if (emit_address_information)
    json_object_set_new(ret, "address", Address2Json(addr));

  return ret;
}

json_t *ChunkArray2Json(t_cfg *cfg, t_ptr_array *chunks, t_bool emit_symbol_address_information, t_bool limit, int max)
{
  json_t *ret = json_object();

  json_t *chunk_arr = json_array();
  json_t *symbol_arr = json_array();

  InsVector *address_vector = new InsVector();
  address_vector->clear();
  
  InsVector *constant_vector = new InsVector();
  constant_vector->clear();

  /* first we collect all symbol information from every chunk and
   * and put it together in one big, fat datastructure. */
  t_edge_to_edge_map *total_exit_edge_map = new t_edge_to_edge_map();
  for (int i = 0; i < PtrArrayCount(chunks); i++)
  {
    t_vmchunk *chunk = reinterpret_cast<t_vmchunk*>(PtrArrayGet(chunks, i));

    if (frontend_id == 4)
    {
      if (!VMCHUNK_INTEGRATED(chunk))
        continue;
    }

    t_edge_to_edge_map *chunk_exit_edge_map = reinterpret_cast<t_edge_to_edge_map *>(VMCHUNK_EXIT_EDGE_MAP(chunk));

    /* append all elements of this chunk map to the big one */
    total_exit_edge_map->insert(chunk_exit_edge_map->begin(), chunk_exit_edge_map->end());

    /* also keep a list of address producers for the chunk */
    t_bbl *tmp;
    CFG_FOREACH_BBL(VMCHUNK_CFG(chunk), tmp)
    {
      t_arm_ins *ins;
      BBL_FOREACH_ARM_INS(tmp, ins)
      {
        if (ARM_INS_OPCODE(ins) == ARM_ADDRESS_PRODUCER)
		  address_vector->push_back(ins);
		else if (ARM_INS_OPCODE(ins) == ARM_CONSTANT_PRODUCER)
		  constant_vector->push_back(ins);
      }
    }
  }

  BblVector *bbl_vector = new BblVector();
  bbl_vector->clear();
  for (t_edge_to_edge_map::iterator it = total_exit_edge_map->begin(); it != total_exit_edge_map->end(); it++)
  {
    t_bbl *tail = CFG_EDGE_TAIL(it->second);
    if (std::find(bbl_vector->begin(), bbl_vector->end(), tail) == bbl_vector->end())
      bbl_vector->push_back(tail);
  }

  int i = 0;
  int symbol_array_index = 0;
  int symbol_array_bbl_offset = 0;
  int symbol_array_address_offset = 0;
  int symbol_array_constant_offset = 0;

  /* construct part 1 of the symbol array, containing the different chunk exit points */
  symbol_array_bbl_offset = symbol_array_index;
  i = 0;
  for (auto bbl : *bbl_vector)
  {
    t_string bbl_name = StringIo("bbl_%d_@G", i, BBL_OLD_ADDRESS(bbl));
    t_object *obj = CFG_OBJECT(cfg);

    /* add every destination BBL to the symbol array, taking care of the fact that
     * the base address of the binary has to be subtracted from the BBL address to create PIC code. */
    json_array_append_new(symbol_arr, Symbol2Json(bbl_name, /*AddressSubUint32(BBL_CADDRESS(bbl), OBJECT_OBJECT_HANDLER(obj)->linkbaseaddress(obj, OBJECT_LAYOUT_SCRIPT(obj)))*/BBL_CADDRESS(bbl), emit_symbol_address_information));

    Free(bbl_name);
    i++;
    symbol_array_index++;
  }

  /* construct part 2 of the symbol array, containing the address producer targets */
  symbol_array_address_offset = symbol_array_index;
  i = 0;
  for (auto addrprod : *address_vector) {
    t_string address_name = StringIo("address_%d_@G", i, ARM_INS_OLD_ADDRESS(addrprod));

    json_array_append_new(symbol_arr, Symbol2Json(address_name, ARM_INS_IMMEDIATE(addrprod), emit_symbol_address_information));

    Free(address_name);
    i++;
    symbol_array_index++;
  }
  
  /* construct part 3 of the symbol array, containing the constant producer targets */
  symbol_array_constant_offset = symbol_array_index;
  i = 0;
  for (auto constprod : *constant_vector) {
	t_string constant_name = StringIo("constant_%d_@G", i, ARM_INS_OLD_ADDRESS(constprod));
	
	json_array_append_new(symbol_arr, Symbol2Json(constant_name, ARM_INS_IMMEDIATE(constprod), emit_symbol_address_information));
	
	Free(constant_name);
	i++;
	symbol_array_index++;
  }

  /* global fields */
  json_object_set_new(ret, "appid", json_string(aspire_options.actc_id));

  t_string seed_str = StringIo("0x%x", diablosoftvm_options.diversity_seed);
  json_object_set_new(ret, "seed", json_string(seed_str));
  Free(seed_str);

  /* now iterate over every chunk and collect the chunk information */
  for (int i = 0; i < PtrArrayCount(chunks); i++)
  {
    if (limit && i == max) break;

    if (frontend_id == 4)
    {
      t_vmchunk *chunk = reinterpret_cast<t_vmchunk*>(PtrArrayGet(chunks, i));
      if (!VMCHUNK_INTEGRATED(chunk))
        continue;
    }

    json_array_append_new(chunk_arr, Chunk2Json(cfg, reinterpret_cast<t_vmchunk*>(PtrArrayGet(chunks, i)), total_exit_edge_map, bbl_vector, address_vector, constant_vector, symbol_array_bbl_offset, symbol_array_address_offset, symbol_array_constant_offset));
  }

  /* TODO: add the constant producers to the symbol array */

  /* convert our data to the JSON format */
  json_object_set_new(ret, "chunks", chunk_arr);
  json_object_set_new(ret, "symbols", symbol_arr);

  delete total_exit_edge_map;
  delete bbl_vector;
  delete address_vector;
  delete constant_vector;

  return ret;
}
