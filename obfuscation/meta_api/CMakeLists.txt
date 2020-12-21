file(GLOB MetaAPI_c "*.c")

file(GLOB MetaAPI_cpp "*.cc")
set_source_files_properties(${MetaAPI_cpp} PROPERTIES COMPILE_FLAGS "-std=c++0x")

if (NOT BUILD_SHARED_LIBS)
  file(GLOB MetaAPI_lib_cc "libraries/*.cc")
endif()

add_subdirectory(grammar)
set_source_files_properties(${Grammar_source_files} PROPERTIES GENERATED TRUE)

add_library(DiabloMetaAPI ${MetaAPI_c} ${MetaAPI_cpp} ${Grammar_source_files} ${MetaAPI_lib_cc})

target_include_current_directories(DiabloMetaAPI)

# include the grammar header file directories
target_include_directories(DiabloMetaAPI
    PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/grammar
    PRIVATE ${ANTLR_INCLUDE_DIR})

target_link_libraries(DiabloMetaAPI
  PRIVATE antlr3c
  PUBLIC Flowgraph
  PRIVATE Obfuscation_Opt
)

add_dependencies(DiabloMetaAPI Antlr Grammar)

add_subdirectory(libraries)
