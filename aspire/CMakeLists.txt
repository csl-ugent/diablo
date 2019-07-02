file(GLOB Aspire_CC "*.cc")
file(GLOB Aspire_H "*.h")

# both needed because regular source files are located in _SOURCE_
# and generated source file (e.g., by opt-gen) are located in _BINARY_
include_directories("${CMAKE_CURRENT_BINARY_DIR}")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}")

add_subdirectory(diablosoftvm)
add_subdirectory(callstack_checks)
add_subdirectory(code_mobility)
add_subdirectory(attestation)
add_subdirectory(self_debugging)
add_subdirectory(transformer)
add_subdirectory(reaction_mechanisms)
add_subdirectory(cf_tagging)
add_subdirectory(softvmproxy)

opt_gen_ext_base_make_library(Aspire ${CMAKE_SOURCE_DIR}/aspire/ aspire aspire_options aspire_options_list AspireOptions aspire_options "-d")

set(common_link_libraries
  PRIVATE AnoptARM
  PRIVATE ARM_Opt
  PRIVATE Aspire_Opt
  PRIVATE Common_Opt
  PRIVATE DiabloAnnotations
  PRIVATE FlowgraphDebugDwarf
  PRIVATE ObjectElf
)

# setup target and binary
# ========================
# SoftVM Proxy
add_executable (diablo-softvm-proxy ${CMAKE_CURRENT_SOURCE_DIR}/softvmproxy/server.cc ${CMAKE_CURRENT_SOURCE_DIR}/softvmproxy/common.cc)
target_link_libraries(diablo-softvm-proxy
  PRIVATE dl
)
set_target_properties(diablo-softvm-proxy PROPERTIES COMPILE_FLAGS "-m32" LINK_FLAGS "-m32 -Wl,-Map,diablo-softvm-proxy.map")
add_executable (diablo-softvm-proxy-test ${CMAKE_CURRENT_SOURCE_DIR}/softvmproxy/client.cc)
target_link_libraries(diablo-softvm-proxy-test
  PRIVATE SoftVMProxy
)

# 0. DUMMY FRONT-END
# ------------------
add_executable (diablo-selfprofiling ${CMAKE_CURRENT_SOURCE_DIR}/BLP00/diablo_aspire_blp00_main.cc ${CMAKE_SOURCE_DIR}/frontends/common.cc ${Aspire_CC})
target_include_current_directories(diablo-selfprofiling)
target_link_libraries(diablo-selfprofiling
  ${common_link_libraries}
)
set_target_properties(diablo-selfprofiling PROPERTIES LINK_FLAGS "-Wl,-Map,diablo-selfprofiling.map")
INSTALL(TARGETS diablo-selfprofiling DESTINATION bin)

# 1. EXTRACTOR
# ------------
add_executable (diablo-extractor ${CMAKE_CURRENT_SOURCE_DIR}/BLP01/diablo_aspire_blp01_main.cc ${CMAKE_SOURCE_DIR}/frontends/common.cc ${Aspire_CC})
target_include_current_directories(diablo-extractor)
target_link_libraries(diablo-extractor
  ${common_link_libraries}
  PRIVATE DiabloSoftVM
  PRIVATE SoftVMProxy
)
set_target_properties(diablo-extractor PROPERTIES LINK_FLAGS "-Wl,-Map,diablo-extractor.map")
INSTALL(TARGETS diablo-extractor DESTINATION bin)

# 4. OBFUSCATOR
# -------------
set(backend_file "${CMAKE_CURRENT_BINARY_DIR}/backend_ARM_generated.cc")
set(architecture ARM)
string(TOUPPER ${architecture} architecture_allcaps)
configure_file("${CMAKE_SOURCE_DIR}/obfuscation/backend_template.cc.in" ${backend_file})
set(obfuscation_link_libraries
  PRIVATE DiabloObfuscationARM
  PRIVATE DiabloObfuscationBase
  PRIVATE Obfuscation_Opt
  PRIVATE SoftVMProxy
)

add_executable (diablo-obfuscator ${CMAKE_CURRENT_SOURCE_DIR}/BLP04/diablo_aspire_blp04_main.cc ${CMAKE_SOURCE_DIR}/frontends/common.cc ${Aspire_CC} ${backend_file})
if (SELF_DEBUGGING)
  target_compile_definitions(diablo-obfuscator
    PRIVATE SELF_DEBUGGING
  )
endif()
target_include_current_directories(diablo-obfuscator)
target_link_libraries(diablo-obfuscator
  ${common_link_libraries}
  PRIVATE CodeMobility
  PRIVATE DiabloSoftVM
  ${obfuscation_link_libraries}
  PRIVATE Attestation
  PRIVATE SelfDebugging
  PRIVATE CallStackChecks
  PRIVATE ReactionMechanisms
  PRIVATE CFTagging
  PRIVATE dl
)
set_target_properties(diablo-obfuscator PROPERTIES LINK_FLAGS "-Wl,-Map,diablo-obfuscator.map")
INSTALL(TARGETS diablo-obfuscator DESTINATION bin)
