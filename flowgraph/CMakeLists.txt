option(ArchitectureI386  "Base requirement: I386 architecture support"   ON)
option(ArchitectureARM  "Base requirement: ARM(32) architecture support" ON)

# TODO: enable other archs in architecture_backends
cmake_dependent_option(ArchitectureI386     "Base requirement: I386 architecture support" ON  "Addresses32" OFF)
cmake_dependent_option(ArchitectureARM      "Base requirement: ARM architecture support"  ON  "Addresses32" OFF)
cmake_dependent_option(ArchitectureARMTHUMB "ARM THUMB architecture support"              OFF "Addresses32" OFF)
cmake_dependent_option(ArchitecturePPC      "PPC32 architecture support"                  OFF "Addresses32" OFF)

cmake_dependent_option(ArchitectureAMD64    "AMD64 architecture support"                  OFF "Addresses64" OFF)

mark_as_advanced(ArchitectureARMTHUMB ArchitecturePPC ArchitectureAMD64)

set(I386SUPPORT ${ArchitectureI386})
set(ARMSUPPORT ${ArchitectureARM})

add_subdirectory(diabloflowgraph)
add_subdirectory(architecture_backends)
add_subdirectory(debug_backends)

set (DIABLOFLOWGRAPH_VERSION_MAJOR 0)
set (DIABLOFLOWGRAPH_VERSION_MINOR 5)

use_global_config(DIABLOFLOWGRAPH
    AMD64SUPPORT HAVE_PECOFF_SUPPORT ARMSUPPORT HAVE_ARM_ADS_SUPPORT HAVE_AR_SUPPORT
    HAVE_BINUTILS_SUPPORT HAVE_ECOFF_SUPPORT HAVE_ELF_SUPPORT HAVE_TICOFF_SUPPORT HAVE_TILINKER_SUPPORT
    HAVE_TRU64LINKER_SUPPORT I386SUPPORT PPC64SUPPORT SPESUPPORT)


configure_diablo_config(flowgraph)
