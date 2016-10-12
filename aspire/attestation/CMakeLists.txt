file(GLOB Attestation_CC "*.cc")

add_library(Attestation ${Attestation_CC})
target_compile_definitions(Attestation
  PRIVATE ATTESTATION_INTERNAL
)
target_include_current_directories(Attestation)
target_link_libraries(Attestation
  PUBLIC CodeMobility
  PUBLIC DiabloAnnotations
  PUBLIC FlowgraphARM
)
