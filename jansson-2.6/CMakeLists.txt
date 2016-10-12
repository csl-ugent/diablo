SET (JanssonSrcDir ${CMAKE_CURRENT_SOURCE_DIR})

# Author: Paul Harris, June 2012
# Additions: Joakim Soderberg, Febuary 2013

SET(USE_URANDOM "Use /dev/urandom to seed the hash function." ON)

# Set some nicer output dirs.
SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)
SET(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)
SET(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)

# Give the debug version a different postfix for windows,
# so both the debug and release version can be built in the
# same build-tree on Windows (MSVC).
if (WIN32)
   SET (CMAKE_DEBUG_POSTFIX "_d")
else (WIN32)
endif (WIN32)

# This is how I thought it should go
# set (JANSSON_VERSION "2.3.1")
# set (JANSSON_SOVERSION 2)

set(JANSSON_DISPLAY_VERSION "2.6")

# This is what is required to match the same numbers as automake's
set (JANSSON_VERSION "4.6.0")
set (JANSSON_SOVERSION 4)

# for CheckFunctionKeywords
set(CMAKE_MODULE_PATH "${JanssonSrcDir}/cmake")

INCLUDE (CheckCSourceCompiles)
include (CheckFunctionExists)
include (CheckFunctionKeywords)
include (CheckIncludeFiles)
include (CheckTypeSize)

if (MSVC)
   # Turn off Microsofts "security" warnings.
   add_definitions( "/W3 /D_CRT_SECURE_NO_WARNINGS /wd4005 /wd4996 /nologo" )
   
   if (STATIC_CRT)
      set(CMAKE_C_FLAGS_RELEASE "/MT")
      set(CMAKE_C_FLAGS_DEBUG "/MTd")
   endif()
   
endif()

if (CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX)
   set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC")
endif()

# babrath: disable this warning as it occurs in the jansson src code
if (HAS_WARNING_SOMETIMES_UNINITIALIZED)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-sometimes-uninitialized")
endif()

check_include_files (endian.h HAVE_ENDIAN_H)
check_include_files (fcntl.h HAVE_FCNTL_H)
check_include_files (sched.h HAVE_SCHED_H)
check_include_files (unistd.h HAVE_UNISTD_H)
check_include_files (sys/param.h HAVE_SYS_PARAM_H)
check_include_files (sys/stat.h HAVE_SYS_STAT_H)
check_include_files (sys/time.h HAVE_SYS_TIME_H)
check_include_files (sys/time.h HAVE_SYS_TYPES_H)

check_function_exists (close HAVE_CLOSE)
check_function_exists (getpid HAVE_GETPID)
check_function_exists (gettimeofday HAVE_GETTIMEOFDAY)
check_function_exists (open HAVE_OPEN)
check_function_exists (read HAVE_READ)
check_function_exists (sched_yield HAVE_SCHED_YIELD)

# Check for the int-type includes
check_include_files (stdint.h HAVE_STDINT_H)

# Check our 64 bit integer sizes
check_type_size (__int64 __INT64)
check_type_size (int64_t INT64_T)
check_type_size ("long long" LONG_LONG_INT)

# Check our 32 bit integer sizes
check_type_size (int32_t INT32_T)
check_type_size (__int32 __INT32)
check_type_size ("long" LONG_INT)
check_type_size ("int" INT)
if (HAVE_INT32_T)
   set (JSON_INT32 int32_t)
elseif (HAVE___INT32)
   set (JSON_INT32 __int32)
elseif (HAVE_LONG_INT AND (${LONG_INT} EQUAL 4))
   set (JSON_INT32 long)
elseif (HAVE_INT AND (${INT} EQUAL 4))
   set (JSON_INT32 int)
else ()
   message (FATAL_ERROR "Could not detect a valid 32-bit integer type")
endif ()

check_type_size (uint32_t UINT32_T)
check_type_size (__uint32 __UINT32)
check_type_size ("unsigned long" UNSIGNED_LONG_INT)
check_type_size ("unsigned int" UNSIGNED_INT)
if (HAVE_UINT32_T)
   set (JSON_UINT32 uint32_t)
elseif (HAVE___UINT32)
   set (JSON_UINT32 __uint32)
elseif (HAVE_UNSIGNED_LONG_INT AND (${UNSIGNED_LONG_INT} EQUAL 4))
   set (JSON_UINT32 "unsigned long")
elseif (HAVE_UNSIGNED_INT AND (${UNSIGNED_INT} EQUAL 4))
   set (JSON_UINT32 "unsigned int")
else ()
   message (FATAL_ERROR "Could not detect a valid unsigned 32-bit integer type")
endif ()

# Check for ssize_t and SSIZE_T existance.
check_type_size(ssize_t SSIZE_T)
check_type_size(SSIZE_T UPPERCASE_SSIZE_T)
if(NOT HAVE_SSIZE_T)
   if(HAVE_UPPERCASE_SSIZE_T)
      set(JSON_SSIZE SSIZE_T)
   else()
      set(JSON_SSIZE int)
   endif()
endif()
set(CMAKE_EXTRA_INCLUDE_FILES "")

# Check for all the variants of strtoll
check_function_exists (strtoll HAVE_STRTOLL)
check_function_exists (strtoq HAVE_STRTOQ)
check_function_exists (_strtoi64 HAVE__STRTOI64)

