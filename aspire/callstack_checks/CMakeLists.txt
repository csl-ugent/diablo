file(GLOB CallStackChecks_CC "*.cc")

#opt_gen(CallStackChecks ${CMAKE_CURRENT_SOURCE_DIR} callstack_checks_cmdline callstack_checks CallStackChecks "-d")

add_library(CallStackChecks ${CallStackChecks_CC})# ${CallStackChecks_opt})
target_include_current_directories(CallStackChecks)
target_link_libraries(CallStackChecks
  PUBLIC DiabloAnnotations
  PUBLIC AnoptARM
  PRIVATE CodeMobility
  PRIVATE SelfDebugging
  PUBLIC Transformer
  PRIVATE DiabloSoftVM
)
