# Diablo includes different files, COPIED FROM THE SAME SOURCE, depending on -DSTAGE. Hence the configure_file. I hate you, Diablo
# Luckily, this is mitigated by bison already outputing the file based on the output filename, but it still matters for the handler

# Disable address sanitizer for opt_gen as it is memory leak hell
if (HAS_FLAG_SANITIZE_ADDRESS)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fno-sanitize=address")
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})

# Stage 0
bison_and_flex_ext(opt_gen_stage0 opt_gen)

add_executable(opt_gen_stage_0 ${BISON_opt_gen_stage0_parser_OUTPUTS} ${FLEX_opt_gen_stage0_lexer_OUTPUTS})
target_compile_definitions(opt_gen_stage_0
  PRIVATE STAGE=0
  PRIVATE USE_ARGV_1
)
target_include_directories(opt_gen_stage_0
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
)
target_link_libraries(opt_gen_stage_0
  PRIVATE ${FLEX_LIBRARIES}
  PRIVATE ${BISON_LIBRARIES}
)

# We will use stage 0 to generate some option files. However we want these to be generated in a separate directory
# OPTIONS_DIR to overcome some CMake bug that results in these option files always being regenerated every build
# after you've switched a git branch.
set(OPTIONS_DIR ${CMAKE_CURRENT_BINARY_DIR}/options)
set(OPT_GEN_STAGE0 ${OPTIONS_DIR}/opt_gen_stage_0)

# Copy stage 0 into the directory where we want to generate the option files
add_custom_command(
  OUTPUT ${OPT_GEN_STAGE0}
  DEPENDS opt_gen_stage_0
  COMMAND ${CMAKE_COMMAND} -E make_directory ${OPTIONS_DIR}
  COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_BINARY_DIR}/opt_gen_stage_0 ${OPTIONS_DIR}
)

# Generate the option files
add_custom_command(
  OUTPUT    ${OPTIONS_DIR}/options.c ${OPTIONS_DIR}/options.h
  DEPENDS   ${OPT_GEN_STAGE0} opt_gen.opt
  COMMAND   ${OPT_GEN_STAGE0} ${CMAKE_CURRENT_SOURCE_DIR}/opt_gen.opt
  COMMENT   "Running stage 0 opt_gen"
  WORKING_DIRECTORY ${OPTIONS_DIR}
)

# Stage 1: build the actual opt_gen executable, start with the grammars
bison_and_flex(opt_gen)

configure_file(${PROJECT_SOURCE_DIR}/support/diablosupport/diablosupport_options.c opt_gen_handler.c COPYONLY)
configure_file(${PROJECT_SOURCE_DIR}/support/diablosupport/diablosupport_options.h opt_gen_handler.h COPYONLY)

add_executable(opt_gen ${OPTIONS_DIR}/options.c opt_gen_handler.c ${BISON_opt_gen_parser_OUTPUTS} ${FLEX_opt_gen_lexer_OUTPUTS})
target_compile_definitions(opt_gen
  PRIVATE STAGE=1
)
target_include_current_directories(opt_gen)
target_include_directories(opt_gen
  PRIVATE ${OPTIONS_DIR}
)
target_link_libraries(opt_gen
  PRIVATE ${FLEX_LIBRARIES}
  PRIVATE ${BISON_LIBRARIES}
)

