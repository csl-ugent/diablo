# set this variable to the name of the grammar file, without ".g"
#set(Grammar_file_name effects)
set(all_Grammar_source_files "")

foreach (Grammar_file_name effects constraints implementation)
    # no editing needed beyond this point
    set(Grammar_source_files
        ${CMAKE_CURRENT_BINARY_DIR}/${Grammar_file_name}Lexer.cc ${CMAKE_CURRENT_BINARY_DIR}/${Grammar_file_name}Parser.cc)

    # collect all grammar source files
    set(all_Grammar_source_files ${all_Grammar_source_files} ${Grammar_source_files})

    # use a combination of "add_custom_command" and "add_custom_target" so the grammar file does not need to be rebuilt
    # every time the make tool is invoked. The "add_custom_command" function is only called when the DEPENDS files of the
    # "add_custom_target" command have been modified.
    add_custom_command(OUTPUT ${Grammar_source_files}
                        COMMAND cp ${CMAKE_CURRENT_SOURCE_DIR}/${Grammar_file_name}.g ${CMAKE_CURRENT_BINARY_DIR}/${Grammar_file_name}.g
                        COMMAND java -cp ${CMAKE_SOURCE_DIR}/antlr-3.3-complete.jar org.antlr.Tool ${Grammar_file_name}.g
                        COMMAND mv ${CMAKE_CURRENT_BINARY_DIR}/${Grammar_file_name}Parser.c ${CMAKE_CURRENT_BINARY_DIR}/${Grammar_file_name}Parser.cc
                        COMMAND mv ${CMAKE_CURRENT_BINARY_DIR}/${Grammar_file_name}Lexer.c ${CMAKE_CURRENT_BINARY_DIR}/${Grammar_file_name}Lexer.cc
                        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${Grammar_file_name}.g)
endforeach(Grammar_file_name)

# this variable is needed in the parent scope (i.e. the scope of the CMakeLists.txt-file including this directory)
# to include it in the list of source files
set(Grammar_source_files
    ${all_Grammar_source_files} PARENT_SCOPE)

add_custom_target(Grammar
                    DEPENDS ${all_Grammar_source_files})

#add_subdirectory(test)
