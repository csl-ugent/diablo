file(GLOB ObjectPeCoff_C "*.c")
file(GLOB ObjectPeCoff_H "*.h")

add_library(ObjectPeCoff ${ObjectPeCoff_C} ${ObjectPeCoff_opt})
target_include_current_directories(ObjectPeCoff)
target_link_libraries(ObjectPeCoff
  PUBLIC Object
)

#install(FILES ${ObjectPeCoff_H} DESTINATION include)
