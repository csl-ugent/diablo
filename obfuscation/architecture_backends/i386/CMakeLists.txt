file(GLOB I386Obfuscation_CC "*.cc")
file(GLOB I386Obfuscation_H "*.h")

set(OPAQUE_BINARIES_DIR "${CMAKE_CURRENT_SOURCE_DIR}/opaque_predicates/binaries/")

add_obfuscation_architecture(i386)

opt_gen_obfuscation(DiabloObfuscationi386 ${CMAKE_CURRENT_SOURCE_DIR} i386_opaque_predicates I386OpaquePredicatesOpt "-d")
opt_gen_obfuscation(DiabloObfuscationi386 ${CMAKE_CURRENT_SOURCE_DIR} i386_inline_function I386InlineFunctionOpt "-d")
opt_gen_obfuscation(DiabloObfuscationi386 ${CMAKE_CURRENT_SOURCE_DIR} i386_instructionselection I386InstructionSelectionOpt "-d")

add_library(DiabloObfuscationi386 ${I386Obfuscation_CC} ${DiabloObfuscationi386_opt})
target_compile_definitions(DiabloObfuscationi386
  PRIVATE OPAQUE_BINARIES_DIR="${OPAQUE_BINARIES_DIR}"
)
target_include_current_directories(DiabloObfuscationi386)
target_link_libraries(DiabloObfuscationi386
  PUBLIC AnoptI386
  PUBLIC DiabloObfuscationGeneric
  PRIVATE DiabloAnnotations
)
