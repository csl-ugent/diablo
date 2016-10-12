file(GLOB Object_C "*.c")
file(GLOB Object_H "*.h")

bison_and_flex(diabloobject_layout_scripts)
bison_and_flex(diabloobject_linker_scripts)
opt_gen(Object ${CMAKE_CURRENT_SOURCE_DIR} diabloobject_cmdline diabloobject DiabloObjectCmdline "-d")

option(UseInstallPrefixForLinkerScripts "Use the install prefix path for linker scripts, rather than the build directory's." OFF)

if (UseInstallPrefixForLinkerScripts)
    set(DATADIR "${CMAKE_INSTALL_PREFIX}/share/diablo/")
else()
    set(DATADIR "${CMAKE_CURRENT_BINARY_DIR}/../data/")
endif(UseInstallPrefixForLinkerScripts)

add_library(Object ${Object_C} ${Object_opt} ${BISON_diabloobject_layout_scripts_parser_OUTPUTS} ${FLEX_diabloobject_layout_scripts_lexer_OUTPUTS} ${BISON_diabloobject_linker_scripts_parser_OUTPUTS} ${FLEX_diabloobject_linker_scripts_lexer_OUTPUTS})
target_compile_definitions(Object
  PRIVATE DIABLOOBJECT_INTERNAL
  PRIVATE DATADIR="${DATADIR}"
)
target_include_current_directories(Object)
target_link_libraries(Object
  PUBLIC Support
)

#install(FILES ${Object_H} DESTINATION include)
