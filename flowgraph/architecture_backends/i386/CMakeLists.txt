file(GLOB FlowgraphI386_C "*.c" "*.cc")
file(GLOB FlowgraphI386_H "*.h" "*.hpp")

opt_gen(FlowgraphI386 ${CMAKE_CURRENT_SOURCE_DIR} diabloi386_cmdline diabloi386 DiabloI386Cmdline "-d")

add_library(FlowgraphI386 ${FlowgraphI386_C} ${FlowgraphI386_opt})
target_compile_definitions(FlowgraphI386
  PRIVATE DIABLOI386_INTERNAL
)
target_include_current_directories(FlowgraphI386)
target_link_libraries(FlowgraphI386
  PUBLIC Flowgraph
  PRIVATE MapBinutils
  PRIVATE ObjectAr
  PRIVATE ObjectElf
)

# TODO make more generic?
if(ObjectPECOFF)
  target_link_libraries(FlowgraphI386
    PRIVATE MapMsil
    PRIVATE ObjectPeCoff
  )
endif()

#install(FILES ${FlowgraphI386_H} DESTINATION include)
