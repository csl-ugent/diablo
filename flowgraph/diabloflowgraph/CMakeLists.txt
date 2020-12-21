file(GLOB Flowgraph_C "*.c" "*.cc")
file(GLOB Flowgraph_H "*.h" "*.hpp")

opt_gen(Flowgraph ${CMAKE_CURRENT_SOURCE_DIR} diabloflowgraph_cmdline diabloflowgraph DiabloFlowgraphCmdline "-d")
opt_gen(Flowgraph ${CMAKE_CURRENT_SOURCE_DIR} diabloflowgraph_new_target_selector_cmdline diabloflowgraph_new_target_selector DiabloFlowgraphNewTargetSelectorCmdline "-d")

add_library(Flowgraph ${Flowgraph_C} ${Flowgraph_opt})
target_compile_definitions(Flowgraph
  PRIVATE DIABLOFLOWGRAPH_INTERNAL
)
target_include_current_directories(Flowgraph)
target_link_libraries(Flowgraph
  PUBLIC Object
  PUBLIC ObjectDebugDwarf
)

#install(FILES ${Flowgraph_H} DESTINATION include)
