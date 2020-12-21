file(GLOB ObjectElf_C "*.c")
file(GLOB ObjectElf_CC "*.cc")
file(GLOB ObjectElf_H "*.h")

opt_gen(ObjectElf ${CMAKE_CURRENT_SOURCE_DIR} diabloelf_cmdline diabloelf DiabloElfCmdline "-d")

add_library(ObjectElf ${ObjectElf_C} ${ObjectElf_CC} ${ObjectElf_opt})
target_compile_definitions(ObjectElf
  PRIVATE DIABLOOBJECT_INTERNAL
)
target_include_current_directories(ObjectElf)
target_link_libraries(ObjectElf
  PUBLIC Object
)

#install(FILES ${ObjectElf_H} DESTINATION include)
# install(FILES builtin/) # TODO
