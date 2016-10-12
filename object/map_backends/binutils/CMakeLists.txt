file(GLOB MapBinutils_C "*.c")
file(GLOB MapBinutils_H "*.h")

bison_and_flex(diablobinutils)
opt_gen(MapBinutils ${CMAKE_CURRENT_SOURCE_DIR} diablobinutils_cmdline diablobinutils DiabloBinutilsCmdline "-d")

add_library(MapBinutils ${MapBinutils_C} ${MapBinutils_opt} ${BISON_diablobinutils_parser_OUTPUTS} ${FLEX_diablobinutils_lexer_OUTPUTS})
target_compile_definitions(MapBinutils
  PRIVATE DIABLOBINUTILS_INTERNAL
)
target_include_current_directories(MapBinutils)
target_link_libraries(MapBinutils
  PUBLIC Object
)

#install(FILES ${MapBinutils_H} DESTINATION include)
