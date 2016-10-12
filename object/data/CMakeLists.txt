file(GLOB LinkerFiles "*ld")

foreach(FullFilePath ${LinkerFiles})
  get_filename_component(FileName ${FullFilePath} NAME)
  configure_file(${FullFilePath} ${CMAKE_CURRENT_BINARY_DIR}/linker_descriptions/${FileName} COPYONLY)
endforeach()

# Install linkerscripts to share/diablo
INSTALL(FILES ${LinkerFiles} DESTINATION share/diablo/linker_descriptions)
