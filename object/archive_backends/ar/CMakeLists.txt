file(GLOB ObjectAr_C "*.c")
file(GLOB ObjectAr_H "*.h")

opt_gen(ObjectAr ${CMAKE_CURRENT_SOURCE_DIR} diabloar_cmdline diabloar DiabloArCmdline "-d")

add_library(ObjectAr ${ObjectAr_C} ${ObjectAr_opt})
target_compile_definitions(ObjectAr
  PRIVATE DIABLOAR_INTERNAL
)
target_include_current_directories(ObjectAr)
target_link_libraries(ObjectAr
  PUBLIC Object
)

#install(FILES ${ObjectAr_H} DESTINATION include)
