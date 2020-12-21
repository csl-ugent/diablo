if (BUILD_SHARED_LIBS)
  add_library(maExampleLibrary SHARED example.cc)
  target_link_libraries(maExampleLibrary PUBLIC DiabloMetaAPI)

  add_library(maDatatypeInteger SHARED dt_integer.cc)
  target_link_libraries(maDatatypeInteger PUBLIC DiabloMetaAPI)

  add_library(maDatatypeString SHARED dt_string.cc)
  target_link_libraries(maDatatypeString PUBLIC DiabloMetaAPI)

  add_library(maDatatypeRegister SHARED dt_register.cc)
  target_link_libraries(maDatatypeRegister PUBLIC DiabloMetaAPI)

  add_library(maDatatypeFunctionPointer SHARED dt_functionpointer.cc)
  target_link_libraries(maDatatypeFunctionPointer PUBLIC DiabloMetaAPI)

  add_library(maDatatypeVoid SHARED dt_void.cc)
  target_link_libraries(maDatatypeVoid PUBLIC DiabloMetaAPI)
endif()
