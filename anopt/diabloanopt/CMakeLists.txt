file(GLOB Anopt_C "*.c")
file(GLOB Anopt_H "*.h")

opt_gen(Anopt ${CMAKE_CURRENT_SOURCE_DIR} diabloanopt_cmdline diabloanopt DiabloAnoptCmdline "-d")

add_library(Anopt ${Anopt_C} ${Anopt_opt})
target_compile_definitions(Anopt
  PRIVATE DIABLOANOPT_INTERNAL
)
target_include_current_directories(Anopt)
target_link_libraries(Anopt
  PUBLIC Flowgraph
)

#install(FILES ${Anopt_H} DESTINATION include)
