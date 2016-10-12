file(GLOB SMC_C diablosmc.c diablosmc_cmdline.c diablosmc_flowgraph.c diablosmc_deflowgraph.c diablosmc_assemble.c diablosmc_dynamic_members.c diablosmc_statelist.c diablosmc_codebytelist.c diablosmc_dots.c diablosmc_write_out.c diablosmc_state.c diablosmc_codebyte.c diablosmc_factoring.c) # explicitly don't do diablosmc_equivalent_instructions.c
file(GLOB SMC_H "*.h")

# Hardcoded dependency on I386
if (ArchitectureI386)
  opt_gen(SMC ${CMAKE_CURRENT_SOURCE_DIR} diablosmc_cmdline diablosmc DiabloSmcCmdline "-d")

  add_library(SMC ${SMC_C} ${SMC_opt})
  target_compile_definitions(SMC
    PRIVATE DIABLOSMC_INTERNAL
  )
  target_include_current_directories(SMC)
  target_link_libraries(SMC
    PRIVATE AnoptI386
    PUBLIC FlowgraphI386
    PUBLIC ObjectElf
  )

#install(FILES ${ObjectAr_H} DESTINATION include)
endif()