# Figure out what variant we should use
if (HAVE_STRTOLL)
   set (JSON_STRTOINT strtoll)
elseif (HAVE_STRTOQ)
   set (JSON_STRTOINT strtoq)
elseif (HAVE__STRTOI64)
   set (JSON_STRTOINT _strtoi64)
else ()
   # fallback to strtol (32 bit)
   # this will set all the required variables
   set (JSON_STRTOINT strtol)
   set (JSON_INT_T long)
   set (JSON_INTEGER_FORMAT "\"ld\"")
endif ()

# if we haven't defined JSON_INT_T, then we have a 64 bit conversion function.
# detect what to use for the 64 bit type.
# Note: I will prefer long long if I can get it, as that is what the automake system aimed for.
if (NOT DEFINED JSON_INT_T)
   if (HAVE_LONG_LONG_INT AND (${LONG_LONG_INT} EQUAL 8))
      set (JSON_INT_T "long long")
   elseif (HAVE_INT64_T)
      set (JSON_INT_T int64_t)
   elseif (HAVE___INT64)
      set (JSON_INT_T __int64)
   else ()
      message (FATAL_ERROR "Could not detect 64 bit type, although I detected the strtoll equivalent")
   endif ()

   # Apparently, Borland BCC and MSVC wants I64d,
   # Borland BCC could also accept LD
   # and gcc wants ldd,
   # I am not sure what cygwin will want, so I will assume I64d

   if (WIN32) # matches both msvc and cygwin
      set (JSON_INTEGER_FORMAT "\"I64d\"")
   else ()
      set (JSON_INTEGER_FORMAT "\"lld\"")
   endif ()
endif ()


# If locale.h and localeconv() are available, define to 1, otherwise to 0.
check_include_files (locale.h HAVE_LOCALE_H)
check_function_exists (localeconv HAVE_LOCALECONV)

if (HAVE_LOCALECONV AND HAVE_LOCALE_H)
   set (JSON_HAVE_LOCALECONV 1)
else ()
   set (JSON_HAVE_LOCALECONV 0)
endif ()

# check if we have setlocale
check_function_exists (setlocale HAVE_SETLOCALE)

# Check what the inline keyword is.
# Note that the original JSON_INLINE was always set to just 'inline', so this goes further.
check_function_keywords("inline")
check_function_keywords("__inline")
check_function_keywords("__inline__")

if (HAVE_INLINE)
   set (JSON_INLINE inline)
elseif (HAVE___INLINE)
   set (JSON_INLINE __inline)
elseif (HAVE___INLINE__)
   set (JSON_INLINE __inline__)
else (HAVE_INLINE)
   # no inline on this platform
   set (JSON_INLINE)
endif (HAVE_INLINE)

# Find our snprintf
check_function_exists (snprintf HAVE_SNPRINTF)
check_function_exists (_snprintf HAVE__SNPRINTF)

if (HAVE_SNPRINTF)
   set (JSON_SNPRINTF snprintf)
elseif (HAVE__SNPRINTF)
   set (JSON_SNPRINTF _snprintf)
endif ()

if (AddressSanitizer)
   add_definitions(-DVALGRIND)
endif ()

check_c_source_compiles ("int main() { unsigned long val; __sync_bool_compare_and_swap(&val, 0, 1); return 0; } " HAVE_SYNC_BUILTINS)
check_c_source_compiles ("int main() { char l; unsigned long v; __atomic_test_and_set(&l, __ATOMIC_RELAXED); __atomic_store_n(&v, 1, __ATOMIC_ACQ_REL); __atomic_load_n(&v, __ATOMIC_ACQUIRE); return 0; }" HAVE_ATOMIC_BUILTINS)

# Create pkg-conf file.
# (We use the same files as ./configure does, so we
#  have to defined the same variables used there).
if(NOT DEFINED CMAKE_INSTALL_LIBDIR)
  set(CMAKE_INSTALL_LIBDIR lib)
endif(NOT DEFINED CMAKE_INSTALL_LIBDIR)
set(prefix      ${CMAKE_INSTALL_PREFIX})
set(exec_prefix ${CMAKE_INSTALL_PREFIX})
set(libdir      ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR})
set(VERSION     ${JANSSON_DISPLAY_VERSION})
configure_file(${JanssonSrcDir}/jansson.pc.in
               ${CMAKE_CURRENT_BINARY_DIR}/jansson.pc @ONLY)

# configure the public config file
configure_file (${JanssonSrcDir}/cmake/jansson_config.h.cmake
                ${CMAKE_CURRENT_BINARY_DIR}/include/jansson_config.h)

# Copy the jansson.h file to the public include folder
file (COPY ${JanssonSrcDir}/src/jansson.h
           DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/include/)

# configure the private config file
configure_file (${JanssonSrcDir}/cmake/config.h.cmake
                ${CMAKE_CURRENT_BINARY_DIR}/private_include/jansson_private_config.h)

# and tell the source code to include it
add_definitions (-DHAVE_CONFIG_H)

include_directories (${CMAKE_CURRENT_BINARY_DIR}/include)
include_directories (${CMAKE_CURRENT_BINARY_DIR}/private_include)

# Add the lib sources.
file (GLOB C_FILES ${JanssonSrcDir}/src/*.c)

add_library (jansson ${C_FILES})
target_include_directories(jansson
  PRIVATE ${CMAKE_CURRENT_BINARY_DIR}
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src
)

