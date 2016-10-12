file(GLOB AspireSoftVM_CC "*.cc")

opt_gen(DiabloSoftVM ${CMAKE_CURRENT_SOURCE_DIR} diablosoftvm_cmdline diablosoftvm DiabloSoftVM "-d")

add_library(DiabloSoftVM ${AspireSoftVM_CC} ${DiabloSoftVM_opt})
target_include_current_directories(DiabloSoftVM)
target_link_libraries(DiabloSoftVM
  PRIVATE AnoptARM
  PUBLIC Aspire_Opt
  PUBLIC DiabloAnnotations
  PUBLIC FlowgraphARM
  PUBLIC jansson
  PUBLIC CodeMobility
  PRIVATE dl
)
