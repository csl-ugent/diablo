file(GLOB Obfuscation_Base_CC "${PROJECT_SOURCE_DIR}/obfuscation/obfuscation_*.cc")
file(GLOB Obfuscation_H "*.h")

set (DIABLOOBFUSCATION_VERSION_MAJOR 1)
set (DIABLOOBFUSCATION_VERSION_MINOR 0)

# The architecture_backends subdirectories will call add_obfuscation_architecture to enable, conditionally, the right architectures
set_property(GLOBAL PROPERTY PropObfuscationArchitectures "")
function(add_obfuscation_architecture architecture)
  get_property(ObfuscationArchitectures GLOBAL PROPERTY PropObfuscationArchitectures)
  list(APPEND ObfuscationArchitectures ${architecture})
  set_property(GLOBAL PROPERTY PropObfuscationArchitectures ${ObfuscationArchitectures})
endfunction()

macro(opt_gen_obfuscation target dir opt fname)
  opt_gen_ext_base(${target} ${dir} ${opt} obfuscation_${opt}_options obfuscation_${opt}_option_list ${fname} ${opt}_opt ${ARGN})
endmacro()

add_subdirectory(generic)
add_subdirectory(architecture_backends)
add_subdirectory(diversity_engine)
add_subdirectory(meta_api)

opt_gen_ext_base_make_library(Obfuscation ${CMAKE_CURRENT_SOURCE_DIR} obfuscation obfuscation_obfuscation_options obfuscation_obfuscation_option_list ObfuscationOpt obfuscation_opt "-d")

add_library(DiabloObfuscationBase ${Obfuscation_Base_CC})
target_include_current_directories(DiabloObfuscationBase)
target_link_libraries(DiabloObfuscationBase
  PRIVATE Anopt
  PUBLIC DiabloAnnotations
  PUBLIC Flowgraph
  PRIVATE Obfuscation_Opt
)

set(common_link_libraries
  PRIVATE Anopt
  PRIVATE Common_Opt
  PRIVATE DiabloAnnotations
  PRIVATE DiabloObfuscationGeneric
  PRIVATE DiabloObfuscationBase
  PRIVATE FlowgraphDebugDwarf
  PRIVATE Obfuscation_Opt
  PRIVATE DiabloAnnotations
)

get_property(ObfuscationArchitectures GLOBAL PROPERTY PropObfuscationArchitectures)
foreach(architecture ${ObfuscationArchitectures})
  message(STATUS "Compiling obfuscation-diablo for architecture: ${architecture}")

  set(backend_file "${CMAKE_CURRENT_BINARY_DIR}/backend_${architecture}_generated.cc")
  string(TOUPPER ${architecture} architecture_allcaps) # Make variable for the configure step
  configure_file("${CMAKE_CURRENT_SOURCE_DIR}/backend_template.cc.in" ${backend_file})

  add_executable (diablo-obfuscation-${architecture} diablo_obfuscation_main.cc ${backend_file} ${CMAKE_SOURCE_DIR}/frontends/common.cc)
  target_include_current_directories(diablo-obfuscation-${architecture})
  target_link_libraries(diablo-obfuscation-${architecture}
    ${common_link_libraries}
    PRIVATE DiabloObfuscation${architecture}
    PRIVATE DiabloMetaAPI
  )


  add_executable (diablo-diversity-${architecture} diablo_diversity_main.cc ${backend_file})
  target_include_current_directories(diablo-diversity-${architecture})
  target_link_libraries(diablo-diversity-${architecture}
    ${common_link_libraries}
    PRIVATE DiabloDiversityEngine
    PRIVATE DiabloObfuscation${architecture}
    PRIVATE DiabloMetaAPI
  )
endforeach()
