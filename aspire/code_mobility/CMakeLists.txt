file(GLOB CodeMobility_CC "*.cc")

opt_gen(CodeMobility ${CMAKE_CURRENT_SOURCE_DIR} code_mobility_cmdline code_mobility CodeMobility "-d")

add_library(CodeMobility ${CodeMobility_CC} ${CodeMobility_opt})
target_include_current_directories(CodeMobility)
target_link_libraries(CodeMobility
  PUBLIC DiabloAnnotations
  PUBLIC AnoptARM
  PUBLIC Transformer
  PRIVATE DiabloObfuscationBase
)
