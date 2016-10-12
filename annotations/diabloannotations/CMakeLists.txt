file(GLOB diabloannotations_CC "*.cc")

add_library(DiabloAnnotations ${diabloannotations_CC})
target_include_current_directories(DiabloAnnotations)
target_link_libraries(DiabloAnnotations
  PUBLIC Flowgraph
  PRIVATE jansson
)
