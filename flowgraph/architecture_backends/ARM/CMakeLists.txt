file(GLOB FlowgraphARM_C "*.c" "*.cc")
file(GLOB FlowgraphARM_H "*.h" "*.hpp")

bison_and_flex(diabloarm_parse_instruction)
opt_gen(FlowgraphARM ${CMAKE_CURRENT_SOURCE_DIR} diabloarm_cmdline diabloarm DiabloArmCmdline "-d")

add_library(FlowgraphARM ${FlowgraphARM_C} ${FlowgraphARM_opt} ${BISON_diabloarm_parse_instruction_parser_OUTPUTS} ${FLEX_diabloarm_parse_instruction_lexer_OUTPUTS})
target_compile_definitions(FlowgraphARM
  PRIVATE DIABLOARM_INTERNAL
)
target_include_current_directories(FlowgraphARM)
target_link_libraries(FlowgraphARM
  PUBLIC Flowgraph
  PRIVATE FlowgraphDebugDwarf
  PRIVATE MapArmAds
  PRIVATE MapBinutils
  PRIVATE MapTiLinker
  PRIVATE ObjectAr
  PRIVATE ObjectElf
  PRIVATE ObjectTiCoff
  PRIVATE m
)

#install(FILES ${FlowgraphARM_H} DESTINATION include)
