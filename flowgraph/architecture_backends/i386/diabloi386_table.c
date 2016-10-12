/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

#include <diabloi386.h>

#define NULL_OP		I386OpDisNone, I386OpAsNone, I386OpCheckNone, I386OpNextNone, 0

#define AH		I386OpDisReg, I386OpAsReg, I386OpCheckReg, I386OpNextReg, bm_AH
#define BH		I386OpDisReg, I386OpAsReg, I386OpCheckReg, I386OpNextReg, bm_BH
#define CH		I386OpDisReg, I386OpAsReg, I386OpCheckReg, I386OpNextReg, bm_CH
#define DH		I386OpDisReg, I386OpAsReg, I386OpCheckReg, I386OpNextReg, bm_DH
#define AL		I386OpDisReg, I386OpAsReg, I386OpCheckReg, I386OpNextReg, bm_AL
#define BL		I386OpDisReg, I386OpAsReg, I386OpCheckReg, I386OpNextReg, bm_BL
#define CL		I386OpDisReg, I386OpAsReg, I386OpCheckReg, I386OpNextReg, bm_CL
#define DL		I386OpDisReg, I386OpAsReg, I386OpCheckReg, I386OpNextReg, bm_DL
#define AX		I386OpDisReg, I386OpAsReg, I386OpCheckReg, I386OpNextReg, bm_AX
#define BX		I386OpDisReg, I386OpAsReg, I386OpCheckReg, I386OpNextReg, bm_BX
#define CX		I386OpDisReg, I386OpAsReg, I386OpCheckReg, I386OpNextReg, bm_CX
#define DX		I386OpDisReg, I386OpAsReg, I386OpCheckReg, I386OpNextReg, bm_DX
#define EAX		I386OpDisReg, I386OpAsReg, I386OpCheckReg, I386OpNextReg, bm_EAX
#define EBX		I386OpDisReg, I386OpAsReg, I386OpCheckReg, I386OpNextReg, bm_EBX
#define ECX		I386OpDisReg, I386OpAsReg, I386OpCheckReg, I386OpNextReg, bm_ECX
#define EDX		I386OpDisReg, I386OpAsReg, I386OpCheckReg, I386OpNextReg, bm_EDX
#define ESI		I386OpDisReg, I386OpAsReg, I386OpCheckReg, I386OpNextReg, bm_ESI
#define EDI		I386OpDisReg, I386OpAsReg, I386OpCheckReg, I386OpNextReg, bm_EDI
#define ESP		I386OpDisReg, I386OpAsReg, I386OpCheckReg, I386OpNextReg, bm_ESP
#define EBP		I386OpDisReg, I386OpAsReg, I386OpCheckReg, I386OpNextReg, bm_EBP
#define eAX		I386OpDisReg, I386OpAsReg, I386OpCheckReg, I386OpNextReg, bm_eAX
#define eBX		I386OpDisReg, I386OpAsReg, I386OpCheckReg, I386OpNextReg, bm_eBX
#define eCX		I386OpDisReg, I386OpAsReg, I386OpCheckReg, I386OpNextReg, bm_eCX
#define eDX		I386OpDisReg, I386OpAsReg, I386OpCheckReg, I386OpNextReg, bm_eDX
#define eSI		I386OpDisReg, I386OpAsReg, I386OpCheckReg, I386OpNextReg, bm_eSI
#define eDI		I386OpDisReg, I386OpAsReg, I386OpCheckReg, I386OpNextReg, bm_eDI
#define eSP		I386OpDisReg, I386OpAsReg, I386OpCheckReg, I386OpNextReg, bm_eSP
#define eBP		I386OpDisReg, I386OpAsReg, I386OpCheckReg, I386OpNextReg, bm_eBP
#define CS		I386OpDisReg, I386OpAsReg, I386OpCheckReg, I386OpNextReg, bm_CS
#define DS		I386OpDisReg, I386OpAsReg, I386OpCheckReg, I386OpNextReg, bm_DS
#define ES		I386OpDisReg, I386OpAsReg, I386OpCheckReg, I386OpNextReg, bm_ES
#define FS		I386OpDisReg, I386OpAsReg, I386OpCheckReg, I386OpNextReg, bm_FS
#define GS		I386OpDisReg, I386OpAsReg, I386OpCheckReg, I386OpNextReg, bm_GS
#define SS		I386OpDisReg, I386OpAsReg, I386OpCheckReg, I386OpNextReg, bm_SS

#define ST		I386OpDisST , I386OpAsST , I386OpCheckST , I386OpNextST, bm_ST
#define ST0		I386OpDisST , I386OpAsST , I386OpCheckST , I386OpNextST, bm_ST0
#define ST1		I386OpDisST , I386OpAsST , I386OpCheckST , I386OpNextST, bm_ST1
#define ST2		I386OpDisST , I386OpAsST , I386OpCheckST , I386OpNextST, bm_ST2
#define ST3		I386OpDisST , I386OpAsST , I386OpCheckST , I386OpNextST, bm_ST3
#define ST4		I386OpDisST , I386OpAsST , I386OpCheckST , I386OpNextST, bm_ST4
#define ST5		I386OpDisST , I386OpAsST , I386OpCheckST , I386OpNextST, bm_ST5
#define ST6		I386OpDisST , I386OpAsST , I386OpCheckST , I386OpNextST, bm_ST6
#define ST7		I386OpDisST , I386OpAsST , I386OpCheckST , I386OpNextST, bm_ST7

#define C_1		I386OpDisConst1, I386OpAsConst1, I386OpCheckConst1, I386OpNextConst1, 0

#define Ap		I386OpDisA, I386OpAsA, I386OpCheckA, I386OpNextA, bm_p
#define Cd		I386OpDisC, I386OpAsC, I386OpCheckC, I386OpNextC, bm_d
#define Dd		I386OpDisD, I386OpAsD, I386OpCheckD, I386OpNextD, bm_d

#define Eb		I386OpDisE, I386OpAsE, I386OpCheckE, I386OpNextE, bm_b
#define Ew		I386OpDisE, I386OpAsE, I386OpCheckE, I386OpNextE, bm_w
#define Ed		I386OpDisE, I386OpAsE, I386OpCheckE, I386OpNextE, bm_d
#define Ev		I386OpDisE, I386OpAsE, I386OpCheckE, I386OpNextE, bm_v
#define Ep		I386OpDisE, I386OpAsE, I386OpCheckE, I386OpNextE, bm_p


#define Fv		I386OpDisF, I386OpAsF, I386OpCheckF, I386OpNextF, bm_v

#define Gb		I386OpDisG, I386OpAsG, I386OpCheckG, I386OpNextG, bm_b
#define Gw		I386OpDisG, I386OpAsG, I386OpCheckG, I386OpNextG, bm_w
#define Gd		I386OpDisG, I386OpAsG, I386OpCheckG, I386OpNextG, bm_d
#define Gv		I386OpDisG, I386OpAsG, I386OpCheckG, I386OpNextG, bm_v

#define sIb		I386OpDissI, I386OpAssI, I386OpChecksI, I386OpNextsI, bm_b  /* the double 's' is _not_ a typo */
#define Ib		I386OpDisI, I386OpAsI, I386OpCheckI, I386OpNextI, bm_b
#define Iw		I386OpDisI, I386OpAsI, I386OpCheckI, I386OpNextI, bm_w
#define Iv		I386OpDisI, I386OpAsI, I386OpCheckI, I386OpNextI, bm_v
#define sIv		I386OpDissI, I386OpAssI, I386OpChecksI, I386OpNextsI, bm_v  /* the double 's' is _not_ a typo */

#define Jb		I386OpDisJ, I386OpAsJ, I386OpCheckJ, I386OpNextJ, bm_b
#define Jv		I386OpDisJ, I386OpAsJ, I386OpCheckJ, I386OpNextJ, bm_v

#define M		I386OpDisM, I386OpAsM, I386OpCheckM, I386OpNextM, 0
#define Ma		I386OpDisM, I386OpAsM, I386OpCheckM, I386OpNextM, bm_a
#define Mb		I386OpDisM, I386OpAsM, I386OpCheckM, I386OpNextM, bm_b
#define Mp		I386OpDisM, I386OpAsM, I386OpCheckM, I386OpNextM, bm_p
#define Mdq		I386OpDisM, I386OpAsM, I386OpCheckM, I386OpNextM, bm_dq
#define Ms		I386OpDisM, I386OpAsM, I386OpCheckM, I386OpNextM, bm_s
/* these are defined only for fpu memory operations */
/* single-precision real */
#define Msr		I386OpDisM, I386OpAsM, I386OpCheckM, I386OpNextM, bm_sr
/* double-precision real */
#define Mdr		I386OpDisM, I386OpAsM, I386OpCheckM, I386OpNextM, bm_dr
/* extended double-precision real (80 bits) */
#define Mer		I386OpDisM, I386OpAsM, I386OpCheckM, I386OpNextM, bm_er
/* packed BCD integer (8 bytes) */
#define Mbcd		I386OpDisM, I386OpAsM, I386OpCheckM, I386OpNextM, bm_bcd
/* word integer */
#define Mw		I386OpDisM, I386OpAsM, I386OpCheckM, I386OpNextM, bm_w
/* dword integer */
#define Md		I386OpDisM, I386OpAsM, I386OpCheckM, I386OpNextM, bm_d
/* qword integer */
#define Mq		I386OpDisM, I386OpAsM, I386OpCheckM, I386OpNextM, bm_q
/* generic two-byte */
#define M2		I386OpDisM, I386OpAsM, I386OpCheckM, I386OpNextM, bm_2byte
/* generic 28 byte */
#define M28		I386OpDisM, I386OpAsM, I386OpCheckM, I386OpNextM, bm_28byte
/* generic 108 byte */
#define M108		I386OpDisM, I386OpAsM, I386OpCheckM, I386OpNextM, bm_108byte
/* generic 512 byte */
#define M512		I386OpDisM, I386OpAsM, I386OpCheckM, I386OpNextM, bm_512byte



#define Ob		I386OpDisO, I386OpAsO, I386OpCheckO, I386OpNextO, bm_b
#define Ov		I386OpDisO, I386OpAsO, I386OpCheckO, I386OpNextO, bm_v

#define Rd		I386OpDisR, I386OpAsR, I386OpCheckR, I386OpNextR, bm_d

#define Sw		I386OpDisS, I386OpAsS, I386OpCheckS, I386OpNextS, bm_w

#define Xb		I386OpDisX, I386OpAsX, I386OpCheckX, I386OpNextX, bm_b
#define Xv		I386OpDisX, I386OpAsX, I386OpCheckX, I386OpNextX, bm_v

#define Yb		I386OpDisY, I386OpAsY, I386OpCheckY, I386OpNextY, bm_b
#define Yv		I386OpDisY, I386OpAsY, I386OpCheckY, I386OpNextY, bm_v

#define Vsd             I386OpDisV, I386OpAsV, I386OpCheckV, I386OpNextV, bm_sd
#define Vss             I386OpDisV, I386OpAsV, I386OpCheckV, I386OpNextV, bm_ss
#define Vps             I386OpDisV, I386OpAsV, I386OpCheckV, I386OpNextV, bm_ps
#define Vpd             I386OpDisV, I386OpAsV, I386OpCheckV, I386OpNextV, bm_pd
#define Vdq             I386OpDisV, I386OpAsV, I386OpCheckV, I386OpNextV, bm_dq
#define Vd              I386OpDisV, I386OpAsV, I386OpCheckV, I386OpNextV, bm_d
#define Vq              I386OpDisV, I386OpAsV, I386OpCheckV, I386OpNextV, bm_q

#define Wsd             I386OpDisW, I386OpAsW, I386OpCheckW, I386OpNextW, bm_sd
#define Wss             I386OpDisW, I386OpAsW, I386OpCheckW, I386OpNextW, bm_ss
#define Wps             I386OpDisW, I386OpAsW, I386OpCheckW, I386OpNextW, bm_ps
#define Wpd             I386OpDisW, I386OpAsW, I386OpCheckW, I386OpNextW, bm_pd
#define Wdq             I386OpDisW, I386OpAsW, I386OpCheckW, I386OpNextW, bm_dq
#define Wq              I386OpDisW, I386OpAsW, I386OpCheckW, I386OpNextW, bm_q

#define Psd             I386OpDisV, I386OpAsV, I386OpCheckV, I386OpNextV, bm_sd
#define Pss             I386OpDisV, I386OpAsV, I386OpCheckV, I386OpNextV, bm_ss
#define Pps             I386OpDisV, I386OpAsV, I386OpCheckV, I386OpNextV, bm_ps
#define Ppd             I386OpDisV, I386OpAsV, I386OpCheckV, I386OpNextV, bm_pd
#define Pq              I386OpDisV, I386OpAsV, I386OpCheckV, I386OpNextV, bm_q
#define Pdq             I386OpDisV, I386OpAsV, I386OpCheckV, I386OpNextV, bm_dq
#define Pd              I386OpDisV, I386OpAsV, I386OpCheckV, I386OpNextV, bm_d

#define Qsd             I386OpDisE, I386OpAsE, I386OpCheckE, I386OpNextE, bm_sd
#define Qss             I386OpDisE, I386OpAsE, I386OpCheckE, I386OpNextE, bm_ss
#define Qps             I386OpDisE, I386OpAsE, I386OpCheckE, I386OpNextE, bm_ps
#define Qpd             I386OpDisE, I386OpAsE, I386OpCheckE, I386OpNextE, bm_pd
#define Qq              I386OpDisE, I386OpAsE, I386OpCheckE, I386OpNextE, bm_q
#define Qdq             I386OpDisE, I386OpAsE, I386OpCheckE, I386OpNextE, bm_dq





