file(GLOB MapMsil_C "*.c")
file(GLOB MapMsil_H "*.h")

add_library(MapMsil ${MapMsil_C} ${MapMsil_opt})
target_include_current_directories(MapMsil)
target_link_libraries(MapMsil
  PRIVATE Object
)

#install(FILES ${MapMsil_H} DESTINATION include)
