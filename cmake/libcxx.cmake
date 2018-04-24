# https://github.com/meta-toolkit/meta-cmake/blob/master/FindLIBCXX.cmake

# Attempts to find libc++ and an appropriate ABI (libc++abi or libcxxrt)
# when using clang and libc++ together.

#message("-- Locating libc++...")
find_library(LIBCXX_LIBRARY NAMES c++ cxx)
if(LIBCXX_LIBRARY)
  #message("-- Located libc++: ${LIBCXX_LIBRARY}")
  option(USE_CLANG_LIBCXX "Use LLVM/clang libc++" OFF)
  set(LIBCXX_OPTIONS "-stdlib=libc++")
  set(LIBCXX_OPTIONS_GCC "-nostdinc++ -nodefaultlibs -lc++")
  get_filename_component(LIBCXX_LIB_PATH ${LIBCXX_LIBRARY} DIRECTORY)
  find_path(LIBCXX_PREFIX c++/v1/algorithm
    PATHS ${LIBCXX_LIB_PATH}/../include
    ${CMAKE_SYSTEM_PREFIX_PATH})
  if (LIBCXX_PREFIX)
    set(LIBCXX_INCLUDE_DIR ${LIBCXX_PREFIX}/c++/v1/)
    set(LIBCXX_OPTIONS_GCC "${LIBCXX_OPTIONS_GCC} -I${LIBCXX_INCLUDE_DIR}")
    #message("-- Located libc++ include path: ${LIBCXX_INCLUDE_DIR}")
  else()
    #message("-- Failed to find libc++ include path!")
  endif()

  #message("--     Locating libc++'s abi...")
  find_library(LIBCXXABI_LIBRARY NAMES c++abi)
  find_library(LIBCXXRT_LIBRARY NAMES cxxrt)
  if(LIBCXXABI_LIBRARY)
    #message("--     Found libc++abi: ${LIBCXXABI_LIBRARY}")
    set(CXXABI_LIBRARY ${LIBCXXABI_LIBRARY})
  elseif(LIBCXXRT_LIBRARY)
    #message("--     Found libcxxrt: ${LIBCXXRT_LIBRARY}")
    set(CXXABI_LIBRARY ${LIBCXXRT_LIBRARY})
  else()
    #message("--     No abi library found. Attempting to continue without one...")
    set(CXXABI_LIBRARY "")
  endif()
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(LIBCXX DEFAULT_MSG
  LIBCXX_LIBRARY
  LIBCXX_INCLUDE_DIR
  LIBCXX_LIB_PATH
  LIBCXX_OPTIONS
  CXXABI_LIBRARY)

if (USE_CLANG_LIBCXX)
  if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    # GNU gcc
    # TODO: this does not work yet (https://gcc.gnu.org/bugzilla/show_bug.cgi?id=59930)
    #set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${LIBCXX_OPTIONS_GCC}")
    message(FATAL_ERROR "Using libc++ with GCC is not supported yet! Edit cmake/libcxx.cmake to implement this.")
  else()
    # LLVM/clang
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${LIBCXX_OPTIONS}")
  endif()
endif()
