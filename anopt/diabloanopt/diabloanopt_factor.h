/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

#define T_BBL_HE(x) ((t_bbl_he*)(x))
#define T_FINGER(x) ((t_finger*)(x)) 

#ifndef DIABLO_FACTOR_TYPEDEFS
#define DIABLO_FACTOR_TYPEDEFS
typedef struct _t_finger t_finger;			/* to easily adapt the sort of fingerprint (int, struct,...)*/
#endif


#ifndef DIABLO_FACTOR_H
#define DIABLO_FACTOR_H
struct _t_finger
{
  t_uint32 nins;
  t_uint8* opc;						/*pointer to array of opcodes (in 8 bits)*/
};




void Factor (t_cfg *cfg);
t_hash_table * BblPart(t_cfg * cfg);
void * BblFinger(t_bbl *bbl);
t_uint32 BblHash(t_hash_table * table, void * bblhe);
t_int32 BblFingComp(void* bblhe, void* bblhe2);
void BblHeFree (void* bblhe,void* data);
void BblHePrint(void* bblhe,void* data);


/* structure that holds all equivalent blocks of a bbl */
typedef struct {
  int nbbls;
  t_bbl ** bbl;
} t_equiv_bbl_holder;

void BblFactoring(t_cfg *cfg, t_randomnumbergenerator *rng);
t_bool MoveDirectCallEdgesFromTo(t_function * from, t_function * to, t_bool use_precomputed_symbol_info);
void WholeFunctionFactoring(t_cfg * cfg);
void FunctionEpilogueFactoring(t_cfg *cfg);
t_bool FindCorrespondingBlocks (t_function *funa, t_function *funb);
t_bool CompareTwoFunctions(t_function *funa, t_function *funb, t_bool transfer_execution_counts);
t_bool CompareTwoBlocks(t_bbl *blocka, t_bbl *blockb);

typedef t_bool (*t_BblFactor)(t_equiv_bbl_holder *,t_bbl *);
CFG_DYNAMIC_MEMBER_GLOBAL_BODY(bbl_factor, BBL_FACTOR, BblFactor, t_BblFactor, {*valp=NULL;}, {}, {});

typedef t_uint32 (*t_BblFingerprint)(t_bbl *);
CFG_DYNAMIC_MEMBER_GLOBAL_BODY(bbl_fingerprint, BBL_FINGERPRINT, BblFingerprint, t_BblFingerprint, {*valp=NULL;}, {}, {});

typedef t_bool (*t_BblCanBeFactored)(t_bbl *);
CFG_DYNAMIC_MEMBER_GLOBAL_BODY(bbl_can_be_factored, BBL_CAN_BE_FACTORED, BblCanBeFactored, t_BblCanBeFactored, {*valp=NULL;}, {}, {});

t_bool BblFactorInit(t_cfg *cfg);
void BblFactorFini(t_cfg * cfg);

#endif
/* vim: set shiftwidth=2 foldmethod=marker: */
