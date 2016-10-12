file(GLOB AnoptARM_C "*.c")
file(GLOB AnoptARM_H "*.h")

opt_gen(AnoptARM ${CMAKE_CURRENT_SOURCE_DIR} diabloanoptarm_cmdline diabloanoptarm DiabloAnoptArmCmdline "-d")

add_library(AnoptARM ${AnoptARM_C} ${AnoptARM_opt})
target_include_current_directories(AnoptARM)
target_link_libraries(AnoptARM
  PUBLIC Anopt
  PUBLIC FlowgraphARM
)

#install(FILES ${AnoptARM_H} DESTINATION include)
