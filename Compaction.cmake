option(Compaction "Build compaction backends" ON)

cmake_dependent_option(CompactionI386 "Build the standard i386 Diablo for compaction" ON "Compaction;ArchitectureI386" OFF)
cmake_dependent_option(CompactionARM "Build the standard ARM Diablo for compaction" ON "Compaction;ArchitectureARM" OFF)

set(common_link_libraries
  PRIVATE FlowgraphDebugDwarf
  PRIVATE Common_Opt
  PRIVATE ObjectElf
)

if (CompactionI386)
  add_executable(diablo-i386 ${CMAKE_SOURCE_DIR}/frontends/diablo_i386_main.c)
  target_link_libraries(diablo-i386
    PRIVATE AnoptI386
    PRIVATE FlowgraphI386
    ${common_link_libraries}
    PRIVATE I386_Opt
  )
  INSTALL(TARGETS diablo-i386 DESTINATION bin)
endif()

if (CompactionARM)
  add_executable (diablo-arm ${CMAKE_SOURCE_DIR}/frontends/diablo_arm_main.cc  ${CMAKE_CURRENT_SOURCE_DIR}/frontends/common.cc)
  target_link_libraries(diablo-arm
    PRIVATE AnoptARM
    PRIVATE FlowgraphARM
    ${common_link_libraries}
    PRIVATE ARM_Opt
  )
  INSTALL(TARGETS diablo-arm DESTINATION bin)
endif()

add_executable (diablo-dwarf ${CMAKE_SOURCE_DIR}/frontends/diablo_dwarf.cc ${CMAKE_CURRENT_SOURCE_DIR}/frontends/common.cc)
target_include_directories(diablo-dwarf
  PRIVATE ${LIBDWARF_INCLUDE_DIR})

target_link_libraries(diablo-dwarf
  PRIVATE dwarf
  PRIVATE ObjectDebugDwarf
  PRIVATE FlowgraphARM
  ${common_link_libraries})

add_dependencies(diablo-dwarf Libdwarf)

INSTALL(TARGETS diablo-dwarf DESTINATION bin)
