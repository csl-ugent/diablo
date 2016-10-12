file(GLOB Diversity_CC "*.cc")
file(GLOB Diversity_H "*.h")

list(APPEND Diversity_CC ${PROJECT_SOURCE_DIR}/obfuscation/obfuscation_architecture_backend.cc)
list(APPEND Diversity_CC ${PROJECT_SOURCE_DIR}/obfuscation/obfuscation_random.cc)
list(APPEND Diversity_CC ${PROJECT_SOURCE_DIR}/obfuscation/obfuscation_transformation.cc)

option(Diversity "Build diversity backends" ON)

cmake_dependent_option(DiversityI386      "Build the Diablo-i386 diversity backend" ON
                       "Diversity;ArchitectureI386" OFF)

cmake_dependent_option(DiversityIterative "Build the iterative diversity backend (if OFF: use Bertrand's engine)" ON
                       "DiversityI386" OFF)
cmake_dependent_option(DiversitySingleSeed "Use a single seed (TODO: document)" ON
                       "DiversityI386" OFF)
cmake_dependent_option(DiversitySplitAfter "Split BBL for call functions after the call as well" OFF
                       "DiversityI386" OFF)
cmake_dependent_option(DiversitySimpleTransformsToApply "TODO describe better, but the default is OFF" OFF
                       "DiversityI386" OFF)
cmake_dependent_option(DiversityIterativeInstSel "Hardcoded instruction selection for iterative diversity" OFF
                       "DiversityIterative" OFF)

set (DIABLODIVERSITY_VERSION_MAJOR 0)
set (DIABLODIVERSITY_VERSION_MINOR 5)

set(FORWARD_TO_ITERATIVE ${DiversityIterative})
set(SINGLESEED ${DiversitySingleSeed})
set(SPLIT_AFTER ${DiversitySplitAfter})
set(SIMPLE_TRANSFORMS_TO_APPLY ${DiversitySimpleTransformsToApply})

configure_diablo_config(diversity)

# TODO instsel

if (DiversityI386)

  opt_gen(Diversity ${CMAKE_CURRENT_SOURCE_DIR} diablodiversity_cmdline diablodiversity DiabloDiversityCmdline "-d")
  opt_gen_ext_base(Diversity ${CMAKE_SOURCE_DIR}/frontends/ diablo_diversity diversity_commandline_options diversity_options_list DiversityOptions diablo_diversity_options "-d") # hate

  add_executable (diablo-diversity ${CMAKE_SOURCE_DIR}/frontends/diablo_diversity_main.c ${Diversity_CC} ${Diversity_opt})
  target_compile_definitions(diablo-diversity
    PRIVATE DIABLODIVERSITY_INTERNAL
    PRIVATE OPAQUE_BINARIES_DIR="${OPAQUE_BINARIES_DIR}"
    )
  target_include_current_directories(diablo-diversity)
  target_link_libraries(diablo-diversity
    PRIVATE AnoptI386
    PRIVATE Common_Opt
    PRIVATE DiabloAnnotations
    PRIVATE DiabloObfuscationi386
    PRIVATE I386_Opt
    PRIVATE m
    PRIVATE SMC
    )

  #install(FILES ${Diversity_H} DESTINATION include)
endif()
