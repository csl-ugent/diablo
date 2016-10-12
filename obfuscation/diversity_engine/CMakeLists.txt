file(GLOB Diversity_Engine_CC "*.cc" "*.c")
file(GLOB Diversity_Engine_H "*.h")

set (DIABLODIVERSITY_VERSION_MAJOR 0)
set (DIABLODIVERSITY_VERSION_MINOR 5)

set(FORWARD_TO_ITERATIVE ${DiversityIterative})
set(SINGLESEED ${DiversitySingleSeed})
set(SPLIT_AFTER ${DiversitySplitAfter})
set(SIMPLE_TRANSFORMS_TO_APPLY ${DiversitySimpleTransformsToApply})

configure_diablo_config(diversity)

opt_gen(Diversity ${CMAKE_CURRENT_SOURCE_DIR} diablodiversity_cmdline diablodiversity DiabloDiversityCmdline "-d")

add_library(DiabloDiversityEngine ${Diversity_Engine_CC} ${Diversity_opt})
target_compile_definitions(DiabloDiversityEngine
  PRIVATE DIABLODIVERSITY_INTERNAL
)
target_include_current_directories(DiabloDiversityEngine)
target_link_libraries(DiabloDiversityEngine
  PRIVATE Anopt
  PRIVATE DiabloAnnotations
  PUBLIC Flowgraph
  PUBLIC DiabloObfuscationGeneric
)
