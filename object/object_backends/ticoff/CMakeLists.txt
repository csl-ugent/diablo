file(GLOB ObjectTiCoff_C "*.c")
file(GLOB ObjectTiCoff_H "*.h")

opt_gen(ObjectTiCoff ${CMAKE_CURRENT_SOURCE_DIR} diabloticoff_cmdline diabloticoff DiabloTiCoffCmdline "-d")

add_library(ObjectTiCoff ${ObjectTiCoff_C} ${ObjectTiCoff_opt})
target_compile_definitions(ObjectTiCoff
  PRIVATE DIABLOTIFOCC_INTERNAL
)
target_include_current_directories(ObjectTiCoff)
target_link_libraries(ObjectTiCoff
  PUBLIC Object
)

#install(FILES ${ObjectTiCoff_H} DESTINATION include)
