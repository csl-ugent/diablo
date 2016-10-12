file(GLOB MapArmAds_C "*.c")
file(GLOB MapArmAds_H "*.h")

opt_gen(MapArmAds ${CMAKE_CURRENT_SOURCE_DIR} diabloarmads_cmdline diabloarmads DiabloArmAdsCmdline "-d")

add_library(MapArmAds ${MapArmAds_C} ${MapArmAds_opt})
target_compile_definitions(MapArmAds
  PRIVATE DIABLOARMADS_INTERNAL
)
target_include_current_directories(MapArmAds)
target_link_libraries(MapArmAds
  PRIVATE Object
)

#install(FILES ${MapArmAds_H} DESTINATION include)
