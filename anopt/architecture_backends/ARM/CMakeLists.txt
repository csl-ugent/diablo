file(GLOB AnoptARM_C "*.c")
file(GLOB AnoptARM_CC "*.cc")
file(GLOB AnoptARM_H "*.h")

if (NOT BUILD_SHARED_LIBS)
  file(GLOB AnoptARMScore_CC "advanced_factoring_libraries/*.cc")
endif()

opt_gen(AnoptARM ${CMAKE_CURRENT_SOURCE_DIR} diabloanoptarm_cmdline diabloanoptarm DiabloAnoptArmCmdline "-d")

add_library(AnoptARM ${AnoptARM_C} ${AnoptARM_CC} ${AnoptARM_opt} ${AnoptARMScore_CC})
target_include_current_directories(AnoptARM)
target_link_libraries(AnoptARM
  PUBLIC Anopt
  PUBLIC FlowgraphARM
  PUBLIC DiabloAnnotations
  PUBLIC Common_Opt
)

add_subdirectory(advanced_factoring_libraries)

#install(FILES ${AnoptARM_H} DESTINATION include)
