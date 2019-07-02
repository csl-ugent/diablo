#ifndef DIABLOANOPTARM_ADVANCED_FACTOR_DEBUG_H
#define DIABLOANOPTARM_ADVANCED_FACTOR_DEBUG_H

//#define DEBUG_AF
//#define DEBUG_AF_DOT
//#define DEBUG_AF_DOT_ONLY_SET 771

#define DEBUG_AF_DEBUGCOUNTER diablosupport_options.debugcounter
#define DEBUG_AF_DEBUGCOUNTER2 diablosupport_options.debugcounter2
//#define DEBUG_AF_DEBUGCOUNTER 0

#ifdef DEBUG_AF
  #define IF_NR_TOTAL_SLICES_GE_DEBUGCOUNTER \
    if (nr_total_slices >= static_cast<t_uint32>(DEBUG_AF_DEBUGCOUNTER))
  #define DEBUGCOUNTER_VALUE \
    DEBUG_AF_DEBUGCOUNTER

  #define IF_PRIORITY_LIST_SIZE_DEBUGCOUNTER \
    if (PriorityListSize() >= static_cast<size_t>(DEBUG_AF_DEBUGCOUNTER2))
  #define DEBUGCOUNTER2_VALUE \
    DEBUG_AF_DEBUGCOUNTER2
#else
  #define IF_NR_TOTAL_SLICES_GE_DEBUGCOUNTER \
    if (false)
  #define DEBUGCOUNTER_VALUE \
    (1000000000)

  #define  IF_PRIORITY_LIST_SIZE_DEBUGCOUNTER\
    if (false)
  #define DEBUGCOUNTER2_VALUE \
    (1000000000)
#endif

#ifdef DEBUG_AF
  #define DumpDots(cfg, prefix, uid) __DumpDots(cfg, prefix, uid)
#else
  #define DumpDots(cfg, prefix, uid)
#endif

void __DumpDots(t_cfg *cfg, std::string prefix, int uid);

#endif
