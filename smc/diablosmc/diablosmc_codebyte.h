/** \file smc_codebyte.h
 * 
 * Codebyte support 
 */

/* Codebyte typedefs {{{ */
#ifndef DIABLOSMC_CODEBYTE_TYPEDEFS_H
#define DIABLOSMC_CODEBYTE_TYPEDEFS_H
#endif /* Codebyte typedefs }}} */

/* Codebyte Globals {{{ */
#ifndef DIABLOSMC_CODEBYTE_GLOBALS_H
#define DIABLOSMC_CODEBYTE_GLOBALS_H
#endif /* Codebyte Globals }}} */
/* Codebyte Defines {{{ */
#ifndef DIABLOSMC_CODEBYTE_DEFINES_H
#define DIABLOSMC_CODEBYTE_DEFINES_H

#define T_CODEBYTE(x) ((t_codebyte *)(x))
#define CFG_FOREACH_CODEBYTE(cfg,codebyte) for (codebyte_ref = CODEBYTELIST_FIRST(CFG_CODEBYTELIST(cfg)), codebyte = codebyte_ref?CODEBYTE_REF_CODEBYTE(codebyte_ref):NULL; codebyte_ref!=NULL; codebyte_ref = CODEBYTE_REF_NEXT(codebyte_ref), codebyte = codebyte_ref?CODEBYTE_REF_CODEBYTE(codebyte_ref):NULL)

#endif /* }}} Graph Defines */
#ifdef DIABLOSMC_FUNCTIONS
#ifndef DIABLOSMC_CODEBYTE_FUNCTIONS
#define DIABLOSMC_CODEBYTE_FUNCTIONS
#endif /* }}} Codebyte Functions */
#endif

/* Codebyte Types {{{ */
#ifdef DIABLOSMC_TYPES
#ifndef DIABLOSMC_CODEBYTE_TYPES
#define DIABLOSMC_CODEBYTE_TYPES
/*! Struct to build a list of codebyte references, as used in t_ins and
 * t_codebyte */
#endif /* }}} Relocatable Types */
#endif
/* vim: set shiftwidth=4 cinoptions={.5s,g0,p5,t0,(0,^-0.5s,n-2,+0.5s foldmethod=marker tw=78 cindent: */
