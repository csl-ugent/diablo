#include <diabloamd64.h>

#define NULL_OP		Amd64OpDisNone, Amd64OpAsNone, Amd64OpCheckNone, Amd64OpNextNone, 0

#define AH		Amd64OpDisReg, Amd64OpAsReg, Amd64OpCheckReg, Amd64OpNextReg, amd64_bm_AH
#define BH		Amd64OpDisReg, Amd64OpAsReg, Amd64OpCheckReg, Amd64OpNextReg, amd64_bm_BH
#define CH		Amd64OpDisReg, Amd64OpAsReg, Amd64OpCheckReg, Amd64OpNextReg, amd64_bm_CH
#define DH		Amd64OpDisReg, Amd64OpAsReg, Amd64OpCheckReg, Amd64OpNextReg, amd64_bm_DH
#define AL		Amd64OpDisReg, Amd64OpAsReg, Amd64OpCheckReg, Amd64OpNextReg, amd64_bm_AL
#define BL		Amd64OpDisReg, Amd64OpAsReg, Amd64OpCheckReg, Amd64OpNextReg, amd64_bm_BL
#define CL		Amd64OpDisReg, Amd64OpAsReg, Amd64OpCheckReg, Amd64OpNextReg, amd64_bm_CL
#define DL		Amd64OpDisReg, Amd64OpAsReg, Amd64OpCheckReg, Amd64OpNextReg, amd64_bm_DL
#define AHrex           Amd64OpDisReg, Amd64OpAsReg, Amd64OpCheckReg, Amd64OpNextReg, amd64_bm_AHrex
#define BHrex           Amd64OpDisReg, Amd64OpAsReg, Amd64OpCheckReg, Amd64OpNextReg, amd64_bm_BHrex
#define CHrex           Amd64OpDisReg, Amd64OpAsReg, Amd64OpCheckReg, Amd64OpNextReg, amd64_bm_CHrex
#define DHrex           Amd64OpDisReg, Amd64OpAsReg, Amd64OpCheckReg, Amd64OpNextReg, amd64_bm_DHrex
#define ALrex           Amd64OpDisReg, Amd64OpAsReg, Amd64OpCheckReg, Amd64OpNextReg, amd64_bm_ALrex
#define BLrex           Amd64OpDisReg, Amd64OpAsReg, Amd64OpCheckReg, Amd64OpNextReg, amd64_bm_BLrex
#define CLrex           Amd64OpDisReg, Amd64OpAsReg, Amd64OpCheckReg, Amd64OpNextReg, amd64_bm_CLrex
#define DLrex           Amd64OpDisReg, Amd64OpAsReg, Amd64OpCheckReg, Amd64OpNextReg, amd64_bm_DLrex
#define AX		Amd64OpDisReg, Amd64OpAsReg, Amd64OpCheckReg, Amd64OpNextReg, amd64_bm_AX
#define BX		Amd64OpDisReg, Amd64OpAsReg, Amd64OpCheckReg, Amd64OpNextReg, amd64_bm_BX
#define CX		Amd64OpDisReg, Amd64OpAsReg, Amd64OpCheckReg, Amd64OpNextReg, amd64_bm_CX
#define DX		Amd64OpDisReg, Amd64OpAsReg, Amd64OpCheckReg, Amd64OpNextReg, amd64_bm_DX
#define EAX		Amd64OpDisReg, Amd64OpAsReg, Amd64OpCheckReg, Amd64OpNextReg, amd64_bm_EAX
#define EBX		Amd64OpDisReg, Amd64OpAsReg, Amd64OpCheckReg, Amd64OpNextReg, amd64_bm_EBX
#define ECX		Amd64OpDisReg, Amd64OpAsReg, Amd64OpCheckReg, Amd64OpNextReg, amd64_bm_ECX
#define EDX		Amd64OpDisReg, Amd64OpAsReg, Amd64OpCheckReg, Amd64OpNextReg, amd64_bm_EDX
#define ESI		Amd64OpDisReg, Amd64OpAsReg, Amd64OpCheckReg, Amd64OpNextReg, amd64_bm_ESI
#define EDI		Amd64OpDisReg, Amd64OpAsReg, Amd64OpCheckReg, Amd64OpNextReg, amd64_bm_EDI
#define ESP		Amd64OpDisReg, Amd64OpAsReg, Amd64OpCheckReg, Amd64OpNextReg, amd64_bm_ESP
#define EBP		Amd64OpDisReg, Amd64OpAsReg, Amd64OpCheckReg, Amd64OpNextReg, amd64_bm_EBP
#define RAX             Amd64OpDisReg, Amd64OpAsReg, Amd64OpCheckReg, Amd64OpNextReg, amd64_bm_RAX
#define RBX             Amd64OpDisReg, Amd64OpAsReg, Amd64OpCheckReg, Amd64OpNextReg, amd64_bm_RBX
#define RCX             Amd64OpDisReg, Amd64OpAsReg, Amd64OpCheckReg, Amd64OpNextReg, amd64_bm_RCX
#define RDX             Amd64OpDisReg, Amd64OpAsReg, Amd64OpCheckReg, Amd64OpNextReg, amd64_bm_RDX
#define RSI             Amd64OpDisReg, Amd64OpAsReg, Amd64OpCheckReg, Amd64OpNextReg, amd64_bm_RSI
#define RDI             Amd64OpDisReg, Amd64OpAsReg, Amd64OpCheckReg, Amd64OpNextReg, amd64_bm_RDI
#define RSP             Amd64OpDisReg, Amd64OpAsReg, Amd64OpCheckReg, Amd64OpNextReg, amd64_bm_RSP
#define RBP             Amd64OpDisReg, Amd64OpAsReg, Amd64OpCheckReg, Amd64OpNextReg, amd64_bm_RBP
#define eAX             Amd64OpDisReg, Amd64OpAsReg, Amd64OpCheckReg, Amd64OpNextReg, amd64_bm_eAX
#define eBX             Amd64OpDisReg, Amd64OpAsReg, Amd64OpCheckReg, Amd64OpNextReg, amd64_bm_eBX
#define eCX             Amd64OpDisReg, Amd64OpAsReg, Amd64OpCheckReg, Amd64OpNextReg, amd64_bm_eCX
#define eDX             Amd64OpDisReg, Amd64OpAsReg, Amd64OpCheckReg, Amd64OpNextReg, amd64_bm_eDX
#define eSI             Amd64OpDisReg, Amd64OpAsReg, Amd64OpCheckReg, Amd64OpNextReg, amd64_bm_eSI
#define eDI             Amd64OpDisReg, Amd64OpAsReg, Amd64OpCheckReg, Amd64OpNextReg, amd64_bm_eDI
#define eSP             Amd64OpDisReg, Amd64OpAsReg, Amd64OpCheckReg, Amd64OpNextReg, amd64_bm_eSP
#define eBP             Amd64OpDisReg, Amd64OpAsReg, Amd64OpCheckReg, Amd64OpNextReg, amd64_bm_eBP
#define rAX		Amd64OpDisReg, Amd64OpAsReg, Amd64OpCheckReg, Amd64OpNextReg, amd64_bm_rAX
#define rBX		Amd64OpDisReg, Amd64OpAsReg, Amd64OpCheckReg, Amd64OpNextReg, amd64_bm_rBX
#define rCX		Amd64OpDisReg, Amd64OpAsReg, Amd64OpCheckReg, Amd64OpNextReg, amd64_bm_rCX
#define rDX		Amd64OpDisReg, Amd64OpAsReg, Amd64OpCheckReg, Amd64OpNextReg, amd64_bm_rDX
#define rSI		Amd64OpDisReg, Amd64OpAsReg, Amd64OpCheckReg, Amd64OpNextReg, amd64_bm_rSI
#define rDI		Amd64OpDisReg, Amd64OpAsReg, Amd64OpCheckReg, Amd64OpNextReg, amd64_bm_rDI
#define rSP		Amd64OpDisReg, Amd64OpAsReg, Amd64OpCheckReg, Amd64OpNextReg, amd64_bm_rSP
#define rBP		Amd64OpDisReg, Amd64OpAsReg, Amd64OpCheckReg, Amd64OpNextReg, amd64_bm_rBP
#define rAXrex          Amd64OpDisReg, Amd64OpAsReg, Amd64OpCheckReg, Amd64OpNextReg, amd64_bm_rAXrex
#define rBXrex          Amd64OpDisReg, Amd64OpAsReg, Amd64OpCheckReg, Amd64OpNextReg, amd64_bm_rBXrex
#define rCXrex          Amd64OpDisReg, Amd64OpAsReg, Amd64OpCheckReg, Amd64OpNextReg, amd64_bm_rCXrex
#define rDXrex          Amd64OpDisReg, Amd64OpAsReg, Amd64OpCheckReg, Amd64OpNextReg, amd64_bm_rDXrex
#define rSIrex          Amd64OpDisReg, Amd64OpAsReg, Amd64OpCheckReg, Amd64OpNextReg, amd64_bm_rSIrex
#define rDIrex          Amd64OpDisReg, Amd64OpAsReg, Amd64OpCheckReg, Amd64OpNextReg, amd64_bm_rDIrex
#define rSPrex          Amd64OpDisReg, Amd64OpAsReg, Amd64OpCheckReg, Amd64OpNextReg, amd64_bm_rSPrex
#define rBPrex          Amd64OpDisReg, Amd64OpAsReg, Amd64OpCheckReg, Amd64OpNextReg, amd64_bm_rBPrex
#define RAXrex          Amd64OpDisReg, Amd64OpAsReg, Amd64OpCheckReg, Amd64OpNextReg, amd64_bm_RAXrex
#define RBXrex          Amd64OpDisReg, Amd64OpAsReg, Amd64OpCheckReg, Amd64OpNextReg, amd64_bm_RBXrex
#define RCXrex          Amd64OpDisReg, Amd64OpAsReg, Amd64OpCheckReg, Amd64OpNextReg, amd64_bm_RCXrex
#define RDXrex          Amd64OpDisReg, Amd64OpAsReg, Amd64OpCheckReg, Amd64OpNextReg, amd64_bm_RDXrex
#define RSIrex          Amd64OpDisReg, Amd64OpAsReg, Amd64OpCheckReg, Amd64OpNextReg, amd64_bm_RSIrex
#define RDIrex          Amd64OpDisReg, Amd64OpAsReg, Amd64OpCheckReg, Amd64OpNextReg, amd64_bm_RDIrex
#define RSPrex          Amd64OpDisReg, Amd64OpAsReg, Amd64OpCheckReg, Amd64OpNextReg, amd64_bm_RSPrex
#define RBPrex          Amd64OpDisReg, Amd64OpAsReg, Amd64OpCheckReg, Amd64OpNextReg, amd64_bm_RBPrex
#define CS		Amd64OpDisReg, Amd64OpAsReg, Amd64OpCheckReg, Amd64OpNextReg, amd64_bm_CS
#define DS		Amd64OpDisReg, Amd64OpAsReg, Amd64OpCheckReg, Amd64OpNextReg, amd64_bm_DS
#define ES		Amd64OpDisReg, Amd64OpAsReg, Amd64OpCheckReg, Amd64OpNextReg, amd64_bm_ES
#define FS		Amd64OpDisReg, Amd64OpAsReg, Amd64OpCheckReg, Amd64OpNextReg, amd64_bm_FS
#define GS		Amd64OpDisReg, Amd64OpAsReg, Amd64OpCheckReg, Amd64OpNextReg, amd64_bm_GS
#define SS		Amd64OpDisReg, Amd64OpAsReg, Amd64OpCheckReg, Amd64OpNextReg, amd64_bm_SS

#define ST		Amd64OpDisST , Amd64OpAsST , Amd64OpCheckST , Amd64OpNextST, amd64_bm_ST
#define ST0		Amd64OpDisST , Amd64OpAsST , Amd64OpCheckST , Amd64OpNextST, amd64_bm_ST0
#define ST1		Amd64OpDisST , Amd64OpAsST , Amd64OpCheckST , Amd64OpNextST, amd64_bm_ST1
#define ST2		Amd64OpDisST , Amd64OpAsST , Amd64OpCheckST , Amd64OpNextST, amd64_bm_ST2
#define ST3		Amd64OpDisST , Amd64OpAsST , Amd64OpCheckST , Amd64OpNextST, amd64_bm_ST3
#define ST4		Amd64OpDisST , Amd64OpAsST , Amd64OpCheckST , Amd64OpNextST, amd64_bm_ST4
#define ST5		Amd64OpDisST , Amd64OpAsST , Amd64OpCheckST , Amd64OpNextST, amd64_bm_ST5
#define ST6		Amd64OpDisST , Amd64OpAsST , Amd64OpCheckST , Amd64OpNextST, amd64_bm_ST6
#define ST7		Amd64OpDisST , Amd64OpAsST , Amd64OpCheckST , Amd64OpNextST, amd64_bm_ST7

#define C_1		Amd64OpDisConst1, Amd64OpAsConst1, Amd64OpCheckConst1, Amd64OpNextConst1, 0

#define Ap		Amd64OpDisA, Amd64OpAsA, Amd64OpCheckA, Amd64OpNextA, amd64_bm_p
#define Cd		Amd64OpDisC, Amd64OpAsC, Amd64OpCheckC, Amd64OpNextC, amd64_bm_d
#define Cq              Amd64OpDisC, Amd64OpAsC, Amd64OpCheckC, Amd64OpNextC, amd64_bm_q
#define Dd		Amd64OpDisD, Amd64OpAsD, Amd64OpCheckD, Amd64OpNextD, amd64_bm_d
#define Dq              Amd64OpDisD, Amd64OpAsD, Amd64OpCheckD, Amd64OpNextD, amd64_bm_q


#define Eb		Amd64OpDisE, Amd64OpAsE, Amd64OpCheckE, Amd64OpNextE, amd64_bm_b
#define Ew		Amd64OpDisE, Amd64OpAsE, Amd64OpCheckE, Amd64OpNextE, amd64_bm_w
#define Ed		Amd64OpDisE, Amd64OpAsE, Amd64OpCheckE, Amd64OpNextE, amd64_bm_d
#define Ev		Amd64OpDisE, Amd64OpAsE, Amd64OpCheckE, Amd64OpNextE, amd64_bm_v
#define Ep		Amd64OpDisE, Amd64OpAsE, Amd64OpCheckE, Amd64OpNextE, amd64_bm_p
#define Ev48            Amd64OpDisE, Amd64OpAsE, Amd64OpCheckE, Amd64OpNextE, amd64_bm_v48
#define Ev8             Amd64OpDisE, Amd64OpAsE, Amd64OpCheckE, Amd64OpNextE, amd64_bm_v8


#define Fv		Amd64OpDisF, Amd64OpAsF, Amd64OpCheckF, Amd64OpNextF, amd64_bm_v
#define Fv8             Amd64OpDisF, Amd64OpAsF, Amd64OpCheckF, Amd64OpNextF, amd64_bm_v8

#define Gb		Amd64OpDisG, Amd64OpAsG, Amd64OpCheckG, Amd64OpNextG, amd64_bm_b
#define Gw		Amd64OpDisG, Amd64OpAsG, Amd64OpCheckG, Amd64OpNextG, amd64_bm_w
#define Gd		Amd64OpDisG, Amd64OpAsG, Amd64OpCheckG, Amd64OpNextG, amd64_bm_d
#define Gv48            Amd64OpDisG, Amd64OpAsG, Amd64OpCheckG, Amd64OpNextG, amd64_bm_v48
#define Gv		Amd64OpDisG, Amd64OpAsG, Amd64OpCheckG, Amd64OpNextG, amd64_bm_v

#define sIb		Amd64OpDissI, Amd64OpAssI, Amd64OpChecksI, Amd64OpNextsI, amd64_bm_b  /* the double 's' is _not_ a typo */
#define Ib		Amd64OpDisI, Amd64OpAsI, Amd64OpCheckI, Amd64OpNextI, amd64_bm_b
#define Iw		Amd64OpDisI, Amd64OpAsI, Amd64OpCheckI, Amd64OpNextI, amd64_bm_w
#define Iv		Amd64OpDisI, Amd64OpAsI, Amd64OpCheckI, Amd64OpNextI, amd64_bm_v
#define Iz              Amd64OpDisI, Amd64OpAsI, Amd64OpCheckI, Amd64OpNextI, amd64_bm_z

#define Jb		Amd64OpDisJ, Amd64OpAsJ, Amd64OpCheckJ, Amd64OpNextJ, amd64_bm_b
#define Jv		Amd64OpDisJ, Amd64OpAsJ, Amd64OpCheckJ, Amd64OpNextJ, amd64_bm_v
#define Jz              Amd64OpDisJ, Amd64OpAsJ, Amd64OpCheckJ, Amd64OpNextJ, amd64_bm_z

#define M		Amd64OpDisM, Amd64OpAsM, Amd64OpCheckM, Amd64OpNextM, 0
#define Ma		Amd64OpDisM, Amd64OpAsM, Amd64OpCheckM, Amd64OpNextM, amd64_bm_a
#define Mb		Amd64OpDisM, Amd64OpAsM, Amd64OpCheckM, Amd64OpNextM, amd64_bm_b
#define Mp		Amd64OpDisM, Amd64OpAsM, Amd64OpCheckM, Amd64OpNextM, amd64_bm_p
#define Mdq		Amd64OpDisM, Amd64OpAsM, Amd64OpCheckM, Amd64OpNextM, amd64_bm_dq
#define Ms		Amd64OpDisM, Amd64OpAsM, Amd64OpCheckM, Amd64OpNextM, amd64_bm_s
/* these are defined only for fpu memory operations */
/* single-precision real */
#define Msr		Amd64OpDisM, Amd64OpAsM, Amd64OpCheckM, Amd64OpNextM, amd64_bm_sr
/* double-precision real */
#define Mdr		Amd64OpDisM, Amd64OpAsM, Amd64OpCheckM, Amd64OpNextM, amd64_bm_dr
/* extended double-precision real (80 bits) */
#define Mer		Amd64OpDisM, Amd64OpAsM, Amd64OpCheckM, Amd64OpNextM, amd64_bm_er
/* packed BCD integer (8 bytes) */
#define Mbcd		Amd64OpDisM, Amd64OpAsM, Amd64OpCheckM, Amd64OpNextM, amd64_bm_bcd
/* word integer */
#define Mw		Amd64OpDisM, Amd64OpAsM, Amd64OpCheckM, Amd64OpNextM, amd64_bm_w
/* dword integer */
#define Md		Amd64OpDisM, Amd64OpAsM, Amd64OpCheckM, Amd64OpNextM, amd64_bm_d
/* qword integer */
#define Mq		Amd64OpDisM, Amd64OpAsM, Amd64OpCheckM, Amd64OpNextM, amd64_bm_q
/* generic two-byte */
#define M2		Amd64OpDisM, Amd64OpAsM, Amd64OpCheckM, Amd64OpNextM, amd64_bm_2byte
/* generic 28 byte */
#define M28		Amd64OpDisM, Amd64OpAsM, Amd64OpCheckM, Amd64OpNextM, amd64_bm_28byte
/* generic 108 byte */
#define M108		Amd64OpDisM, Amd64OpAsM, Amd64OpCheckM, Amd64OpNextM, amd64_bm_108byte
/* generic 512 byte */
#define M512		Amd64OpDisM, Amd64OpAsM, Amd64OpCheckM, Amd64OpNextM, amd64_bm_512byte



#define Ob		Amd64OpDisO, Amd64OpAsO, Amd64OpCheckO, Amd64OpNextO, amd64_bm_b
#define Ov		Amd64OpDisO, Amd64OpAsO, Amd64OpCheckO, Amd64OpNextO, amd64_bm_v