t_i386_opcode_entry dis386[] = 
{
  /* 00 */
  {I386_ADD       , Eb     , Gb     , NULL_OP, TRUE  , DU         },
  {I386_ADD       , Ev     , Gv     , NULL_OP, TRUE  , DU         },
  {I386_ADD       , Gb     , Eb     , NULL_OP, TRUE  , DU         },
  {I386_ADD       , Gv     , Ev     , NULL_OP, TRUE  , DU         },
  {I386_ADD       , AL     , sIb    , NULL_OP, FALSE , DU         },
  {I386_ADD       , eAX    , Iv     , NULL_OP, FALSE , DU         },
  {I386_PUSH      , NULL_OP, ES     , NULL_OP, FALSE , 0          },
  {I386_POP       , ES     , NULL_OP, NULL_OP, FALSE , 0          },
  /* 08 */                                   
  {I386_OR        , Eb     , Gb     , NULL_OP, TRUE  , DU         },
  {I386_OR        , Ev     , Gv     , NULL_OP, TRUE  , DU         },
  {I386_OR        , Gb     , Eb     , NULL_OP, TRUE  , DU         },
  {I386_OR        , Gv     , Ev     , NULL_OP, TRUE  , DU         },
  {I386_OR        , AL     , Ib     , NULL_OP, FALSE , DU         },
  {I386_OR        , eAX    , Iv     , NULL_OP, FALSE , DU         },
  {I386_PUSH      , NULL_OP, CS     , NULL_OP, FALSE , 0          },
  {TWOBYTE_ESC    , NULL_OP, NULL_OP, NULL_OP, FALSE , 0          },
  /* 10 */                                   
  {I386_ADC       , Eb     , Gb     , NULL_OP, TRUE  , DU         },
  {I386_ADC       , Ev     , Gv     , NULL_OP, TRUE  , DU         },
  {I386_ADC       , Gb     , Eb     , NULL_OP, TRUE  , DU         },
  {I386_ADC       , Gv     , Ev     , NULL_OP, TRUE  , DU         },
  {I386_ADC       , AL     , Ib     , NULL_OP, FALSE , DU         },
  {I386_ADC       , eAX    , Iv     , NULL_OP, FALSE , DU         },
  {I386_PUSH      , NULL_OP, SS     , NULL_OP, FALSE , 0          },
  {I386_POP       , SS     , NULL_OP, NULL_OP, FALSE , 0          },
  /* 18 */                                   
  {I386_SBB       , Eb     , Gb     , NULL_OP, TRUE  , DU         },
  {I386_SBB       , Ev     , Gv     , NULL_OP, TRUE  , DU         },
  {I386_SBB       , Gb     , Eb     , NULL_OP, TRUE  , DU         },
  {I386_SBB       , Gv     , Ev     , NULL_OP, TRUE  , DU         },
  {I386_SBB       , AL     , sIb    , NULL_OP, FALSE , DU         },
  {I386_SBB       , eAX    , Iv     , NULL_OP, FALSE , DU         },
  {I386_PUSH      , NULL_OP, DS     , NULL_OP, FALSE , 0          },
  {I386_POP       , DS     , NULL_OP, NULL_OP, FALSE , 0          },
  /* 20 */                                   
  {I386_AND       , Eb     , Gb     , NULL_OP, TRUE  , DU         },
  {I386_AND       , Ev     , Gv     , NULL_OP, TRUE  , DU         },
  {I386_AND       , Gb     , Eb     , NULL_OP, TRUE  , DU         },
  {I386_AND       , Gv     , Ev     , NULL_OP, TRUE  , DU         },
  {I386_AND       , AL     , Ib     , NULL_OP, FALSE , DU         },
  {I386_AND       , eAX    , Iv     , NULL_OP, FALSE , DU         },
  {INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, FALSE , 0          },
  {I386_DAA       , NULL_OP, NULL_OP, NULL_OP, FALSE , 0          },
  /* 28 */                                   
  {I386_SUB       , Eb     , Gb     , NULL_OP, TRUE  , DU         },
  {I386_SUB       , Ev     , Gv     , NULL_OP, TRUE  , DU         },
  {I386_SUB       , Gb     , Eb     , NULL_OP, TRUE  , DU         },
  {I386_SUB       , Gv     , Ev     , NULL_OP, TRUE  , DU         },
  {I386_SUB       , AL     , sIb    , NULL_OP, FALSE , DU         },
  {I386_SUB       , eAX    , sIv    , NULL_OP, FALSE , DU         },
  {INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, FALSE , 0          },
  {I386_DAS       , AL     , NULL_OP, NULL_OP, FALSE , DU         },
  /* 30 */                                   
  {I386_XOR       , Eb     , Gb     , NULL_OP, TRUE  , DU         },
  {I386_XOR       , Ev     , Gv     , NULL_OP, TRUE  , DU         },
  {I386_XOR       , Gb     , Eb     , NULL_OP, TRUE  , DU         },
  {I386_XOR       , Gv     , Ev     , NULL_OP, TRUE  , DU         },
  {I386_XOR       , AL     , Ib     , NULL_OP, FALSE , DU         },
  {I386_XOR       , eAX    , Iv     , NULL_OP, FALSE , DU         },
  {INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, FALSE , 0          },
  {I386_AAA       , AL     , NULL_OP, NULL_OP, FALSE , DU         },
  /* 38 */                                   
  {I386_CMP       , NULL_OP, Eb     , Gb     , TRUE  , 0          },
  {I386_CMP       , NULL_OP, Ev     , Gv     , TRUE  , 0          },
  {I386_CMP       , NULL_OP, Gb     , Eb     , TRUE  , 0          },
  {I386_CMP       , NULL_OP, Gv     , Ev     , TRUE  , 0          },
  {I386_CMP       , NULL_OP, AL     , Ib     , FALSE , 0          },
  {I386_CMP       , NULL_OP, eAX    , Iv     , FALSE , 0          },
  {INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, FALSE , 0          },
  {I386_AAS       , AL     , NULL_OP, NULL_OP, FALSE , DU         },
  /* 40 */                                   
  {I386_INC       , eAX    , NULL_OP, NULL_OP, FALSE , DU         },
  {I386_INC       , eCX    , NULL_OP, NULL_OP, FALSE , DU         },
  {I386_INC       , eDX    , NULL_OP, NULL_OP, FALSE , DU         },
  {I386_INC       , eBX    , NULL_OP, NULL_OP, FALSE , DU         },
  {I386_INC       , eSP    , NULL_OP, NULL_OP, FALSE , DU         },
  {I386_INC       , eBP    , NULL_OP, NULL_OP, FALSE , DU         },
  {I386_INC       , eSI    , NULL_OP, NULL_OP, FALSE , DU         },
  {I386_INC       , eDI    , NULL_OP, NULL_OP, FALSE , DU         },
  /* 48 */                                   
  {I386_DEC       , eAX    , NULL_OP, NULL_OP, FALSE , DU         },
  {I386_DEC       , eCX    , NULL_OP, NULL_OP, FALSE , DU         },
  {I386_DEC       , eDX    , NULL_OP, NULL_OP, FALSE , DU         },
  {I386_DEC       , eBX    , NULL_OP, NULL_OP, FALSE , DU         },
  {I386_DEC       , eSP    , NULL_OP, NULL_OP, FALSE , DU         },
  {I386_DEC       , eBP    , NULL_OP, NULL_OP, FALSE , DU         },
  {I386_DEC       , eSI    , NULL_OP, NULL_OP, FALSE , DU         },
  {I386_DEC       , eDI    , NULL_OP, NULL_OP, FALSE , DU         },
  /* 50 */                                   
  {I386_PUSH      , NULL_OP, eAX    , NULL_OP, FALSE , 0          },
  {I386_PUSH      , NULL_OP, eCX    , NULL_OP, FALSE , 0          },
  {I386_PUSH      , NULL_OP, eDX    , NULL_OP, FALSE , 0          },
  {I386_PUSH      , NULL_OP, eBX    , NULL_OP, FALSE , 0          },
  {I386_PUSH      , NULL_OP, eSP    , NULL_OP, FALSE , 0          },
  {I386_PUSH      , NULL_OP, eBP    , NULL_OP, FALSE , 0          },
  {I386_PUSH      , NULL_OP, eSI    , NULL_OP, FALSE , 0          },
  {I386_PUSH      , NULL_OP, eDI    , NULL_OP, FALSE , 0          },
  /* 58 */                                   
  {I386_POP       , eAX    , NULL_OP, NULL_OP, FALSE , 0          },
  {I386_POP       , eCX    , NULL_OP, NULL_OP, FALSE , 0          },
  {I386_POP       , eDX    , NULL_OP, NULL_OP, FALSE , 0          },
  {I386_POP       , eBX    , NULL_OP, NULL_OP, FALSE , 0          },
  {I386_POP       , eSP    , NULL_OP, NULL_OP, FALSE , 0          },
  {I386_POP       , eBP    , NULL_OP, NULL_OP, FALSE , 0          },
  {I386_POP       , eSI    , NULL_OP, NULL_OP, FALSE , 0          },
  {I386_POP       , eDI    , NULL_OP, NULL_OP, FALSE , 0          },
  /* 60 */                                   
  {I386_PUSHA     , NULL_OP, NULL_OP, NULL_OP, FALSE , 0          },
  {I386_POPA      , NULL_OP, NULL_OP, NULL_OP, FALSE , 0          },
  {I386_BOUND     , Gv     , Ma     , NULL_OP, TRUE  , 0          },
  {I386_ARPL      , Ew     , Gw     , NULL_OP, TRUE  , 0          },
  {INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, FALSE , 0          },
  {INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, FALSE , 0          },
  {INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, FALSE , 0          },
  {INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, FALSE , 0          },
  /* 68 */                                   
  {I386_PUSH      , NULL_OP, Iv     , NULL_OP, FALSE , 0          },
  {I386_IMULexp2  , Gv     , Ev     , Iv     , TRUE  , 0          }, /* explicit dest, source1 and immediate operand */
  {I386_PUSH      , NULL_OP, sIb    , NULL_OP, FALSE , 0          },
  {I386_IMULexp2  , Gv     , Ev     , sIb    , TRUE  , 0          }, /* explicit dest, source1 and immediate operand */
  {I386_INSB      , Yb     , DX     , NULL_OP, FALSE , 0          },
  {I386_INSD      , Yv     , DX     , NULL_OP, FALSE , 0          },
  {I386_OUTSB     , NULL_OP, DX     , Xb     , FALSE , 0          },
  {I386_OUTSD     , NULL_OP, DX     , Xv     , FALSE , 0          },
  /* 70 */                                   
  {I386_Jcc       , NULL_OP, Jb     , NULL_OP, FALSE , 0          },
  {I386_Jcc       , NULL_OP, Jb     , NULL_OP, FALSE , 0          },
  {I386_Jcc       , NULL_OP, Jb     , NULL_OP, FALSE , 0          },
  {I386_Jcc       , NULL_OP, Jb     , NULL_OP, FALSE , 0          },
  {I386_Jcc       , NULL_OP, Jb     , NULL_OP, FALSE , 0          },
  {I386_Jcc       , NULL_OP, Jb     , NULL_OP, FALSE , 0          },
  {I386_Jcc       , NULL_OP, Jb     , NULL_OP, FALSE , 0          },
  {I386_Jcc       , NULL_OP, Jb     , NULL_OP, FALSE , 0          },
  /* 78 */                                   
  {I386_Jcc       , NULL_OP, Jb     , NULL_OP, FALSE , 0          },
  {I386_Jcc       , NULL_OP, Jb     , NULL_OP, FALSE , 0          },
  {I386_Jcc       , NULL_OP, Jb     , NULL_OP, FALSE , 0          },
  {I386_Jcc       , NULL_OP, Jb     , NULL_OP, FALSE , 0          },
  {I386_Jcc       , NULL_OP, Jb     , NULL_OP, FALSE , 0          },
  {I386_Jcc       , NULL_OP, Jb     , NULL_OP, FALSE , 0          },
  {I386_Jcc       , NULL_OP, Jb     , NULL_OP, FALSE , 0          },
  {I386_Jcc       , NULL_OP, Jb     , NULL_OP, FALSE , 0          },
  /* 80 */                                   
  {GRP1a          , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
  {GRP1b          , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
  {GRP1c          , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
  {GRP1d          , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
  {I386_TEST      , NULL_OP, Eb     , Gb     , TRUE  , 0          },
  {I386_TEST      , NULL_OP, Ev     , Gv     , TRUE  , 0          },
  {I386_XCHG      , Eb     , Gb     , NULL_OP, TRUE  , DU|S1D     },
  {I386_XCHG      , Ev     , Gv     , NULL_OP, TRUE  , DU|S1D     },
  /* 88 */                                   
  {I386_MOV       , Eb     , Gb     , NULL_OP, TRUE  , 0          },
  {I386_MOV       , Ev     , Gv     , NULL_OP, TRUE  , 0          },
  {I386_MOV       , Gb     , Eb     , NULL_OP, TRUE  , 0          },
  {I386_MOV       , Gv     , Ev     , NULL_OP, TRUE  , 0          },
  {I386_MOV       , Ew     , Sw     , NULL_OP, TRUE  , 0          },
  {I386_LEA       , Gv     , M      , NULL_OP, TRUE  , 0          },
  {I386_MOV       , Sw     , Ew     , NULL_OP, TRUE  , 0          },
  {I386_POP       , Ev     , NULL_OP, NULL_OP, TRUE  , 0          },
  /* 90 */                                   
  {I386_NOP       , NULL_OP, NULL_OP, NULL_OP, FALSE , 0          },
  {I386_XCHG      , eCX    , eAX    , NULL_OP, FALSE , DU|S1D     },
  {I386_XCHG      , eDX    , eAX    , NULL_OP, FALSE , DU|S1D     },
  {I386_XCHG      , eBX    , eAX    , NULL_OP, FALSE , DU|S1D     },
  {I386_XCHG      , eSP    , eAX    , NULL_OP, FALSE , DU|S1D     },
  {I386_XCHG      , eBP    , eAX    , NULL_OP, FALSE , DU|S1D     },
  {I386_XCHG      , eSI    , eAX    , NULL_OP, FALSE , DU|S1D     },
  {I386_XCHG      , eDI    , eAX    , NULL_OP, FALSE , DU|S1D     },
  /* 98 */                                   
  {I386_CWDE      , eAX    , NULL_OP, NULL_OP, FALSE , DU         },
  {I386_CDQ       , eDX    , eAX    , NULL_OP, FALSE , S1D        },
  {I386_CALLF     , NULL_OP, Ap     , NULL_OP, FALSE , 0          },
  {I386_WAIT      , NULL_OP, NULL_OP, NULL_OP, FALSE , 0          },
  {I386_PUSHF     , NULL_OP, Fv     , NULL_OP, FALSE , 0          },
  {I386_POPF      , Fv     , NULL_OP, NULL_OP, FALSE , 0          },
  {I386_SAHF      , NULL_OP, AH     , NULL_OP, FALSE , 0          },
  {I386_LAHF      , AH     , NULL_OP, NULL_OP, FALSE , 0          },
  /* a0 */                                   
  {I386_MOV       , AL     , Ob     , NULL_OP, FALSE , 0          },
  {I386_MOV       , eAX    , Ov     , NULL_OP, FALSE , 0          },
  {I386_MOV       , Ob     , AL     , NULL_OP, FALSE , 0          },
  {I386_MOV       , Ov     , eAX    , NULL_OP, FALSE , 0          },
  {I386_MOVSB     , Yb     , Xb     , NULL_OP, FALSE , 0          },
  {I386_MOVSD     , Yv     , Xv     , NULL_OP, FALSE , 0          },
  {I386_CMPSB     , NULL_OP, Yb     , Xb     , FALSE , 0          },
  {I386_CMPSD     , NULL_OP, Yv     , Xv     , FALSE , 0          },
  /* a8 */                                   
  {I386_TEST      , NULL_OP, AL     , Ib     , FALSE , 0          },
  {I386_TEST      , NULL_OP, eAX    , Iv     , FALSE , 0          },
  {I386_STOSB     , Yb     , AL     , NULL_OP, FALSE , 0          },
  {I386_STOSD     , Yv     , eAX    , NULL_OP, FALSE , 0          },
  {I386_LODSB     , AL     , Xb     , NULL_OP, FALSE , 0          },
  {I386_LODSD     , eAX    , Xv     , NULL_OP, FALSE , 0          },
  {I386_SCASB     , NULL_OP, AL     , Yb     , FALSE , 0          },
  {I386_SCASD     , NULL_OP, eAX    , Yv     , FALSE , 0          },
  /* b0 */                                   
  {I386_MOV       , AL     , Ib     , NULL_OP, FALSE , 0          },
  {I386_MOV       , CL     , Ib     , NULL_OP, FALSE , 0          },
  {I386_MOV       , DL     , Ib     , NULL_OP, FALSE , 0          },
  {I386_MOV       , BL     , Ib     , NULL_OP, FALSE , 0          },
  {I386_MOV       , AH     , Ib     , NULL_OP, FALSE , 0          },
  {I386_MOV       , CH     , Ib     , NULL_OP, FALSE , 0          },
  {I386_MOV       , DH     , Ib     , NULL_OP, FALSE , 0          },
  {I386_MOV       , BH     , Ib     , NULL_OP, FALSE , 0          },
  /* b8 */                                   
  {I386_MOV       , eAX    , Iv     , NULL_OP, FALSE , 0          },
  {I386_MOV       , eCX    , Iv     , NULL_OP, FALSE , 0          },
  {I386_MOV       , eDX    , Iv     , NULL_OP, FALSE , 0          },
  {I386_MOV       , eBX    , Iv     , NULL_OP, FALSE , 0          },
  {I386_MOV       , eSP    , Iv     , NULL_OP, FALSE , 0          },
  {I386_MOV       , eBP    , Iv     , NULL_OP, FALSE , 0          },
  {I386_MOV       , eSI    , Iv     , NULL_OP, FALSE , 0          },
  {I386_MOV       , eDI    , Iv     , NULL_OP, FALSE , 0          },
  /* c0 */                                   
  {GRP2a          , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
  {GRP2b          , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
  {I386_RET       , NULL_OP, Iw     , NULL_OP, FALSE , 0          },
  {I386_RET       , NULL_OP, NULL_OP, NULL_OP, FALSE , 0          },
  {I386_LES       , Gv     , Mp     , NULL_OP, TRUE  , 0          },
  {I386_LDS       , Gv     , Mp     , NULL_OP, TRUE  , 0          },
  {GRP3a          , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
  {GRP3b          , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
  /* c8 */                                   
  {I386_ENTER     , Iw     , Ib     , NULL_OP, FALSE , 0          },
  {I386_LEAVE     , NULL_OP, NULL_OP, NULL_OP, FALSE , 0          },
  {I386_RETF      , NULL_OP, Iw     , NULL_OP, FALSE , 0          },
  {I386_RETF      , NULL_OP, NULL_OP, NULL_OP, FALSE , 0          },
  {I386_INT3      , NULL_OP, NULL_OP, NULL_OP, FALSE , 0          },
  {I386_INT       , NULL_OP, Ib     , NULL_OP, FALSE , 0          },
  {I386_INTO      , NULL_OP, NULL_OP, NULL_OP, FALSE , 0          },
  {I386_IRET      , NULL_OP, NULL_OP, NULL_OP, FALSE , 0          },
  /* d0 */                                   
  {GRP4a          , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
  {GRP4b          , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
  {GRP4c          , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
  {GRP4d          , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
  {I386_AAM       , AX     , Ib     , NULL_OP, FALSE , DU         },
  {I386_AAD       , AX     , Ib     , NULL_OP, FALSE , DU         },
  {INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, FALSE , 0          },
  {I386_XLAT      , AL     , NULL_OP, NULL_OP, FALSE , DU         },
  /* d8 */                                   
  {FPU_ESC        , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
  {FPU_ESC        , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
  {FPU_ESC        , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
  {FPU_ESC        , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
  {FPU_ESC        , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
  {FPU_ESC        , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
  {FPU_ESC        , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
  {FPU_ESC        , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
  /* e0 */                                   
  {I386_LOOPNZ    , NULL_OP, Jb     , NULL_OP, FALSE , 0          },
  {I386_LOOPZ     , NULL_OP, Jb     , NULL_OP, FALSE , 0          },
  {I386_LOOP      , NULL_OP, Jb     , NULL_OP, FALSE , 0          },
  {I386_JECXZ     , NULL_OP, Jb     , NULL_OP, FALSE , 0          },
  {I386_IN        , AL     , Ib     , NULL_OP, FALSE , 0          },
  {I386_IN        , eAX    , Ib     , NULL_OP, FALSE , 0          },
  {I386_OUT       , NULL_OP, Ib     , AL     , FALSE , 0          },
  {I386_OUT       , NULL_OP, Ib     , eAX    , FALSE , 0          },
  /* e8 */                                   
  {I386_CALL      , NULL_OP, Jv     , NULL_OP, FALSE , 0          },
  {I386_JMP       , NULL_OP, Jv     , NULL_OP, FALSE , 0          },
  {I386_JMPF      , NULL_OP, Ap     , NULL_OP, FALSE , 0          },
  {I386_JMP       , NULL_OP, Jb     , NULL_OP, FALSE , 0          },
  {I386_IN        , AL     , DX     , NULL_OP, FALSE , 0          },
  {I386_IN        , eAX    , DX     , NULL_OP, FALSE , 0          },
  {I386_OUT       , NULL_OP, DX     , AL     , FALSE , 0          },
  {I386_OUT       , NULL_OP, DX     , eAX    , FALSE , 0          },
  /* f0 */                                   
  {INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, FALSE , 0          },    /* LOCK prefix  */
  {INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, FALSE , 0          },
  {INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, FALSE , 0          },    /* REPNE prefix */
  {INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, FALSE , 0          },    /* REPE prefix  */
  {I386_HLT       , NULL_OP, NULL_OP, NULL_OP, FALSE , 0          },
  {I386_CMC       , NULL_OP, NULL_OP, NULL_OP, FALSE , 0          },
  {GRP5a          , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
  {GRP5b          , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
  /* f8 */                                   
  {I386_CLC       , NULL_OP, NULL_OP, NULL_OP, FALSE , 0          },
  {I386_STC       , NULL_OP, NULL_OP, NULL_OP, FALSE , 0          },
  {I386_CLI       , NULL_OP, NULL_OP, NULL_OP, FALSE , 0          },
  {I386_STI       , NULL_OP, NULL_OP, NULL_OP, FALSE , 0          },
  {I386_CLD       , NULL_OP, NULL_OP, NULL_OP, FALSE , 0          },
  {I386_STD       , NULL_OP, NULL_OP, NULL_OP, FALSE , 0          },
  {GRP6a          , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
  {GRP6b          , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
  /*{LAST_OPC	  , NULL_OP, NULL_OP, NULL_OP, FALSE ,         	  }*/
};


t_i386_opcode_entry dis386_twobyte[] = 
{
  /* 00 */
  {GRP7a          , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
  {GRP7b          , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
  {I386_LAR       , Gv     , Ew     , NULL_OP, TRUE  , 0          },
  {I386_LSL       , Gv     , Ew     , NULL_OP, TRUE  , 0          },
  {INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, FALSE , 0          },
  {INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, FALSE , 0          },
  {I386_CLTS      , NULL_OP, NULL_OP, NULL_OP, FALSE , 0          },
  {INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, FALSE , 0          },
  /* 08 */                                   
  {I386_INVD      , NULL_OP, NULL_OP, NULL_OP, FALSE , 0          },
  {I386_WBINVD    , NULL_OP, NULL_OP, NULL_OP, FALSE , 0          },
  {INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, FALSE , 0          },
  {I386_UD2       , NULL_OP, NULL_OP, NULL_OP, FALSE , 0          },
  {INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, FALSE , 0          },
  {INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, FALSE , 0          },
  {INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, FALSE , 0          },
  {I386_3DNOW_OPC , NULL_OP, NULL_OP, NULL_OP, FALSE , 0          },
  /* 10 */                                   
  {PREFIX_GRP_1   , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },     /* all kinds of MMX and SSE(2) crap */
  {PREFIX_GRP_4   , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
  {PREFIX_GRP_33  , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
  {PREFIX_GRP_3A  , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
  {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, FALSE , 0          },
  {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, FALSE , 0          },
  {PREFIX_GRP_38  , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
  {PREFIX_GRP_3B  , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
  /* 18 */                                   
  {GRP16          , NULL_OP, NULL_OP, NULL_OP, TRUE , 0          },
  {INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, FALSE , 0          },
  {INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, FALSE , 0          },
  {INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, FALSE , 0          },
  {INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, FALSE , 0          },
  {INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, FALSE , 0          },
  {INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, FALSE , 0          },
  {INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, FALSE , 0          },
  /* 20 */                                   
  {I386_MOV       , Rd     , Cd     , NULL_OP, TRUE  , 0          },
  {I386_MOV       , Rd     , Dd     , NULL_OP, TRUE  , 0          },
  {I386_MOV       , Cd     , Rd     , NULL_OP, TRUE  , 0          },
  {I386_MOV       , Dd     , Rd     , NULL_OP, TRUE  , 0          },
  {INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
  {INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
  {INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
  {INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
  /* 28 */                                   
  {PREFIX_GRP_9   , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
  {PREFIX_GRP_36   , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
  {PREFIX_GRP_3   , NULL_OP, NULL_OP, NULL_OP, TRUE , 0          },
  {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, FALSE , 0          },
  {PREFIX_GRP_2   , NULL_OP, NULL_OP, NULL_OP, TRUE , 0          },
  {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, FALSE , 0          },
  {PREFIX_GRP_47  , NULL_OP, NULL_OP, NULL_OP, TRUE , 0          },
  {PREFIX_GRP_5   , NULL_OP, NULL_OP, NULL_OP, TRUE , 0          },
  /* 30 */                                   
  {I386_WRMSR     , ECX    , EDX    , EAX    , FALSE , DU         },
  {I386_RDTSC     , EDX    , EAX    , NULL_OP, FALSE , S1D        },
  {I386_RDMSR     , EDX    , EAX    , ECX    , FALSE , S1D        },
  {I386_RDPMC     , EDX    , EAX    , ECX    , FALSE , S1D        },
  {I386_SYSENTER  , NULL_OP, NULL_OP, NULL_OP, FALSE , 0          },
  {I386_SYSEXIT   , NULL_OP, NULL_OP, NULL_OP, FALSE , 0          },
  {INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, FALSE , 0          },
  {INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, FALSE , 0          },
  /* 38 */                                   
  {INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, FALSE , 0          },
  {INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, FALSE , 0          },
  {INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, FALSE , 0          },
  {INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, FALSE , 0          },
  {I386_MOVNTI    , Gv     , Ev     , NULL_OP, FALSE , 0          },
  {INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, FALSE , 0          },
  {INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, FALSE , 0          },
  {INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, FALSE , 0          },
  /* 40 */                                   
  {I386_CMOVcc    , Gv     , Ev     , NULL_OP, TRUE  , 0          },
  {I386_CMOVcc    , Gv     , Ev     , NULL_OP, TRUE  , 0          },
  {I386_CMOVcc    , Gv     , Ev     , NULL_OP, TRUE  , 0          },
  {I386_CMOVcc    , Gv     , Ev     , NULL_OP, TRUE  , 0          },
  {I386_CMOVcc    , Gv     , Ev     , NULL_OP, TRUE  , 0          },
  {I386_CMOVcc    , Gv     , Ev     , NULL_OP, TRUE  , 0          },
  {I386_CMOVcc    , Gv     , Ev     , NULL_OP, TRUE  , 0          },
  {I386_CMOVcc    , Gv     , Ev     , NULL_OP, TRUE  , 0          },
  /* 48 */                                   
  {I386_CMOVcc    , Gv     , Ev     , NULL_OP, TRUE  , 0          },
  {I386_CMOVcc    , Gv     , Ev     , NULL_OP, TRUE  , 0          },
  {I386_CMOVcc    , Gv     , Ev     , NULL_OP, TRUE  , 0          },
  {I386_CMOVcc    , Gv     , Ev     , NULL_OP, TRUE  , 0          },
  {I386_CMOVcc    , Gv     , Ev     , NULL_OP, TRUE  , 0          },
  {I386_CMOVcc    , Gv     , Ev     , NULL_OP, TRUE  , 0          },
  {I386_CMOVcc    , Gv     , Ev     , NULL_OP, TRUE  , 0          },
  {I386_CMOVcc    , Gv     , Ev     , NULL_OP, TRUE  , 0          },
  /* 50 */                                   
  {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, FALSE , 0          },
  {PREFIX_GRP_48   , NULL_OP, NULL_OP, NULL_OP, TRUE , 0          },
  {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, FALSE , 0          },
  {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, FALSE , 0          },
  {PREFIX_GRP_D   , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
  {PREFIX_GRP_C   , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
  {PREFIX_GRP_E   , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
  {PREFIX_GRP_6   , NULL_OP, NULL_OP, NULL_OP, TRUE , 0          },
  /* 58 */                                   
  {PREFIX_GRP_35   , NULL_OP, NULL_OP, NULL_OP, TRUE , 0          },
  {PREFIX_GRP_39   , NULL_OP, NULL_OP, NULL_OP, TRUE , 0          },
  {PREFIX_GRP_8   , NULL_OP, NULL_OP, NULL_OP, TRUE , 0          },
  {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, FALSE , 0          },
  {PREFIX_GRP_37   , NULL_OP, NULL_OP, NULL_OP, TRUE , 0          },
  {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, FALSE , 0          },
  {PREFIX_GRP_46   , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
  {PREFIX_GRP_A  , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
  /* 60 */                                   
  {PREFIX_GRP_11   , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
  {PREFIX_GRP_23   , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
  {PREFIX_GRP_1A   , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
  {PREFIX_GRP_2A   , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
  {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
  {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
  {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
  {PREFIX_GRP_26   , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
  /* 68 */                                   
  {PREFIX_GRP_12   , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
  {PREFIX_GRP_24   , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
  {PREFIX_GRP_29   , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
  {PREFIX_GRP_27   , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
  {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, FALSE , 0          },
  {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, FALSE , 0          },
  {PREFIX_GRP_10   , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
  {PREFIX_GRP_F   , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
  /* 70 */                                   
  {PREFIX_GRP_13  , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
  {GRP12          , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
  {GRP13          , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
  {GRP14          , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
  {PREFIX_GRP_43  , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
  {PREFIX_GRP_25  , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
  {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
  {I386_EMMS     , NULL_OP, NULL_OP, NULL_OP, FALSE , 0          },
  /* 78 */                                   
  {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, FALSE , 0          },
  {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, FALSE , 0          },
  {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, FALSE , 0          },
  {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, FALSE , 0          },
  {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, FALSE , 0          },
  {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, FALSE , 0          },
  {PREFIX_GRP_2C   , NULL_OP, NULL_OP, NULL_OP, TRUE , 0          },
  {PREFIX_GRP_18   , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
  /* 80 */                                   
  {I386_Jcc       , NULL_OP, Jv     , NULL_OP, FALSE , 0          },
  {I386_Jcc       , NULL_OP, Jv     , NULL_OP, FALSE , 0          },
  {I386_Jcc       , NULL_OP, Jv     , NULL_OP, FALSE , 0          },
  {I386_Jcc       , NULL_OP, Jv     , NULL_OP, FALSE , 0          },
  {I386_Jcc       , NULL_OP, Jv     , NULL_OP, FALSE , 0          },
  {I386_Jcc       , NULL_OP, Jv     , NULL_OP, FALSE , 0          },
  {I386_Jcc       , NULL_OP, Jv     , NULL_OP, FALSE , 0          },
  {I386_Jcc       , NULL_OP, Jv     , NULL_OP, FALSE , 0          },
  /* 88 */                                   
  {I386_Jcc       , NULL_OP, Jv     , NULL_OP, FALSE , 0          },
  {I386_Jcc       , NULL_OP, Jv     , NULL_OP, FALSE , 0          },
  {I386_Jcc       , NULL_OP, Jv     , NULL_OP, FALSE , 0          },
  {I386_Jcc       , NULL_OP, Jv     , NULL_OP, FALSE , 0          },
  {I386_Jcc       , NULL_OP, Jv     , NULL_OP, FALSE , 0          },
  {I386_Jcc       , NULL_OP, Jv     , NULL_OP, FALSE , 0          },
  {I386_Jcc       , NULL_OP, Jv     , NULL_OP, FALSE , 0          },
  {I386_Jcc       , NULL_OP, Jv     , NULL_OP, FALSE , 0          },
  /* 90 */                                   
  {I386_SETcc     , Eb     , NULL_OP, NULL_OP, TRUE  , 0          },
  {I386_SETcc     , Eb     , NULL_OP, NULL_OP, TRUE  , 0          },
  {I386_SETcc     , Eb     , NULL_OP, NULL_OP, TRUE  , 0          },
  {I386_SETcc     , Eb     , NULL_OP, NULL_OP, TRUE  , 0          },
  {I386_SETcc     , Eb     , NULL_OP, NULL_OP, TRUE  , 0          },
  {I386_SETcc     , Eb     , NULL_OP, NULL_OP, TRUE  , 0          },
  {I386_SETcc     , Eb     , NULL_OP, NULL_OP, TRUE  , 0          },
  {I386_SETcc     , Eb     , NULL_OP, NULL_OP, TRUE  , 0          },
  /* 98 */                                   
  {I386_SETcc     , Eb     , NULL_OP, NULL_OP, TRUE  , 0          },
  {I386_SETcc     , Eb     , NULL_OP, NULL_OP, TRUE  , 0          },
  {I386_SETcc     , Eb     , NULL_OP, NULL_OP, TRUE  , 0          },
  {I386_SETcc     , Eb     , NULL_OP, NULL_OP, TRUE  , 0          },
  {I386_SETcc     , Eb     , NULL_OP, NULL_OP, TRUE  , 0          },
  {I386_SETcc     , Eb     , NULL_OP, NULL_OP, TRUE  , 0          },
  {I386_SETcc     , Eb     , NULL_OP, NULL_OP, TRUE  , 0          },
  {I386_SETcc     , Eb     , NULL_OP, NULL_OP, TRUE  , 0          },
  /* a0 */                                   
  {I386_PUSH      , NULL_OP, FS     , NULL_OP, FALSE , 0          },
  {I386_POP       , FS     , NULL_OP, NULL_OP, FALSE , 0          },
  {I386_CPUID     , NULL_OP, EAX    , NULL_OP, FALSE , 0          },
  {I386_BT        , NULL_OP, Ev     , Gv     , TRUE  , 0          },
  {I386_SHLD      , Ev     , Gv     , Ib     , TRUE  , DU         },
  {I386_SHLD      , Ev     , Gv     , CL     , TRUE  , DU         },
  {INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
  {INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
  /* a8 */                                   
  {I386_PUSH      , NULL_OP, GS     , NULL_OP, FALSE , 0          },
  {I386_POP       , GS     , NULL_OP, NULL_OP, FALSE , 0          },
  {I386_RSM       , NULL_OP, NULL_OP, NULL_OP, FALSE , 0          },
  {I386_BTS       , NULL_OP, Ev     , Gv     , TRUE  , S1D        },
  {I386_SHRD      , Ev     , Gv     , Ib     , TRUE  , DU         },
  {I386_SHRD      , Ev     , Gv     , CL     , TRUE  , DU         },
  {GRP15a         , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
  {I386_IMULexp1  , Gv     , Ev     , NULL_OP, TRUE  , DU         },  /* explicit dest, dest is source1 */
  /* b0 */                                   
  {I386_CMPXCHG   , Eb     , Gb     , NULL_OP, TRUE  , DU|S1D     },
  {I386_CMPXCHG   , Ev     , Gv     , NULL_OP, TRUE  , DU|S1D     },
  {I386_LSS       , SS     , Gv     , Mp     , TRUE  , S1D        },
  {I386_BTR       , NULL_OP, Ev     , Gv     , TRUE  , S1D        },
  {I386_LFS       , FS     , Gv     , Mp     , TRUE  , S1D        },
  {I386_LGS       , GS     , Gv     , Mp     , TRUE  , S1D        },
  {I386_MOVZX     , Gv     , Eb     , NULL_OP, TRUE  , 0          },
  {I386_MOVZX     , Gv     , Ew     , NULL_OP, TRUE  , 0          },
  /* b8 */                                   
  {INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, FALSE , 0          },
  {I386_UD2       , NULL_OP, NULL_OP, NULL_OP, FALSE , 0          },
  {GRP8a          , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
  {I386_BTC       , NULL_OP, Ev     , Gv     , TRUE  , S1D        },
  {I386_BSF       , Gv     , Ev     , NULL_OP, TRUE  , 0          },
  {I386_BSR       , Gv     , Ev     , NULL_OP, TRUE  , 0          },
  {I386_MOVSX     , Gv     , Eb     , NULL_OP, TRUE  , 0          },
  {I386_MOVSX     , Gv     , Ew     , NULL_OP, TRUE  , 0          },
  /* c0 */                                   
  {I386_XADD      , Eb     , Gb     , NULL_OP, TRUE  , DU|S1D     },
  {I386_XADD      , Ev     , Gv     , NULL_OP, TRUE  , DU|S1D     },
  {PREFIX_GRP_B   , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
  {I386_MOVNTI    , Ed     , Gd     , NULL_OP, TRUE  , 0          },
  {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
  {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
  {PREFIX_GRP_34   , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
  {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
  /* c8 */                                   
  {I386_BSWAP     , EAX    , NULL_OP, NULL_OP, FALSE , DU         },
  {I386_BSWAP     , ECX    , NULL_OP, NULL_OP, FALSE , DU         },
  {I386_BSWAP     , EDX    , NULL_OP, NULL_OP, FALSE , DU         },
  {I386_BSWAP     , EBX    , NULL_OP, NULL_OP, FALSE , DU         },
  {I386_BSWAP     , ESP    , NULL_OP, NULL_OP, FALSE , DU         },
  {I386_BSWAP     , EBP    , NULL_OP, NULL_OP, FALSE , DU         },
  {I386_BSWAP     , ESI    , NULL_OP, NULL_OP, FALSE , DU         },
  {I386_BSWAP     , EDI    , NULL_OP, NULL_OP, FALSE , DU         },
  /* d0 */                                   
  {INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, FALSE , 0          },
  {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
  {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
  {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
  {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, FALSE , 0          },
  {PREFIX_GRP_15   , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
  {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, FALSE , 0          },
  {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, FALSE , 0          },
  /* d8 */                                   
  {PREFIX_GRP_3C   , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
  {PREFIX_GRP_42   , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
  {PREFIX_GRP_44   , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
  {PREFIX_GRP_1B   , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
  {PREFIX_GRP_28   , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
  {PREFIX_GRP_41   , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
  {PREFIX_GRP_45   , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
  {PREFIX_GRP_40   , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
  /* e0 */                                   
  {PREFIX_GRP_2D   , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
  {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
  {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
  {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, FALSE , 0          },
  {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, FALSE , 0          },
  {PREFIX_GRP_2F   , NULL_OP, NULL_OP, NULL_OP, TRUE , 0          },
  {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, FALSE , 0          },
  {PREFIX_GRP_19  , NULL_OP, NULL_OP, NULL_OP, TRUE , 0          },
  /* e8 */                                   
  {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
  {PREFIX_GRP_2E   , NULL_OP, NULL_OP, NULL_OP, TRUE , 0          },
  {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, FALSE , 0          },
  {PREFIX_GRP_1E   , NULL_OP, NULL_OP, NULL_OP, TRUE , 0          },
  {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
  {PREFIX_GRP_30   , NULL_OP, NULL_OP, NULL_OP, TRUE , 0          },
  {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
  {PREFIX_GRP_7   , NULL_OP, NULL_OP, NULL_OP, TRUE , 0          },
  /* f0 */                                   
  {INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, FALSE , 0          },
  {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
  {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
  {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
  {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, FALSE , 0          },
  {PREFIX_GRP_1F  , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
  {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, FALSE , 0          },
  {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, FALSE , 0          },
  /* f8 */                                   
  {PREFIX_GRP_3D  , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
  {PREFIX_GRP_14   , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
  {PREFIX_GRP_32  , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
  {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, FALSE , 0          },
  {PREFIX_GRP_2B  , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
  {PREFIX_GRP_17  , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
  {PREFIX_GRP_31  , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
  {INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, FALSE , 0          }
};
  
t_i386_opcode_entry dis386_grps[][8] =
{
  /* GRP1a */
  {
    {I386_ADD       , Eb     , sIb    , NULL_OP, TRUE  , DU         },
    {I386_OR        , Eb     , Ib     , NULL_OP, TRUE  , DU         },
    {I386_ADC       , Eb     , Ib     , NULL_OP, TRUE  , DU         },
    {I386_SBB       , Eb     , sIb    , NULL_OP, TRUE  , DU         },
    {I386_AND       , Eb     , Ib     , NULL_OP, TRUE  , DU         },
    {I386_SUB       , Eb     , sIb    , NULL_OP, TRUE  , DU         },
    {I386_XOR       , Eb     , Ib     , NULL_OP, TRUE  , DU         },
    {I386_CMP       , NULL_OP, Eb     , Ib     , TRUE  , 0          },
  },
  /* GRP1b */
  {
    {I386_ADD       , Ev     , Iv     , NULL_OP, TRUE  , DU         },
    {I386_OR        , Ev     , Iv     , NULL_OP, TRUE  , DU         },
    {I386_ADC       , Ev     , Iv     , NULL_OP, TRUE  , DU         },
    {I386_SBB       , Ev     , Iv     , NULL_OP, TRUE  , DU         },
    {I386_AND       , Ev     , Iv     , NULL_OP, TRUE  , DU         },
    {I386_SUB       , Ev     , sIv    , NULL_OP, TRUE  , DU         },
    {I386_XOR       , Ev     , Iv     , NULL_OP, TRUE  , DU         },
    {I386_CMP       , NULL_OP, Ev     , Iv     , TRUE  , 0          },
  },
  /* GRP1c */
  {
    {INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
  },
  /* GRP1d */
  {
    {I386_ADD       , Ev     , sIb    , NULL_OP, TRUE  , DU         },
    {I386_OR        , Ev     , sIb    , NULL_OP, TRUE  , DU         },
    {I386_ADC       , Ev     , sIb    , NULL_OP, TRUE  , DU         },
    {I386_SBB       , Ev     , sIb    , NULL_OP, TRUE  , DU         },
    {I386_AND       , Ev     , sIb    , NULL_OP, TRUE  , DU         },
    {I386_SUB       , Ev     , sIb    , NULL_OP, TRUE  , DU         },
    {I386_XOR       , Ev     , sIb    , NULL_OP, TRUE  , DU         },
    {I386_CMP       , NULL_OP, Ev     , sIb    , TRUE  , 0          },
  },
  /* GRP2a */
  {
    {I386_ROL       , Eb     , Ib     , NULL_OP, TRUE  , DU         },
    {I386_ROR       , Eb     , Ib     , NULL_OP, TRUE  , DU         },
    {I386_RCL       , Eb     , Ib     , NULL_OP, TRUE  , DU         },
    {I386_RCR       , Eb     , Ib     , NULL_OP, TRUE  , DU         },
    {I386_SHL       , Eb     , Ib     , NULL_OP, TRUE  , DU         },
    {I386_SHR       , Eb     , Ib     , NULL_OP, TRUE  , DU         },
    {INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {I386_SAR       , Eb     , Ib     , NULL_OP, TRUE  , DU         },
  },
  /* GRP2b */
  {
    {I386_ROL       , Ev     , Ib     , NULL_OP, TRUE  , DU         },
    {I386_ROR       , Ev     , Ib     , NULL_OP, TRUE  , DU         },
    {I386_RCL       , Ev     , Ib     , NULL_OP, TRUE  , DU         },
    {I386_RCR       , Ev     , Ib     , NULL_OP, TRUE  , DU         },
    {I386_SHL       , Ev     , Ib     , NULL_OP, TRUE  , DU         },
    {I386_SHR       , Ev     , Ib     , NULL_OP, TRUE  , DU         },
    {INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {I386_SAR       , Ev     , Ib     , NULL_OP, TRUE  , DU         },
  },
  /* GRP3a */
  {
    {I386_MOV       , Eb     , Ib     , NULL_OP, TRUE  , 0          },
    {INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
  },
  /* GRP3b */
  {
    {I386_MOV       , Ev     , Iv     , NULL_OP, TRUE  , 0          },
    {INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
  },
  /* GRP4a */
  {
    {I386_ROL       , Eb     , C_1    , NULL_OP, TRUE  , DU         },
    {I386_ROR       , Eb     , C_1    , NULL_OP, TRUE  , DU         },
    {I386_RCL       , Eb     , C_1    , NULL_OP, TRUE  , DU         },
    {I386_RCR       , Eb     , C_1    , NULL_OP, TRUE  , DU         },
    {I386_SHL       , Eb     , C_1    , NULL_OP, TRUE  , DU         },
    {I386_SHR       , Eb     , C_1    , NULL_OP, TRUE  , DU         },
    {INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {I386_SAR       , Eb     , C_1    , NULL_OP, TRUE  , DU         },
  },
  /* GRP4b */
  {
    {I386_ROL       , Ev     , C_1    , NULL_OP, TRUE  , DU         },
    {I386_ROR       , Ev     , C_1    , NULL_OP, TRUE  , DU         },
    {I386_RCL       , Ev     , C_1    , NULL_OP, TRUE  , DU         },
    {I386_RCR       , Ev     , C_1    , NULL_OP, TRUE  , DU         },
    {I386_SHL       , Ev     , C_1    , NULL_OP, TRUE  , DU         },
    {I386_SHR       , Ev     , C_1    , NULL_OP, TRUE  , DU         },
    {INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {I386_SAR       , Ev     , C_1    , NULL_OP, TRUE  , DU         },
  },
  /* GRP4c */
  {
    {I386_ROL       , Eb     , CL     , NULL_OP, TRUE  , DU         },
    {I386_ROR       , Eb     , CL     , NULL_OP, TRUE  , DU         },
    {I386_RCL       , Eb     , CL     , NULL_OP, TRUE  , DU         },
    {I386_RCR       , Eb     , CL     , NULL_OP, TRUE  , DU         },
    {I386_SHL       , Eb     , CL     , NULL_OP, TRUE  , DU         },
    {I386_SHR       , Eb     , CL     , NULL_OP, TRUE  , DU         },
    {INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {I386_SAR       , Eb     , CL     , NULL_OP, TRUE  , DU         },
  },
  /* GRP4d */
  {
    {I386_ROL       , Ev     , CL     , NULL_OP, TRUE  , DU         },
    {I386_ROR       , Ev     , CL     , NULL_OP, TRUE  , DU         },
    {I386_RCL       , Ev     , CL     , NULL_OP, TRUE  , DU         },
    {I386_RCR       , Ev     , CL     , NULL_OP, TRUE  , DU         },
    {I386_SHL       , Ev     , CL     , NULL_OP, TRUE  , DU         },
    {I386_SHR       , Ev     , CL     , NULL_OP, TRUE  , DU         },
    {INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {I386_SAR       , Ev     , CL     , NULL_OP, TRUE  , DU         },
  },
  /* GRP5a */
  {
    {I386_TEST      , NULL_OP, Eb     , Ib     , TRUE  , 0          },
    {INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {I386_NOT       , Eb     , NULL_OP, NULL_OP, TRUE  , DU         },
    {I386_NEG       , Eb     , NULL_OP, NULL_OP, TRUE  , DU         },
    {I386_MUL       , AL     , Eb     , NULL_OP, TRUE  , DU         },
    {I386_IMUL      , AL     , Eb     , NULL_OP, TRUE  , DU         },
    {I386_DIV       , AL     , Eb     , NULL_OP, TRUE  , DU         },
    {I386_IDIV      , AL     , Eb     , NULL_OP, TRUE  , DU         },
  },
  /* GRP5b */
  {
    {I386_TEST      , NULL_OP, Ev     , Iv     , TRUE  , 0          },
    {INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {I386_NOT       , Ev     , NULL_OP, NULL_OP, TRUE  , DU         },
    {I386_NEG       , Ev     , NULL_OP, NULL_OP, TRUE  , DU         },
    {I386_MUL       , eAX    , Ev     , NULL_OP, TRUE  , DU         },
    {I386_IMUL      , eAX    , Ev     , NULL_OP, TRUE  , DU         },
    {I386_DIV       , eAX    , Ev     , NULL_OP, TRUE  , DU         },
    {I386_IDIV      , eAX    , Ev     , NULL_OP, TRUE  , DU         },
  },
  /* GRP6a */
  {
    {I386_INC       , Eb     , NULL_OP, NULL_OP, TRUE  , DU         },
    {I386_DEC       , Eb     , NULL_OP, NULL_OP, TRUE  , DU         },
    {INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
  },
  /* GRP6b */
  {
    {I386_INC       , Ev     , NULL_OP, NULL_OP, TRUE  , DU         },
    {I386_DEC       , Ev     , NULL_OP, NULL_OP, TRUE  , DU         },
    {I386_CALL      , NULL_OP, Ev     , NULL_OP, TRUE  , 0          },
    {I386_CALLF     , NULL_OP, Ep     , NULL_OP, TRUE  , 0          },
    {I386_JMP       , NULL_OP, Ev     , NULL_OP, TRUE  , 0          },
    {I386_JMPF      , NULL_OP, Ep     , NULL_OP, TRUE  , 0          },
    {I386_PUSH      , NULL_OP, Ev     , NULL_OP, TRUE  , 0          },
    {INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
  },
  /* GRP7a */
  {
    {I386_SLDT      , Ew     , NULL_OP, NULL_OP, TRUE  , 0          },
    {I386_STR       , Ev     , NULL_OP, NULL_OP, TRUE  , 0          },
    {I386_LLDT      , NULL_OP, Ew     , NULL_OP, TRUE  , 0          },
    {I386_LTR       , NULL_OP, Ew     , NULL_OP, TRUE  , 0          },
    {I386_VERR      , NULL_OP, Ew     , NULL_OP, TRUE  , 0          },
    {I386_VERW      , NULL_OP, Ew     , NULL_OP, TRUE  , 0          },
    {INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
  },
  /* GRP7b */
  {
    {I386_SGDT      , Ms     , NULL_OP, NULL_OP, TRUE  , 0          },
    {I386_SIDT      , Ms     , NULL_OP, NULL_OP, TRUE  , 0          },
    {I386_LGDT      , NULL_OP, Ms     , NULL_OP, TRUE  , 0          },
    {I386_LIDT      , NULL_OP, Ms     , NULL_OP, TRUE  , 0          },
    {I386_SMSW      , Ew     , NULL_OP, NULL_OP, TRUE  , 0          },
    {INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {I386_LMSW      , NULL_OP, Ew     , NULL_OP, TRUE  , 0          },
    {I386_INVLPG    , Mb     , NULL_OP, NULL_OP, TRUE  , 0          },
  },
  /* GRP8a */
  {
    {INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {I386_BT        , NULL_OP, Ev     , Ib     , TRUE  , 0          },
    {I386_BTS       , NULL_OP, Ev     , Ib     , TRUE  , S1D        },
    {I386_BTR       , NULL_OP, Ev     , Ib     , TRUE  , S1D        },
    {I386_BTC       , NULL_OP, Ev     , Ib     , TRUE  , S1D        },
  },
  /* GRP12 */
  {
    {INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {PREFIX_GRP_3F  , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {PREFIX_GRP_3E  , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {PREFIX_GRP_16  , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
  },
  /* GRP13 */
  {
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {PREFIX_GRP_20  , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {PREFIX_GRP_22  , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {PREFIX_GRP_21  , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
  },
  /* GRP14 */
  {
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {PREFIX_GRP_1C  , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {I386_PSLLDQ    , Wdq    , Ib     , NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {PREFIX_GRP_1D  , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {I386_PSLLDQ    , Wdq    , Ib     , NULL_OP, TRUE  , 0          },
  },
  /* GRP15a */
  {
    {I386_FXSAVE    , M512   , NULL_OP, NULL_OP, TRUE  , 0          },
    {I386_FXRSTOR   , NULL_OP, M512   , NULL_OP, TRUE  , 0          },
    {I386_STMXCSR   , Md     , NULL_OP, NULL_OP, TRUE  , 0          },
    {I386_LDMXCSR   , NULL_OP, Md     , NULL_OP, TRUE  , 0          },
    {INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {I386_CLFLUSH   , NULL_OP, Mb     , NULL_OP, TRUE  , 0          },
  },
  /* GRP15a11 */
  {
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {I386_SFENCE    , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {I386_LFENCE    , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {I386_MFENCE    , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },

  },
  /* GRP16 */
  {
    {I386_PREFETCH_NTA, NULL_OP, Mb     , NULL_OP, TRUE, 0          },
    {I386_PREFETCH_T0 , NULL_OP, Mb     , NULL_OP, TRUE, 0          },
    {I386_PREFETCH_T1 , NULL_OP, Mb     , NULL_OP, TRUE, 0          },
    {I386_PREFETCH_T2 , NULL_OP, Mb     , NULL_OP, TRUE, 0          },
    {INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
  },
  /* PREFIX_GRP_1 */
  {
    {I386_MOVSD_SSE , Vsd    , Wsd    , NULL_OP, TRUE  , 0          },
    {I386_MOVSS_SSE , Vss    , Wss    , NULL_OP, TRUE  , 0          },
    {I386_MOVUPS    , Vps    , Wps    , NULL_OP, TRUE  , 0          },
    {I386_MOVUPD    , Vpd    , Wpd    , NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
  },
  /* PREFIX_GRP_2 */
  {
    {I386_CVTTSD2SI , Gd     , Wsd    , NULL_OP, TRUE  , 0          },
    {I386_CVTTSS2SI , Gd     , Wss    , NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
  },
  /* PREFIX_GRP_3 */
  {
    {I386_CVTSI2SD  , Vsd    , Ed     , NULL_OP, TRUE  , 0          },
    {I386_CVTSI2SS  , Vss    , Ed     , NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
  },
  /* PREFIX_GRP_4 */
  {
    {I386_MOVSD_SSE , Wsd    , Vsd    , NULL_OP, TRUE  , 0          },
    {I386_MOVSS_SSE , Wss    , Vss    , NULL_OP, TRUE  , 0          },
    {I386_MOVUPS    , Wps    , Vps    , NULL_OP, TRUE  , 0          },
    {I386_MOVUPD    , Wpd    , Vpd    , NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
  },
  /* PREFIX_GRP_5 */
  {
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {I386_COMISS    , Vps    , Wps    , NULL_OP, TRUE  , 0          },
    {I386_COMISD    , Vsd    , Wsd    , NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
  },
  /* PREFIX_GRP_6 */
  {
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {I386_XORPS     , Vps    , Wps    , NULL_OP, TRUE  , 0          },
    {I386_XORPD     , Vpd    , Wpd    , NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
  },
  /* PREFIX_GRP_7 */
  {
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {I386_PXOR      , Pq     , Qq     , NULL_OP, TRUE  , 0          },
    {I386_PXORD     , Vdq    , Wdq    , NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
  },
  /* PREFIX_GRP_8 */
  {
    {I386_CVTSD2SS  , Vss    , Wsd    , NULL_OP, TRUE  , 0          },
    {I386_CVTSD2SD  , Vsd    , Wss    , NULL_OP, TRUE  , 0          },
    {I386_CVTSD2PD  , Vpd    , Wq     , NULL_OP, TRUE  , 0          },
    {I386_CVTSD2PS  , Vps    , Wpd    , NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
  },
  /* PREFIX_GRP_9 */
  {
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {I386_MOVAPS    , Vps    , Wps    , NULL_OP, TRUE  , 0          },
    {I386_MOVAPD    , Vpd    , Wpd    , NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
  },
  /* PREFIX_GRP_A */
  {
    {I386_MAXSD     , Vsd    , Wsd    , NULL_OP, TRUE  , 0          },
    {I386_MAXSS     , Vss    , Wss    , NULL_OP, TRUE  , 0          },
    {I386_MAXPS     , Vps    , Wps    , NULL_OP, TRUE  , 0          },
    {I386_MAXPD     , Vpd    , Wpd    , NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
  },
  /* PREFIX_GRP_B */
  {
    {I386_CMPSD_SSE , Vsd    , Wsd    , Ib     , TRUE  , 0          },
    {I386_CMPSS_SSE , Vss    , Wss    , Ib     , TRUE  , 0          },
    {I386_CMPPS_SSE , Vps    , Wps    , Ib     , TRUE  , 0          },
    {I386_CMPPD_SSE , Vpd    , Wpd    , Ib     , TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
  },
  /* PREFIX_GRP_C */
  {
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {I386_ANDNPS    , Vps    , Wps    , NULL_OP, TRUE  , 0          },
    {I386_ANDNPD    , Vpd    , Wpd    , NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
  },
  /* PREFIX_GRP_D */
  {
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {I386_ANDPS     , Vps    , Wps    , NULL_OP, TRUE  , 0          },
    {I386_ANDPD     , Vpd    , Wpd    , NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
  },
  /* PREFIX_GRP_E */
  {
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {I386_ORPS      , Vps    , Wps    , NULL_OP, TRUE  , 0          },
    {I386_ORPD      , Vpd    , Wpd    , NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
  },
  /* PREFIX_GRP_F */
  {
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {I386_MOVDQU    , Vdq    , Wdq    , NULL_OP, TRUE  , 0          },
    {I386_MOVQ      , Pq     , Qq     , NULL_OP, TRUE  , 0          },
    {I386_MOVDQA    , Vdq    , Wdq    , NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
  },
  /* PREFIX_GRP_10 */
  {
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {I386_MOVD      , Pd     , Ed     , NULL_OP, TRUE  , 0          },
    {I386_MOVD2     , Vd     , Ed     , NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
  },
  /* PREFIX_GRP_11 */
  {
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {I386_PUNPCKLBW , Pq     , Qq     , NULL_OP, TRUE  , 0          },
    {I386_PUNPCKLBW2, Vdq    , Qdq    , NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
  },
  /* PREFIX_GRP_12 */
  {
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {I386_PUNPCKHBW , Pq     , Qq     , NULL_OP, TRUE  , 0          },
    {I386_PUNPCKHBW2, Vdq    , Qdq    , NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
  },
  /* PREFIX_GRP_13 */
  {
    {I386_PSHUFLW   , Vdq    , Wdq    , Ib     , TRUE  , 0          },
    {I386_PSHUFHW   , Vdq    , Wdq    , Ib     , TRUE  , 0          },
    {I386_PSHUFW    , Pq     , Qq     , Ib     , TRUE  , 0          },
    {I386_PSHUFD    , Vdq    , Wdq    , Ib     , TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
  },
  /* PREFIX_GRP_14 */
  {
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {I386_PSUBW     , Pq     , Qq     , NULL_OP, TRUE  , 0          },
    {I386_PSUBW2    , Vdq    , Wdq    , NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
  },
  /* PREFIX_GRP_15 */
  {
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {I386_PMULLW    , Pq     , Qq     , NULL_OP, TRUE  , 0          },
    {I386_PMULLW2   , Vdq    , Wdq    , NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
  },
  /* PREFIX_GRP_16 */
  {
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {I386_PSLLW     , Pq     , Ib     , NULL_OP, TRUE  , 0          },
    {I386_PSLLW2    , Pdq    , Ib     , NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
  },
  /* PREFIX_GRP_17 */
  {
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {I386_PADDW     , Pq     , Qq     , NULL_OP, TRUE  , 0          },
    {I386_PADDW2    , Vdq    , Wdq    , NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
  },
  /* PREFIX_GRP_18 */
  {
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {I386_MOVDQU    , Wdq    , Vdq    , NULL_OP, TRUE  , 0          },
    {I386_MOVQ      , Qq     , Pq     , NULL_OP, TRUE  , 0          },
    {I386_MOVDQA    , Wdq    , Vdq    , NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
  },
  /* PREFIX_GRP_19 */
  {
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {I386_MOVNTQ    , Mq     , Vq     , NULL_OP, TRUE  , 0          },
    {I386_MOVNTQD   , Mdq    , Vdq    , NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
  },
  /* PREFIX_GRP_1A */
  {
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {I386_PUNPCKLDQ , Pq     , Qq     , NULL_OP, TRUE  , 0          },
    {I386_PUNPCKLDQ2, Vdq    , Wdq    , NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
  },
  /* PREFIX_GRP_1B */
  {
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {I386_PAND      , Pq     , Qq     , NULL_OP, TRUE  , 0          },
    {I386_PAND2     , Vdq    , Wdq    , NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
  },
  /* PREFIX_GRP_1C */
  {
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {I386_PSRLQ     , Pq     , Ib     , NULL_OP, TRUE  , 0          },
    {I386_PSRLQ2    , Wdq    , Ib     , NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
  },
  /* PREFIX_GRP_1D */
  {
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {I386_PSLLQ     , Pq     , Ib     , NULL_OP, TRUE  , 0          },
    {I386_PSLLQ2    , Wdq    , Ib     , NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
  },
  /* PREFIX_GRP_1E */
  {
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {I386_POR       , Pq     , Qq     , NULL_OP, TRUE  , 0          },
    {I386_POR2      , Vdq    , Wdq    , NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
  },
  /* PREFIX_GRP_1F */
  {
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {I386_PMADDWD   , Pq     , Qq     , NULL_OP, TRUE  , 0          },
    {I386_PMADDWD2  , Vdq    , Wdq    , NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
  },
  /* PREFIX_GRP_20 */
  {
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {I386_PSRLD     , Pq     , Ib     , NULL_OP, TRUE  , 0          },
    {I386_PSRLD2    , Wdq    , Ib     , NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
  },
  /* PREFIX_GRP_21 */
  {
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {I386_PSLLD     , Pq     , Ib     , NULL_OP, TRUE  , 0          },
    {I386_PSLLD2    , Wdq    , Ib     , NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
  },
  /* PREFIX_GRP_22 */
  {
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {I386_PSRAD     , Pq     , Ib     , NULL_OP, TRUE  , 0          },
    {I386_PSRAD2    , Wdq    , Ib     , NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
  },
  /* PREFIX_GRP_23 */
  {
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {I386_PUNPCKLWD , Pq     , Qq     , NULL_OP, TRUE  , 0          },
    {I386_PUNPCKLWD2, Vdq    , Wdq    , NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
  },
  /* PREFIX_GRP_24 */
  {
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {I386_PUNPCKHWD , Pq     , Qq     , NULL_OP, TRUE  , 0          },
    {I386_PUNPCKHWD2, Vdq    , Qdq    , NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
  },
  /* PREFIX_GRP_25 */
  {
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {I386_PCMPEQW   , Pq     , Qq     , NULL_OP, TRUE  , 0          },
    {I386_PCMPEQW2  , Vdq    , Wdq    , NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
  },
  /* PREFIX_GRP_26 */
  {
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {I386_PACKUSWB  , Pq     , Qq     , NULL_OP, TRUE  , 0          },
    {I386_PACKUSWB2 , Vdq    , Wdq    , NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
  },
  /* PREFIX_GRP_27 */
  {
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {I386_PACKSSDW  , Pq     , Qq     , NULL_OP, TRUE  , 0          },
    {I386_PACKSSDW2 , Vdq    , Qdq    , NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
  },
  /* PREFIX_GRP_28 */
  {
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {I386_PADDUSB   , Pq     , Qq     , NULL_OP, TRUE  , 0          },
    {I386_PADDUSB2  , Vdq    , Wdq    , NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
  },
  /* PREFIX_GRP_29 */
  {
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {I386_PUNPCKHDQ , Pq     , Qq     , NULL_OP, TRUE  , 0          },
    {I386_PUNPCKHDQ2, Vdq    , Qdq    , NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
  },
  /* PREFIX_GRP_2A */
  {
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {I386_PACKSSWB  , Pq     , Qq     , NULL_OP, TRUE  , 0          },
    {I386_PACKSSWB2 , Vdq    , Wdq    , NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
  },
  /* PREFIX_GRP_2B */
  {
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {I386_PADDW     , Pq     , Qq     , NULL_OP, TRUE  , 0          },
    {I386_PADDW2    , Vdq    , Wdq    , NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
  },
  /* PREFIX_GRP_2C */
  {
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {I386_MOVD3     ,  Ed    , Pd     , NULL_OP, TRUE  , 0          },
    {I386_MOVD4     , Ed     , Vd     , NULL_OP, TRUE  , 0          },
    {I386_MOVQ3     , Vq     , Wq     , NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
  },
  /* PREFIX_GRP_2D */
  {
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {I386_PAVGB     , Pq     , Qq     , NULL_OP, TRUE  , 0          },
    {I386_PAVGB2    , Vdq    , Wdq    , NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
  },
  /* PREFIX_GRP_2E */
  {
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {I386_PSUBSW    , Pq     , Qq     , NULL_OP, TRUE  , 0          },
    {I386_PSUBSW2   , Vdq    , Wdq    , NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
  },
  /* PREFIX_GRP_2F */
  {
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {I386_PMULHW    , Pq     , Qq     , NULL_OP, TRUE  , 0          },
    {I386_PMULHW2   , Vdq    , Wdq    , NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
  },
  /* PREFIX_GRP_30 */
  {
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {I386_PADDSW    , Pq     , Qq     , NULL_OP, TRUE  , 0          },
    {I386_PADDSW2   , Vdq    , Wdq    , NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
  },
  /* PREFIX_GRP_31 */
  {
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {I386_PADDD     , Pq     , Qq     , NULL_OP, TRUE  , 0          },
    {I386_PADDD2    , Vdq    , Wdq    , NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
  },
  /* PREFIX_GRP_32 */
  {
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {I386_PSUBD     , Pq     , Qq     , NULL_OP, TRUE  , 0          },
    {I386_PSUBD2    , Vdq    , Wdq    , NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
  },
  /* PREFIX_GRP_33 */
  {
    {I386_MOVDDUP   , Vq     , Wq     , NULL_OP, TRUE  , 0          },
    {I386_MOVSLDUP  , Vps    , Wps    , NULL_OP, TRUE  , 0          },
    {I386_MOVLPS    , Vq     , Mq     , NULL_OP, TRUE  , 0          },
    {I386_MOVLPD    , Vq     , Mq     , NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
  },
  /* PREFIX_GRP_34 */
  {
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {I386_PSHUFPS   , Vps    , Wps    , Ib     , TRUE  , 0          },
    {I386_PSHUFPD   , Vpd    , Wpd    , Ib     , TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
  },
  /* PREFIX_GRP_35 */
  {
    {I386_ADDSD     , Vsd    , Wsd    , NULL_OP, TRUE  , 0          },
    {I386_ADDSS     , Vss    , Wss    , NULL_OP, TRUE  , 0          },
    {I386_ADDPS     , Vps    , Wps    , NULL_OP, TRUE  , 0          },
    {I386_ADDPD     , Vpd    , Wpd    , NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
  },
  /* PREFIX_GRP_36 */
  {
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {I386_MOVAPS    , Wps    , Vps    , NULL_OP, TRUE  , 0          },
    {I386_MOVAPD    , Wpd    , Vpd    , NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
  },
  /* PREFIX_GRP_37 */
  {
    {I386_SUBSD     , Vsd    , Wsd    , NULL_OP, TRUE  , 0          },
    {I386_SUBSS     , Vss    , Wss    , NULL_OP, TRUE  , 0          },
    {I386_SUBPS     , Vps    , Wps    , NULL_OP, TRUE  , 0          },
    {I386_SUBPD     , Vpd    , Wpd    , NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
  },
  /* PREFIX_GRP_38 */
  {
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {I386_MOVSHDUP  , Vps    , Wps    , NULL_OP, TRUE  , 0          },
    {I386_MOVHPS    , Vq     , Mq     , NULL_OP, TRUE  , 0          },
    {I386_MOVHPD    , Vq     , Mq     , NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
  },
  /* PREFIX_GRP_39 */
  {
    {I386_MULSD     , Vsd    , Wsd    , NULL_OP, TRUE  , 0          },
    {I386_MULSS     , Vss    , Wss    , NULL_OP, TRUE  , 0          },
    {I386_MULPS     , Vps    , Wps    , NULL_OP, TRUE  , 0          },
    {I386_MULPD     , Vpd    , Wpd    , NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
  },
  /* PREFIX_GRP_3A */
  {
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {I386_MOVLPS    , Mq     , Vq     , NULL_OP, TRUE  , 0          },
    {I386_MOVLPD    , Mq     , Vq     , NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
  },
  /* PREFIX_GRP_3B */
  {
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {I386_MOVHPS    , Mq     , Vq     , NULL_OP, TRUE  , 0          },
    {I386_MOVHPD    , Mq     , Vq     , NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
  },
  /* PREFIX_GRP_3C */
  {
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {I386_PSUBUSB   , Pq     , Qq     , NULL_OP, TRUE  , 0          },
    {I386_PSUBUSB2  , Vdq    , Wdq    , NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
  },
  /* PREFIX_GRP_3D */
  {
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {I386_PSUBB     , Pq     , Qq     , NULL_OP, TRUE  , 0          },
    {I386_PSUBB2    , Vdq    , Wdq    , NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
  },
  /* PREFIX_GRP_3E */
  {
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {I386_PSRAW     , Pq     , Ib     , NULL_OP, TRUE  , 0          },
    {I386_PSRAW2    , Pdq    , Ib     , NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
  },
  /* PREFIX_GRP_3F */
  {
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {I386_PSRLW     , Pq     , Ib     , NULL_OP, TRUE  , 0          },
    {I386_PSRLW2    , Pdq    , Ib     , NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
  },
  /* PREFIX_GRP_40 */
  {
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {I386_PANDN     , Pq     , Qq     , NULL_OP, TRUE  , 0          },
    {I386_PANDN2    , Vdq    , Wdq    , NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
  },
  /* PREFIX_GRP_41 */
  {
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {I386_PADDUSW   , Pq     , Qq     , NULL_OP, TRUE  , 0          },
    {I386_PADDUSW2  , Vdq    , Wdq    , NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
  },
  /* PREFIX_GRP_42 */
  {
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {I386_PSUBUSW   , Pq     , Qq     , NULL_OP, TRUE  , 0          },
    {I386_PSUBUSW2  , Vdq    , Wdq    , NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
  },
  /* PREFIX_GRP_43 */
  {
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {I386_PCMPEQB   , Pq     , Qq     , NULL_OP, TRUE  , 0          },
    {I386_PCMPEQB2  , Vdq    , Wdq    , NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
  },
  /* PREFIX_GRP_44 */
  {
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {I386_PMINUB    , Pq     , Qq     , NULL_OP, TRUE  , 0          },
    {I386_PMINUB2   , Vdq    , Wdq    , NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
  },
  /* PREFIX_GRP_45 */
  {
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {I386_PMAXUB    , Pq     , Qq     , NULL_OP, TRUE  , 0          },
    {I386_PMAXUB2   , Vdq    , Wdq    , NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
  },
  /* PREFIX_GRP_46 */
  {
    {I386_DIVSD     , Vsd    , Wsd    , NULL_OP, TRUE  , 0          },
    {I386_DIVSS     , Vss    , Wss    , NULL_OP, TRUE  , 0          },
    {I386_DIVPS     , Vps    , Wps    , NULL_OP, TRUE  , 0          },
    {I386_DIVPD     , Vpd    , Wpd    , NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
  },
  /* PREFIX_GRP_47 */
  {
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {I386_UCOMISS   , Vps    , Wps    , NULL_OP, TRUE  , 0          },
    {I386_UCOMISD   , Vsd    , Wsd    , NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
  },
  /* PREFIX_GRP_48 */
  {
    {I386_SQRTSD    , Vsd    , Wsd    , NULL_OP, TRUE  , 0          },
    {I386_SQRTSS    , Vss    , Wss    , NULL_OP, TRUE  , 0          },
    {I386_SQRTPS    , Vps    , Wps    , NULL_OP, TRUE  , 0          },
    {I386_SQRTPD    , Vpd    , Wpd    , NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
  },
};

t_i386_opcode_entry dis386_3dnow[] = 
{
  /* 00 */
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
  /* 08 */
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {I386_PI2FD     , Vdq, Wdq, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
  /* 10 */
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
  /* 18 */
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {I386_PF2ID     , Vdq, Wdq, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
  /* 20 */
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
  /* 28 */
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
  /* 30 */
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
  /* 38 */
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
  /* 40 */
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
  /* 48 */
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
  /* 50 */
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
  /* 58 */
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
  /* 60 */
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
  /* 68 */
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
  /* 70 */
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
  /* 78 */
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
  /* 80 */
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
  /* 88 */
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {I386_PFPNACC   , Vdq, Wdq, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
  /* 90 */
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
  /* 98 */
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {I386_PFSUB     , Vdq, Wdq, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {I386_PFADD     , Vdq, Wdq, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
  /* A0 */
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
  /* A8 */
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {I386_PFSUBR    , Vdq, Wdq, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {I386_PFACC     , Vdq, Wdq, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
  /* B0 */
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {I386_PFMUL     , Vdq, Wdq, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
  /* B8 */
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {I386_PSWAPD    , Vdq, Wdq, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {I386_PAVGUSB   , Vdq, Wdq, NULL_OP, TRUE  , 0          },
  /* C0 */
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
  /* C8 */
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
  /* d0 */
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
  /* d8 */
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
  /* e0 */
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
  /* e8 */
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
  /* f0 */
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
  /* f8 */
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
};

t_i386_opcode_entry dis386_fpu_mem[][8] = 
{
  /* d8 */
  {
    {I386_FADD      , ST     , Msr    , NULL_OP, TRUE  , DU         },
    {I386_FMUL      , ST     , Msr    , NULL_OP, TRUE  , DU         },
    {I386_FCOM      , NULL_OP, ST     , Msr    , TRUE  , 0          },
    {I386_FCOMP     , NULL_OP, ST     , Msr    , TRUE  , 0          },
    {I386_FSUB      , ST     , Msr    , NULL_OP, TRUE  , DU         },
    {I386_FSUBR     , ST     , Msr    , NULL_OP, TRUE  , DU         },
    {I386_FDIV      , ST     , Msr    , NULL_OP, TRUE  , DU         },
    {I386_FDIVR     , ST     , Msr    , NULL_OP, TRUE  , DU         },
  },
  /* d9 */
  {
    {I386_FLD       , NULL_OP, Msr    , NULL_OP, TRUE  , 0          },
    {INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {I386_FST       , Msr    , ST     , NULL_OP, TRUE  , 0          },
    {I386_FSTP      , Msr    , ST     , NULL_OP, TRUE  , 0          },
    {I386_FLDENV    , NULL_OP, M28    , NULL_OP, TRUE  , 0          },
    {I386_FLDCW     , NULL_OP, M2     , NULL_OP, TRUE  , 0          },
    {I386_FSTENV    , M28    , NULL_OP, NULL_OP, TRUE  , 0          },
    {I386_FSTCW     , M2     , NULL_OP, NULL_OP, TRUE  , 0          },
  },
  /* da */
  {
    {I386_FIADD     , ST     , Md     , NULL_OP, TRUE  , DU         },
    {I386_FIMUL     , ST     , Md     , NULL_OP, TRUE  , DU         },
    {I386_FICOM     , NULL_OP, ST     , Md     , TRUE  , 0          },
    {I386_FICOMP    , NULL_OP, ST     , Md     , TRUE  , 0          },
    {I386_FISUB     , ST     , Md     , NULL_OP, TRUE  , DU         },
    {I386_FISUBR    , ST     , Md     , NULL_OP, TRUE  , DU         },
    {I386_FIDIV     , ST     , Md     , NULL_OP, TRUE  , DU         },
    {I386_FIDIVR    , ST     , Md     , NULL_OP, TRUE  , DU         },
  },
  /* db */
  {
    {I386_FILD      , NULL_OP, Md     , NULL_OP, TRUE  , 0          },
    {INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {I386_FIST      , Md     , ST     , NULL_OP, TRUE  , 0          },
    {I386_FISTP     , Md     , ST     , NULL_OP, TRUE  , 0          },
    {INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {I386_FLD       , NULL_OP, Mer    , NULL_OP, TRUE  , 0          },
    {INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {I386_FSTP      , Mer    , ST     , NULL_OP, TRUE  , 0          },
  },
  /* dc */
  {
    {I386_FADD      , ST     , Mdr    , NULL_OP, TRUE  , DU         },
    {I386_FMUL      , ST     , Mdr    , NULL_OP, TRUE  , DU         },
    {I386_FCOM      , NULL_OP, ST     , Mdr    , TRUE  , 0          },
    {I386_FCOMP     , NULL_OP, ST     , Mdr    , TRUE  , 0          },
    {I386_FSUB      , ST     , Mdr    , NULL_OP, TRUE  , DU         },
    {I386_FSUBR     , ST     , Mdr    , NULL_OP, TRUE  , DU         },
    {I386_FDIV      , ST     , Mdr    , NULL_OP, TRUE  , DU         },
    {I386_FDIVR     , ST     , Mdr    , NULL_OP, TRUE  , DU         },
  },
  /* dd */
  {
    {I386_FLD       , NULL_OP, Mdr    , NULL_OP, TRUE  , 0          },
    {INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {I386_FST       , Mdr    , ST     , NULL_OP, TRUE  , 0          },
    {I386_FSTP      , Mdr    , ST     , NULL_OP, TRUE  , 0          },
    {I386_FRSTOR    , NULL_OP, M108   , NULL_OP, TRUE  , 0          },
    {INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {I386_FSAVE     , M108   , NULL_OP, NULL_OP, TRUE  , 0          },
    {I386_FSTSW     , M2     , NULL_OP, NULL_OP, TRUE  , 0          },
  },
  /* de */
  {
    {I386_FIADD     , ST     , Mw     , NULL_OP, TRUE  , DU         },
    {I386_FIMUL     , ST     , Mw     , NULL_OP, TRUE  , DU         },
    {I386_FICOM     , NULL_OP, ST     , Mw     , TRUE  , 0          },
    {I386_FICOMP    , NULL_OP, ST     , Mw     , TRUE  , 0          },
    {I386_FISUB     , ST     , Mw     , NULL_OP, TRUE  , DU         },
    {I386_FISUBR    , ST     , Mw     , NULL_OP, TRUE  , DU         },
    {I386_FIDIV     , ST     , Mw     , NULL_OP, TRUE  , DU         },
    {I386_FIDIVR    , ST     , Mw     , NULL_OP, TRUE  , DU         },
  },
  /* df */
  {
    {I386_FILD      , NULL_OP, Mw     , NULL_OP, TRUE  , 0          },
    {INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
    {I386_FIST      , Mw     , ST     , NULL_OP, TRUE  , 0          },
    {I386_FISTP     , Mw     , ST     , NULL_OP, TRUE  , 0          },
    {I386_FBLD      , NULL_OP, Mbcd   , NULL_OP, TRUE  , 0          },
    {I386_FILD      , NULL_OP, Mq     , NULL_OP, TRUE  , 0          },
    {I386_FBSTP     , Mbcd   , NULL_OP, NULL_OP, TRUE  , 0          },
    {I386_FISTP     , Mq     , ST     , NULL_OP, TRUE  , 0          },
  }
};

t_i386_opcode_entry dis386_fpu_reg[8][8][8] = 
{
  /* d8 */
  {
    /* reg = 0 */
    {
      {I386_FADD    , ST     , ST0    , NULL_OP, TRUE  , DU         },
      {I386_FADD    , ST     , ST1    , NULL_OP, TRUE  , DU         },
      {I386_FADD    , ST     , ST2    , NULL_OP, TRUE  , DU         },
      {I386_FADD    , ST     , ST3    , NULL_OP, TRUE  , DU         },
      {I386_FADD    , ST     , ST4    , NULL_OP, TRUE  , DU         },
      {I386_FADD    , ST     , ST5    , NULL_OP, TRUE  , DU         },
      {I386_FADD    , ST     , ST6    , NULL_OP, TRUE  , DU         },
      {I386_FADD    , ST     , ST7    , NULL_OP, TRUE  , DU         }
    },                                                 
    /* reg = 1 */                                      
    {                                                  
      {I386_FMUL    , ST     , ST0    , NULL_OP, TRUE  , DU         },
      {I386_FMUL    , ST     , ST1    , NULL_OP, TRUE  , DU         },
      {I386_FMUL    , ST     , ST2    , NULL_OP, TRUE  , DU         },
      {I386_FMUL    , ST     , ST3    , NULL_OP, TRUE  , DU         },
      {I386_FMUL    , ST     , ST4    , NULL_OP, TRUE  , DU         },
      {I386_FMUL    , ST     , ST5    , NULL_OP, TRUE  , DU         },
      {I386_FMUL    , ST     , ST6    , NULL_OP, TRUE  , DU         },
      {I386_FMUL    , ST     , ST7    , NULL_OP, TRUE  , DU         }
    },                                                 
    /* reg = 2 */                                      
    {                                                  
      {I386_FCOM    , NULL_OP, ST     , ST0    , TRUE  , 0          },
      {I386_FCOM    , NULL_OP, ST     , ST1    , TRUE  , 0          },
      {I386_FCOM    , NULL_OP, ST     , ST2    , TRUE  , 0          },
      {I386_FCOM    , NULL_OP, ST     , ST3    , TRUE  , 0          },
      {I386_FCOM    , NULL_OP, ST     , ST4    , TRUE  , 0          },
      {I386_FCOM    , NULL_OP, ST     , ST5    , TRUE  , 0          },
      {I386_FCOM    , NULL_OP, ST     , ST6    , TRUE  , 0          },
      {I386_FCOM    , NULL_OP, ST     , ST7    , TRUE  , 0          }
    },                                                 
    /* reg = 3 */                                      
    {                                                  
      {I386_FCOMP   , NULL_OP, ST     , ST0    , TRUE  , 0          },
      {I386_FCOMP   , NULL_OP, ST     , ST1    , TRUE  , 0          },
      {I386_FCOMP   , NULL_OP, ST     , ST2    , TRUE  , 0          },
      {I386_FCOMP   , NULL_OP, ST     , ST3    , TRUE  , 0          },
      {I386_FCOMP   , NULL_OP, ST     , ST4    , TRUE  , 0          },
      {I386_FCOMP   , NULL_OP, ST     , ST5    , TRUE  , 0          },
      {I386_FCOMP   , NULL_OP, ST     , ST6    , TRUE  , 0          },
      {I386_FCOMP   , NULL_OP, ST     , ST7    , TRUE  , 0          }
    },                                                 
    /* reg = 4 */                                      
    {                                                  
      {I386_FSUB    , ST     , ST0    , NULL_OP, TRUE  , DU         },
      {I386_FSUB    , ST     , ST1    , NULL_OP, TRUE  , DU         },
      {I386_FSUB    , ST     , ST2    , NULL_OP, TRUE  , DU         },
      {I386_FSUB    , ST     , ST3    , NULL_OP, TRUE  , DU         },
      {I386_FSUB    , ST     , ST4    , NULL_OP, TRUE  , DU         },
      {I386_FSUB    , ST     , ST5    , NULL_OP, TRUE  , DU         },
      {I386_FSUB    , ST     , ST6    , NULL_OP, TRUE  , DU         },
      {I386_FSUB    , ST     , ST7    , NULL_OP, TRUE  , DU         }
    },                                                 
    /* reg = 5 */                                      
    {                                                  
      {I386_FSUBR   , ST     , ST0    , NULL_OP, TRUE  , DU         },
      {I386_FSUBR   , ST     , ST1    , NULL_OP, TRUE  , DU         },
      {I386_FSUBR   , ST     , ST2    , NULL_OP, TRUE  , DU         },
      {I386_FSUBR   , ST     , ST3    , NULL_OP, TRUE  , DU         },
      {I386_FSUBR   , ST     , ST4    , NULL_OP, TRUE  , DU         },
      {I386_FSUBR   , ST     , ST5    , NULL_OP, TRUE  , DU         },
      {I386_FSUBR   , ST     , ST6    , NULL_OP, TRUE  , DU         },
      {I386_FSUBR   , ST     , ST7    , NULL_OP, TRUE  , DU         }
    },                                                 
    /* reg = 6 */                                      
    {                                                  
      {I386_FDIV    , ST     , ST0    , NULL_OP, TRUE  , DU         },
      {I386_FDIV    , ST     , ST1    , NULL_OP, TRUE  , DU         },
      {I386_FDIV    , ST     , ST2    , NULL_OP, TRUE  , DU         },
      {I386_FDIV    , ST     , ST3    , NULL_OP, TRUE  , DU         },
      {I386_FDIV    , ST     , ST4    , NULL_OP, TRUE  , DU         },
      {I386_FDIV    , ST     , ST5    , NULL_OP, TRUE  , DU         },
      {I386_FDIV    , ST     , ST6    , NULL_OP, TRUE  , DU         },
      {I386_FDIV    , ST     , ST7    , NULL_OP, TRUE  , DU         }
    },                                                 
    /* reg = 7 */                                      
    {                                                  
      {I386_FDIVR   , ST     , ST0    , NULL_OP, TRUE  , DU         },
      {I386_FDIVR   , ST     , ST1    , NULL_OP, TRUE  , DU         },
      {I386_FDIVR   , ST     , ST2    , NULL_OP, TRUE  , DU         },
      {I386_FDIVR   , ST     , ST3    , NULL_OP, TRUE  , DU         },
      {I386_FDIVR   , ST     , ST4    , NULL_OP, TRUE  , DU         },
      {I386_FDIVR   , ST     , ST5    , NULL_OP, TRUE  , DU         },
      {I386_FDIVR   , ST     , ST6    , NULL_OP, TRUE  , DU         },
      {I386_FDIVR   , ST     , ST7    , NULL_OP, TRUE  , DU         }
    }
  },
  /* d9 */
  {
    /* reg = 0 */
    {
      {I386_FLD     , ST     , ST0    , NULL_OP, TRUE  , 0          },
      {I386_FLD     , ST     , ST1    , NULL_OP, TRUE  , 0          },
      {I386_FLD     , ST     , ST2    , NULL_OP, TRUE  , 0          },
      {I386_FLD     , ST     , ST3    , NULL_OP, TRUE  , 0          },
      {I386_FLD     , ST     , ST4    , NULL_OP, TRUE  , 0          },
      {I386_FLD     , ST     , ST5    , NULL_OP, TRUE  , 0          },
      {I386_FLD     , ST     , ST6    , NULL_OP, TRUE  , 0          },
      {I386_FLD     , ST     , ST7    , NULL_OP, TRUE  , 0          }
    },                                                 
    /* reg = 1 */                                      
    {                                                  
      {I386_FXCH    , ST     , ST0    , NULL_OP, TRUE  , DU|S1D     },
      {I386_FXCH    , ST     , ST1    , NULL_OP, TRUE  , DU|S1D     },
      {I386_FXCH    , ST     , ST2    , NULL_OP, TRUE  , DU|S1D     },
      {I386_FXCH    , ST     , ST3    , NULL_OP, TRUE  , DU|S1D     },
      {I386_FXCH    , ST     , ST4    , NULL_OP, TRUE  , DU|S1D     },
      {I386_FXCH    , ST     , ST5    , NULL_OP, TRUE  , DU|S1D     },
      {I386_FXCH    , ST     , ST6    , NULL_OP, TRUE  , DU|S1D     },
      {I386_FXCH    , ST     , ST7    , NULL_OP, TRUE  , DU|S1D     }
    },                                                 
    /* reg = 2 */                                      
    {                                                  
      {I386_FNOP    , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
      {INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
      {INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
      {INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
      {INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
      {INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
      {INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
      {INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          }
    },                                                 
    /* reg = 3 */                                      
    {                                                  
      {INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
      {INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
      {INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
      {INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
      {INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
      {INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
      {INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
      {INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          }
    },                                                 
    /* reg = 4 */                                      
    {                                                  
      {I386_FCHS    , ST     , NULL_OP, NULL_OP, TRUE  , DU         },
      {I386_FABS    , ST     , NULL_OP, NULL_OP, TRUE  , DU         },
      {INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
      {INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
      {I386_FTST    , NULL_OP, ST     , NULL_OP, TRUE  , 0          },
      {I386_FXAM    , NULL_OP, ST     , NULL_OP, TRUE  , 0          },
      {INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
      {INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          }
    },                                                 
    /* reg = 5 */                                      
    {                                                  
      {I386_FLD1    , ST     , NULL_OP, NULL_OP, TRUE  , 0          },
      {I386_FLDL2T  , ST     , NULL_OP, NULL_OP, TRUE  , 0          },
      {I386_FLDL2E  , ST     , NULL_OP, NULL_OP, TRUE  , 0          },
      {I386_FLDPI   , ST     , NULL_OP, NULL_OP, TRUE  , 0          },
      {I386_FLDLG2  , ST     , NULL_OP, NULL_OP, TRUE  , 0          },
      {I386_FLDLN2  , ST     , NULL_OP, NULL_OP, TRUE  , 0          },
      {I386_FLDZ    , ST     , NULL_OP, NULL_OP, TRUE  , 0          },
      {INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          }
    },                                                 
    /* reg = 6 */                                      
    {                                                  
      {I386_F2XM1   , ST     , NULL_OP, NULL_OP, TRUE  , DU         },
      {I386_FYL2X   , ST     , ST1    , NULL_OP, TRUE  , DU         },
      {I386_FPTAN   , ST     , NULL_OP, NULL_OP, TRUE  , DU         },
      {I386_FPATAN  , ST     , ST1    , NULL_OP, TRUE  , DU         },
      {I386_FXTRACT , ST     , NULL_OP, NULL_OP, TRUE  , DU         },
      {I386_FPREM1  , ST     , ST1    , NULL_OP, TRUE  , DU         },
      {I386_FDECSTP , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
      {I386_FINCSTP , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          }
    },                                                 
    /* reg = 7 */                                      
    {                                                  
      {I386_FPREM   , ST     , ST1    , NULL_OP, TRUE  , DU         },
      {I386_FYL2XP1 , ST     , ST1    , NULL_OP, TRUE  , DU         },
      {I386_FSQRT   , ST     , NULL_OP, NULL_OP, TRUE  , DU         },
      {I386_FSINCOS , ST     , NULL_OP, NULL_OP, TRUE  , DU         },
      {I386_FRNDINT , ST     , NULL_OP, NULL_OP, TRUE  , DU         },
      {I386_FSCALE  , ST     , ST1    , NULL_OP, TRUE  , DU         },
      {I386_FSIN    , ST     , NULL_OP, NULL_OP, TRUE  , DU         },
      {I386_FCOS    , ST     , NULL_OP, NULL_OP, TRUE  , DU         }
    }
  },
  /* da */
  {
    /* reg = 0 */
    {
      {I386_FCMOVcc , ST     , ST0    , NULL_OP, TRUE  , 0          },
      {I386_FCMOVcc , ST     , ST1    , NULL_OP, TRUE  , 0          },
      {I386_FCMOVcc , ST     , ST2    , NULL_OP, TRUE  , 0          },
      {I386_FCMOVcc , ST     , ST3    , NULL_OP, TRUE  , 0          },
      {I386_FCMOVcc , ST     , ST4    , NULL_OP, TRUE  , 0          },
      {I386_FCMOVcc , ST     , ST5    , NULL_OP, TRUE  , 0          },
      {I386_FCMOVcc , ST     , ST6    , NULL_OP, TRUE  , 0          },
      {I386_FCMOVcc , ST     , ST7    , NULL_OP, TRUE  , 0          }
    },                                                 
    /* reg = 1 */                                      
    {                                                  
      {I386_FCMOVcc , ST     , ST0    , NULL_OP, TRUE  , 0          },
      {I386_FCMOVcc , ST     , ST1    , NULL_OP, TRUE  , 0          },
      {I386_FCMOVcc , ST     , ST2    , NULL_OP, TRUE  , 0          },
      {I386_FCMOVcc , ST     , ST3    , NULL_OP, TRUE  , 0          },
      {I386_FCMOVcc , ST     , ST4    , NULL_OP, TRUE  , 0          },
      {I386_FCMOVcc , ST     , ST5    , NULL_OP, TRUE  , 0          },
      {I386_FCMOVcc , ST     , ST6    , NULL_OP, TRUE  , 0          },
      {I386_FCMOVcc , ST     , ST7    , NULL_OP, TRUE  , 0          }
    },                                                 
    /* reg = 2 */                                      
    {                                                  
      {I386_FCMOVcc , ST     , ST0    , NULL_OP, TRUE  , 0          },
      {I386_FCMOVcc , ST     , ST1    , NULL_OP, TRUE  , 0          },
      {I386_FCMOVcc , ST     , ST2    , NULL_OP, TRUE  , 0          },
      {I386_FCMOVcc , ST     , ST3    , NULL_OP, TRUE  , 0          },
      {I386_FCMOVcc , ST     , ST4    , NULL_OP, TRUE  , 0          },
      {I386_FCMOVcc , ST     , ST5    , NULL_OP, TRUE  , 0          },
      {I386_FCMOVcc , ST     , ST6    , NULL_OP, TRUE  , 0          },
      {I386_FCMOVcc , ST     , ST7    , NULL_OP, TRUE  , 0          }
    },                                                 
    /* reg = 3 */                                      
    {                                                  
      {I386_FCMOVcc , ST     , ST0    , NULL_OP, TRUE  , 0          },
      {I386_FCMOVcc , ST     , ST1    , NULL_OP, TRUE  , 0          },
      {I386_FCMOVcc , ST     , ST2    , NULL_OP, TRUE  , 0          },
      {I386_FCMOVcc , ST     , ST3    , NULL_OP, TRUE  , 0          },
      {I386_FCMOVcc , ST     , ST4    , NULL_OP, TRUE  , 0          },
      {I386_FCMOVcc , ST     , ST5    , NULL_OP, TRUE  , 0          },
      {I386_FCMOVcc , ST     , ST6    , NULL_OP, TRUE  , 0          },
      {I386_FCMOVcc , ST     , ST7    , NULL_OP, TRUE  , 0          }
    },                                                 
    /* reg = 4 */                                      
    {                                                  
      {INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
      {INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
      {INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
      {INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
      {INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
      {INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
      {INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
      {INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          }
    },                                                 
    /* reg = 5 */                                      
    {                                                  
      {INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
      {I386_FUCOMPP , NULL_OP, ST     , ST1    , TRUE  , 0          },
      {INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
      {INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
      {INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
      {INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
      {INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
      {INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          }
    },                                                 
    /* reg = 6 */                                      
    {                                                  
      {INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
      {INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
      {INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
      {INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
      {INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
      {INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
      {INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
      {INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          }
    },                                                 
    /* reg = 7 */                                      
    {                                                  
      {INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
      {INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
      {INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
      {INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
      {INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
      {INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
      {INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
      {INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          }
    }
  },
  /* db */
  {
    /* reg = 0 */
    {
      {I386_FCMOVcc , ST     , ST0    , NULL_OP, TRUE  , 0          },
      {I386_FCMOVcc , ST     , ST1    , NULL_OP, TRUE  , 0          },
      {I386_FCMOVcc , ST     , ST2    , NULL_OP, TRUE  , 0          },
      {I386_FCMOVcc , ST     , ST3    , NULL_OP, TRUE  , 0          },
      {I386_FCMOVcc , ST     , ST4    , NULL_OP, TRUE  , 0          },
      {I386_FCMOVcc , ST     , ST5    , NULL_OP, TRUE  , 0          },
      {I386_FCMOVcc , ST     , ST6    , NULL_OP, TRUE  , 0          },
      {I386_FCMOVcc , ST     , ST7    , NULL_OP, TRUE  , 0          }
    },                                                 
    /* reg = 1 */                                      
    {                                                  
      {I386_FCOMI   , NULL_OP, ST     , ST0    , TRUE  , 0          },
      {I386_FCOMI   , NULL_OP, ST     , ST1    , TRUE  , 0          },
      {I386_FCOMI   , NULL_OP, ST     , ST2    , TRUE  , 0          },
      {I386_FCOMI   , NULL_OP, ST     , ST3    , TRUE  , 0          },
      {I386_FCOMI   , NULL_OP, ST     , ST4    , TRUE  , 0          },
      {I386_FCOMI   , NULL_OP, ST     , ST5    , TRUE  , 0          },
      {I386_FCOMI   , NULL_OP, ST     , ST6    , TRUE  , 0          },
      {I386_FCOMI   , NULL_OP, ST     , ST7    , TRUE  , 0          }
    },                                                 
    /* reg = 2 */                                      
    {                                                  
      {I386_FCMOVcc , ST     , ST0    , NULL_OP, TRUE  , 0          },
      {I386_FCMOVcc , ST     , ST1    , NULL_OP, TRUE  , 0          },
      {I386_FCMOVcc , ST     , ST2    , NULL_OP, TRUE  , 0          },
      {I386_FCMOVcc , ST     , ST3    , NULL_OP, TRUE  , 0          },
      {I386_FCMOVcc , ST     , ST4    , NULL_OP, TRUE  , 0          },
      {I386_FCMOVcc , ST     , ST5    , NULL_OP, TRUE  , 0          },
      {I386_FCMOVcc , ST     , ST6    , NULL_OP, TRUE  , 0          },
      {I386_FCMOVcc , ST     , ST7    , NULL_OP, TRUE  , 0          }
    },                                                 
    /* reg = 3 */                                      
    {                                                  
      {I386_FCMOVcc , ST     , ST0    , NULL_OP, TRUE  , 0          },
      {I386_FCMOVcc , ST     , ST1    , NULL_OP, TRUE  , 0          },
      {I386_FCMOVcc , ST     , ST2    , NULL_OP, TRUE  , 0          },
      {I386_FCMOVcc , ST     , ST3    , NULL_OP, TRUE  , 0          },
      {I386_FCMOVcc , ST     , ST4    , NULL_OP, TRUE  , 0          },
      {I386_FCMOVcc , ST     , ST5    , NULL_OP, TRUE  , 0          },
      {I386_FCMOVcc , ST     , ST6    , NULL_OP, TRUE  , 0          },
      {I386_FCMOVcc , ST     , ST7    , NULL_OP, TRUE  , 0          }
    },                                                 
    /* reg = 4 */                                      
    {                                                  
      {INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
      {INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
      {I386_FCLEX   , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
      {I386_FINIT   , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
      {INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
      {INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
      {INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
      {INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          }
    },                                                 
    /* reg = 5 */                                      
    {                                                  
      {I386_FUCOMI  , NULL_OP, ST     , ST0    , TRUE  , 0          },
      {I386_FUCOMI  , NULL_OP, ST     , ST1    , TRUE  , 0          },
      {I386_FUCOMI  , NULL_OP, ST     , ST2    , TRUE  , 0          },
      {I386_FUCOMI  , NULL_OP, ST     , ST3    , TRUE  , 0          },
      {I386_FUCOMI  , NULL_OP, ST     , ST4    , TRUE  , 0          },
      {I386_FUCOMI  , NULL_OP, ST     , ST5    , TRUE  , 0          },
      {I386_FUCOMI  , NULL_OP, ST     , ST6    , TRUE  , 0          },
      {I386_FUCOMI  , NULL_OP, ST     , ST7    , TRUE  , 0          }
    },                                                 
    /* reg = 6 */                                      
    {                                                  
      {I386_FCMOVcc , ST     , ST0    , NULL_OP, TRUE  , 0          },
      {I386_FCMOVcc , ST     , ST1    , NULL_OP, TRUE  , 0          },
      {I386_FCMOVcc , ST     , ST2    , NULL_OP, TRUE  , 0          },
      {I386_FCMOVcc , ST     , ST3    , NULL_OP, TRUE  , 0          },
      {I386_FCMOVcc , ST     , ST4    , NULL_OP, TRUE  , 0          },
      {I386_FCMOVcc , ST     , ST5    , NULL_OP, TRUE  , 0          },
      {I386_FCMOVcc , ST     , ST6    , NULL_OP, TRUE  , 0          },
      {I386_FCMOVcc , ST     , ST7    , NULL_OP, TRUE  , 0          }
    },                                                 
    /* reg = 7 */                                      
    {                                                  
      {INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
      {INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
      {INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
      {INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
      {INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
      {INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
      {INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
      {INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          }
    }
  },
  /* dc */
  {
    /* reg = 0 */
    {
      {I386_FADD    , ST0    , ST     , NULL_OP, TRUE  , DU         },
      {I386_FADD    , ST1    , ST     , NULL_OP, TRUE  , DU         },
      {I386_FADD    , ST2    , ST     , NULL_OP, TRUE  , DU         },
      {I386_FADD    , ST3    , ST     , NULL_OP, TRUE  , DU         },
      {I386_FADD    , ST4    , ST     , NULL_OP, TRUE  , DU         },
      {I386_FADD    , ST5    , ST     , NULL_OP, TRUE  , DU         },
      {I386_FADD    , ST6    , ST     , NULL_OP, TRUE  , DU         },
      {I386_FADD    , ST7    , ST     , NULL_OP, TRUE  , DU         }
    },                                                 
    /* reg = 1 */                                      
    {                                                  
      {I386_FMUL    , ST0    , ST     , NULL_OP, TRUE  , DU         },
      {I386_FMUL    , ST1    , ST     , NULL_OP, TRUE  , DU         },
      {I386_FMUL    , ST2    , ST     , NULL_OP, TRUE  , DU         },
      {I386_FMUL    , ST3    , ST     , NULL_OP, TRUE  , DU         },
      {I386_FMUL    , ST4    , ST     , NULL_OP, TRUE  , DU         },
      {I386_FMUL    , ST5    , ST     , NULL_OP, TRUE  , DU         },
      {I386_FMUL    , ST6    , ST     , NULL_OP, TRUE  , DU         },
      {I386_FMUL    , ST7    , ST     , NULL_OP, TRUE  , DU         }
    },                                                 
    /* reg = 2 */                                      
    {                                                  
      {INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
      {INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
      {INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
      {INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
      {INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
      {INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
      {INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
      {INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          }
    },                                                 
    /* reg = 3 */                                      
    {                                                  
      {INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
      {INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
      {INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
      {INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
      {INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
      {INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
      {INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
      {INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          }
    },                                                 
    /* reg = 4 */                                      
    {                                                  
      {I386_FSUBR   , ST0    , ST     , NULL_OP, TRUE  , DU         },
      {I386_FSUBR   , ST1    , ST     , NULL_OP, TRUE  , DU         },
      {I386_FSUBR   , ST2    , ST     , NULL_OP, TRUE  , DU         },
      {I386_FSUBR   , ST3    , ST     , NULL_OP, TRUE  , DU         },
      {I386_FSUBR   , ST4    , ST     , NULL_OP, TRUE  , DU         },
      {I386_FSUBR   , ST5    , ST     , NULL_OP, TRUE  , DU         },
      {I386_FSUBR   , ST6    , ST     , NULL_OP, TRUE  , DU         },
      {I386_FSUBR   , ST7    , ST     , NULL_OP, TRUE  , DU         }
    },                                                 
    /* reg = 5 */                                      
    {                                                  
      {I386_FSUB    , ST0    , ST     , NULL_OP, TRUE  , DU         },
      {I386_FSUB    , ST1    , ST     , NULL_OP, TRUE  , DU         },
      {I386_FSUB    , ST2    , ST     , NULL_OP, TRUE  , DU         },
      {I386_FSUB    , ST3    , ST     , NULL_OP, TRUE  , DU         },
      {I386_FSUB    , ST4    , ST     , NULL_OP, TRUE  , DU         },
      {I386_FSUB    , ST5    , ST     , NULL_OP, TRUE  , DU         },
      {I386_FSUB    , ST6    , ST     , NULL_OP, TRUE  , DU         },
      {I386_FSUB    , ST7    , ST     , NULL_OP, TRUE  , DU         }
    },                                                 
    /* reg = 6 */                                      
    {                                                  
      {I386_FDIVR   , ST0    , ST     , NULL_OP, TRUE  , DU         },
      {I386_FDIVR   , ST1    , ST     , NULL_OP, TRUE  , DU         },
      {I386_FDIVR   , ST2    , ST     , NULL_OP, TRUE  , DU         },
      {I386_FDIVR   , ST3    , ST     , NULL_OP, TRUE  , DU         },
      {I386_FDIVR   , ST4    , ST     , NULL_OP, TRUE  , DU         },
      {I386_FDIVR   , ST5    , ST     , NULL_OP, TRUE  , DU         },
      {I386_FDIVR   , ST6    , ST     , NULL_OP, TRUE  , DU         },
      {I386_FDIVR   , ST7    , ST     , NULL_OP, TRUE  , DU         }
    },                                                 
    /* reg = 7 */                                      
    {                                                  
      {I386_FDIV    , ST0    , ST     , NULL_OP, TRUE  , DU         },
      {I386_FDIV    , ST1    , ST     , NULL_OP, TRUE  , DU         },
      {I386_FDIV    , ST2    , ST     , NULL_OP, TRUE  , DU         },
      {I386_FDIV    , ST3    , ST     , NULL_OP, TRUE  , DU         },
      {I386_FDIV    , ST4    , ST     , NULL_OP, TRUE  , DU         },
      {I386_FDIV    , ST5    , ST     , NULL_OP, TRUE  , DU         },
      {I386_FDIV    , ST6    , ST     , NULL_OP, TRUE  , DU         },
      {I386_FDIV    , ST7    , ST     , NULL_OP, TRUE  , DU         }
    }
  },
  /* dd */
  {
    /* reg = 0 */
    {
      {I386_FFREE   , ST0    , NULL_OP, NULL_OP, TRUE  , 0          },
      {I386_FFREE   , ST1    , NULL_OP, NULL_OP, TRUE  , 0          },
      {I386_FFREE   , ST2    , NULL_OP, NULL_OP, TRUE  , 0          },
      {I386_FFREE   , ST3    , NULL_OP, NULL_OP, TRUE  , 0          },
      {I386_FFREE   , ST4    , NULL_OP, NULL_OP, TRUE  , 0          },
      {I386_FFREE   , ST5    , NULL_OP, NULL_OP, TRUE  , 0          },
      {I386_FFREE   , ST6    , NULL_OP, NULL_OP, TRUE  , 0          },
      {I386_FFREE   , ST7    , NULL_OP, NULL_OP, TRUE  , 0          }
    },                                                 
    /* reg = 1 */                                      
    {                                                  
      {INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
      {INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
      {INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
      {INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
      {INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
      {INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
      {INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
      {INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          }
    },                                                 
    /* reg = 2 */                                      
    {                                                  
      {I386_FST     , ST0    , ST     , NULL_OP, TRUE  , 0          },
      {I386_FST     , ST1    , ST     , NULL_OP, TRUE  , 0          },
      {I386_FST     , ST2    , ST     , NULL_OP, TRUE  , 0          },
      {I386_FST     , ST3    , ST     , NULL_OP, TRUE  , 0          },
      {I386_FST     , ST4    , ST     , NULL_OP, TRUE  , 0          },
      {I386_FST     , ST5    , ST     , NULL_OP, TRUE  , 0          },
      {I386_FST     , ST6    , ST     , NULL_OP, TRUE  , 0          },
      {I386_FST     , ST7    , ST     , NULL_OP, TRUE  , 0          }
    },                                                 
    /* reg = 3 */                                      
    {                                                  
      {I386_FSTP    , ST0    , ST     , NULL_OP, TRUE  , 0          },
      {I386_FSTP    , ST1    , ST     , NULL_OP, TRUE  , 0          },
      {I386_FSTP    , ST2    , ST     , NULL_OP, TRUE  , 0          },
      {I386_FSTP    , ST3    , ST     , NULL_OP, TRUE  , 0          },
      {I386_FSTP    , ST4    , ST     , NULL_OP, TRUE  , 0          },
      {I386_FSTP    , ST5    , ST     , NULL_OP, TRUE  , 0          },
      {I386_FSTP    , ST6    , ST     , NULL_OP, TRUE  , 0          },
      {I386_FSTP    , ST7    , ST     , NULL_OP, TRUE  , 0          }
    },                                                 
    /* reg = 4 */                                      
    {                                                  
      {I386_FUCOM   , NULL_OP, ST     , ST0    , TRUE  , 0          },
      {I386_FUCOM   , NULL_OP, ST     , ST1    , TRUE  , 0          },
      {I386_FUCOM   , NULL_OP, ST     , ST2    , TRUE  , 0          },
      {I386_FUCOM   , NULL_OP, ST     , ST3    , TRUE  , 0          },
      {I386_FUCOM   , NULL_OP, ST     , ST4    , TRUE  , 0          },
      {I386_FUCOM   , NULL_OP, ST     , ST5    , TRUE  , 0          },
      {I386_FUCOM   , NULL_OP, ST     , ST6    , TRUE  , 0          },
      {I386_FUCOM   , NULL_OP, ST     , ST7    , TRUE  , 0          }
    },                                                 
    /* reg = 5 */                                      
    {                                                  
      {I386_FUCOMP  , NULL_OP, ST     , ST0    , TRUE  , 0          },
      {I386_FUCOMP  , NULL_OP, ST     , ST1    , TRUE  , 0          },
      {I386_FUCOMP  , NULL_OP, ST     , ST2    , TRUE  , 0          },
      {I386_FUCOMP  , NULL_OP, ST     , ST3    , TRUE  , 0          },
      {I386_FUCOMP  , NULL_OP, ST     , ST4    , TRUE  , 0          },
      {I386_FUCOMP  , NULL_OP, ST     , ST5    , TRUE  , 0          },
      {I386_FUCOMP  , NULL_OP, ST     , ST6    , TRUE  , 0          },
      {I386_FUCOMP  , NULL_OP, ST     , ST7    , TRUE  , 0          }
    },                                                 
    /* reg = 6 */                                      
    {                                                  
      {INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
      {INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
      {INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
      {INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
      {INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
      {INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
      {INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
      {INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          }
    },                                                 
    /* reg = 7 */                                      
    {                                                  
      {INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
      {INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
      {INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
      {INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
      {INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
      {INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
      {INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
      {INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          }
    }
  },
  /* de */
  {
    /* reg = 0 */
    {
      {I386_FADDP   , ST0    , ST     , NULL_OP, TRUE  , DU         },
      {I386_FADDP   , ST1    , ST     , NULL_OP, TRUE  , DU         },
      {I386_FADDP   , ST2    , ST     , NULL_OP, TRUE  , DU         },
      {I386_FADDP   , ST3    , ST     , NULL_OP, TRUE  , DU         },
      {I386_FADDP   , ST4    , ST     , NULL_OP, TRUE  , DU         },
      {I386_FADDP   , ST5    , ST     , NULL_OP, TRUE  , DU         },
      {I386_FADDP   , ST6    , ST     , NULL_OP, TRUE  , DU         },
      {I386_FADDP   , ST7    , ST     , NULL_OP, TRUE  , DU         }
    },                                                 
    /* reg = 1 */                                      
    {                                                  
      {I386_FMULP   , ST0    , ST     , NULL_OP, TRUE  , DU         },
      {I386_FMULP   , ST1    , ST     , NULL_OP, TRUE  , DU         },
      {I386_FMULP   , ST2    , ST     , NULL_OP, TRUE  , DU         },
      {I386_FMULP   , ST3    , ST     , NULL_OP, TRUE  , DU         },
      {I386_FMULP   , ST4    , ST     , NULL_OP, TRUE  , DU         },
      {I386_FMULP   , ST5    , ST     , NULL_OP, TRUE  , DU         },
      {I386_FMULP   , ST6    , ST     , NULL_OP, TRUE  , DU         },
      {I386_FMULP   , ST7    , ST     , NULL_OP, TRUE  , DU         }
    },                                                 
    /* reg = 2 */                                      
    {                                                  
      {INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
      {INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
      {INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
      {INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
      {INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
      {INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
      {INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
      {INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          }
    },                                                 
    /* reg = 3 */                                      
    {                                                  
      {INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
      {I386_FCOMPP  , NULL_OP, ST     , ST1    , TRUE  , 0          },
      {INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
      {INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
      {INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
      {INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
      {INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
      {INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          }
    },                                                 
    /* reg = 4 */                                      
    {                                                  
      {I386_FSUBRP  , ST0    , ST     , NULL_OP, TRUE  , DU         },
      {I386_FSUBRP  , ST1    , ST     , NULL_OP, TRUE  , DU         },
      {I386_FSUBRP  , ST2    , ST     , NULL_OP, TRUE  , DU         },
      {I386_FSUBRP  , ST3    , ST     , NULL_OP, TRUE  , DU         },
      {I386_FSUBRP  , ST4    , ST     , NULL_OP, TRUE  , DU         },
      {I386_FSUBRP  , ST5    , ST     , NULL_OP, TRUE  , DU         },
      {I386_FSUBRP  , ST6    , ST     , NULL_OP, TRUE  , DU         },
      {I386_FSUBRP  , ST7    , ST     , NULL_OP, TRUE  , DU         }
    },                                                 
    /* reg = 5 */                                      
    {                                                  
      {I386_FSUBP   , ST0    , ST     , NULL_OP, TRUE  , DU         },
      {I386_FSUBP   , ST1    , ST     , NULL_OP, TRUE  , DU         },
      {I386_FSUBP   , ST2    , ST     , NULL_OP, TRUE  , DU         },
      {I386_FSUBP   , ST3    , ST     , NULL_OP, TRUE  , DU         },
      {I386_FSUBP   , ST4    , ST     , NULL_OP, TRUE  , DU         },
      {I386_FSUBP   , ST5    , ST     , NULL_OP, TRUE  , DU         },
      {I386_FSUBP   , ST6    , ST     , NULL_OP, TRUE  , DU         },
      {I386_FSUBP   , ST7    , ST     , NULL_OP, TRUE  , DU         }
    },                                                 
    /* reg = 6 */                                      
    {                                                  
      {I386_FDIVRP  , ST0    , ST     , NULL_OP, TRUE  , DU         },
      {I386_FDIVRP  , ST1    , ST     , NULL_OP, TRUE  , DU         },
      {I386_FDIVRP  , ST2    , ST     , NULL_OP, TRUE  , DU         },
      {I386_FDIVRP  , ST3    , ST     , NULL_OP, TRUE  , DU         },
      {I386_FDIVRP  , ST4    , ST     , NULL_OP, TRUE  , DU         },
      {I386_FDIVRP  , ST5    , ST     , NULL_OP, TRUE  , DU         },
      {I386_FDIVRP  , ST6    , ST     , NULL_OP, TRUE  , DU         },
      {I386_FDIVRP  , ST7    , ST     , NULL_OP, TRUE  , DU         }
    },                                                 
    /* reg = 7 */                                      
    {                                                  
      {I386_FDIVP   , ST0    , ST     , NULL_OP, TRUE  , DU         },
      {I386_FDIVP   , ST1    , ST     , NULL_OP, TRUE  , DU         },
      {I386_FDIVP   , ST2    , ST     , NULL_OP, TRUE  , DU         },
      {I386_FDIVP   , ST3    , ST     , NULL_OP, TRUE  , DU         },
      {I386_FDIVP   , ST4    , ST     , NULL_OP, TRUE  , DU         },
      {I386_FDIVP   , ST5    , ST     , NULL_OP, TRUE  , DU         },
      {I386_FDIVP   , ST6    , ST     , NULL_OP, TRUE  , DU         },
      {I386_FDIVP   , ST7    , ST     , NULL_OP, TRUE  , DU         }
    }
  },
  /* df */
  {
    /* reg = 0 */
    {
      {INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
      {INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
      {INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
      {INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
      {INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
      {INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
      {INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
      {INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          }
    },                                                 
    /* reg = 1 */                                      
    {                                                  
      {INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
      {INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
      {INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
      {INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
      {INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
      {INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
      {INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
      {INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          }
    },                                                 
    /* reg = 2 */                                      
    {                                                  
      {INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
      {INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
      {INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
      {INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
      {INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
      {INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
      {INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
      {INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          }
    },                                                 
    /* reg = 3 */                                      
    {                                                  
      {INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
      {INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
      {INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
      {INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
      {INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
      {INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
      {INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
      {INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          }
    },                                                 
    /* reg = 4 */                                      
    {                                                  
      {I386_FSTSW   , AX     , NULL_OP, NULL_OP, TRUE  , 0          },
      {INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
      {INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
      {INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
      {INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
      {INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
      {INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
      {INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          }
    },                                                 
    /* reg = 5 */                                      
    {                                                  
      {I386_FUCOMIP , NULL_OP, ST     , ST0    , TRUE  , 0          },
      {I386_FUCOMIP , NULL_OP, ST     , ST1    , TRUE  , 0          },
      {I386_FUCOMIP , NULL_OP, ST     , ST2    , TRUE  , 0          },
      {I386_FUCOMIP , NULL_OP, ST     , ST3    , TRUE  , 0          },
      {I386_FUCOMIP , NULL_OP, ST     , ST4    , TRUE  , 0          },
      {I386_FUCOMIP , NULL_OP, ST     , ST5    , TRUE  , 0          },
      {I386_FUCOMIP , NULL_OP, ST     , ST6    , TRUE  , 0          },
      {I386_FUCOMIP , NULL_OP, ST     , ST7    , TRUE  , 0          }
    },                                                 
    /* reg = 6 */                                      
    {                                                  
      {I386_FCOMIP  , NULL_OP, ST     , ST0    , TRUE  , 0          },
      {I386_FCOMIP  , NULL_OP, ST     , ST1    , TRUE  , 0          },
      {I386_FCOMIP  , NULL_OP, ST     , ST2    , TRUE  , 0          },
      {I386_FCOMIP  , NULL_OP, ST     , ST3    , TRUE  , 0          },
      {I386_FCOMIP  , NULL_OP, ST     , ST4    , TRUE  , 0          },
      {I386_FCOMIP  , NULL_OP, ST     , ST5    , TRUE  , 0          },
      {I386_FCOMIP  , NULL_OP, ST     , ST6    , TRUE  , 0          },
      {I386_FCOMIP  , NULL_OP, ST     , ST7    , TRUE  , 0          }
    },                                                 
    /* reg = 7 */                                      
    {                                                  
      {INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
      {INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
      {INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
      {INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
      {INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
      {INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
      {INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          },
      {INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  , 0          }
    }
  }
};

t_hash_table * i386_opcode_hash = NULL;

static t_uint32 OpcodeHash(const void *key, const t_hash_table * ht)
{
  t_i386_opcode * opc = (t_i386_opcode *) key;
  return (t_uint32) *opc;
}

static t_int32 OpcodeCmp(const void * key1, const void * key2)
{
  t_i386_opcode * opc1 = (t_i386_opcode *) key1;
  t_i386_opcode * opc2 = (t_i386_opcode *) key2;
  return (*opc1) - (*opc2);
}

static void OpcodeNodeFree(const void * he, void * data)
{
  Free(he);
}

void I386CreateOpcodeHashTable(void)
{
  int i,j,k;
  t_i386_opcode_entry * entry;
  
  i386_opcode_hash = HashTableNew(MAX_I386_OPCODE,0,OpcodeHash,OpcodeCmp,OpcodeNodeFree);

  /* one byte opcodes */
  for (i = 0; i < 256; i++)
  {
    entry = &(dis386[i]);
    if ((entry->opcode >= 0) && (entry->opcode != UNSUPPORTED_OPC) && (entry->opcode != INVALID_OPC))
    {
      t_i386_opcode_he * he = Calloc(1,sizeof(t_i386_opcode_he));
      he->entry = entry;
      HASH_TABLE_NODE_SET_KEY(&he->node,  &(entry->opcode));
      HashTableInsert(i386_opcode_hash, (void *)he);
    }
  }
  /* two byte opcodes */
  for (i = 0; i < 256; i++)
  {
    entry = &(dis386_twobyte[i]);
    if ((entry->opcode >= 0) && (entry->opcode != UNSUPPORTED_OPC) && (entry->opcode != INVALID_OPC))
    {
      t_i386_opcode_he * he = Calloc(1,sizeof(t_i386_opcode_he));
      he->entry = entry;
      HASH_TABLE_NODE_SET_KEY(&he->node, &(entry->opcode));
      HashTableInsert(i386_opcode_hash, (void *)he);
    }
  }

  /* fpu memory operations */
  for (i = 0; i < 8; i++)
    for (j = 0; j < 8; j++)
    {
      entry = &(dis386_fpu_mem[i][j]);
      if ((entry->opcode >= 0) && (entry->opcode != UNSUPPORTED_OPC) && (entry->opcode != INVALID_OPC))
      {
	t_i386_opcode_he * he = Calloc(1,sizeof(t_i386_opcode_he));
	he->entry = entry;
	HASH_TABLE_NODE_SET_KEY(&he->node, &(entry->opcode));
	HashTableInsert(i386_opcode_hash, (void *)he);
      }
    }

  /* fpu register operations */
  for (i = 0; i < 8; i++)
    for (j = 0; j < 8; j++)
      for (k = 0; k < 8; k++)
      {
	entry = &(dis386_fpu_reg[i][j][k]);
	if ((entry->opcode >= 0) && (entry->opcode != UNSUPPORTED_OPC) && (entry->opcode != INVALID_OPC))
	{
	  t_i386_opcode_he * he = Calloc(1,sizeof(t_i386_opcode_he));
	  he->entry = entry;
	  HASH_TABLE_NODE_SET_KEY(&he->node, &(entry->opcode));
	  HashTableInsert(i386_opcode_hash, (void *)he);
	}
      }

  /* extended opcode groups */
  for (i = 0; i < (sizeof(dis386_grps) / (8 * sizeof(t_i386_opcode_entry))); i++)
  {
    for (j = 0; j < 8; j++)
    {
      entry = &(dis386_grps[i][j]);
      if ((entry->opcode >= 0) && (entry->opcode != UNSUPPORTED_OPC) && (entry->opcode != INVALID_OPC))
      {
	t_i386_opcode_he * he = Calloc(1,sizeof(t_i386_opcode_he));
	he->entry = entry;
	HASH_TABLE_NODE_SET_KEY(&he->node, &(entry->opcode));
	HashTableInsert(i386_opcode_hash, (void *)he);
      }
    }
  }
}

void I386DestroyOpcodeHashTable(void)
{
  HashTableFree(i386_opcode_hash);
}
