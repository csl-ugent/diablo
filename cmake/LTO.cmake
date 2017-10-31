# If CMake AND the compiler support IPO/LTO:
# create a CMake option to optionally enable it.
if (POLICY CMP0069)
  include(CheckIPOSupported)
  check_ipo_supported(RESULT result)
  if (result)
    option(LTO "Build with LTO enabled." OFF)
  endif()
  if(LTO)
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
  endif()
endif()

