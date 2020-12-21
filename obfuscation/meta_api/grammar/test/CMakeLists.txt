add_definitions(-DGRAMMAR_TEST)

add_executable(ImplementationGrammarTest ${CMAKE_CURRENT_SOURCE_DIR}/implementation.cc
  ${CMAKE_CURRENT_BINARY_DIR}/../implementationLexer.cc ${CMAKE_CURRENT_BINARY_DIR}/../implementationParser.cc)

# include the grammar header file directories
target_include_directories(ImplementationGrammarTest
  PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/..
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../..
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
  PRIVATE ${ANTLR_INCLUDE_DIR})

target_link_libraries(ImplementationGrammarTest
  PRIVATE antlr3c
)

add_dependencies(ImplementationGrammarTest Antlr Grammar)