#define O8b              Amd64OpDisO8, Amd64OpAsO8, Amd64OpCheckO8, Amd64OpNextO, amd64_bm_b
#define O8v              Amd64OpDisO8, Amd64OpAsO8, Amd64OpCheckO8, Amd64OpNextO, amd64_bm_v


#define Rd		Amd64OpDisR, Amd64OpAsR, Amd64OpCheckR, Amd64OpNextR, amd64_bm_d
#define Rq              Amd64OpDisR, Amd64OpAsR, Amd64OpCheckR, Amd64OpNextR, amd64_bm_q


#define Sw		Amd64OpDisS, Amd64OpAsS, Amd64OpCheckS, Amd64OpNextS, amd64_bm_w

#define Xb		Amd64OpDisX, Amd64OpAsX, Amd64OpCheckX, Amd64OpNextX, amd64_bm_b
#define Xv		Amd64OpDisX, Amd64OpAsX, Amd64OpCheckX, Amd64OpNextX, amd64_bm_v

#define Yb		Amd64OpDisY, Amd64OpAsY, Amd64OpCheckY, Amd64OpNextY, amd64_bm_b
#define Yv		Amd64OpDisY, Amd64OpAsY, Amd64OpCheckY, Amd64OpNextY, amd64_bm_v

#define Vsd             Amd64OpDisV, Amd64OpAsV, Amd64OpCheckV, Amd64OpNextV, amd64_bm_sd
#define Vss             Amd64OpDisV, Amd64OpAsV, Amd64OpCheckV, Amd64OpNextV, amd64_bm_ss
#define Vps             Amd64OpDisV, Amd64OpAsV, Amd64OpCheckV, Amd64OpNextV, amd64_bm_ps
#define Vpd             Amd64OpDisV, Amd64OpAsV, Amd64OpCheckV, Amd64OpNextV, amd64_bm_pd
#define Vdq             Amd64OpDisV, Amd64OpAsV, Amd64OpCheckV, Amd64OpNextV, amd64_bm_dq
#define Vd              Amd64OpDisV, Amd64OpAsV, Amd64OpCheckV, Amd64OpNextV, amd64_bm_d
#define Vq              Amd64OpDisV, Amd64OpAsV, Amd64OpCheckV, Amd64OpNextV, amd64_bm_q

#define Wsd             Amd64OpDisW, Amd64OpAsW, Amd64OpCheckW, Amd64OpNextW, amd64_bm_sd
#define Wss             Amd64OpDisW, Amd64OpAsW, Amd64OpCheckW, Amd64OpNextW, amd64_bm_ss
#define Wps             Amd64OpDisW, Amd64OpAsW, Amd64OpCheckW, Amd64OpNextW, amd64_bm_ps
#define Wpd             Amd64OpDisW, Amd64OpAsW, Amd64OpCheckW, Amd64OpNextW, amd64_bm_pd
#define Wdq             Amd64OpDisW, Amd64OpAsW, Amd64OpCheckW, Amd64OpNextW, amd64_bm_dq
#define Wq              Amd64OpDisW, Amd64OpAsW, Amd64OpCheckW, Amd64OpNextW, amd64_bm_q

#define Psd             Amd64OpDisV, Amd64OpAsV, Amd64OpCheckV, Amd64OpNextV, amd64_bm_sd
#define Pss             Amd64OpDisV, Amd64OpAsV, Amd64OpCheckV, Amd64OpNextV, amd64_bm_ss
#define Pps             Amd64OpDisV, Amd64OpAsV, Amd64OpCheckV, Amd64OpNextV, amd64_bm_ps
#define Ppd             Amd64OpDisV, Amd64OpAsV, Amd64OpCheckV, Amd64OpNextV, amd64_bm_pd
#define Pq              Amd64OpDisV, Amd64OpAsV, Amd64OpCheckV, Amd64OpNextV, amd64_bm_q
#define Pdq             Amd64OpDisV, Amd64OpAsV, Amd64OpCheckV, Amd64OpNextV, amd64_bm_dq
#define Pd              Amd64OpDisV, Amd64OpAsV, Amd64OpCheckV, Amd64OpNextV, amd64_bm_d

#define Qsd             Amd64OpDisE, Amd64OpAsE, Amd64OpCheckE, Amd64OpNextE, amd64_bm_sd
#define Qss             Amd64OpDisE, Amd64OpAsE, Amd64OpCheckE, Amd64OpNextE, amd64_bm_ss
#define Qps             Amd64OpDisE, Amd64OpAsE, Amd64OpCheckE, Amd64OpNextE, amd64_bm_ps
#define Qpd             Amd64OpDisE, Amd64OpAsE, Amd64OpCheckE, Amd64OpNextE, amd64_bm_pd
#define Qq              Amd64OpDisE, Amd64OpAsE, Amd64OpCheckE, Amd64OpNextE, amd64_bm_q
#define Qdq             Amd64OpDisE, Amd64OpAsE, Amd64OpCheckE, Amd64OpNextE, amd64_bm_dq

t_amd64_opcode_entry nopinstr =  {AMD64_NOP       , NULL_OP, NULL_OP, NULL_OP, FALSE ,            };
  

