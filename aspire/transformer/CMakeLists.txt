file(GLOB Transformer_CC "*.cc")

add_library(Transformer ${Transformer_CC})
target_include_current_directories(Transformer)
target_link_libraries(Transformer
  PUBLIC DiabloAnnotations
  PUBLIC FlowgraphARM
)
