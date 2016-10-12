if(ArchitectureI386)
  add_subdirectory(i386)
endif()
if(ArchitectureARM)
  add_subdirectory(ARM)
endif()

# TODO: other archs