t_amd64_opcode_entry disamd64[] = 
{
  /* 00 */
  {AMD64_ADD       , Eb     , Gb     , NULL_OP, TRUE  , DU         },
  {AMD64_ADD       , Ev     , Gv     , NULL_OP, TRUE  , DU         },
  {AMD64_ADD       , Gb     , Eb     , NULL_OP, TRUE  , DU         },
  {AMD64_ADD       , Gv     , Ev     , NULL_OP, TRUE  , DU         },
  {AMD64_ADD       , AL     , Ib     , NULL_OP, FALSE , DU         },
  {AMD64_ADD       , rAX    , Iz     , NULL_OP, FALSE , DU         },
  {AMD64_INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, FALSE ,            },
  {AMD64_INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, FALSE ,            },
  /* 08 */                                   
  {AMD64_OR        , Eb     , Gb     , NULL_OP, TRUE  , DU         },
  {AMD64_OR        , Ev     , Gv     , NULL_OP, TRUE  , DU         },
  {AMD64_OR        , Gb     , Eb     , NULL_OP, TRUE  , DU         },
  {AMD64_OR        , Gv     , Ev     , NULL_OP, TRUE  , DU         },
  {AMD64_OR        , AL     , Ib     , NULL_OP, FALSE , DU         },
  {AMD64_OR        , rAX    , Iz     , NULL_OP, FALSE , DU         },
  {AMD64_INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, FALSE ,            },
  {AMD64_TWOBYTE_ESC    , NULL_OP, NULL_OP, NULL_OP, FALSE ,            },
  /* 10 */                                   
  {AMD64_ADC       , Eb     , Gb     , NULL_OP, TRUE  , DU         },
  {AMD64_ADC       , Ev     , Gv     , NULL_OP, TRUE  , DU         },
  {AMD64_ADC       , Gb     , Eb     , NULL_OP, TRUE  , DU         },
  {AMD64_ADC       , Gv     , Ev     , NULL_OP, TRUE  , DU         },
  {AMD64_ADC       , AL     , Ib     , NULL_OP, FALSE , DU         },
  {AMD64_ADC       , rAX    , Iz     , NULL_OP, FALSE , DU         },
  {AMD64_INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, FALSE ,            },
  {AMD64_INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, FALSE ,            },
  /* 18 */                                   
  {AMD64_SBB       , Eb     , Gb     , NULL_OP, TRUE  , DU         },
  {AMD64_SBB       , Ev     , Gv     , NULL_OP, TRUE  , DU         },
  {AMD64_SBB       , Gb     , Eb     , NULL_OP, TRUE  , DU         },
  {AMD64_SBB       , Gv     , Ev     , NULL_OP, TRUE  , DU         },
  {AMD64_SBB       , AL     , Ib     , NULL_OP, FALSE , DU         },
  {AMD64_SBB       , rAX    , Iz     , NULL_OP, FALSE , DU         },
  {AMD64_INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, FALSE ,            },
  {AMD64_INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, FALSE ,            },
  /* 20 */                                   
  {AMD64_AND       , Eb     , Gb     , NULL_OP, TRUE  , DU         },
  {AMD64_AND       , Ev     , Gv     , NULL_OP, TRUE  , DU         },
  {AMD64_AND       , Gb     , Eb     , NULL_OP, TRUE  , DU         },
  {AMD64_AND       , Gv     , Ev     , NULL_OP, TRUE  , DU         },
  {AMD64_AND       , AL     , Ib     , NULL_OP, FALSE , DU         },
  {AMD64_AND       , rAX    , Iz     , NULL_OP, FALSE , DU         },
  {AMD64_INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, FALSE ,            },
  {AMD64_INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, FALSE ,            },
  /* 28 */                                   
  {AMD64_SUB       , Eb     , Gb     , NULL_OP, TRUE  , DU         },
  {AMD64_SUB       , Ev     , Gv     , NULL_OP, TRUE  , DU         },
  {AMD64_SUB       , Gb     , Eb     , NULL_OP, TRUE  , DU         },
  {AMD64_SUB       , Gv     , Ev     , NULL_OP, TRUE  , DU         },
  {AMD64_SUB       , AL     , Ib     , NULL_OP, FALSE , DU         },
  {AMD64_SUB       , rAX    , Iz     , NULL_OP, FALSE , DU         },
  {AMD64_INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, FALSE ,            },
  {AMD64_INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, FALSE ,            },
  /* 30 */                                   
  {AMD64_XOR       , Eb     , Gb     , NULL_OP, TRUE  , DU         },
  {AMD64_XOR       , Ev     , Gv     , NULL_OP, TRUE  , DU         },
  {AMD64_XOR       , Gb     , Eb     , NULL_OP, TRUE  , DU         },
  {AMD64_XOR       , Gv     , Ev     , NULL_OP, TRUE  , DU         },
  {AMD64_XOR       , AL     , Ib     , NULL_OP, FALSE , DU         },
  {AMD64_XOR       , rAX    , Iz     , NULL_OP, FALSE , DU         },
  {AMD64_INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, FALSE ,            },
  {AMD64_INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, FALSE ,            },
  /* 38 */                                   
  {AMD64_CMP       , NULL_OP, Eb     , Gb     , TRUE  ,            },
  {AMD64_CMP       , NULL_OP, Ev     , Gv     , TRUE  ,            },
  {AMD64_CMP       , NULL_OP, Gb     , Eb     , TRUE  ,            },
  {AMD64_CMP       , NULL_OP, Gv     , Ev     , TRUE  ,            },
  {AMD64_CMP       , NULL_OP, AL     , Ib     , FALSE ,            },
  {AMD64_CMP       , NULL_OP, rAX    , Iz     , FALSE ,            },
  {AMD64_INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, FALSE ,            },
  {AMD64_INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, FALSE ,            },
  /* 40 */                                   
  /*reassigned by rex prefixes*/
  {AMD64_INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, FALSE ,            },
  {AMD64_INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, FALSE ,            },
  {AMD64_INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, FALSE ,            },
  {AMD64_INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, FALSE ,            },
  {AMD64_INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, FALSE ,            },
  {AMD64_INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, FALSE ,            },
  {AMD64_INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, FALSE ,            },
  {AMD64_INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, FALSE ,            },
  /* 48 */                                   
  /*reassigned by rex prefixes*/
  {AMD64_INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, FALSE ,            },
  {AMD64_INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, FALSE ,            },
  {AMD64_INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, FALSE ,            },
  {AMD64_INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, FALSE ,            },
  {AMD64_INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, FALSE ,            },
  {AMD64_INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, FALSE ,            },
  {AMD64_INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, FALSE ,            },
  {AMD64_INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, FALSE ,            },
  /* 50 */                                   
  {AMD64_PUSH      , NULL_OP, RAXrex , NULL_OP, FALSE ,            },
  {AMD64_PUSH      , NULL_OP, RCXrex , NULL_OP, FALSE ,            },
  {AMD64_PUSH      , NULL_OP, RDXrex , NULL_OP, FALSE ,            },
  {AMD64_PUSH      , NULL_OP, RBXrex , NULL_OP, FALSE ,            },
  {AMD64_PUSH      , NULL_OP, RSPrex , NULL_OP, FALSE ,            },
  {AMD64_PUSH      , NULL_OP, RBPrex , NULL_OP, FALSE ,            },
  {AMD64_PUSH      , NULL_OP, RSIrex , NULL_OP, FALSE ,            },
  {AMD64_PUSH      , NULL_OP, RDIrex , NULL_OP, FALSE ,            },
  /* 58 */                                   
  {AMD64_POP       , RAXrex , NULL_OP, NULL_OP, FALSE ,            },
  {AMD64_POP       , RCXrex , NULL_OP, NULL_OP, FALSE ,            },
  {AMD64_POP       , RDXrex , NULL_OP, NULL_OP, FALSE ,            },
  {AMD64_POP       , RBXrex , NULL_OP, NULL_OP, FALSE ,            },
  {AMD64_POP       , RSPrex , NULL_OP, NULL_OP, FALSE ,            },
  {AMD64_POP       , RBPrex , NULL_OP, NULL_OP, FALSE ,            },
  {AMD64_POP       , RSIrex , NULL_OP, NULL_OP, FALSE ,            },
  {AMD64_POP       , RDIrex , NULL_OP, NULL_OP, FALSE ,            },
  /* 60 */                                   
  {AMD64_INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, FALSE ,            },
  {AMD64_INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, FALSE ,            },
  {AMD64_INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, FALSE ,            },
  {AMD64_MOVSXD    , Gv     , Ed     , NULL_OP, TRUE  ,            },
  {AMD64_INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, FALSE ,            },
  {AMD64_INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, FALSE ,            },
  {AMD64_INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, FALSE ,            },
  {AMD64_INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, FALSE ,            },
  /* 68 */                                   
  {AMD64_PUSH      , NULL_OP, Iz     , NULL_OP, FALSE ,            },
  {AMD64_IMULexp2  , Gv     , Ev     , Iz     , TRUE  ,            }, /* explicit dest, source1 and immediate operand */
  {AMD64_PUSH      , NULL_OP, sIb    , NULL_OP, FALSE ,            },
  {AMD64_IMULexp2  , Gv     , Ev     , sIb    , TRUE  ,            }, /* explicit dest, source1 and immediate operand */
  {AMD64_INSB      , Yb     , DX     , NULL_OP, FALSE ,            },
  {AMD64_INSD      , Yv     , DX     , NULL_OP, FALSE ,            },
  {AMD64_OUTSB     , NULL_OP, DX     , Xb     , FALSE ,            },
  {AMD64_OUTSD     , NULL_OP, DX     , Xv     , FALSE ,            },
  /* 70 */                                   
  {AMD64_Jcc       , NULL_OP, Jb     , NULL_OP, FALSE ,            },
  {AMD64_Jcc       , NULL_OP, Jb     , NULL_OP, FALSE ,            },
  {AMD64_Jcc       , NULL_OP, Jb     , NULL_OP, FALSE ,            },
  {AMD64_Jcc       , NULL_OP, Jb     , NULL_OP, FALSE ,            },
  {AMD64_Jcc       , NULL_OP, Jb     , NULL_OP, FALSE ,            },
  {AMD64_Jcc       , NULL_OP, Jb     , NULL_OP, FALSE ,            },
  {AMD64_Jcc       , NULL_OP, Jb     , NULL_OP, FALSE ,            },
  {AMD64_Jcc       , NULL_OP, Jb     , NULL_OP, FALSE ,            },
  /* 78 */                                   
  {AMD64_Jcc       , NULL_OP, Jb     , NULL_OP, FALSE ,            },
  {AMD64_Jcc       , NULL_OP, Jb     , NULL_OP, FALSE ,            },
  {AMD64_Jcc       , NULL_OP, Jb     , NULL_OP, FALSE ,            },
  {AMD64_Jcc       , NULL_OP, Jb     , NULL_OP, FALSE ,            },
  {AMD64_Jcc       , NULL_OP, Jb     , NULL_OP, FALSE ,            },
  {AMD64_Jcc       , NULL_OP, Jb     , NULL_OP, FALSE ,            },
  {AMD64_Jcc       , NULL_OP, Jb     , NULL_OP, FALSE ,            },
  {AMD64_Jcc       , NULL_OP, Jb     , NULL_OP, FALSE ,            },
  /* 80 */                                   
  {AMD64_GRP1a          , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
  {AMD64_GRP1b          , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
  {AMD64_INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, FALSE ,            },
  {AMD64_GRP1d          , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
  {AMD64_TEST      , NULL_OP, Eb     , Gb     , TRUE  ,            },
  {AMD64_TEST      , NULL_OP, Ev     , Gv     , TRUE  ,            },
  {AMD64_XCHG      , Eb     , Gb     , NULL_OP, TRUE  , DU|S1D     },
  {AMD64_XCHG      , Ev     , Gv     , NULL_OP, TRUE  , DU|S1D     },
  /* 88 */                                   
  {AMD64_MOV       , Eb     , Gb     , NULL_OP, TRUE  ,            },
  {AMD64_MOV       , Ev     , Gv     , NULL_OP, TRUE  ,            },
  {AMD64_MOV       , Gb     , Eb     , NULL_OP, TRUE  ,            },
  {AMD64_MOV       , Gv     , Ev     , NULL_OP, TRUE  ,            },
  {AMD64_MOV       , Ew     , Sw     , NULL_OP, TRUE  ,            },
  {AMD64_LEA       , Gv     , M      , NULL_OP, TRUE  ,            },
  {AMD64_MOV       , Sw     , Ew     , NULL_OP, TRUE  ,            },
  {AMD64_POP       , Ev8    , NULL_OP, NULL_OP, TRUE  ,            },
  /* 90 */                                   
  {AMD64_XCHG      , rAXrex , rAX    , NULL_OP, FALSE , DU|S1D     },  //opm als rax,rax altijd nop (ook met 32 bit versie)
  {AMD64_XCHG      , rCXrex , rAX    , NULL_OP, FALSE , DU|S1D     },
  {AMD64_XCHG      , rDXrex , rAX    , NULL_OP, FALSE , DU|S1D     },
  {AMD64_XCHG      , rBXrex , rAX    , NULL_OP, FALSE , DU|S1D     },
  {AMD64_XCHG      , rSPrex , rAX    , NULL_OP, FALSE , DU|S1D     },
  {AMD64_XCHG      , rBPrex , rAX    , NULL_OP, FALSE , DU|S1D     },
  {AMD64_XCHG      , rSIrex , rAX    , NULL_OP, FALSE , DU|S1D     },
  {AMD64_XCHG      , rDIrex , rAX    , NULL_OP, FALSE , DU|S1D     },
  /* 98 */                                   
  {AMD64_CWDE      , rAX    , NULL_OP, NULL_OP, FALSE , DU         },
  {AMD64_CDQ       , rDX    , rAX    , NULL_OP, FALSE , S1D        },
  {AMD64_INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, FALSE ,            },
  {AMD64_WAIT      , NULL_OP, NULL_OP, NULL_OP, FALSE ,            },
  {AMD64_PUSHF     , NULL_OP, Fv8    , NULL_OP, FALSE ,            },
  {AMD64_POPF      , Fv8    , NULL_OP, NULL_OP, FALSE ,            },
  {AMD64_SAHF      , NULL_OP, AH     , NULL_OP, FALSE ,            },
  {AMD64_LAHF      , AH     , NULL_OP, NULL_OP, FALSE ,            },
  /* a0 */                                   
  {AMD64_MOV       , AL     , O8b    , NULL_OP, FALSE ,            },
  {AMD64_MOV       , rAX    , O8v    , NULL_OP, FALSE ,            },
  {AMD64_MOV       , O8b    , AL     , NULL_OP, FALSE ,            },
  {AMD64_MOV       , O8v    , rAX    , NULL_OP, FALSE ,            },
  {AMD64_MOVSB     , Yb     , Xb     , NULL_OP, FALSE ,            },
  {AMD64_MOVSD     , Yv     , Xv     , NULL_OP, FALSE ,            },
  {AMD64_CMPSB     , NULL_OP, Yb     , Xb     , FALSE ,            },
  {AMD64_CMPSD     , NULL_OP, Yv     , Xv     , FALSE ,            },
  /* a8 */                                   
  {AMD64_TEST      , NULL_OP, AL     , Ib     , FALSE ,            },
  {AMD64_TEST      , NULL_OP, rAX    , Iz     , FALSE ,            },
  {AMD64_STOSB     , Yb     , AL     , NULL_OP, FALSE ,            },
  {AMD64_STOSD     , Yv     , rAX    , NULL_OP, FALSE ,            },
  {AMD64_LODSB     , AL     , Xb     , NULL_OP, FALSE ,            },
  {AMD64_LODSD     , rAX    , Xv     , NULL_OP, FALSE ,            },
  {AMD64_SCASB     , NULL_OP, AL     , Yb     , FALSE ,            },
  {AMD64_SCASD     , NULL_OP, rAX    , Yv     , FALSE ,            },
  /* b0 */                                   
  {AMD64_MOV       , ALrex  , Ib     , NULL_OP, FALSE ,            },
  {AMD64_MOV       , CLrex  , Ib     , NULL_OP, FALSE ,            },
  {AMD64_MOV       , DLrex  , Ib     , NULL_OP, FALSE ,            },
  {AMD64_MOV       , BLrex  , Ib     , NULL_OP, FALSE ,            },
  {AMD64_MOV       , AHrex  , Ib     , NULL_OP, FALSE ,            },
  {AMD64_MOV       , CHrex  , Ib     , NULL_OP, FALSE ,            },
  {AMD64_MOV       , DHrex  , Ib     , NULL_OP, FALSE ,            },
  {AMD64_MOV       , BHrex  , Ib     , NULL_OP, FALSE ,            },
  /* b8 */                                   
  {AMD64_MOV       , rAXrex , Iv     , NULL_OP, FALSE ,            },
  {AMD64_MOV       , rCXrex , Iv     , NULL_OP, FALSE ,            },
  {AMD64_MOV       , rDXrex , Iv     , NULL_OP, FALSE ,            },
  {AMD64_MOV       , rBXrex , Iv     , NULL_OP, FALSE ,            },
  {AMD64_MOV       , rSPrex , Iv     , NULL_OP, FALSE ,            },
  {AMD64_MOV       , rBPrex , Iv     , NULL_OP, FALSE ,            },
  {AMD64_MOV       , rSIrex , Iv     , NULL_OP, FALSE ,            },
  {AMD64_MOV       , rDIrex , Iv     , NULL_OP, FALSE ,            },
  /* c0 */                                   
  {AMD64_GRP2a          , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
  {AMD64_GRP2b          , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
  {AMD64_RET       , NULL_OP, Iw     , NULL_OP, FALSE ,            },
  {AMD64_RET       , NULL_OP, NULL_OP, NULL_OP, FALSE ,            },
  {AMD64_INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, FALSE ,            },
  {AMD64_INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, FALSE ,            },
  {AMD64_GRP3a          , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
  {AMD64_GRP3b          , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
  /* c8 */                                   
  {AMD64_ENTER     , Iw     , Ib     , NULL_OP, FALSE ,            },
  {AMD64_LEAVE     , NULL_OP, NULL_OP, NULL_OP, FALSE ,            },
  {AMD64_RETF      , NULL_OP, Iw     , NULL_OP, FALSE ,            },
  {AMD64_RETF      , NULL_OP, NULL_OP, NULL_OP, FALSE ,            },
  {AMD64_INT3      , NULL_OP, NULL_OP, NULL_OP, FALSE ,            },
  {AMD64_INT       , NULL_OP, Ib     , NULL_OP, FALSE ,            },
  {AMD64_INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, FALSE ,            },
  {AMD64_IRET      , NULL_OP, NULL_OP, NULL_OP, FALSE ,            },
  /* d0 */                                   
  {AMD64_GRP4a          , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
  {AMD64_GRP4b          , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
  {AMD64_GRP4c          , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
  {AMD64_GRP4d          , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
  {AMD64_INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, FALSE ,            },
  {AMD64_INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, FALSE ,            },
  {AMD64_INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, FALSE ,            },
  {AMD64_XLAT      , AL     , NULL_OP, NULL_OP, FALSE , DU         },
  /* d8 */                                   
  {AMD64_FPU_ESC        , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
  {AMD64_FPU_ESC        , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
  {AMD64_FPU_ESC        , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
  {AMD64_FPU_ESC        , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
  {AMD64_FPU_ESC        , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
  {AMD64_FPU_ESC        , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
  {AMD64_FPU_ESC        , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
  {AMD64_FPU_ESC        , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
  /* e0 */                                   
  {AMD64_LOOPNZ    , NULL_OP, Jb     , NULL_OP, FALSE ,            },
  {AMD64_LOOPZ     , NULL_OP, Jb     , NULL_OP, FALSE ,            },
  {AMD64_LOOP      , NULL_OP, Jb     , NULL_OP, FALSE ,            },
  {AMD64_JRCXZ     , NULL_OP, Jb     , NULL_OP, FALSE ,            },
  {AMD64_IN        , AL     , Ib     , NULL_OP, FALSE ,            },
  {AMD64_IN        , eAX    , Ib     , NULL_OP, FALSE ,            },
  {AMD64_OUT       , NULL_OP, Ib     , AL     , FALSE ,            },
  {AMD64_OUT       , NULL_OP, Ib     , eAX    , FALSE ,            },
  /* e8 */                                   
  {AMD64_CALL      , NULL_OP, Jz     , NULL_OP, FALSE ,            },
  {AMD64_JMP       , NULL_OP, Jv     , NULL_OP, FALSE ,            },
  {AMD64_INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, FALSE ,            },
  {AMD64_JMP       , NULL_OP, Jb     , NULL_OP, FALSE ,            },
  {AMD64_IN        , AL     , DX     , NULL_OP, FALSE ,            },
  {AMD64_IN        , eAX    , DX     , NULL_OP, FALSE ,            },
  {AMD64_OUT       , NULL_OP, DX     , AL     , FALSE ,            },
  {AMD64_OUT       , NULL_OP, DX     , eAX    , FALSE ,            },
  /* f0 */                                   
  {AMD64_INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, FALSE ,            },    /* LOCK prefix  */
  {AMD64_INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, FALSE ,            },
  {AMD64_INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, FALSE ,            },    /* REPNE prefix */
  {AMD64_INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, FALSE ,            },    /* REPE prefix  */
  {AMD64_HLT       , NULL_OP, NULL_OP, NULL_OP, FALSE ,            },
  {AMD64_CMC       , NULL_OP, NULL_OP, NULL_OP, FALSE ,            },
  {AMD64_GRP5a          , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
  {AMD64_GRP5b          , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
  /* f8 */                                   
  {AMD64_CLC       , NULL_OP, NULL_OP, NULL_OP, FALSE ,            },
  {AMD64_STC       , NULL_OP, NULL_OP, NULL_OP, FALSE ,            },
  {AMD64_CLI       , NULL_OP, NULL_OP, NULL_OP, FALSE ,            },
  {AMD64_STI       , NULL_OP, NULL_OP, NULL_OP, FALSE ,            },
  {AMD64_CLD       , NULL_OP, NULL_OP, NULL_OP, FALSE ,            },
  {AMD64_STD       , NULL_OP, NULL_OP, NULL_OP, FALSE ,            },
  {AMD64_GRP6a          , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
  {AMD64_GRP6b          , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
  /*{LAST_OPC	  , NULL_OP, NULL_OP, NULL_OP, FALSE ,         	  }*/
};


t_amd64_opcode_entry disamd64_twobyte[] = 
{
  /* 00 */
  {AMD64_GRP7a          , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
  {AMD64_GRP7b          , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
  {AMD64_LAR       , Gv     , Ew     , NULL_OP, TRUE  ,            },
  {AMD64_LSL       , Gv     , Ew     , NULL_OP, TRUE  ,            },
  {AMD64_INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, FALSE ,            },
  {AMD64_SYSCALL    , NULL_OP, NULL_OP, NULL_OP, FALSE ,            },
  {AMD64_CLTS      , NULL_OP, NULL_OP, NULL_OP, FALSE ,            },
  {AMD64_INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, FALSE ,            },
  /* 08 */                                   
  {AMD64_INVD      , NULL_OP, NULL_OP, NULL_OP, FALSE ,            },
  {AMD64_WBINVD    , NULL_OP, NULL_OP, NULL_OP, FALSE ,            },
  {AMD64_INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, FALSE ,            },
  {AMD64_UD2       , NULL_OP, NULL_OP, NULL_OP, FALSE ,            },
  {AMD64_INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, FALSE ,            },
  {AMD64_INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, FALSE ,            },
  {AMD64_INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, FALSE ,            },
  {AMD64_3DNOW_OPC , NULL_OP, NULL_OP, NULL_OP, FALSE ,            },
  /* 10 */                                   
  {AMD64_PREFIX_GRP_1   , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },     /* all kinds of MMX and SSE(2) crap */
  {AMD64_PREFIX_GRP_4   , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
  {AMD64_PREFIX_GRP_33  , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
  {AMD64_PREFIX_GRP_3A  , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
  {AMD64_PREFIX_GRP_49  , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
  {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, FALSE ,            },
  {AMD64_PREFIX_GRP_38  , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
  {AMD64_PREFIX_GRP_3B  , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
  /* 18 */                                   
  {AMD64_GRP16          , NULL_OP, NULL_OP, NULL_OP, TRUE ,            },
  {AMD64_INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, FALSE ,            },
  {AMD64_INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, FALSE ,            },
  {AMD64_INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, FALSE ,            },
  {AMD64_INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, FALSE ,            },
  {AMD64_INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, FALSE ,            },
  {AMD64_INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, FALSE ,            },
  {AMD64_INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, FALSE ,            },
  /* 20 */                                   
  {AMD64_MOV       , Rq     , Cq     , NULL_OP, TRUE  ,            },
  {AMD64_MOV       , Rq     , Dq     , NULL_OP, TRUE  ,            },
  {AMD64_MOV       , Cq     , Rq     , NULL_OP, TRUE  ,            },
  {AMD64_MOV       , Dq     , Rq     , NULL_OP, TRUE  ,            },
  {AMD64_INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
  {AMD64_INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
  {AMD64_INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
  {AMD64_INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
  /* 28 */                                   
  {AMD64_PREFIX_GRP_9, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },//prefix_grp_9
  {AMD64_PREFIX_GRP_36, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },//prefix_grp_36
  {AMD64_PREFIX_GRP_3   , NULL_OP, NULL_OP, NULL_OP, TRUE ,            },
  {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, FALSE ,            },
  {AMD64_PREFIX_GRP_2   , NULL_OP, NULL_OP, NULL_OP, TRUE ,            },
  {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, FALSE ,            },
  {AMD64_PREFIX_GRP_47  , NULL_OP, NULL_OP, NULL_OP, TRUE ,            },
  {AMD64_PREFIX_GRP_5   , NULL_OP, NULL_OP, NULL_OP, TRUE ,            },
  /* 30 */                                   
  {AMD64_WRMSR     , ECX    , EDX    , EAX    , FALSE , DU         },
  {AMD64_RDTSC     , EDX    , EAX    , NULL_OP, FALSE , S1D        },
  {AMD64_RDMSR     , EDX    , EAX    , ECX    , FALSE , S1D        },
  {AMD64_RDPMC     , EDX    , EAX    , ECX    , FALSE , S1D        },
  {AMD64_INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, FALSE ,            },
  {AMD64_INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, FALSE ,            },
  {AMD64_INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, FALSE ,            },
  {AMD64_INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, FALSE ,            },
  /* 38 */                                   
  {AMD64_INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, FALSE ,            },
  {AMD64_INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, FALSE ,            },
  {AMD64_INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, FALSE ,            },
  {AMD64_INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, FALSE ,            },
  {AMD64_MOVNTI    , Gv     , Ev     , NULL_OP, FALSE ,            },
  {AMD64_INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, FALSE ,            },
  {AMD64_INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, FALSE ,            },
  {AMD64_INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, FALSE ,            },
  /* 40 */                                   
  {AMD64_CMOVcc    , Gv     , Ev     , NULL_OP, TRUE  ,            },
  {AMD64_CMOVcc    , Gv     , Ev     , NULL_OP, TRUE  ,            },
  {AMD64_CMOVcc    , Gv     , Ev     , NULL_OP, TRUE  ,            },
  {AMD64_CMOVcc    , Gv     , Ev     , NULL_OP, TRUE  ,            },
  {AMD64_CMOVcc    , Gv     , Ev     , NULL_OP, TRUE  ,            },
  {AMD64_CMOVcc    , Gv     , Ev     , NULL_OP, TRUE  ,            },
  {AMD64_CMOVcc    , Gv     , Ev     , NULL_OP, TRUE  ,            },
  {AMD64_CMOVcc    , Gv     , Ev     , NULL_OP, TRUE  ,            },
  /* 48 */                                   
  {AMD64_CMOVcc    , Gv     , Ev     , NULL_OP, TRUE  ,            },
  {AMD64_CMOVcc    , Gv     , Ev     , NULL_OP, TRUE  ,            },
  {AMD64_CMOVcc    , Gv     , Ev     , NULL_OP, TRUE  ,            },
  {AMD64_CMOVcc    , Gv     , Ev     , NULL_OP, TRUE  ,            },
  {AMD64_CMOVcc    , Gv     , Ev     , NULL_OP, TRUE  ,            },
  {AMD64_CMOVcc    , Gv     , Ev     , NULL_OP, TRUE  ,            },
  {AMD64_CMOVcc    , Gv     , Ev     , NULL_OP, TRUE  ,            },
  {AMD64_CMOVcc    , Gv     , Ev     , NULL_OP, TRUE  ,            },
  /* 50 */                                   
  {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, FALSE ,            },
  {AMD64_PREFIX_GRP_48   , NULL_OP, NULL_OP, NULL_OP, TRUE ,            },
  {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, FALSE ,            },
  {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, FALSE ,            },
  {AMD64_PREFIX_GRP_D   , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
  {AMD64_PREFIX_GRP_C   , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
  {AMD64_PREFIX_GRP_E   , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
  {AMD64_PREFIX_GRP_6   , NULL_OP, NULL_OP, NULL_OP, TRUE ,            },
  /* 58 */                                   
  {AMD64_PREFIX_GRP_35   , NULL_OP, NULL_OP, NULL_OP, TRUE ,            },
  {AMD64_PREFIX_GRP_39   , NULL_OP, NULL_OP, NULL_OP, TRUE ,            },
  {AMD64_PREFIX_GRP_8   , NULL_OP, NULL_OP, NULL_OP, TRUE ,            },
  {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, FALSE ,            },
  {AMD64_PREFIX_GRP_37   , NULL_OP, NULL_OP, NULL_OP, TRUE ,            },
  {AMD64_PREFIX_GRP_4B   , NULL_OP, NULL_OP, NULL_OP, TRUE ,            },
  {AMD64_PREFIX_GRP_46   , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
  {AMD64_PREFIX_GRP_A  , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
  /* 60 */                                   
  {AMD64_PREFIX_GRP_11   , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
  {AMD64_PREFIX_GRP_23   , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
  {AMD64_PREFIX_GRP_1A   , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
  {AMD64_PREFIX_GRP_2A   , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
  {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
  {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
  {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
  {AMD64_PREFIX_GRP_26   , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
  /* 68 */                                   
  {AMD64_PREFIX_GRP_12   , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
  {AMD64_PREFIX_GRP_24   , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
  {AMD64_PREFIX_GRP_29   , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
  {AMD64_PREFIX_GRP_27   , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
  {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, FALSE ,            },
  {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, FALSE ,            },
  {AMD64_PREFIX_GRP_10   , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
  {AMD64_PREFIX_GRP_F   , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
  /* 70 */                                   
  {AMD64_PREFIX_GRP_13  , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
  {AMD64_GRP12          , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
  {AMD64_GRP13          , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
  {AMD64_GRP14          , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
  {AMD64_PREFIX_GRP_43  , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
  {AMD64_PREFIX_GRP_25  , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
  {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
  {AMD64_EMMS     , NULL_OP, NULL_OP, NULL_OP, FALSE ,            },
  /* 78 */                                   
  {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, FALSE ,            },
  {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, FALSE ,            },
  {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, FALSE ,            },
  {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, FALSE ,            },
  {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, FALSE ,            },
  {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, FALSE ,            },
  {AMD64_PREFIX_GRP_2C   , NULL_OP, NULL_OP, NULL_OP, TRUE ,            },
  {AMD64_PREFIX_GRP_18   , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
  /* 80 */                                   
  {AMD64_Jcc       , NULL_OP, Jv     , NULL_OP, FALSE ,            },
  {AMD64_Jcc       , NULL_OP, Jv     , NULL_OP, FALSE ,            },
  {AMD64_Jcc       , NULL_OP, Jv     , NULL_OP, FALSE ,            },
  {AMD64_Jcc       , NULL_OP, Jv     , NULL_OP, FALSE ,            },
  {AMD64_Jcc       , NULL_OP, Jv     , NULL_OP, FALSE ,            },
  {AMD64_Jcc       , NULL_OP, Jv     , NULL_OP, FALSE ,            },
  {AMD64_Jcc       , NULL_OP, Jv     , NULL_OP, FALSE ,            },
  {AMD64_Jcc       , NULL_OP, Jv     , NULL_OP, FALSE ,            },
  /* 88 */                                   
  {AMD64_Jcc       , NULL_OP, Jv     , NULL_OP, FALSE ,            },
  {AMD64_Jcc       , NULL_OP, Jv     , NULL_OP, FALSE ,            },
  {AMD64_Jcc       , NULL_OP, Jv     , NULL_OP, FALSE ,            },
  {AMD64_Jcc       , NULL_OP, Jv     , NULL_OP, FALSE ,            },
  {AMD64_Jcc       , NULL_OP, Jv     , NULL_OP, FALSE ,            },
  {AMD64_Jcc       , NULL_OP, Jv     , NULL_OP, FALSE ,            },
  {AMD64_Jcc       , NULL_OP, Jv     , NULL_OP, FALSE ,            },
  {AMD64_Jcc       , NULL_OP, Jv     , NULL_OP, FALSE ,            },
  /* 90 */                                   
  {AMD64_SETcc     , Eb     , NULL_OP, NULL_OP, TRUE  ,            },
  {AMD64_SETcc     , Eb     , NULL_OP, NULL_OP, TRUE  ,            },
  {AMD64_SETcc     , Eb     , NULL_OP, NULL_OP, TRUE  ,            },
  {AMD64_SETcc     , Eb     , NULL_OP, NULL_OP, TRUE  ,            },
  {AMD64_SETcc     , Eb     , NULL_OP, NULL_OP, TRUE  ,            },
  {AMD64_SETcc     , Eb     , NULL_OP, NULL_OP, TRUE  ,            },
  {AMD64_SETcc     , Eb     , NULL_OP, NULL_OP, TRUE  ,            },
  {AMD64_SETcc     , Eb     , NULL_OP, NULL_OP, TRUE  ,            },
  /* 98 */                                   
  {AMD64_SETcc     , Eb     , NULL_OP, NULL_OP, TRUE  ,            },
  {AMD64_SETcc     , Eb     , NULL_OP, NULL_OP, TRUE  ,            },
  {AMD64_SETcc     , Eb     , NULL_OP, NULL_OP, TRUE  ,            },
  {AMD64_SETcc     , Eb     , NULL_OP, NULL_OP, TRUE  ,            },
  {AMD64_SETcc     , Eb     , NULL_OP, NULL_OP, TRUE  ,            },
  {AMD64_SETcc     , Eb     , NULL_OP, NULL_OP, TRUE  ,            },
  {AMD64_SETcc     , Eb     , NULL_OP, NULL_OP, TRUE  ,            },
  {AMD64_SETcc     , Eb     , NULL_OP, NULL_OP, TRUE  ,            },
  /* a0 */                                   
  {AMD64_PUSH      , NULL_OP, FS     , NULL_OP, FALSE ,            },
  {AMD64_POP       , FS     , NULL_OP, NULL_OP, FALSE ,            },
  {AMD64_CPUID     , NULL_OP, EAX    , NULL_OP, FALSE ,            },
  {AMD64_BT        , NULL_OP, Ev     , Gv     , TRUE  ,            },
  {AMD64_SHLD      , Ev     , Gv     , Ib     , TRUE  , DU         },
  {AMD64_SHLD      , Ev     , Gv     , CL     , TRUE  , DU         },
  {AMD64_INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
  {AMD64_INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
  /* a8 */                                   
  {AMD64_PUSH      , NULL_OP, GS     , NULL_OP, FALSE ,            },
  {AMD64_POP       , GS     , NULL_OP, NULL_OP, FALSE ,            },
  {AMD64_RSM       , NULL_OP, NULL_OP, NULL_OP, FALSE ,            },
  {AMD64_BTS       , NULL_OP, Ev     , Gv     , TRUE  , S1D        },
  {AMD64_SHRD      , Ev     , Gv     , Ib     , TRUE  , DU         },
  {AMD64_SHRD      , Ev     , Gv     , CL     , TRUE  , DU         },
  {AMD64_GRP15a         , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
  {AMD64_IMULexp1  , Gv     , Ev     , NULL_OP, TRUE  , DU         },  /* explicit dest, dest is source1 */
  /* b0 */                                   
  {AMD64_CMPXCHG   , Eb     , Gb     , NULL_OP, TRUE  , DU|S1D     },
  {AMD64_CMPXCHG   , Ev     , Gv     , NULL_OP, TRUE  , DU|S1D     },
  {AMD64_LSS       , SS     , Gv     , Mp     , TRUE  , S1D        },
  {AMD64_BTR       , NULL_OP, Ev     , Gv     , TRUE  , S1D        },
  {AMD64_LFS       , FS     , Gv     , Mp     , TRUE  , S1D        },
  {AMD64_LGS       , GS     , Gv     , Mp     , TRUE  , S1D        },
  {AMD64_MOVZX     , Gv     , Eb     , NULL_OP, TRUE  ,            },
  {AMD64_MOVZX     , Gv     , Ew     , NULL_OP, TRUE  ,            },
  /* b8 */                                   
  {AMD64_INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, FALSE ,            },
  {AMD64_UD2       , NULL_OP, NULL_OP, NULL_OP, FALSE ,            },
  {AMD64_GRP8a          , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
  {AMD64_BTC       , NULL_OP, Ev     , Gv     , TRUE  , S1D        },
  {AMD64_BSF       , Gv     , Ev     , NULL_OP, TRUE  ,            },
  {AMD64_BSR       , Gv     , Ev     , NULL_OP, TRUE  ,            },
  {AMD64_MOVSX     , Gv     , Eb     , NULL_OP, TRUE  ,            },
  {AMD64_MOVSX     , Gv     , Ew     , NULL_OP, TRUE  ,            },
  /* c0 */                                   
  {AMD64_XADD      , Eb     , Gb     , NULL_OP, TRUE  , DU|S1D     },
  {AMD64_XADD      , Ev     , Gv     , NULL_OP, TRUE  , DU|S1D     },
  {AMD64_PREFIX_GRP_B   , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
  {AMD64_MOVNTI    , Ed     , Gv     , NULL_OP, TRUE  ,            },
  {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
  {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
  {AMD64_PREFIX_GRP_34   , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
  {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
  /* c8 */                                   
  {AMD64_BSWAP     , rAXrex , NULL_OP, NULL_OP, FALSE , DU         },
  {AMD64_BSWAP     , rCXrex , NULL_OP, NULL_OP, FALSE , DU         },
  {AMD64_BSWAP     , rDXrex , NULL_OP, NULL_OP, FALSE , DU         },
  {AMD64_BSWAP     , rBXrex , NULL_OP, NULL_OP, FALSE , DU         },
  {AMD64_BSWAP     , rSPrex , NULL_OP, NULL_OP, FALSE , DU         },
  {AMD64_BSWAP     , rBPrex , NULL_OP, NULL_OP, FALSE , DU         },
  {AMD64_BSWAP     , rSIrex , NULL_OP, NULL_OP, FALSE , DU         },
  {AMD64_BSWAP     , rDIrex , NULL_OP, NULL_OP, FALSE , DU         },
  /* d0 */                                   
  {AMD64_INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, FALSE ,            },
  {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
  {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
  {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
  {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, FALSE ,            },
  {AMD64_PREFIX_GRP_15   , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
  {AMD64_PREFIX_GRP_4A   , NULL_OP, NULL_OP, NULL_OP, TRUE ,            },
  {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, FALSE ,            },
  /* d8 */                                   
  {AMD64_PREFIX_GRP_3C   , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
  {AMD64_PREFIX_GRP_42   , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
  {AMD64_PREFIX_GRP_44   , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
  {AMD64_PREFIX_GRP_1B   , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
  {AMD64_PREFIX_GRP_28   , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
  {AMD64_PREFIX_GRP_41   , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
  {AMD64_PREFIX_GRP_45   , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
  {AMD64_PREFIX_GRP_40   , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
  /* e0 */                                   
  {AMD64_PREFIX_GRP_2D   , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
  {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
  {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
  {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, FALSE ,            },
  {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, FALSE ,            },
  {AMD64_PREFIX_GRP_2F   , NULL_OP, NULL_OP, NULL_OP, TRUE ,            },
  {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, FALSE ,            },
  {AMD64_PREFIX_GRP_19  , NULL_OP, NULL_OP, NULL_OP, TRUE ,            },
  /* e8 */                                   
  {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
  {AMD64_PREFIX_GRP_2E   , NULL_OP, NULL_OP, NULL_OP, TRUE ,            },
  {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, FALSE ,            },
  {AMD64_PREFIX_GRP_1E   , NULL_OP, NULL_OP, NULL_OP, TRUE ,            },
  {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
  {AMD64_PREFIX_GRP_30   , NULL_OP, NULL_OP, NULL_OP, TRUE ,            },
  {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
  {AMD64_PREFIX_GRP_7   , NULL_OP, NULL_OP, NULL_OP, TRUE ,            },
  /* f0 */                                   
  {AMD64_INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, FALSE ,            },
  {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
  {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
  {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
  {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, FALSE ,            },
  {AMD64_PREFIX_GRP_1F  , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
  {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, FALSE ,            },
  {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, FALSE ,            },
  /* f8 */                                   
  {AMD64_PREFIX_GRP_3D  , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
  {AMD64_PREFIX_GRP_14   , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
  {AMD64_PREFIX_GRP_32  , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
  {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, FALSE ,            },
  {AMD64_PREFIX_GRP_2B  , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
  {AMD64_PREFIX_GRP_17  , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
  {AMD64_PREFIX_GRP_31  , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
  {AMD64_INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, FALSE ,            }
};
  
t_amd64_opcode_entry disamd64_grps[][8] =
{
  /* AMD64_GRP1a */
  {
    {AMD64_ADD       , Eb     , Ib     , NULL_OP, TRUE  , DU         },
    {AMD64_OR        , Eb     , Ib     , NULL_OP, TRUE  , DU         },
    {AMD64_ADC       , Eb     , Ib     , NULL_OP, TRUE  , DU         },
    {AMD64_SBB       , Eb     , Ib     , NULL_OP, TRUE  , DU         },
    {AMD64_AND       , Eb     , Ib     , NULL_OP, TRUE  , DU         },
    {AMD64_SUB       , Eb     , Ib     , NULL_OP, TRUE  , DU         },
    {AMD64_XOR       , Eb     , Ib     , NULL_OP, TRUE  , DU         },
    {AMD64_CMP       , NULL_OP, Eb     , Ib     , TRUE  ,            },
  },
  /* AMD64_GRP1b */
  {
    {AMD64_ADD       , Ev     , Iz     , NULL_OP, TRUE  , DU         },
    {AMD64_OR        , Ev     , Iz     , NULL_OP, TRUE  , DU         },
    {AMD64_ADC       , Ev     , Iz     , NULL_OP, TRUE  , DU         },
    {AMD64_SBB       , Ev     , Iz     , NULL_OP, TRUE  , DU         },
    {AMD64_AND       , Ev     , Iz     , NULL_OP, TRUE  , DU         },
    {AMD64_SUB       , Ev     , Iz     , NULL_OP, TRUE  , DU         },
    {AMD64_XOR       , Ev     , Iz     , NULL_OP, TRUE  , DU         },
    {AMD64_CMP       , NULL_OP, Ev     , Iz     , TRUE  ,            },
  },
  /* AMD64_GRP1c */
  {
    {AMD64_INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
  },
  /* AMD64_GRP1d */
  {
    {AMD64_ADD       , Ev     , sIb    , NULL_OP, TRUE  , DU         },
    {AMD64_OR        , Ev     , sIb    , NULL_OP, TRUE  , DU         },
    {AMD64_ADC       , Ev     , sIb    , NULL_OP, TRUE  , DU         },
    {AMD64_SBB       , Ev     , sIb    , NULL_OP, TRUE  , DU         },
    {AMD64_AND       , Ev     , sIb    , NULL_OP, TRUE  , DU         },
    {AMD64_SUB       , Ev     , sIb    , NULL_OP, TRUE  , DU         },
    {AMD64_XOR       , Ev     , sIb    , NULL_OP, TRUE  , DU         },
    {AMD64_CMP       , NULL_OP, Ev     , sIb    , TRUE  ,            },
  },
  /* AMD64_GRP2a */
  {
    {AMD64_ROL       , Eb     , Ib     , NULL_OP, TRUE  , DU         },
    {AMD64_ROR       , Eb     , Ib     , NULL_OP, TRUE  , DU         },
    {AMD64_RCL       , Eb     , Ib     , NULL_OP, TRUE  , DU         },
    {AMD64_RCR       , Eb     , Ib     , NULL_OP, TRUE  , DU         },
    {AMD64_SHL       , Eb     , Ib     , NULL_OP, TRUE  , DU         },
    {AMD64_SHR       , Eb     , Ib     , NULL_OP, TRUE  , DU         },
    {AMD64_INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_SAR       , Eb     , Ib     , NULL_OP, TRUE  , DU         },
  },
  /* AMD64_GRP2b */
  {
    {AMD64_ROL       , Ev     , Ib     , NULL_OP, TRUE  , DU         },
    {AMD64_ROR       , Ev     , Ib     , NULL_OP, TRUE  , DU         },
    {AMD64_RCL       , Ev     , Ib     , NULL_OP, TRUE  , DU         },
    {AMD64_RCR       , Ev     , Ib     , NULL_OP, TRUE  , DU         },
    {AMD64_SHL       , Ev     , Ib     , NULL_OP, TRUE  , DU         },
    {AMD64_SHR       , Ev     , Ib     , NULL_OP, TRUE  , DU         },
    {AMD64_INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_SAR       , Ev     , Ib     , NULL_OP, TRUE  , DU         },
  },
  /* AMD64_GRP3a */
  {
    {AMD64_MOV       , Eb     , Ib     , NULL_OP, TRUE  ,            },
    {AMD64_INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
  },
  /* AMD64_GRP3b */
  {
    {AMD64_MOV       , Ev     , Iz     , NULL_OP, TRUE  ,            },
    {AMD64_INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
  },
  /* AMD64_GRP4a */
  {
    {AMD64_ROL       , Eb     , C_1    , NULL_OP, TRUE  , DU         },
    {AMD64_ROR       , Eb     , C_1    , NULL_OP, TRUE  , DU         },
    {AMD64_RCL       , Eb     , C_1    , NULL_OP, TRUE  , DU         },
    {AMD64_RCR       , Eb     , C_1    , NULL_OP, TRUE  , DU         },
    {AMD64_SHL       , Eb     , C_1    , NULL_OP, TRUE  , DU         },
    {AMD64_SHR       , Eb     , C_1    , NULL_OP, TRUE  , DU         },
    {AMD64_INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_SAR       , Eb     , C_1    , NULL_OP, TRUE  , DU         },
  },
  /* AMD64_GRP4b */
  {
    {AMD64_ROL       , Ev     , C_1    , NULL_OP, TRUE  , DU         },
    {AMD64_ROR       , Ev     , C_1    , NULL_OP, TRUE  , DU         },
    {AMD64_RCL       , Ev     , C_1    , NULL_OP, TRUE  , DU         },
    {AMD64_RCR       , Ev     , C_1    , NULL_OP, TRUE  , DU         },
    {AMD64_SHL       , Ev     , C_1    , NULL_OP, TRUE  , DU         },
    {AMD64_SHR       , Ev     , C_1    , NULL_OP, TRUE  , DU         },
    {AMD64_INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_SAR       , Ev     , C_1    , NULL_OP, TRUE  , DU         },
  },
  /* AMD64_GRP4c */
  {
    {AMD64_ROL       , Eb     , CL     , NULL_OP, TRUE  , DU         },
    {AMD64_ROR       , Eb     , CL     , NULL_OP, TRUE  , DU         },
    {AMD64_RCL       , Eb     , CL     , NULL_OP, TRUE  , DU         },
    {AMD64_RCR       , Eb     , CL     , NULL_OP, TRUE  , DU         },
    {AMD64_SHL       , Eb     , CL     , NULL_OP, TRUE  , DU         },
    {AMD64_SHR       , Eb     , CL     , NULL_OP, TRUE  , DU         },
    {AMD64_INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_SAR       , Eb     , CL     , NULL_OP, TRUE  , DU         },
  },
  /* AMD64_GRP4d */
  {
    {AMD64_ROL       , Ev     , CL     , NULL_OP, TRUE  , DU         },
    {AMD64_ROR       , Ev     , CL     , NULL_OP, TRUE  , DU         },
    {AMD64_RCL       , Ev     , CL     , NULL_OP, TRUE  , DU         },
    {AMD64_RCR       , Ev     , CL     , NULL_OP, TRUE  , DU         },
    {AMD64_SHL       , Ev     , CL     , NULL_OP, TRUE  , DU         },
    {AMD64_SHR       , Ev     , CL     , NULL_OP, TRUE  , DU         },
    {AMD64_INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_SAR       , Ev     , CL     , NULL_OP, TRUE  , DU         },
  },
  /* AMD64_GRP5a */
  {
    {AMD64_TEST      , NULL_OP, Eb     , Ib     , TRUE  ,            },
    {AMD64_INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_NOT       , Eb     , NULL_OP, NULL_OP, TRUE  , DU         },
    {AMD64_NEG       , Eb     , NULL_OP, NULL_OP, TRUE  , DU         },
    {AMD64_MUL       , AL     , Eb     , NULL_OP, TRUE  , DU         },
    {AMD64_IMUL      , AL     , Eb     , NULL_OP, TRUE  , DU         },
    {AMD64_DIV       , AL     , Eb     , NULL_OP, TRUE  , DU         },
    {AMD64_IDIV      , AL     , Eb     , NULL_OP, TRUE  , DU         },
  },
  /* AMD64_GRP5b */
  {
    {AMD64_TEST      , NULL_OP, Ev     , Iz     , TRUE  ,            },
    {AMD64_INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_NOT       , Ev     , NULL_OP, NULL_OP, TRUE  , DU         },
    {AMD64_NEG       , Ev     , NULL_OP, NULL_OP, TRUE  , DU         },
    {AMD64_MUL       , rAX    , Ev     , NULL_OP, TRUE  , DU         },
    {AMD64_IMUL      , rAX    , Ev     , NULL_OP, TRUE  , DU         },
    {AMD64_DIV       , rAX    , Ev     , NULL_OP, TRUE  , DU         },
    {AMD64_IDIV      , rAX    , Ev     , NULL_OP, TRUE  , DU         },
  },
  /* AMD64_GRP6a */
  {
    {AMD64_INC       , Eb     , NULL_OP, NULL_OP, TRUE  , DU         },
    {AMD64_DEC       , Eb     , NULL_OP, NULL_OP, TRUE  , DU         },
    {AMD64_INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
  },
  /* AMD64_GRP6b */
  {
    {AMD64_INC       , Ev     , NULL_OP, NULL_OP, TRUE  , DU         },
    {AMD64_DEC       , Ev     , NULL_OP, NULL_OP, TRUE  , DU         },
    {AMD64_CALL      , NULL_OP, Ev8    , NULL_OP, TRUE  ,            },
    {AMD64_CALLF     , NULL_OP, Ep     , NULL_OP, TRUE  ,            },
    {AMD64_JMP       , NULL_OP, Ev8    , NULL_OP, TRUE  ,            },
    {AMD64_JMPF      , NULL_OP, Ep     , NULL_OP, TRUE  ,            },
    {AMD64_PUSH      , NULL_OP, Ev8    , NULL_OP, TRUE  ,            },
    {AMD64_INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
  },
  /* AMD64_GRP7a */
  {
    {AMD64_SLDT      , Ew     , NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_STR       , Ev     , NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_LLDT      , NULL_OP, Ew     , NULL_OP, TRUE  ,            },
    {AMD64_LTR       , NULL_OP, Ew     , NULL_OP, TRUE  ,            },
    {AMD64_VERR      , NULL_OP, Ew     , NULL_OP, TRUE  ,            },
    {AMD64_VERW      , NULL_OP, Ew     , NULL_OP, TRUE  ,            },
    {AMD64_INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
  },
  /* AMD64_GRP7b */
  {
    {AMD64_SGDT      , Ms     , NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_SIDT      , Ms     , NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_LGDT      , NULL_OP, Ms     , NULL_OP, TRUE  ,            },
    {AMD64_LIDT      , NULL_OP, Ms     , NULL_OP, TRUE  ,            },
    {AMD64_SMSW      , Ew     , NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_LMSW      , NULL_OP, Ew     , NULL_OP, TRUE  ,            },
    {AMD64_INVLPG    , Mb     , NULL_OP, NULL_OP, TRUE  ,            },
  },
  /* AMD64_GRP8a */
  {
    {AMD64_INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_BT        , NULL_OP, Ev     , Ib     , TRUE  ,            },
    {AMD64_BTS       , NULL_OP, Ev     , Ib     , TRUE  , S1D        },
    {AMD64_BTR       , NULL_OP, Ev     , Ib     , TRUE  , S1D        },
    {AMD64_BTC       , NULL_OP, Ev     , Ib     , TRUE  , S1D        },
  },
  /* AMD64_GRP12 */
  {
    {AMD64_INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_PREFIX_GRP_3F  , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_PREFIX_GRP_3E  , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_PREFIX_GRP_16  , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
  },
  /* AMD64_GRP13 */
  {
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_PREFIX_GRP_20  , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_PREFIX_GRP_22  , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_PREFIX_GRP_21  , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
  },
  /* AMD64_GRP14 */
  {
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_PREFIX_GRP_1C  , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_PSLLDQ    , Wdq    , Ib     , NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_PREFIX_GRP_1D  , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_PSLLDQ    , Wdq    , Ib     , NULL_OP, TRUE  ,            },
  },
  /* AMD64_GRP15a */
  {
    {AMD64_FXSAVE    , M512   , NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_FXRSTOR   , NULL_OP, M512   , NULL_OP, TRUE  ,            },
    {AMD64_STMXCSR   , Md     , NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_LDMXCSR   , NULL_OP, Md     , NULL_OP, TRUE  ,            },
    {AMD64_INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
  },
  /* AMD64_GRP15a11 */
  {
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_SFENCE    , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_LFENCE    , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_MFENCE    , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },

  },
  /* AMD64_GRP16 */
  {
    {AMD64_PREFETCH_NTA, NULL_OP, NULL_OP, NULL_OP, TRUE,            },
    {AMD64_PREFETCH_T0 , NULL_OP, NULL_OP, NULL_OP, TRUE,            },
    {AMD64_PREFETCH_T1 , NULL_OP, NULL_OP, NULL_OP, TRUE,            },
    {AMD64_PREFETCH_T2 , NULL_OP, NULL_OP, NULL_OP, TRUE,            },
    {AMD64_INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
  },
  /* AMD64_PREFIX_GRP_1 */
  {
    {AMD64_MOVSD_SSE , Vsd    , Wsd    , NULL_OP, TRUE  ,            },
    {AMD64_MOVSS     , Vss    , Wss    , NULL_OP, TRUE  ,            },
    {AMD64_MOVUPS    , Vps    , Wps    , NULL_OP, TRUE  ,            },
    {AMD64_MOVUPD    , Vpd    , Wpd    , NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
  },
  /* AMD64_PREFIX_GRP_2 */
  {
    {AMD64_CVTTSD2SI , Gv     , Wsd    , NULL_OP, TRUE  ,            },
    {AMD64_CVTTSS2SI , Gv     , Wss    , NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
  },
  /* AMD64_PREFIX_GRP_3 */
  {
    {AMD64_CVTSI2SD  , Vsd    , Ev     , NULL_OP, TRUE  ,            },
    {AMD64_CVTSI2SS  , Vss    , Ev     , NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
  },
  /* AMD64_PREFIX_GRP_4 */
  {
    {AMD64_MOVSD_SSE, Wsd    , Vsd    , NULL_OP, TRUE  ,            },
    {AMD64_MOVSS    , Wss    , Vss    , NULL_OP, TRUE  ,            },
    {AMD64_MOVUPS   , Wps    , Vps    , NULL_OP, TRUE  ,            },
    {AMD64_MOVUPD   , Wpd    , Vpd    , NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
  },
  /* AMD64_PREFIX_GRP_5 */
  {
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_COMISS   , Vps    , Wps    , NULL_OP, TRUE  ,DU          },
    {AMD64_COMISD   , Vsd    , Wsd    , NULL_OP, TRUE  ,DU          },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
  },
  /* AMD64_PREFIX_GRP_6 */
  {
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_XORPS    , Vps    , Wps    , NULL_OP, TRUE  ,DU          },
    {AMD64_XORPD    , Vpd    , Wpd    , NULL_OP, TRUE  ,DU          },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
  },
  /* AMD64_PREFIX_GRP_7 */
  {
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_PXOR     , Pq     , Qq     , NULL_OP, TRUE  ,DU          },
    {AMD64_PXORD    , Vdq    , Wdq    , NULL_OP, TRUE  ,DU          },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
  },
  /* AMD64_PREFIX_GRP_8 */
  {
    {AMD64_CVTSD2SS , Vss    , Wsd    , NULL_OP, TRUE  ,            },
    {AMD64_CVTSD2SD , Vsd    , Wss    , NULL_OP, TRUE  ,            },
    {AMD64_CVTSD2PD , Vpd    , Wq     , NULL_OP, TRUE  ,            },
    {AMD64_CVTSD2PS , Vps    , Wpd    , NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
  },
  /* AMD64_PREFIX_GRP_9 */
  {
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_MOVAPS   , Vps    , Wps    , NULL_OP, TRUE  ,            },
    {AMD64_MOVAPD   , Vpd    , Wpd    , NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
  },
  /* AMD64_PREFIX_GRP_A */
  {
    {AMD64_MAXSD    , Vsd    , Wsd    , NULL_OP, TRUE  ,DU          },
    {AMD64_MAXSS    , Vss    , Wss    , NULL_OP, TRUE  ,DU          },
    {AMD64_MAXPS    , Vps    , Wps    , NULL_OP, TRUE  ,DU          },
    {AMD64_MAXPD    , Vpd    , Wpd    , NULL_OP, TRUE  ,DU          },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
  },
  /* AMD64_PREFIX_GRP_B */
  {
    {AMD64_CMPSD_SSE, Vsd    , Wsd    , Ib     , TRUE  ,DU          },
    {AMD64_CMPSS    , Vss    , Wss    , Ib     , TRUE  ,DU          },
    {AMD64_CMPPS    , Vps    , Wps    , Ib     , TRUE  ,DU          },
    {AMD64_CMPPD    , Vpd    , Wpd    , Ib     , TRUE  ,DU          },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
  },
  /* AMD64_PREFIX_GRP_C */
  {
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_ANDNPS   , Vps    , Wps    , NULL_OP, TRUE  ,DU          },
    {AMD64_ANDNPD   , Vpd    , Wpd    , NULL_OP, TRUE  ,DU          },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
  },
  /* AMD64_PREFIX_GRP_D */
  {
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_ANDPS    , Vps    , Wps    , NULL_OP, TRUE  ,DU          },
    {AMD64_ANDPD    , Vpd    , Wpd    , NULL_OP, TRUE  ,DU          },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
  },
  /* AMD64_PREFIX_GRP_E */
  {
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_ORPS     , Vps    , Wps    , NULL_OP, TRUE  ,DU          },
    {AMD64_ORPD     , Vpd    , Wpd    , NULL_OP, TRUE  ,DU          },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
  },
  /* AMD64_PREFIX_GRP_F */
  {
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_MOVDQU    , Vdq    , Wdq    , NULL_OP, TRUE  ,            },
    {AMD64_MOVQ      , Pq     , Qq     , NULL_OP, TRUE  ,            },
    {AMD64_MOVDQA    , Vdq    , Wdq    , NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
  },
  /* AMD64_PREFIX_GRP_10 */
  {
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_MOVD      , Pd     , Ed     , NULL_OP, TRUE  ,            },
    {AMD64_MOVD2     , Vd     , Ed     , NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
  },
  /* AMD64_PREFIX_GRP_11 */
  {
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_PUNPCKLBW , Pq     , Qq     , NULL_OP, TRUE  ,            },
    {AMD64_PUNPCKLBW2, Vdq    , Qdq    , NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
  },
  /* AMD64_PREFIX_GRP_12 */
  {
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_PUNPCKHBW, Pq     , Qq     , NULL_OP, TRUE  ,            },
    {AMD64_PUNPCKHBW2, Vdq   , Qdq    , NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
  },
  /* AMD64_PREFIX_GRP_13 */
  {
    {AMD64_PSHUFLW  , Vdq    , Wdq    , Ib     , TRUE  ,            },
    {AMD64_PSHUFHW  , Vdq    , Wdq    , Ib     , TRUE  ,            },
    {AMD64_PSHUFW   , Pq     , Qq     , Ib     , TRUE  ,            },
    {AMD64_PSHUFD   , Vdq    , Wdq    , Ib     , TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
  },
  /* AMD64_PREFIX_GRP_14 */
  {
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_PSUBW    , Pq     , Qq     , NULL_OP, TRUE  ,DU          },
    {AMD64_PSUBW2   , Vdq    , Wdq    , NULL_OP, TRUE  ,DU          },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
  },
  /* AMD64_PREFIX_GRP_15 */
  {
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_PMULLW   , Pq     , Qq     , NULL_OP, TRUE  ,DU          },
    {AMD64_PMULLW2  , Vdq    , Wdq    , NULL_OP, TRUE  ,DU          },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
  },
  /* AMD64_PREFIX_GRP_16 */
  {
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_PSLLW    , Pq     , Ib     , NULL_OP, TRUE  ,DU          },
    {AMD64_PSLLW2   , Pdq    , Ib     , NULL_OP, TRUE  ,DU          },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
  },
  /* AMD64_PREFIX_GRP_17 */
  {
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_PADDW    , Pq     , Qq     , NULL_OP, TRUE  ,DU          },
    {AMD64_PADDW2   , Vdq    , Wdq    , NULL_OP, TRUE  ,DU          },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
  },
  /* AMD64_PREFIX_GRP_18 */
  {
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_MOVDQU   , Wdq    , Vdq    , NULL_OP, TRUE  ,            },
    {AMD64_MOVQ     , Qq     , Pq     , NULL_OP, TRUE  ,            },
    {AMD64_MOVDQA   , Wdq    , Vdq    , NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
  },
  /* AMD64_PREFIX_GRP_19 */
  {
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_MOVNTQ    , Mq     , Vq     , NULL_OP, TRUE  ,            },
    {AMD64_MOVNTQD   , Mdq    , Vdq    , NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
  },
  /* AMD64_PREFIX_GRP_1A */
  {
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_PUNPCKLDQ, Pq     , Qq     , NULL_OP, TRUE  ,            },
    {AMD64_PUNPCKLDQ2,Vdq    , Wdq    , NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
  },
  /* AMD64_PREFIX_GRP_1B */
  {
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_PAND     , Pq     , Qq     , NULL_OP, TRUE  ,DU          },
    {AMD64_PAND2    , Vdq    , Wdq    , NULL_OP, TRUE  ,DU          },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
  },
  /* AMD64_PREFIX_GRP_1C */
  {
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_PSRLQ    , Pq     , Ib     , NULL_OP, TRUE  ,DU          },
    {AMD64_PSRLQ2   , Wdq    , Ib     , NULL_OP, TRUE  ,DU          },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
  },
  /* AMD64_PREFIX_GRP_1D */
  {
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_PSLLQ    , Pq     , Ib     , NULL_OP, TRUE  ,DU          },
    {AMD64_PSLLQ2   , Wdq    , Ib     , NULL_OP, TRUE  ,DU          },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
  },
  /* AMD64_PREFIX_GRP_1E */
  {
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_POR      , Pq     , Qq     , NULL_OP, TRUE  ,DU          },
    {AMD64_POR2     , Vdq    , Wdq    , NULL_OP, TRUE  ,DU          },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
  },
  /* AMD64_PREFIX_GRP_1F */
  {
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_PMADDWD  , Pq     , Qq     , NULL_OP, TRUE  ,DU          },
    {AMD64_PMADDWD2 , Vdq    , Wdq    , NULL_OP, TRUE  ,DU          },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
  },
  /* AMD64_PREFIX_GRP_20 */
  {
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_PSRLD    , Pq     , Ib     , NULL_OP, TRUE  ,DU          },
    {AMD64_PSRLD2   , Wdq    , Ib     , NULL_OP, TRUE  ,DU          },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
  },
  /* AMD64_PREFIX_GRP_21 */
  {
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_PSLLD    , Pq     , Ib     , NULL_OP, TRUE  ,DU          },
    {AMD64_PSLLD2   , Wdq    , Ib     , NULL_OP, TRUE  ,DU          },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
  },
  /* AMD64_PREFIX_GRP_22 */
  {
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_PSRAD    , Pq     , Ib     , NULL_OP, TRUE  ,DU          },
    {AMD64_PSRAD2   , Wdq    , Ib     , NULL_OP, TRUE  ,DU          },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
  },
  /* AMD64_PREFIX_GRP_23 */
  {
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_PUNPCKLWD, Pq     , Qq     , NULL_OP, TRUE  ,            },
    {AMD64_PUNPCKLWD2,Vdq    , Wdq    , NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
  },
  /* AMD64_PREFIX_GRP_24 */
  {
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_PUNPCKHWD, Pq     , Qq     , NULL_OP, TRUE  ,            },
    {AMD64_PUNPCKHWD2,Vdq    , Qdq    , NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
  },
  /* AMD64_PREFIX_GRP_25 */
  {
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_PCMPEQW  , Pq     , Qq     , NULL_OP, TRUE  ,DU          },
    {AMD64_PCMPEQW2 , Vdq    , Wdq    , NULL_OP, TRUE  ,DU          },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
  },
  /* AMD64_PREFIX_GRP_26 */
  {
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_PACKUSWB , Pq     , Qq     , NULL_OP, TRUE  ,            },
    {AMD64_PACKUSWB2, Vdq    , Wdq    , NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
  },
  /* AMD64_PREFIX_GRP_27 */
  {
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_PACKSSDW , Pq     , Qq     , NULL_OP, TRUE  ,            },
    {AMD64_PACKSSDW2, Vdq    , Qdq    , NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
  },
  /* AMD64_PREFIX_GRP_28 */
  {
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_PADDUSB  , Pq     , Qq     , NULL_OP, TRUE  ,DU          },
    {AMD64_PADDUSB2 , Vdq    , Wdq    , NULL_OP, TRUE  ,DU          },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
  },
  /* AMD64_PREFIX_GRP_29 */
  {
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_PUNPCKHDQ, Pq     , Qq     , NULL_OP, TRUE  ,            },
    {AMD64_PUNPCKHDQ2,Vdq    , Qdq    , NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
  },
  /* AMD64_PREFIX_GRP_2A */
  {
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_PACKSSWB  , Pq     , Qq     , NULL_OP, TRUE  ,            },
    {AMD64_PACKSSWB2 , Vdq    , Wdq    , NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
  },
  /* AMD64_PREFIX_GRP_2B */
  {
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_PADDW    , Pq     , Qq     , NULL_OP, TRUE  ,DU          },
    {AMD64_PADDW2   , Vdq    , Wdq    , NULL_OP, TRUE  ,DU          },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
  },
  /* AMD64_PREFIX_GRP_2C */
  {
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_MOVD3     ,  Ed    , Pd     , NULL_OP, TRUE  ,            },
    {AMD64_MOVD4     , Ed     , Vd     , NULL_OP, TRUE  ,            },
    {AMD64_MOVQ3     , Vq     , Wq     , NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
  },
  /* AMD64_PREFIX_GRP_2D */
  {
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_PAVGB    , Pq     , Qq     , NULL_OP, TRUE  ,DU          },
    {AMD64_PAVGB2   , Vdq    , Wdq    , NULL_OP, TRUE  ,DU          },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
  },
  /* AMD64_PREFIX_GRP_2E */
  {
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_PSUBSW   , Pq     , Qq     , NULL_OP, TRUE  ,DU          },
    {AMD64_PSUBSW2  , Vdq    , Wdq    , NULL_OP, TRUE  ,DU          },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
  },
  /* AMD64_PREFIX_GRP_2F */
  {
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_PMULHW   , Pq     , Qq     , NULL_OP, TRUE  ,DU          },
    {AMD64_PMULHW2  , Vdq    , Wdq    , NULL_OP, TRUE  ,DU          },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
  },
  /* AMD64_PREFIX_GRP_30 */
  {
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_PADDSW   , Pq     , Qq     , NULL_OP, TRUE  ,DU          },
    {AMD64_PADDSW2  , Vdq    , Wdq    , NULL_OP, TRUE  ,DU          },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
  },
  /* AMD64_PREFIX_GRP_31 */
  {
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_PADDD    , Pq     , Qq     , NULL_OP, TRUE  ,DU          },
    {AMD64_PADDD2   , Vdq    , Wdq    , NULL_OP, TRUE  ,DU          },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
  },
  /* AMD64_PREFIX_GRP_32 */
  {
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_PSUBD    , Pq     , Qq     , NULL_OP, TRUE  ,DU          },
    {AMD64_PSUBD2   , Vdq    , Wdq    , NULL_OP, TRUE  ,DU          },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
  },
  /* AMD64_PREFIX_GRP_33 */
  {
    {AMD64_MOVDDUP  , Vq     , Wq     , NULL_OP, TRUE  ,            },
    {AMD64_MOVSLDUP , Vps    , Wps    , NULL_OP, TRUE  ,            },
    {AMD64_MOVLPS   , Vq     , Mq     , NULL_OP, TRUE  ,            },
    {AMD64_MOVLPD   , Vq     , Mq     , NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
  },
  /* AMD64_PREFIX_GRP_34 */
  {
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_PSHUFPS  , Vps    , Wps    , Ib     , TRUE  ,DU          },
    {AMD64_PSHUFPD  , Vpd    , Wpd    , Ib     , TRUE  ,DU          },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
  },
  /* AMD64_PREFIX_GRP_35 */
  {
    {AMD64_ADDSD    , Vsd    , Wsd    , NULL_OP, TRUE  ,DU          },
    {AMD64_ADDSS    , Vss    , Wss    , NULL_OP, TRUE  ,DU          },
    {AMD64_ADDPS    , Vps    , Wps    , NULL_OP, TRUE  ,DU          },
    {AMD64_ADDPD    , Vpd    , Wpd    , NULL_OP, TRUE  ,DU          },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
  },
  /* AMD64_PREFIX_GRP_36 */
  {
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_MOVAPS   , Wps    , Vps    , NULL_OP, TRUE  ,            },
    {AMD64_MOVAPD   , Wpd    , Vpd    , NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
  },
  /* AMD64_PREFIX_GRP_37 */
  {
    {AMD64_SUBSD    , Vsd    , Wsd    , NULL_OP, TRUE  ,DU          },
    {AMD64_SUBSS    , Vss    , Wss    , NULL_OP, TRUE  ,DU          },
    {AMD64_SUBPS    , Vps    , Wps    , NULL_OP, TRUE  ,DU          },
    {AMD64_SUBPD    , Vpd    , Wpd    , NULL_OP, TRUE  ,DU          },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
  },
  /* AMD64_PREFIX_GRP_38 */
  {
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_MOVSHDUP  , Vps    , Wps    , NULL_OP, TRUE  ,            },
    {AMD64_MOVHPS    , Vq     , Mq     , NULL_OP, TRUE  ,            },
    {AMD64_MOVHPD    , Vq     , Mq     , NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
  },
  /* AMD64_PREFIX_GRP_39 */
  {
    {AMD64_MULSD    , Vsd    , Wsd    , NULL_OP, TRUE  ,DU          },
    {AMD64_MULSS    , Vss    , Wss    , NULL_OP, TRUE  ,DU          },
    {AMD64_MULPS    , Vps    , Wps    , NULL_OP, TRUE  ,DU          },
    {AMD64_MULPD    , Vpd    , Wpd    , NULL_OP, TRUE  ,DU          },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
  },
  /* AMD64_PREFIX_GRP_3A */
  {
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_MOVLPS   , Mq     , Vq     , NULL_OP, TRUE  ,            },
    {AMD64_MOVLPD   , Mq     , Vq     , NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
  },
  /* AMD64_PREFIX_GRP_3B */
  {
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_MOVHPS   , Mq     , Vq     , NULL_OP, TRUE  ,            },
    {AMD64_MOVHPD   , Mq     , Vq     , NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
  },
  /* AMD64_PREFIX_GRP_3C */
  {
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_PSUBUSB  , Pq     , Qq     , NULL_OP, TRUE  ,DU          },
    {AMD64_PSUBUSB2 , Vdq    , Wdq    , NULL_OP, TRUE  ,DU          },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
  },
  /* AMD64_PREFIX_GRP_3D */
  {
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_PSUBB    , Pq     , Qq     , NULL_OP, TRUE  ,DU          },
    {AMD64_PSUBB2   , Vdq    , Wdq    , NULL_OP, TRUE  ,DU          },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
  },
  /* AMD64_PREFIX_GRP_3E */
  {
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_PSRAW    , Pq     , Ib     , NULL_OP, TRUE  ,DU          },
    {AMD64_PSRAW2   , Pdq    , Ib     , NULL_OP, TRUE  ,DU          },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
  },
  /* AMD64_PREFIX_GRP_3F */
  {
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,DU          },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,DU          },
    {AMD64_PSRLW    , Pq     , Ib     , NULL_OP, TRUE  ,DU          },
    {AMD64_PSRLW2   , Pdq    , Ib     , NULL_OP, TRUE  ,DU          },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
  },
  /* AMD64_PREFIX_GRP_40 */
  {
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_PANDN    , Pq     , Qq     , NULL_OP, TRUE  ,DU          },
    {AMD64_PANDN2   , Vdq    , Wdq    , NULL_OP, TRUE  ,DU          },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
  },
  /* AMD64_PREFIX_GRP_41 */
  {
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_PADDUSW  , Pq     , Qq     , NULL_OP, TRUE  ,DU          },
    {AMD64_PADDUSW2 , Vdq    , Wdq    , NULL_OP, TRUE  ,DU          },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
  },
  /* AMD64_PREFIX_GRP_42 */
  {
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_PSUBUSW  , Pq     , Qq     , NULL_OP, TRUE  ,DU          },
    {AMD64_PSUBUSW2 , Vdq    , Wdq    , NULL_OP, TRUE  ,DU          },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
  },
  /* AMD64_PREFIX_GRP_43 */
  {
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_PCMPEQB  , Pq     , Qq     , NULL_OP, TRUE  ,DU          },
    {AMD64_PCMPEQB2 , Vdq    , Wdq    , NULL_OP, TRUE  ,DU          },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
  },
  /* AMD64_PREFIX_GRP_44 */
  {
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_PMINUB   , Pq     , Qq     , NULL_OP, TRUE  ,DU          },
    {AMD64_PMINUB2  , Vdq    , Wdq    , NULL_OP, TRUE  ,DU          },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
  },
  /* AMD64_PREFIX_GRP_45 */
  {
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_PMAXUB   , Pq     , Qq     , NULL_OP, TRUE  ,DU          },
    {AMD64_PMAXUB2  , Vdq    , Wdq    , NULL_OP, TRUE  ,DU          },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
  },
  /* AMD64_PREFIX_GRP_46 */
  {
    {AMD64_DIVSD    , Vsd    , Wsd    , NULL_OP, TRUE  ,DU          },
    {AMD64_DIVSS    , Vss    , Wss    , NULL_OP, TRUE  ,DU          },
    {AMD64_DIVPS    , Vps    , Wps    , NULL_OP, TRUE  ,DU          },
    {AMD64_DIVPD    , Vpd    , Wpd    , NULL_OP, TRUE  ,DU          },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
  },
  /* AMD64_PREFIX_GRP_47 */
  {
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UCOMISS  , Vps    , Wps    , NULL_OP, TRUE  ,DU          },
    {AMD64_UCOMISD  , Vsd    , Wsd    , NULL_OP, TRUE  ,DU          },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
  },
  /* AMD64_PREFIX_GRP_48 */
  {
    {AMD64_SQRTSD   , Vsd    , Wsd    , NULL_OP, TRUE  ,            },
    {AMD64_SQRTSS   , Vss    , Wss    , NULL_OP, TRUE  ,            },
    {AMD64_SQRTPS   , Vps    , Wps    , NULL_OP, TRUE  ,            },
    {AMD64_SQRTPD   , Vpd    , Wpd    , NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
  },
  /* AMD64_PREFIX_GRP_49 */
  {
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNPCKLPD , Vpd    , Wpd    , NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
  },
  /* AMD64_PREFIX_GRP_4A*/
  {
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_MOVDQ2Q  , Pq     , Wdq    , NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
  },
  /* AMD64_PREFIX_GRP_4B*/
  {
    {AMD64_MINSD    , Vsd    , Wsd    , NULL_OP, TRUE  ,DU          }, //F2
    {AMD64_MINSS    , Vsd    , Wsd    , NULL_OP, TRUE  ,DU          }, //F3
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            }, //
    {AMD64_MINPD    , Vpd    , Wpd    , NULL_OP, TRUE  ,DU          }, //66
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
  },
};

t_amd64_opcode_entry disamd64_3dnow[] = 
{
  /* 00 */
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
  /* 08 */
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_PI2FD     , Vdq, Wdq, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
  /* 10 */
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
  /* 18 */
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_PF2ID     , Vdq, Wdq, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
  /* 20 */
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
  /* 28 */
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
  /* 30 */
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
  /* 38 */
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
  /* 40 */
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
  /* 48 */
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
  /* 50 */
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
  /* 58 */
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
  /* 60 */
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
  /* 68 */
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
  /* 70 */
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
  /* 78 */
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
  /* 80 */
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
  /* 88 */
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_PFPNACC   , Vdq, Wdq, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
  /* 90 */
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
  /* 98 */
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_PFSUB     , Vdq, Wdq, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_PFADD     , Vdq, Wdq, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
  /* A0 */
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
  /* A8 */
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_PFSUBR    , Vdq, Wdq, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_PFACC     , Vdq, Wdq, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
  /* B0 */
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_PFMUL     , Vdq, Wdq, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
  /* B8 */
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_PSWAPD    , Vdq, Wdq, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_PAVGUSB   , Vdq, Wdq, NULL_OP, TRUE  ,            },
  /* C0 */
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
  /* C8 */
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
  /* d0 */
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
  /* d8 */
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
  /* e0 */
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
  /* e8 */
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
  /* f0 */
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
  /* f8 */
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_UNSUPPORTED_OPC, NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
};

t_amd64_opcode_entry disamd64_fpu_mem[][8] = 
{
  /* d8 */
  {
    {AMD64_FADD      , ST     , Msr    , NULL_OP, TRUE  , DU         },
    {AMD64_FMUL      , ST     , Msr    , NULL_OP, TRUE  , DU         },
    {AMD64_FCOM      , NULL_OP, ST     , Msr    , TRUE  ,            },
    {AMD64_FCOMP     , NULL_OP, ST     , Msr    , TRUE  ,            },
    {AMD64_FSUB      , ST     , Msr    , NULL_OP, TRUE  , DU         },
    {AMD64_FSUBR     , ST     , Msr    , NULL_OP, TRUE  , DU         },
    {AMD64_FDIV      , ST     , Msr    , NULL_OP, TRUE  , DU         },
    {AMD64_FDIVR     , ST     , Msr    , NULL_OP, TRUE  , DU         },
  },
  /* d9 */
  {
    {AMD64_FLD       , NULL_OP, Msr    , NULL_OP, TRUE  ,            },
    {AMD64_INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_FST       , Msr    , ST     , NULL_OP, TRUE  ,            },
    {AMD64_FSTP      , Msr    , ST     , NULL_OP, TRUE  ,            },
    {AMD64_FLDENV    , NULL_OP, M28    , NULL_OP, TRUE  ,            },
    {AMD64_FLDCW     , NULL_OP, M2     , NULL_OP, TRUE  ,            },
    {AMD64_FSTENV    , M28    , NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_FSTCW     , M2     , NULL_OP, NULL_OP, TRUE  ,            },
  },
  /* da */
  {
    {AMD64_FIADD     , ST     , Md     , NULL_OP, TRUE  , DU         },
    {AMD64_FIMUL     , ST     , Md     , NULL_OP, TRUE  , DU         },
    {AMD64_FICOM     , NULL_OP, ST     , Md     , TRUE  ,            },
    {AMD64_FICOMP    , NULL_OP, ST     , Md     , TRUE  ,            },
    {AMD64_FISUB     , ST     , Md     , NULL_OP, TRUE  , DU         },
    {AMD64_FISUBR    , ST     , Md     , NULL_OP, TRUE  , DU         },
    {AMD64_FIDIV     , ST     , Md     , NULL_OP, TRUE  , DU         },
    {AMD64_FIDIVR    , ST     , Md     , NULL_OP, TRUE  , DU         },
  },
  /* db */
  {
    {AMD64_FILD      , NULL_OP, Md     , NULL_OP, TRUE  ,            },
    {AMD64_INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_FIST      , Md     , ST     , NULL_OP, TRUE  ,            },
    {AMD64_FISTP     , Md     , ST     , NULL_OP, TRUE  ,            },
    {AMD64_INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_FLD       , NULL_OP, Mer    , NULL_OP, TRUE  ,            },
    {AMD64_INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_FSTP      , Mer    , ST     , NULL_OP, TRUE  ,            },
  },
  /* dc */
  {
    {AMD64_FADD      , ST     , Mdr    , NULL_OP, TRUE  , DU         },
    {AMD64_FMUL      , ST     , Mdr    , NULL_OP, TRUE  , DU         },
    {AMD64_FCOM      , NULL_OP, ST     , Mdr    , TRUE  ,            },
    {AMD64_FCOMP     , NULL_OP, ST     , Mdr    , TRUE  ,            },
    {AMD64_FSUB      , ST     , Mdr    , NULL_OP, TRUE  , DU         },
    {AMD64_FSUBR     , ST     , Mdr    , NULL_OP, TRUE  , DU         },
    {AMD64_FDIV      , ST     , Mdr    , NULL_OP, TRUE  , DU         },
    {AMD64_FDIVR     , ST     , Mdr    , NULL_OP, TRUE  , DU         },
  },
  /* dd */
  {
    {AMD64_FLD       , NULL_OP, Mdr    , NULL_OP, TRUE  ,            },
    {AMD64_INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_FST       , Mdr    , ST     , NULL_OP, TRUE  ,            },
    {AMD64_FSTP      , Mdr    , ST     , NULL_OP, TRUE  ,            },
    {AMD64_FRSTOR    , NULL_OP, M108   , NULL_OP, TRUE  ,            },
    {AMD64_INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_FSAVE     , M108   , NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_FSTSW     , M2     , NULL_OP, NULL_OP, TRUE  ,            },
  },
  /* de */
  {
    {AMD64_FIADD     , ST     , Mw     , NULL_OP, TRUE  , DU         },
    {AMD64_FIMUL     , ST     , Mw     , NULL_OP, TRUE  , DU         },
    {AMD64_FICOM     , NULL_OP, ST     , Mw     , TRUE  ,            },
    {AMD64_FICOMP    , NULL_OP, ST     , Mw     , TRUE  ,            },
    {AMD64_FISUB     , ST     , Mw     , NULL_OP, TRUE  , DU         },
    {AMD64_FISUBR    , ST     , Mw     , NULL_OP, TRUE  , DU         },
    {AMD64_FIDIV     , ST     , Mw     , NULL_OP, TRUE  , DU         },
    {AMD64_FIDIVR    , ST     , Mw     , NULL_OP, TRUE  , DU         },
  },
  /* df */
  {
    {AMD64_FILD      , NULL_OP, Mw     , NULL_OP, TRUE  ,            },
    {AMD64_INVALID_OPC    , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_FIST      , Mw     , ST     , NULL_OP, TRUE  ,            },
    {AMD64_FISTP     , Mw     , ST     , NULL_OP, TRUE  ,            },
    {AMD64_FBLD      , NULL_OP, Mbcd   , NULL_OP, TRUE  ,            },
    {AMD64_FILD      , NULL_OP, Mq     , NULL_OP, TRUE  ,            },
    {AMD64_FBSTP     , Mbcd   , NULL_OP, NULL_OP, TRUE  ,            },
    {AMD64_FISTP     , Mq     , ST     , NULL_OP, TRUE  ,            },
  }
};

t_amd64_opcode_entry disamd64_fpu_reg[8][8][8] = 
{
  /* d8 */
  {
    /* reg = 0 */
    {
      {AMD64_FADD    , ST     , ST0    , NULL_OP, TRUE  , DU         },
      {AMD64_FADD    , ST     , ST1    , NULL_OP, TRUE  , DU         },
      {AMD64_FADD    , ST     , ST2    , NULL_OP, TRUE  , DU         },
      {AMD64_FADD    , ST     , ST3    , NULL_OP, TRUE  , DU         },
      {AMD64_FADD    , ST     , ST4    , NULL_OP, TRUE  , DU         },
      {AMD64_FADD    , ST     , ST5    , NULL_OP, TRUE  , DU         },
      {AMD64_FADD    , ST     , ST6    , NULL_OP, TRUE  , DU         },
      {AMD64_FADD    , ST     , ST7    , NULL_OP, TRUE  , DU         }
    },                                                 
    /* reg = 1 */                                      
    {                                                  
      {AMD64_FMUL    , ST     , ST0    , NULL_OP, TRUE  , DU         },
      {AMD64_FMUL    , ST     , ST1    , NULL_OP, TRUE  , DU         },
      {AMD64_FMUL    , ST     , ST2    , NULL_OP, TRUE  , DU         },
      {AMD64_FMUL    , ST     , ST3    , NULL_OP, TRUE  , DU         },
      {AMD64_FMUL    , ST     , ST4    , NULL_OP, TRUE  , DU         },
      {AMD64_FMUL    , ST     , ST5    , NULL_OP, TRUE  , DU         },
      {AMD64_FMUL    , ST     , ST6    , NULL_OP, TRUE  , DU         },
      {AMD64_FMUL    , ST     , ST7    , NULL_OP, TRUE  , DU         }
    },                                                 
    /* reg = 2 */                                      
    {                                                  
      {AMD64_FCOM    , NULL_OP, ST     , ST0    , TRUE  ,            },
      {AMD64_FCOM    , NULL_OP, ST     , ST1    , TRUE  ,            },
      {AMD64_FCOM    , NULL_OP, ST     , ST2    , TRUE  ,            },
      {AMD64_FCOM    , NULL_OP, ST     , ST3    , TRUE  ,            },
      {AMD64_FCOM    , NULL_OP, ST     , ST4    , TRUE  ,            },
      {AMD64_FCOM    , NULL_OP, ST     , ST5    , TRUE  ,            },
      {AMD64_FCOM    , NULL_OP, ST     , ST6    , TRUE  ,            },
      {AMD64_FCOM    , NULL_OP, ST     , ST7    , TRUE  ,            }
    },                                                 
    /* reg = 3 */                                      
    {                                                  
      {AMD64_FCOMP   , NULL_OP, ST     , ST0    , TRUE  ,            },
      {AMD64_FCOMP   , NULL_OP, ST     , ST1    , TRUE  ,            },
      {AMD64_FCOMP   , NULL_OP, ST     , ST2    , TRUE  ,            },
      {AMD64_FCOMP   , NULL_OP, ST     , ST3    , TRUE  ,            },
      {AMD64_FCOMP   , NULL_OP, ST     , ST4    , TRUE  ,            },
      {AMD64_FCOMP   , NULL_OP, ST     , ST5    , TRUE  ,            },
      {AMD64_FCOMP   , NULL_OP, ST     , ST6    , TRUE  ,            },
      {AMD64_FCOMP   , NULL_OP, ST     , ST7    , TRUE  ,            }
    },                                                 
    /* reg = 4 */                                      
    {                                                  
      {AMD64_FSUB    , ST     , ST0    , NULL_OP, TRUE  , DU         },
      {AMD64_FSUB    , ST     , ST1    , NULL_OP, TRUE  , DU         },
      {AMD64_FSUB    , ST     , ST2    , NULL_OP, TRUE  , DU         },
      {AMD64_FSUB    , ST     , ST3    , NULL_OP, TRUE  , DU         },
      {AMD64_FSUB    , ST     , ST4    , NULL_OP, TRUE  , DU         },
      {AMD64_FSUB    , ST     , ST5    , NULL_OP, TRUE  , DU         },
      {AMD64_FSUB    , ST     , ST6    , NULL_OP, TRUE  , DU         },
      {AMD64_FSUB    , ST     , ST7    , NULL_OP, TRUE  , DU         }
    },                                                 
    /* reg = 5 */                                      
    {                                                  
      {AMD64_FSUBR   , ST     , ST0    , NULL_OP, TRUE  , DU         },
      {AMD64_FSUBR   , ST     , ST1    , NULL_OP, TRUE  , DU         },
      {AMD64_FSUBR   , ST     , ST2    , NULL_OP, TRUE  , DU         },
      {AMD64_FSUBR   , ST     , ST3    , NULL_OP, TRUE  , DU         },
      {AMD64_FSUBR   , ST     , ST4    , NULL_OP, TRUE  , DU         },
      {AMD64_FSUBR   , ST     , ST5    , NULL_OP, TRUE  , DU         },
      {AMD64_FSUBR   , ST     , ST6    , NULL_OP, TRUE  , DU         },
      {AMD64_FSUBR   , ST     , ST7    , NULL_OP, TRUE  , DU         }
    },                                                 
    /* reg = 6 */                                      
    {                                                  
      {AMD64_FDIV    , ST     , ST0    , NULL_OP, TRUE  , DU         },
      {AMD64_FDIV    , ST     , ST1    , NULL_OP, TRUE  , DU         },
      {AMD64_FDIV    , ST     , ST2    , NULL_OP, TRUE  , DU         },
      {AMD64_FDIV    , ST     , ST3    , NULL_OP, TRUE  , DU         },
      {AMD64_FDIV    , ST     , ST4    , NULL_OP, TRUE  , DU         },
      {AMD64_FDIV    , ST     , ST5    , NULL_OP, TRUE  , DU         },
      {AMD64_FDIV    , ST     , ST6    , NULL_OP, TRUE  , DU         },
      {AMD64_FDIV    , ST     , ST7    , NULL_OP, TRUE  , DU         }
    },                                                 
    /* reg = 7 */                                      
    {                                                  
      {AMD64_FDIVR   , ST     , ST0    , NULL_OP, TRUE  , DU         },
      {AMD64_FDIVR   , ST     , ST1    , NULL_OP, TRUE  , DU         },
      {AMD64_FDIVR   , ST     , ST2    , NULL_OP, TRUE  , DU         },
      {AMD64_FDIVR   , ST     , ST3    , NULL_OP, TRUE  , DU         },
      {AMD64_FDIVR   , ST     , ST4    , NULL_OP, TRUE  , DU         },
      {AMD64_FDIVR   , ST     , ST5    , NULL_OP, TRUE  , DU         },
      {AMD64_FDIVR   , ST     , ST6    , NULL_OP, TRUE  , DU         },
      {AMD64_FDIVR   , ST     , ST7    , NULL_OP, TRUE  , DU         }
    }
  },
  /* d9 */
  {
    /* reg = 0 */
    {
      {AMD64_FLD     , ST     , ST0    , NULL_OP, TRUE  ,            },
      {AMD64_FLD     , ST     , ST1    , NULL_OP, TRUE  ,            },
      {AMD64_FLD     , ST     , ST2    , NULL_OP, TRUE  ,            },
      {AMD64_FLD     , ST     , ST3    , NULL_OP, TRUE  ,            },
      {AMD64_FLD     , ST     , ST4    , NULL_OP, TRUE  ,            },
      {AMD64_FLD     , ST     , ST5    , NULL_OP, TRUE  ,            },
      {AMD64_FLD     , ST     , ST6    , NULL_OP, TRUE  ,            },
      {AMD64_FLD     , ST     , ST7    , NULL_OP, TRUE  ,            }
    },                                                 
    /* reg = 1 */                                      
    {                                                  
      {AMD64_FXCH    , ST     , ST0    , NULL_OP, TRUE  , DU|S1D     },
      {AMD64_FXCH    , ST     , ST1    , NULL_OP, TRUE  , DU|S1D     },
      {AMD64_FXCH    , ST     , ST2    , NULL_OP, TRUE  , DU|S1D     },
      {AMD64_FXCH    , ST     , ST3    , NULL_OP, TRUE  , DU|S1D     },
      {AMD64_FXCH    , ST     , ST4    , NULL_OP, TRUE  , DU|S1D     },
      {AMD64_FXCH    , ST     , ST5    , NULL_OP, TRUE  , DU|S1D     },
      {AMD64_FXCH    , ST     , ST6    , NULL_OP, TRUE  , DU|S1D     },
      {AMD64_FXCH    , ST     , ST7    , NULL_OP, TRUE  , DU|S1D     }
    },                                                 
    /* reg = 2 */                                      
    {                                                  
      {AMD64_FNOP    , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
      {AMD64_INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
      {AMD64_INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
      {AMD64_INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
      {AMD64_INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
      {AMD64_INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
      {AMD64_INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
      {AMD64_INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            }
    },                                                 
    /* reg = 3 */                                      
    {                                                  
      {AMD64_INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
      {AMD64_INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
      {AMD64_INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
      {AMD64_INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
      {AMD64_INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
      {AMD64_INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
      {AMD64_INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
      {AMD64_INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            }
    },                                                 
    /* reg = 4 */                                      
    {                                                  
      {AMD64_FCHS    , ST     , NULL_OP, NULL_OP, TRUE  , DU         },
      {AMD64_FABS    , ST     , NULL_OP, NULL_OP, TRUE  , DU         },
      {AMD64_INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
      {AMD64_INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
      {AMD64_FTST    , NULL_OP, ST     , NULL_OP, TRUE  ,            },
      {AMD64_FXAM    , NULL_OP, ST     , NULL_OP, TRUE  ,            },
      {AMD64_INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
      {AMD64_INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            }
    },                                                 
    /* reg = 5 */                                      
    {                                                  
      {AMD64_FLD1    , ST     , NULL_OP, NULL_OP, TRUE  ,            },
      {AMD64_FLDL2T  , ST     , NULL_OP, NULL_OP, TRUE  ,            },
      {AMD64_FLDL2E  , ST     , NULL_OP, NULL_OP, TRUE  ,            },
      {AMD64_FLDPI   , ST     , NULL_OP, NULL_OP, TRUE  ,            },
      {AMD64_FLDLG2  , ST     , NULL_OP, NULL_OP, TRUE  ,            },
      {AMD64_FLDLN2  , ST     , NULL_OP, NULL_OP, TRUE  ,            },
      {AMD64_FLDZ    , ST     , NULL_OP, NULL_OP, TRUE  ,            },
      {AMD64_INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            }
    },                                                 
    /* reg = 6 */                                      
    {                                                  
      {AMD64_F2XM1   , ST     , NULL_OP, NULL_OP, TRUE  , DU         },
      {AMD64_FYL2X   , ST     , ST1    , NULL_OP, TRUE  , DU         },
      {AMD64_FPTAN   , ST     , NULL_OP, NULL_OP, TRUE  , DU         },
      {AMD64_FPATAN  , ST     , ST1    , NULL_OP, TRUE  , DU         },
      {AMD64_FXTRACT , ST     , NULL_OP, NULL_OP, TRUE  , DU         },
      {AMD64_FPREM1  , ST     , ST1    , NULL_OP, TRUE  , DU         },
      {AMD64_FDECSTP , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
      {AMD64_FINCSTP , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            }
    },                                                 
    /* reg = 7 */                                      
    {                                                  
      {AMD64_FPREM   , ST     , ST1    , NULL_OP, TRUE  , DU         },
      {AMD64_FYL2XP1 , ST     , ST1    , NULL_OP, TRUE  , DU         },
      {AMD64_FSQRT   , ST     , NULL_OP, NULL_OP, TRUE  , DU         },
      {AMD64_FSINCOS , ST     , NULL_OP, NULL_OP, TRUE  , DU         },
      {AMD64_FRNDINT , ST     , NULL_OP, NULL_OP, TRUE  , DU         },
      {AMD64_FSCALE  , ST     , ST1    , NULL_OP, TRUE  , DU         },
      {AMD64_FSIN    , ST     , NULL_OP, NULL_OP, TRUE  , DU         },
      {AMD64_FCOS    , ST     , NULL_OP, NULL_OP, TRUE  , DU         }
    }
  },
  /* da */
  {
    /* reg = 0 */
    {
      {AMD64_FCMOVcc , ST     , ST0    , NULL_OP, TRUE  ,            },
      {AMD64_FCMOVcc , ST     , ST1    , NULL_OP, TRUE  ,            },
      {AMD64_FCMOVcc , ST     , ST2    , NULL_OP, TRUE  ,            },
      {AMD64_FCMOVcc , ST     , ST3    , NULL_OP, TRUE  ,            },
      {AMD64_FCMOVcc , ST     , ST4    , NULL_OP, TRUE  ,            },
      {AMD64_FCMOVcc , ST     , ST5    , NULL_OP, TRUE  ,            },
      {AMD64_FCMOVcc , ST     , ST6    , NULL_OP, TRUE  ,            },
      {AMD64_FCMOVcc , ST     , ST7    , NULL_OP, TRUE  ,            }
    },                                                 
    /* reg = 1 */                                      
    {                                                  
      {AMD64_FCMOVcc , ST     , ST0    , NULL_OP, TRUE  ,            },
      {AMD64_FCMOVcc , ST     , ST1    , NULL_OP, TRUE  ,            },
      {AMD64_FCMOVcc , ST     , ST2    , NULL_OP, TRUE  ,            },
      {AMD64_FCMOVcc , ST     , ST3    , NULL_OP, TRUE  ,            },
      {AMD64_FCMOVcc , ST     , ST4    , NULL_OP, TRUE  ,            },
      {AMD64_FCMOVcc , ST     , ST5    , NULL_OP, TRUE  ,            },
      {AMD64_FCMOVcc , ST     , ST6    , NULL_OP, TRUE  ,            },
      {AMD64_FCMOVcc , ST     , ST7    , NULL_OP, TRUE  ,            }
    },                                                 
    /* reg = 2 */                                      
    {                                                  
      {AMD64_FCMOVcc , ST     , ST0    , NULL_OP, TRUE  ,            },
      {AMD64_FCMOVcc , ST     , ST1    , NULL_OP, TRUE  ,            },
      {AMD64_FCMOVcc , ST     , ST2    , NULL_OP, TRUE  ,            },
      {AMD64_FCMOVcc , ST     , ST3    , NULL_OP, TRUE  ,            },
      {AMD64_FCMOVcc , ST     , ST4    , NULL_OP, TRUE  ,            },
      {AMD64_FCMOVcc , ST     , ST5    , NULL_OP, TRUE  ,            },
      {AMD64_FCMOVcc , ST     , ST6    , NULL_OP, TRUE  ,            },
      {AMD64_FCMOVcc , ST     , ST7    , NULL_OP, TRUE  ,            }
    },                                                 
    /* reg = 3 */                                      
    {                                                  
      {AMD64_FCMOVcc , ST     , ST0    , NULL_OP, TRUE  ,            },
      {AMD64_FCMOVcc , ST     , ST1    , NULL_OP, TRUE  ,            },
      {AMD64_FCMOVcc , ST     , ST2    , NULL_OP, TRUE  ,            },
      {AMD64_FCMOVcc , ST     , ST3    , NULL_OP, TRUE  ,            },
      {AMD64_FCMOVcc , ST     , ST4    , NULL_OP, TRUE  ,            },
      {AMD64_FCMOVcc , ST     , ST5    , NULL_OP, TRUE  ,            },
      {AMD64_FCMOVcc , ST     , ST6    , NULL_OP, TRUE  ,            },
      {AMD64_FCMOVcc , ST     , ST7    , NULL_OP, TRUE  ,            }
    },                                                 
    /* reg = 4 */                                      
    {                                                  
      {AMD64_INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
      {AMD64_INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
      {AMD64_INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
      {AMD64_INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
      {AMD64_INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
      {AMD64_INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
      {AMD64_INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
      {AMD64_INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            }
    },                                                 
    /* reg = 5 */                                      
    {                                                  
      {AMD64_INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
      {AMD64_FUCOMPP , NULL_OP, ST     , ST1    , TRUE  ,            },
      {AMD64_INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
      {AMD64_INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
      {AMD64_INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
      {AMD64_INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
      {AMD64_INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
      {AMD64_INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            }
    },                                                 
    /* reg = 6 */                                      
    {                                                  
      {AMD64_INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
      {AMD64_INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
      {AMD64_INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
      {AMD64_INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
      {AMD64_INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
      {AMD64_INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
      {AMD64_INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
      {AMD64_INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            }
    },                                                 
    /* reg = 7 */                                      
    {                                                  
      {AMD64_INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
      {AMD64_INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
      {AMD64_INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
      {AMD64_INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
      {AMD64_INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
      {AMD64_INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
      {AMD64_INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
      {AMD64_INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            }
    }
  },
  /* db */
  {
    /* reg = 0 */
    {
      {AMD64_FCMOVcc , ST     , ST0    , NULL_OP, TRUE  ,            },
      {AMD64_FCMOVcc , ST     , ST1    , NULL_OP, TRUE  ,            },
      {AMD64_FCMOVcc , ST     , ST2    , NULL_OP, TRUE  ,            },
      {AMD64_FCMOVcc , ST     , ST3    , NULL_OP, TRUE  ,            },
      {AMD64_FCMOVcc , ST     , ST4    , NULL_OP, TRUE  ,            },
      {AMD64_FCMOVcc , ST     , ST5    , NULL_OP, TRUE  ,            },
      {AMD64_FCMOVcc , ST     , ST6    , NULL_OP, TRUE  ,            },
      {AMD64_FCMOVcc , ST     , ST7    , NULL_OP, TRUE  ,            }
    },                                                 
    /* reg = 1 */                                      
    {                                                  
      {AMD64_FCOMI   , NULL_OP, ST     , ST0    , TRUE  ,            },
      {AMD64_FCOMI   , NULL_OP, ST     , ST1    , TRUE  ,            },
      {AMD64_FCOMI   , NULL_OP, ST     , ST2    , TRUE  ,            },
      {AMD64_FCOMI   , NULL_OP, ST     , ST3    , TRUE  ,            },
      {AMD64_FCOMI   , NULL_OP, ST     , ST4    , TRUE  ,            },
      {AMD64_FCOMI   , NULL_OP, ST     , ST5    , TRUE  ,            },
      {AMD64_FCOMI   , NULL_OP, ST     , ST6    , TRUE  ,            },
      {AMD64_FCOMI   , NULL_OP, ST     , ST7    , TRUE  ,            }
    },                                                 
    /* reg = 2 */                                      
    {                                                  
      {AMD64_FCMOVcc , ST     , ST0    , NULL_OP, TRUE  ,            },
      {AMD64_FCMOVcc , ST     , ST1    , NULL_OP, TRUE  ,            },
      {AMD64_FCMOVcc , ST     , ST2    , NULL_OP, TRUE  ,            },
      {AMD64_FCMOVcc , ST     , ST3    , NULL_OP, TRUE  ,            },
      {AMD64_FCMOVcc , ST     , ST4    , NULL_OP, TRUE  ,            },
      {AMD64_FCMOVcc , ST     , ST5    , NULL_OP, TRUE  ,            },
      {AMD64_FCMOVcc , ST     , ST6    , NULL_OP, TRUE  ,            },
      {AMD64_FCMOVcc , ST     , ST7    , NULL_OP, TRUE  ,            }
    },                                                 
    /* reg = 3 */                                      
    {                                                  
      {AMD64_FCMOVcc , ST     , ST0    , NULL_OP, TRUE  ,            },
      {AMD64_FCMOVcc , ST     , ST1    , NULL_OP, TRUE  ,            },
      {AMD64_FCMOVcc , ST     , ST2    , NULL_OP, TRUE  ,            },
      {AMD64_FCMOVcc , ST     , ST3    , NULL_OP, TRUE  ,            },
      {AMD64_FCMOVcc , ST     , ST4    , NULL_OP, TRUE  ,            },
      {AMD64_FCMOVcc , ST     , ST5    , NULL_OP, TRUE  ,            },
      {AMD64_FCMOVcc , ST     , ST6    , NULL_OP, TRUE  ,            },
      {AMD64_FCMOVcc , ST     , ST7    , NULL_OP, TRUE  ,            }
    },                                                 
    /* reg = 4 */                                      
    {                                                  
      {AMD64_INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
      {AMD64_INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
      {AMD64_FCLEX   , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
      {AMD64_FINIT   , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
      {AMD64_INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
      {AMD64_INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
      {AMD64_INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
      {AMD64_INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            }
    },                                                 
    /* reg = 5 */                                      
    {                                                  
      {AMD64_FUCOMI  , NULL_OP, ST     , ST0    , TRUE  ,            },
      {AMD64_FUCOMI  , NULL_OP, ST     , ST1    , TRUE  ,            },
      {AMD64_FUCOMI  , NULL_OP, ST     , ST2    , TRUE  ,            },
      {AMD64_FUCOMI  , NULL_OP, ST     , ST3    , TRUE  ,            },
      {AMD64_FUCOMI  , NULL_OP, ST     , ST4    , TRUE  ,            },
      {AMD64_FUCOMI  , NULL_OP, ST     , ST5    , TRUE  ,            },
      {AMD64_FUCOMI  , NULL_OP, ST     , ST6    , TRUE  ,            },
      {AMD64_FUCOMI  , NULL_OP, ST     , ST7    , TRUE  ,            }
    },                                                 
    /* reg = 6 */                                      
    {                                                  
      {AMD64_FCMOVcc , ST     , ST0    , NULL_OP, TRUE  ,            },
      {AMD64_FCMOVcc , ST     , ST1    , NULL_OP, TRUE  ,            },
      {AMD64_FCMOVcc , ST     , ST2    , NULL_OP, TRUE  ,            },
      {AMD64_FCMOVcc , ST     , ST3    , NULL_OP, TRUE  ,            },
      {AMD64_FCMOVcc , ST     , ST4    , NULL_OP, TRUE  ,            },
      {AMD64_FCMOVcc , ST     , ST5    , NULL_OP, TRUE  ,            },
      {AMD64_FCMOVcc , ST     , ST6    , NULL_OP, TRUE  ,            },
      {AMD64_FCMOVcc , ST     , ST7    , NULL_OP, TRUE  ,            }
    },                                                 
    /* reg = 7 */                                      
    {                                                  
      {AMD64_INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
      {AMD64_INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
      {AMD64_INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
      {AMD64_INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
      {AMD64_INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
      {AMD64_INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
      {AMD64_INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
      {AMD64_INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            }
    }
  },
  /* dc */
  {
    /* reg = 0 */
    {
      {AMD64_FADD    , ST0    , ST     , NULL_OP, TRUE  , DU         },
      {AMD64_FADD    , ST1    , ST     , NULL_OP, TRUE  , DU         },
      {AMD64_FADD    , ST2    , ST     , NULL_OP, TRUE  , DU         },
      {AMD64_FADD    , ST3    , ST     , NULL_OP, TRUE  , DU         },
      {AMD64_FADD    , ST4    , ST     , NULL_OP, TRUE  , DU         },
      {AMD64_FADD    , ST5    , ST     , NULL_OP, TRUE  , DU         },
      {AMD64_FADD    , ST6    , ST     , NULL_OP, TRUE  , DU         },
      {AMD64_FADD    , ST7    , ST     , NULL_OP, TRUE  , DU         }
    },                                                 
    /* reg = 1 */                                      
    {                                                  
      {AMD64_FMUL    , ST0    , ST     , NULL_OP, TRUE  , DU         },
      {AMD64_FMUL    , ST1    , ST     , NULL_OP, TRUE  , DU         },
      {AMD64_FMUL    , ST2    , ST     , NULL_OP, TRUE  , DU         },
      {AMD64_FMUL    , ST3    , ST     , NULL_OP, TRUE  , DU         },
      {AMD64_FMUL    , ST4    , ST     , NULL_OP, TRUE  , DU         },
      {AMD64_FMUL    , ST5    , ST     , NULL_OP, TRUE  , DU         },
      {AMD64_FMUL    , ST6    , ST     , NULL_OP, TRUE  , DU         },
      {AMD64_FMUL    , ST7    , ST     , NULL_OP, TRUE  , DU         }
    },                                                 
    /* reg = 2 */                                      
    {                                                  
      {AMD64_INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
      {AMD64_INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
      {AMD64_INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
      {AMD64_INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
      {AMD64_INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
      {AMD64_INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
      {AMD64_INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
      {AMD64_INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            }
    },                                                 
    /* reg = 3 */                                      
    {                                                  
      {AMD64_INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
      {AMD64_INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
      {AMD64_INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
      {AMD64_INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
      {AMD64_INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
      {AMD64_INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
      {AMD64_INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
      {AMD64_INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            }
    },                                                 
    /* reg = 4 */                                      
    {                                                  
      {AMD64_FSUBR   , ST0    , ST     , NULL_OP, TRUE  , DU         },
      {AMD64_FSUBR   , ST1    , ST     , NULL_OP, TRUE  , DU         },
      {AMD64_FSUBR   , ST2    , ST     , NULL_OP, TRUE  , DU         },
      {AMD64_FSUBR   , ST3    , ST     , NULL_OP, TRUE  , DU         },
      {AMD64_FSUBR   , ST4    , ST     , NULL_OP, TRUE  , DU         },
      {AMD64_FSUBR   , ST5    , ST     , NULL_OP, TRUE  , DU         },
      {AMD64_FSUBR   , ST6    , ST     , NULL_OP, TRUE  , DU         },
      {AMD64_FSUBR   , ST7    , ST     , NULL_OP, TRUE  , DU         }
    },                                                 
    /* reg = 5 */                                      
    {                                                  
      {AMD64_FSUB    , ST0    , ST     , NULL_OP, TRUE  , DU         },
      {AMD64_FSUB    , ST1    , ST     , NULL_OP, TRUE  , DU         },
      {AMD64_FSUB    , ST2    , ST     , NULL_OP, TRUE  , DU         },
      {AMD64_FSUB    , ST3    , ST     , NULL_OP, TRUE  , DU         },
      {AMD64_FSUB    , ST4    , ST     , NULL_OP, TRUE  , DU         },
      {AMD64_FSUB    , ST5    , ST     , NULL_OP, TRUE  , DU         },
      {AMD64_FSUB    , ST6    , ST     , NULL_OP, TRUE  , DU         },
      {AMD64_FSUB    , ST7    , ST     , NULL_OP, TRUE  , DU         }
    },                                                 
    /* reg = 6 */                                      
    {                                                  
      {AMD64_FDIVR   , ST0    , ST     , NULL_OP, TRUE  , DU         },
      {AMD64_FDIVR   , ST1    , ST     , NULL_OP, TRUE  , DU         },
      {AMD64_FDIVR   , ST2    , ST     , NULL_OP, TRUE  , DU         },
      {AMD64_FDIVR   , ST3    , ST     , NULL_OP, TRUE  , DU         },
      {AMD64_FDIVR   , ST4    , ST     , NULL_OP, TRUE  , DU         },
      {AMD64_FDIVR   , ST5    , ST     , NULL_OP, TRUE  , DU         },
      {AMD64_FDIVR   , ST6    , ST     , NULL_OP, TRUE  , DU         },
      {AMD64_FDIVR   , ST7    , ST     , NULL_OP, TRUE  , DU         }
    },                                                 
    /* reg = 7 */                                      
    {                                                  
      {AMD64_FDIV    , ST0    , ST     , NULL_OP, TRUE  , DU         },
      {AMD64_FDIV    , ST1    , ST     , NULL_OP, TRUE  , DU         },
      {AMD64_FDIV    , ST2    , ST     , NULL_OP, TRUE  , DU         },
      {AMD64_FDIV    , ST3    , ST     , NULL_OP, TRUE  , DU         },
      {AMD64_FDIV    , ST4    , ST     , NULL_OP, TRUE  , DU         },
      {AMD64_FDIV    , ST5    , ST     , NULL_OP, TRUE  , DU         },
      {AMD64_FDIV    , ST6    , ST     , NULL_OP, TRUE  , DU         },
      {AMD64_FDIV    , ST7    , ST     , NULL_OP, TRUE  , DU         }
    }
  },
  /* dd */
  {
    /* reg = 0 */
    {
      {AMD64_FFREE   , ST0    , NULL_OP, NULL_OP, TRUE  ,            },
      {AMD64_FFREE   , ST1    , NULL_OP, NULL_OP, TRUE  ,            },
      {AMD64_FFREE   , ST2    , NULL_OP, NULL_OP, TRUE  ,            },
      {AMD64_FFREE   , ST3    , NULL_OP, NULL_OP, TRUE  ,            },
      {AMD64_FFREE   , ST4    , NULL_OP, NULL_OP, TRUE  ,            },
      {AMD64_FFREE   , ST5    , NULL_OP, NULL_OP, TRUE  ,            },
      {AMD64_FFREE   , ST6    , NULL_OP, NULL_OP, TRUE  ,            },
      {AMD64_FFREE   , ST7    , NULL_OP, NULL_OP, TRUE  ,            }
    },                                                 
    /* reg = 1 */                                      
    {                                                  
      {AMD64_INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
      {AMD64_INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
      {AMD64_INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
      {AMD64_INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
      {AMD64_INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
      {AMD64_INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
      {AMD64_INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
      {AMD64_INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            }
    },                                                 
    /* reg = 2 */                                      
    {                                                  
      {AMD64_FST     , ST0    , ST     , NULL_OP, TRUE  ,            },
      {AMD64_FST     , ST1    , ST     , NULL_OP, TRUE  ,            },
      {AMD64_FST     , ST2    , ST     , NULL_OP, TRUE  ,            },
      {AMD64_FST     , ST3    , ST     , NULL_OP, TRUE  ,            },
      {AMD64_FST     , ST4    , ST     , NULL_OP, TRUE  ,            },
      {AMD64_FST     , ST5    , ST     , NULL_OP, TRUE  ,            },
      {AMD64_FST     , ST6    , ST     , NULL_OP, TRUE  ,            },
      {AMD64_FST     , ST7    , ST     , NULL_OP, TRUE  ,            }
    },                                                 
    /* reg = 3 */                                      
    {                                                  
      {AMD64_FSTP    , ST0    , ST     , NULL_OP, TRUE  ,            },
      {AMD64_FSTP    , ST1    , ST     , NULL_OP, TRUE  ,            },
      {AMD64_FSTP    , ST2    , ST     , NULL_OP, TRUE  ,            },
      {AMD64_FSTP    , ST3    , ST     , NULL_OP, TRUE  ,            },
      {AMD64_FSTP    , ST4    , ST     , NULL_OP, TRUE  ,            },
      {AMD64_FSTP    , ST5    , ST     , NULL_OP, TRUE  ,            },
      {AMD64_FSTP    , ST6    , ST     , NULL_OP, TRUE  ,            },
      {AMD64_FSTP    , ST7    , ST     , NULL_OP, TRUE  ,            }
    },                                                 
    /* reg = 4 */                                      
    {                                                  
      {AMD64_FUCOM   , NULL_OP, ST     , ST0    , TRUE  ,            },
      {AMD64_FUCOM   , NULL_OP, ST     , ST1    , TRUE  ,            },
      {AMD64_FUCOM   , NULL_OP, ST     , ST2    , TRUE  ,            },
      {AMD64_FUCOM   , NULL_OP, ST     , ST3    , TRUE  ,            },
      {AMD64_FUCOM   , NULL_OP, ST     , ST4    , TRUE  ,            },
      {AMD64_FUCOM   , NULL_OP, ST     , ST5    , TRUE  ,            },
      {AMD64_FUCOM   , NULL_OP, ST     , ST6    , TRUE  ,            },
      {AMD64_FUCOM   , NULL_OP, ST     , ST7    , TRUE  ,            }
    },                                                 
    /* reg = 5 */                                      
    {                                                  
      {AMD64_FUCOMP  , NULL_OP, ST     , ST0    , TRUE  ,            },
      {AMD64_FUCOMP  , NULL_OP, ST     , ST1    , TRUE  ,            },
      {AMD64_FUCOMP  , NULL_OP, ST     , ST2    , TRUE  ,            },
      {AMD64_FUCOMP  , NULL_OP, ST     , ST3    , TRUE  ,            },
      {AMD64_FUCOMP  , NULL_OP, ST     , ST4    , TRUE  ,            },
      {AMD64_FUCOMP  , NULL_OP, ST     , ST5    , TRUE  ,            },
      {AMD64_FUCOMP  , NULL_OP, ST     , ST6    , TRUE  ,            },
      {AMD64_FUCOMP  , NULL_OP, ST     , ST7    , TRUE  ,            }
    },                                                 
    /* reg = 6 */                                      
    {                                                  
      {AMD64_INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
      {AMD64_INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
      {AMD64_INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
      {AMD64_INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
      {AMD64_INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
      {AMD64_INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
      {AMD64_INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
      {AMD64_INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            }
    },                                                 
    /* reg = 7 */                                      
    {                                                  
      {AMD64_INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
      {AMD64_INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
      {AMD64_INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
      {AMD64_INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
      {AMD64_INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
      {AMD64_INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
      {AMD64_INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
      {AMD64_INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            }
    }
  },
  /* de */
  {
    /* reg = 0 */
    {
      {AMD64_FADDP   , ST0    , ST     , NULL_OP, TRUE  , DU         },
      {AMD64_FADDP   , ST1    , ST     , NULL_OP, TRUE  , DU         },
      {AMD64_FADDP   , ST2    , ST     , NULL_OP, TRUE  , DU         },
      {AMD64_FADDP   , ST3    , ST     , NULL_OP, TRUE  , DU         },
      {AMD64_FADDP   , ST4    , ST     , NULL_OP, TRUE  , DU         },
      {AMD64_FADDP   , ST5    , ST     , NULL_OP, TRUE  , DU         },
      {AMD64_FADDP   , ST6    , ST     , NULL_OP, TRUE  , DU         },
      {AMD64_FADDP   , ST7    , ST     , NULL_OP, TRUE  , DU         }
    },                                                 
    /* reg = 1 */                                      
    {                                                  
      {AMD64_FMULP   , ST0    , ST     , NULL_OP, TRUE  , DU         },
      {AMD64_FMULP   , ST1    , ST     , NULL_OP, TRUE  , DU         },
      {AMD64_FMULP   , ST2    , ST     , NULL_OP, TRUE  , DU         },
      {AMD64_FMULP   , ST3    , ST     , NULL_OP, TRUE  , DU         },
      {AMD64_FMULP   , ST4    , ST     , NULL_OP, TRUE  , DU         },
      {AMD64_FMULP   , ST5    , ST     , NULL_OP, TRUE  , DU         },
      {AMD64_FMULP   , ST6    , ST     , NULL_OP, TRUE  , DU         },
      {AMD64_FMULP   , ST7    , ST     , NULL_OP, TRUE  , DU         }
    },                                                 
    /* reg = 2 */                                      
    {                                                  
      {AMD64_INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
      {AMD64_INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
      {AMD64_INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
      {AMD64_INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
      {AMD64_INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
      {AMD64_INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
      {AMD64_INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
      {AMD64_INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            }
    },                                                 
    /* reg = 3 */                                      
    {                                                  
      {AMD64_INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
      {AMD64_FCOMPP  , NULL_OP, ST     , ST1    , TRUE  ,            },
      {AMD64_INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
      {AMD64_INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
      {AMD64_INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
      {AMD64_INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
      {AMD64_INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
      {AMD64_INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            }
    },                                                 
    /* reg = 4 */                                      
    {                                                  
      {AMD64_FSUBRP  , ST0    , ST     , NULL_OP, TRUE  , DU         },
      {AMD64_FSUBRP  , ST1    , ST     , NULL_OP, TRUE  , DU         },
      {AMD64_FSUBRP  , ST2    , ST     , NULL_OP, TRUE  , DU         },
      {AMD64_FSUBRP  , ST3    , ST     , NULL_OP, TRUE  , DU         },
      {AMD64_FSUBRP  , ST4    , ST     , NULL_OP, TRUE  , DU         },
      {AMD64_FSUBRP  , ST5    , ST     , NULL_OP, TRUE  , DU         },
      {AMD64_FSUBRP  , ST6    , ST     , NULL_OP, TRUE  , DU         },
      {AMD64_FSUBRP  , ST7    , ST     , NULL_OP, TRUE  , DU         }
    },                                                 
    /* reg = 5 */                                      
    {                                                  
      {AMD64_FSUBP   , ST0    , ST     , NULL_OP, TRUE  , DU         },
      {AMD64_FSUBP   , ST1    , ST     , NULL_OP, TRUE  , DU         },
      {AMD64_FSUBP   , ST2    , ST     , NULL_OP, TRUE  , DU         },
      {AMD64_FSUBP   , ST3    , ST     , NULL_OP, TRUE  , DU         },
      {AMD64_FSUBP   , ST4    , ST     , NULL_OP, TRUE  , DU         },
      {AMD64_FSUBP   , ST5    , ST     , NULL_OP, TRUE  , DU         },
      {AMD64_FSUBP   , ST6    , ST     , NULL_OP, TRUE  , DU         },
      {AMD64_FSUBP   , ST7    , ST     , NULL_OP, TRUE  , DU         }
    },                                                 
    /* reg = 6 */                                      
    {                                                  
      {AMD64_FDIVRP  , ST0    , ST     , NULL_OP, TRUE  , DU         },
      {AMD64_FDIVRP  , ST1    , ST     , NULL_OP, TRUE  , DU         },
      {AMD64_FDIVRP  , ST2    , ST     , NULL_OP, TRUE  , DU         },
      {AMD64_FDIVRP  , ST3    , ST     , NULL_OP, TRUE  , DU         },
      {AMD64_FDIVRP  , ST4    , ST     , NULL_OP, TRUE  , DU         },
      {AMD64_FDIVRP  , ST5    , ST     , NULL_OP, TRUE  , DU         },
      {AMD64_FDIVRP  , ST6    , ST     , NULL_OP, TRUE  , DU         },
      {AMD64_FDIVRP  , ST7    , ST     , NULL_OP, TRUE  , DU         }
    },                                                 
    /* reg = 7 */                                      
    {                                                  
      {AMD64_FDIVP   , ST0    , ST     , NULL_OP, TRUE  , DU         },
      {AMD64_FDIVP   , ST1    , ST     , NULL_OP, TRUE  , DU         },
      {AMD64_FDIVP   , ST2    , ST     , NULL_OP, TRUE  , DU         },
      {AMD64_FDIVP   , ST3    , ST     , NULL_OP, TRUE  , DU         },
      {AMD64_FDIVP   , ST4    , ST     , NULL_OP, TRUE  , DU         },
      {AMD64_FDIVP   , ST5    , ST     , NULL_OP, TRUE  , DU         },
      {AMD64_FDIVP   , ST6    , ST     , NULL_OP, TRUE  , DU         },
      {AMD64_FDIVP   , ST7    , ST     , NULL_OP, TRUE  , DU         }
    }
  },
  /* df */
  {
    /* reg = 0 */
    {
      {AMD64_FFREEP , ST0 , NULL_OP, NULL_OP, TRUE  ,            },
      {AMD64_FFREEP , ST1 , NULL_OP, NULL_OP, TRUE  ,            },
      {AMD64_FFREEP , ST2 , NULL_OP, NULL_OP, TRUE  ,            },
      {AMD64_FFREEP , ST3 , NULL_OP, NULL_OP, TRUE  ,            },
      {AMD64_FFREEP , ST4 , NULL_OP, NULL_OP, TRUE  ,            },
      {AMD64_FFREEP , ST5 , NULL_OP, NULL_OP, TRUE  ,            },
      {AMD64_FFREEP , ST6 , NULL_OP, NULL_OP, TRUE  ,            },
      {AMD64_FFREEP , ST7 , NULL_OP, NULL_OP, TRUE  ,            }
    },                                                 
    /* reg = 1 */                                      
    {                                                  
      {AMD64_INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
      {AMD64_INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
      {AMD64_INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
      {AMD64_INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
      {AMD64_INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
      {AMD64_INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
      {AMD64_INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
      {AMD64_INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            }
    },                                                 
    /* reg = 2 */                                      
    {                                                  
      {AMD64_INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
      {AMD64_INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
      {AMD64_INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
      {AMD64_INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
      {AMD64_INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
      {AMD64_INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
      {AMD64_INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
      {AMD64_INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            }
    },                                                 
    /* reg = 3 */                                      
    {                                                  
      {AMD64_INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
      {AMD64_INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
      {AMD64_INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
      {AMD64_INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
      {AMD64_INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
      {AMD64_INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
      {AMD64_INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
      {AMD64_INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            }
    },                                                 
    /* reg = 4 */                                      
    {                                                  
      {AMD64_FSTSW   , AX     , NULL_OP, NULL_OP, TRUE  ,            },
      {AMD64_INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
      {AMD64_INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
      {AMD64_INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
      {AMD64_INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
      {AMD64_INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
      {AMD64_INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
      {AMD64_INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            }
    },                                                 
    /* reg = 5 */                                      
    {                                                  
      {AMD64_FUCOMIP , NULL_OP, ST     , ST0    , TRUE  ,            },
      {AMD64_FUCOMIP , NULL_OP, ST     , ST1    , TRUE  ,            },
      {AMD64_FUCOMIP , NULL_OP, ST     , ST2    , TRUE  ,            },
      {AMD64_FUCOMIP , NULL_OP, ST     , ST3    , TRUE  ,            },
      {AMD64_FUCOMIP , NULL_OP, ST     , ST4    , TRUE  ,            },
      {AMD64_FUCOMIP , NULL_OP, ST     , ST5    , TRUE  ,            },
      {AMD64_FUCOMIP , NULL_OP, ST     , ST6    , TRUE  ,            },
      {AMD64_FUCOMIP , NULL_OP, ST     , ST7    , TRUE  ,            }
    },                                                 
    /* reg = 6 */                                      
    {                                                  
      {AMD64_FCOMIP  , NULL_OP, ST     , ST0    , TRUE  ,            },
      {AMD64_FCOMIP  , NULL_OP, ST     , ST1    , TRUE  ,            },
      {AMD64_FCOMIP  , NULL_OP, ST     , ST2    , TRUE  ,            },
      {AMD64_FCOMIP  , NULL_OP, ST     , ST3    , TRUE  ,            },
      {AMD64_FCOMIP  , NULL_OP, ST     , ST4    , TRUE  ,            },
      {AMD64_FCOMIP  , NULL_OP, ST     , ST5    , TRUE  ,            },
      {AMD64_FCOMIP  , NULL_OP, ST     , ST6    , TRUE  ,            },
      {AMD64_FCOMIP  , NULL_OP, ST     , ST7    , TRUE  ,            }
    },                                                 
    /* reg = 7 */                                      
    {                                                  
      {AMD64_INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
      {AMD64_INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
      {AMD64_INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
      {AMD64_INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
      {AMD64_INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
      {AMD64_INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
      {AMD64_INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            },
      {AMD64_INVALID_OPC  , NULL_OP, NULL_OP, NULL_OP, TRUE  ,            }
    }
  }
};

t_hash_table * amd64_opcode_hash = NULL;

static t_uint32 OpcodeHash(void *key, t_hash_table * ht)
{
  t_amd64_opcode * opc = (t_amd64_opcode *) key;
  return (t_uint32) *opc;
}

static t_int32 OpcodeCmp(void * key1, void * key2)
{
  t_amd64_opcode * opc1 = (t_amd64_opcode *) key1;
  t_amd64_opcode * opc2 = (t_amd64_opcode *) key2;
  return (*opc1) - (*opc2);
}

static void OpcodeNodeFree(const void * he, void * data)
{
  Free(he);
}

void Amd64CreateOpcodeHashTable(void)
{
  int i,j,k;
  t_amd64_opcode_entry * entry;
  
  amd64_opcode_hash = HashTableNew(MAX_AMD64_OPCODE,0,OpcodeHash,OpcodeCmp,OpcodeNodeFree);

  /* one byte opcodes */
  for (i = 0; i < 256; i++)
  {
    entry = &(disamd64[i]);
    if ((entry->opcode >= 0) && (entry->opcode != AMD64_UNSUPPORTED_OPC) && (entry->opcode != AMD64_INVALID_OPC))
    {
      t_amd64_opcode_he * he = Calloc(1,sizeof(t_amd64_opcode_he));
      he->entry = entry;
      HASH_TABLE_NODE_SET_KEY(&he->node,  &(entry->opcode));
      HashTableInsert(amd64_opcode_hash, (void *)he);
    }
  }
  /* two byte opcodes */
  for (i = 0; i < 256; i++)
  {
    entry = &(disamd64_twobyte[i]);
    if ((entry->opcode >= 0) && (entry->opcode != AMD64_UNSUPPORTED_OPC) && (entry->opcode != AMD64_INVALID_OPC))
    {
      t_amd64_opcode_he * he = Calloc(1,sizeof(t_amd64_opcode_he));
      he->entry = entry;
      HASH_TABLE_NODE_SET_KEY(&he->node, &(entry->opcode));
      HashTableInsert(amd64_opcode_hash, (void *)he);
    }
  }

  /* fpu memory operations */
  for (i = 0; i < 8; i++)
    for (j = 0; j < 8; j++)
    {
      entry = &(disamd64_fpu_mem[i][j]);
      if ((entry->opcode >= 0) && (entry->opcode != AMD64_UNSUPPORTED_OPC) && (entry->opcode != AMD64_INVALID_OPC))
      {
	t_amd64_opcode_he * he = Calloc(1,sizeof(t_amd64_opcode_he));
	he->entry = entry;
	HASH_TABLE_NODE_SET_KEY(&he->node, &(entry->opcode));
	HashTableInsert(amd64_opcode_hash, (void *)he);
      }
    }

  /* fpu register operations */
  for (i = 0; i < 8; i++)
    for (j = 0; j < 8; j++)
      for (k = 0; k < 8; k++)
      {
	entry = &(disamd64_fpu_reg[i][j][k]);
	if ((entry->opcode >= 0) && (entry->opcode != AMD64_UNSUPPORTED_OPC) && (entry->opcode != AMD64_INVALID_OPC))
	{
	  t_amd64_opcode_he * he = Calloc(1,sizeof(t_amd64_opcode_he));
	  he->entry = entry;
	  HASH_TABLE_NODE_SET_KEY(&he->node, &(entry->opcode));
	  HashTableInsert(amd64_opcode_hash, (void *)he);
	}
      }

  /* extended opcode groups */
  for (i = 0; i < AMD64_FPU_ESC-AMD64_GRP1a; i++)
  {
    for (j = 0; j < 8; j++)
    {
      entry = &(disamd64_grps[i][j]);
      if ((entry->opcode >= 0) && (entry->opcode != AMD64_UNSUPPORTED_OPC) && (entry->opcode != AMD64_INVALID_OPC))
      {
	t_amd64_opcode_he * he = Calloc(1,sizeof(t_amd64_opcode_he));
	he->entry = entry;
	HASH_TABLE_NODE_SET_KEY(&he->node, &(entry->opcode));
	HashTableInsert(amd64_opcode_hash, (void *)he);
      }
    }
  }
}
