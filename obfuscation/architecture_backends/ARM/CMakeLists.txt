file(GLOB ARMObfuscation_CC "*.cc")
file(GLOB ARMObfuscation_H "*.h")

add_obfuscation_architecture(ARM)

add_library(DiabloObfuscationARM ${ARMObfuscation_CC})
target_include_current_directories(DiabloObfuscationARM)
target_link_libraries(DiabloObfuscationARM
  PUBLIC AnoptARM
  PRIVATE FlowgraphDebugDwarf
  PRIVATE DiabloAnnotations
  PUBLIC DiabloObfuscationGeneric
)
