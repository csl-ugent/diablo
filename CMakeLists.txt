cmake_minimum_required(VERSION 2.8.12)
cmake_policy(VERSION 2.8.12)
project(diablo)

# Versioning
set(DIABLO_MAJOR_VERSION 0)
set(DIABLO_MINOR_VERSION 9)
set(DEB_PACKAGE_VERSION 1)

include(CMakeDependentOption)
include(cmake/VersionFromVCS.cmake)
include(cmake/functions_macros.cmake)
include(cmake/GeneralConfigureValues.cmake)# These need to be included/generated *before* we use their values

find_package(BISON)
find_package(FLEX)

# set up include-directories
include_directories(
  "${PROJECT_SOURCE_DIR}"   # to find foo/foo.h
  "${PROJECT_BINARY_DIR}")  # to find foo/config.h

set(diablo_configs_dir ${PROJECT_BINARY_DIR}/configs)
file(MAKE_DIRECTORY ${diablo_configs_dir})
include_directories(${diablo_configs_dir})

# Set the output directory for executables
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})

# Allow Diablo binaries to find the shared libraries also built from the source, when installed somewhere else
SET(CMAKE_SKIP_RPATH TRUE)
APPEND_CMAKE_INSTALL_RPATH("../lib32")
APPEND_CMAKE_INSTALL_RPATH("../lib")
APPEND_CMAKE_INSTALL_RPATH("judy")
APPEND_CMAKE_INSTALL_RPATH("../judy")

# Generate a compilation database
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# We require C++0x and gnu99
set(CMAKE_CXX_FLAGS "-std=c++0x ${CMAKE_CXX_FLAGS}")
set(CMAKE_C_FLAGS "-std=gnu99 ${CMAKE_C_FLAGS}")

# Some common C/C++ options
if (HAS_FLAG_STRICT_ALIASING)
  set(COMMON_FLAGS "-fno-strict-aliasing ${COMMON_FLAGS}")
endif()

# Optional option. If adding ASAN is supported by the compiler, add it as a CMake option
if (HAS_FLAG_SANITIZE_ADDRESS)
  option(AddressSanitizer "Compile with Address Sanitizer enabled." OFF)
endif()
if(AddressSanitizer)
  set(COMMON_FLAGS "-fsanitize=address ${COMMON_FLAGS}")
endif()

# Optional option. If adding UBSAN is supported by the compiler, add it as a CMake option
if (HAS_FLAG_SANITIZE_UNDEFINED)
  option(UndefinedSanitizer "Compile with Undefined Behavior Sanitizer enabled." OFF)
endif()
if(UndefinedSanitizer)
  set(COMMON_FLAGS "-fsanitize=undefined ${COMMON_FLAGS}")
endif()

# If possible, enable -Wall with -Wno-unused (generates too much warnings)
if (HAS_WARNING_ALL)
  if (HAS_WARNING_UNUSED)
    set(COMMON_FLAGS "-Wall -Wno-unused ${COMMON_FLAGS}")
  else (HAS_WARNING_UNUSED)
    set(COMMON_FLAGS "-Wall ${COMMON_FLAGS}")
  endif()
endif()

# Enable the -Wsign-compare warning for C++ code (not for C though, that's a lost cause)
if (HAS_WARNING_SIGN_COMPARE)
  set(CMAKE_CXX_FLAGS "-Wsign-compare ${CMAKE_CXX_FLAGS}")
endif()

# Add these common options to the C/C++ flags
set(CMAKE_CXX_FLAGS "${COMMON_FLAGS} ${CMAKE_CXX_FLAGS}")
set(CMAKE_C_FLAGS "${COMMON_FLAGS} ${CMAKE_C_FLAGS}")

add_version_info_from_vcs(DIABLO_SVN_REVISION)

# Add sub-directories
add_subdirectory(support)
add_subdirectory(object)
add_subdirectory(flowgraph)
add_subdirectory(anopt)
add_subdirectory(judy)
add_subdirectory(frontends)
add_subdirectory(obfuscation)
add_subdirectory(aspire)
add_subdirectory(jansson-2.6)
add_subdirectory(annotations)

# The interesting stuff goes here
# ===============================

# Offer the user the choice of overriding the installation directories
set(INSTALL_LIB_DIR lib CACHE PATH "Installation directory for libraries")
set(INSTALL_BIN_DIR bin CACHE PATH "Installation directory for executables")
set(INSTALL_INCLUDE_DIR include CACHE PATH "Installation directory for header files")
set(INSTALL_CMAKE_DIR ${DEF_INSTALL_CMAKE_DIR} CACHE PATH "Installation directory for CMake files")

# Export the package for use from the build-tree
# (this registers the build-tree with a global CMake-registry)
export(PACKAGE diablo)
include(Compaction.cmake)

# Make install
# ============
INSTALL(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/diablo-arm DESTINATION bin)
INSTALL(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/diablo-i386 DESTINATION bin)
INSTALL(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/diablo-extractor DESTINATION bin)
INSTALL(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/diablo-obfuscator DESTINATION bin)
INSTALL(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/diablo-selfprofiling DESTINATION bin)

