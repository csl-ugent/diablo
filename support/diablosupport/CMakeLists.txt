file(GLOB Support_C "*.c" "*.cc")
file(GLOB Support_H "*.h" "*.hpp")

option(Addresses32 "Base requirement: support 32 bit target architectures" ON)
option(Addresses64 "Support 64 bit target architectures (WARNING: makes diablo no longer compile!)" OFF) # TODO

if (NOT(Addresses32 OR Addresses64))
  message(FATAL_ERROR "Cannot disable 32 and 64 bit addresses")
endif()

bison_and_flex(diablosupport_conffile)
bison_and_flex(diablosupport_dotfile)
opt_gen(Support ${CMAKE_CURRENT_SOURCE_DIR} diablosupport_cmdline diablosupport DiabloSupportCmdline "-d")

add_library(Support ${Support_C} ${Support_CC} ${Support_opt} ${BISON_diablosupport_conffile_parser_OUTPUTS} ${FLEX_diablosupport_conffile_lexer_OUTPUTS} ${BISON_diablosupport_dotfile_parser_OUTPUTS} ${FLEX_diablosupport_dotfile_lexer_OUTPUTS})
target_compile_definitions(Support
  PRIVATE DIABLOSUPPORT_INTERNAL
  # We have an additional optimization that we want enabled for Release builds:
  PUBLIC $<$<CONFIG:Release>:CLASS_EXPANDS_IN_HEADER>
# If Addresses32 is enabled, make BIT32ADDRSUPPORT a public define, so it will also be defined in all targets
# that depend on Support. Analogous for Addresses64.
  PUBLIC $<$<BOOL:${Addresses32}>:BIT32ADDRSUPPORT>
  PUBLIC $<$<BOOL:${Addresses64}>:BIT64ADDRSUPPORT>
)
target_include_current_directories(Support)
target_link_libraries(Support
  INTERFACE DiabloJudy1
  INTERFACE DiabloJudyL
  PRIVATE rt
  PRIVATE dl
)

#install(FILES ${Support_H} DESTINATION include)
