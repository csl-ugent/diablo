file(GLOB GenericObfuscation_CC "*.cc")
file(GLOB GenericObfuscation_H "*.h")

opt_gen_obfuscation(DiabloObfuscationGeneric ${CMAKE_CURRENT_SOURCE_DIR} branch_function BranchFunctionOpt "-d")
opt_gen_obfuscation(DiabloObfuscationGeneric ${CMAKE_CURRENT_SOURCE_DIR} flatten_function FlattenFunctionOpt "-d")
opt_gen_obfuscation(DiabloObfuscationGeneric ${CMAKE_CURRENT_SOURCE_DIR} opaque_predicate OpaquePredicateOpt "-d")
opt_gen_obfuscation(DiabloObfuscationGeneric ${CMAKE_CURRENT_SOURCE_DIR} flip_branches FlipBranchesOpt "-d")
opt_gen_obfuscation(DiabloObfuscationGeneric ${CMAKE_CURRENT_SOURCE_DIR} split_twoway_predicate SplitTwoWayPredicateOpt "-d")

add_library(DiabloObfuscationGeneric ${GenericObfuscation_CC} ${DiabloObfuscationGeneric_opt})
target_include_current_directories(DiabloObfuscationGeneric)
target_link_libraries(DiabloObfuscationGeneric
  PUBLIC Anopt
  PRIVATE DiabloAnnotations
  PRIVATE m
  PUBLIC DiabloObfuscationBase
)
