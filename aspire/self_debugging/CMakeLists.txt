file(GLOB SelfDebugging_CC "*.cc")
file(GLOB SelfDebugging_H "*.h")

if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/self_debugging.cc")
  set(SELF_DEBUGGING TRUE PARENT_SCOPE)
endif()

opt_gen(SelfDebugging ${CMAKE_CURRENT_SOURCE_DIR} self_debugging_cmdline self_debugging SelfDebugging "-d")

add_library(SelfDebugging ${SelfDebugging_CC} ${SelfDebugging_opt})
target_include_current_directories(SelfDebugging)
target_link_libraries(SelfDebugging
  PUBLIC AnoptARM
  PUBLIC DiabloAnnotations
  PUBLIC Transformer
  )
