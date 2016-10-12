file(GLOB FlowgraphDebugDwarf_CC "*.cc")
file(GLOB FlowgraphDebugDwarf_H "*.h")

add_library(FlowgraphDebugDwarf ${FlowgraphDebugDwarf_CC})
target_include_current_directories(FlowgraphDebugDwarf)
target_link_libraries(FlowgraphDebugDwarf
  PUBLIC Flowgraph
  PUBLIC ObjectDebugDwarf
)
