file(GLOB AnoptI386_C "*.c")
file(GLOB AnoptI386_H "*.h")

opt_gen(AnoptI386 ${CMAKE_CURRENT_SOURCE_DIR} diabloanopti386_cmdline diabloanopti386 DiabloAnoptI386Cmdline "-d")

add_library(AnoptI386 ${AnoptI386_C} ${AnoptI386_opt})
target_include_current_directories(AnoptI386)
target_link_libraries(AnoptI386
  PUBLIC Anopt
  PUBLIC FlowgraphI386
)

#install(FILES ${AnoptI386_H} DESTINATION include)