INSTALL(PROGRAMS ${CMAKE_CURRENT_SOURCE_DIR}/self-profiling/binary_profile_to_plaintext.py DESTINATION bin)
INSTALL(PROGRAMS ${CMAKE_CURRENT_SOURCE_DIR}/self-profiling/reverse-translate.py DESTINATION bin/scripts/profiles)
INSTALL(PROGRAMS ${CMAKE_CURRENT_SOURCE_DIR}/self-profiling/listfile.py DESTINATION bin/scripts/profiles)
INSTALL(PROGRAMS ${CMAKE_CURRENT_SOURCE_DIR}/self-profiling/binutils.py DESTINATION bin/scripts/profiles)
INSTALL(PROGRAMS ${CMAKE_CURRENT_SOURCE_DIR}/self-profiling/profile.py DESTINATION bin/scripts/profiles)

INSTALL(PROGRAMS ${CMAKE_CURRENT_SOURCE_DIR}/self-profiling/binary_profile_to_plaintext.py DESTINATION bin)
INSTALL(PROGRAMS ${CMAKE_CURRENT_SOURCE_DIR}/self-profiling/binary_profile_to_plaintext.py DESTINATION bin/scripts/profiles)
INSTALL(PROGRAMS ${CMAKE_CURRENT_SOURCE_DIR}/self-profiling/reverse-translate.py DESTINATION bin/scripts/profiles)
INSTALL(PROGRAMS ${CMAKE_CURRENT_SOURCE_DIR}/self-profiling/listfile.py DESTINATION bin/scripts/profiles)
INSTALL(PROGRAMS ${CMAKE_CURRENT_SOURCE_DIR}/self-profiling/binutils.py DESTINATION bin/scripts/profiles)
INSTALL(PROGRAMS ${CMAKE_CURRENT_SOURCE_DIR}/self-profiling/profile.py DESTINATION bin/scripts/profiles)

# uninstall target
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/cmake_uninstall.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/cmake/cmake_uninstall.cmake"
    IMMEDIATE @ONLY)

add_custom_target(uninstall
    COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake/cmake_uninstall.cmake)

# Get SVN revision number
# =======================

set(PACKAGE_VERSION "${DIABLO_MAJOR_VERSION}.${DIABLO_MINOR_VERSION}+${DIABLO_SVN_REVISION}")

if(DEB_PACKAGE_VERSION)
    set(PACKAGE_VERSION "${PACKAGE_VERSION}-${DEB_PACKAGE_VERSION}")
endif()

# Packing Diablo in a Debian package
# ===================================
SET (CPACK_SET_DESTDIR "on")
SET (CPACK_PACKAGING_INSTALL_PREFIX "/usr/local/bin")

SET (CPACK_GENERATOR "DEB")
SET (CPACK_DEBIAN_PACKAGE_MAINTAINER "Diablo Team <diablo-team@elis.ugent.be>")

set (CPACK_PACKAGE_VERSION_MAJOR ${DIABLO_MAJOR_VERSION})
set (CPACK_PACKAGE_VERSION_MINOR ${DIABLO_MINOR_VERSION})
set (CPACK_PACKAGE_VERSION ${PACKAGE_VERSION})

SET (CPACK_DEBIAN_ARCHITECTURE $ {CMAKE_SYSTEM_PROCESSOR})
SET (CPACK_DEBIAN_PACKAGE_DEPENDS "libc6, libgcc1, flex, bison")

SET (CPACK_PACKAGE_DESCRIPTION "Diablo is a retargetable link-time binary rewriting framework.")
SET (CPACK_DEBIAN_PACKAGE_HOMEPAGE "http://diablo.elis.ugent.be")

# Generate package name ourselfs
# ==============================
string(TOLOWER "${CMAKE_PROJECT_NAME}" CPACK_PACKAGE_NAME_LOWERCASE)
find_program(DPKG_PROGRAM dpkg DOC "dpkg program of Debian-based systems")
if(DPKG_PROGRAM)
    execute_process(
        COMMAND ${DPKG_PROGRAM} --print-architecture
        OUTPUT_VARIABLE CPACK_DEBIAN_PACKAGE_ARCHITECTURE
        OUTPUT_STRIP_TRAILING_WHITESPACE
        )
    set(CPACK_PACKAGE_FILE_NAME
        "${CPACK_PACKAGE_NAME_LOWERCASE}_${PACKAGE_VERSION}_${CPACK_DEBIAN_PACKAGE_ARCHITECTURE}")   

else(DPKG_PROGRAM)
    set(CPACK_PACKAGE_FILE_NAME
        "${CPACK_PACKAGE_NAME_LOWERCASE}_${PACKAGE_VERSION}_${CMAKE_SYSTEM_NAME}")
endif(DPKG_PROGRAM)
INCLUDE(CPack)
