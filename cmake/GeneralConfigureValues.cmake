include(CheckFunctionExists)
include(CheckIncludeFiles)
include(CheckCCompilerFlag)
include(CheckCSourceCompiles)
include(CheckCSourceRuns)
include(CheckTypeSize)

# These are only the checks that are actually used, as far as I can see ...
# To actually use them, you have to add lines like use_global_config(DIABLOSUPPORT HAVE_GETENV) to the respective subdirectory CMakeLists.txt
# that has the configure_file. I know. Sigh.

check_c_compiler_flag(-fno-aggressive-loop-optimizations HAS_FLAG_AGGRESSIVE_LOOP_OPTIMIZATIONS)
check_c_compiler_flag(-fno-sanitize=address HAS_FLAG_SANITIZE_ADDRESS)
check_c_compiler_flag(-fsanitize=undefined HAS_FLAG_SANITIZE_UNDEFINED)
check_c_compiler_flag(-fstrict-aliasing HAS_FLAG_STRICT_ALIASING)
check_c_compiler_flag(-Wall HAS_WARNING_ALL)
check_c_compiler_flag(-Warray-bounds HAS_WARNING_ARRAY_BOUNDS)
check_c_compiler_flag(-Wsign-compare HAS_WARNING_SIGN_COMPARE)
check_c_compiler_flag(-Wsometimes-uninitialized HAS_WARNING_SOMETIMES_UNINITIALIZED)
check_c_compiler_flag(-Wunused HAS_WARNING_UNUSED)

check_function_exists(mkdir HAVE_MKDIR)
check_function_exists(stat HAVE_STAT)
check_function_exists(vasprintf HAVE_VASPRINTF)

check_include_files(libgen.h HAVE_LIBGEN_H)
check_include_files(sys/stat.h HAVE_SYS_STAT_H)
check_include_files(sys/types.h HAVE_SYS_TYPES_H)
check_include_files(sys/wait.h HAVE_SYS_WAIT_H)
