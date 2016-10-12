file(GLOB MapTiLinker_C "*.c")
file(GLOB MapTiLinker_H "*.h")

bison_and_flex(diablotilinker)
opt_gen(MapTiLinker ${CMAKE_CURRENT_SOURCE_DIR} diablotilinker_cmdline diablotilinker DiabloTiLinkerCmdline "-d")

add_library(MapTiLinker ${MapTiLinker_C} ${MapTiLinker_opt} ${BISON_diablotilinker_parser_OUTPUTS} ${FLEX_diablotilinker_lexer_OUTPUTS})
target_compile_definitions(MapTiLinker
  PRIVATE DIABLOTILINKER_INTERNAL
)
target_include_current_directories(MapTiLinker)
target_link_libraries(MapTiLinker
  PUBLIC Object
)

#install(FILES ${MapTiLinker_H} DESTINATION include)
