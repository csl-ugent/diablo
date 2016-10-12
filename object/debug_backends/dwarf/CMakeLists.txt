file(GLOB ObjectDebugDwarf_CC "*.cc")
file(GLOB ObjectDebugDwarf_H "*.h")

opt_gen(ObjectDebugDwarf ${CMAKE_CURRENT_SOURCE_DIR} diabloobject_dwarf_cmdline diabloobject_dwarf DiabloObjectDwarfCmdline "-d")

add_library(ObjectDebugDwarf ${ObjectDebugDwarf_CC} ${ObjectDebugDwarf_opt})
target_include_current_directories(ObjectDebugDwarf)
target_link_libraries(ObjectDebugDwarf
  PUBLIC Object
)
