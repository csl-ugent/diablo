# Set to local use from cmake/GeneralConfigureValues.cmake Sigh.
# These can then be used by both subdirectories
use_global_config(DIABLOSUPPORT
  HAVE_LIBGEN_H HAVE_MKDIR HAVE_STAT HAVE_SYS_STAT_H HAVE_SYS_TYPES_H HAVE_SYS_WAIT_H
  HAVE_VASPRINTF BUILD_SHARED_LIBS)

set (DIABLOSUPPORT_VERSION_MAJOR 0)
set (DIABLOSUPPORT_VERSION_MINOR 5)

option(DIABLOSUPPORT_MALLOC_DEBUG "Debug Diablo Malloc" ON)
option(DIABLOSUPPORT_LINUX_TIMING "Enable timed phases" ON)

add_subdirectory(opt_gen)
add_subdirectory(diablosupport)

configure_diablo_config(support)

# Generate version header
configure_file(
  version.h.in
  version.h
)
