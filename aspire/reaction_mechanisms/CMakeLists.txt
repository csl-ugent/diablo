file(GLOB ReactionMechanisms_CC "*.cc")

add_library(ReactionMechanisms ${ReactionMechanisms_CC})
target_compile_definitions(ReactionMechanisms
  PRIVATE REACTION_MECHANISMS_INTERNAL
)
target_include_current_directories(ReactionMechanisms)
target_link_libraries(ReactionMechanisms
  PUBLIC DiabloAnnotations
  PUBLIC FlowgraphARM
  PRIVATE SelfDebugging
)
