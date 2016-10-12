# Boilerplate. 
#
# This section should include standard boilerplate code, such as the call to
# AC_INIT (which must be first), AM_INIT_AUTOMAKE, AC_CONFIG_HEADER, and
# perhaps AC_REVISION.

AC_INIT(diablodiversity/diablodiversity.h)
AC_CONFIG_AUX_DIR(config)
AM_INIT_AUTOMAKE(diablodiversity,0.1)
AC_LANG(C)
AC_CONFIG_HEADERS(diablodiversity/config.h)
AC_SUBST(ac_aux_dir)

# Options. 
#
# The next section should include macros which add command-line options to
# configure, such as AC_ARG_ENABLE. It is typical to put support code for the
# option in this section as well.

# 32 bit - 64 bit

CFLAGS="$CFLAGS -Werror-implicit-function-declaration"

BIT32="no";
AC_ARG_ENABLE([bit32], AC_HELP_STRING([--disable-bit32], [disable 32 bit address support]), [BIT32=$enableval], [BIT32=yes])

if test "$BIT32" = "yes"; then
   AC_DEFINE([BIT32ADDRSUPPORT], 1, [Define to enable 32 bit address support])
fi
 
BIT64="no";
AC_ARG_ENABLE([bit64], AC_HELP_STRING([--disable-bit64], [disable 64 bit address support]), [BIT64=$enableval], [BIT64=yes])

if test "$BIT64" = "yes"; then
   AC_DEFINE([BIT64ADDRSUPPORT], 1, [Define to enable 64 bit address support])
fi

if test "$BIT32" = "no" -a "$BIT64" = "no"; then
    echo
    echo Cannot disable 32 and 64 bit addresses
    echo 
    exit
fi

if test "$BIT32" = "no"; then
AC_SUBST(ADDREXT, opt64)
AC_SUBST(ADDR_CFLAGS,"-DBIT64ADDRSUPPORT")
elif test "$BIT64" = "no"; then
AC_SUBST(ADDREXT, opt32)
AC_SUBST(ADDR_CFLAGS,"-DBIT32ADDRSUPPORT")
else
AC_SUBST(ADDREXT, generic)
AC_SUBST(ADDR_CFLAGS,"-DBIT32ADDRSUPPORT -DBIT64ADDRSUPPORT")
fi


 
# Programs. 
#
# Next it is traditional to check for programs that are either needed by the
# configure process, the build process, or by one of the programs being built.
# This usually involves calls to macros like AC_CHECK_PROG and AC_PATH_TOOL.

AM_PROG_LIBTOOL
AM_PROG_LEX
AC_PROG_YACC

# Libraries. 
#
# Checks for libraries come before checks for other object visible to C (or
# C++, or anything else). This is necessary because some other checks work by
# trying to link or run a program; by checking for libraries first you ensure
# that the resulting programs can be linked.


PKG_CHECK_MODULES(FLOWGRAPH, diabloflowgraph_$ADDREXT >= 0.1)
AC_SUBST(FLOWGRAPH_CFLAGS)
AC_SUBST(FLOWGRAPH_LIBS)

# Headers. 
#
# Next come checks for existence of headers.


# Typedefs and structures. 
# 
# We do checks for typedefs after checking for headers for the simple reason
# that typedefs appear in headers, and we need to know which headers we can use
# before we look inside them.

# Functions. 
# 
# Finally we check for functions. These come last because functions have
# dependencies on the preceding items: when searching for functions, libraries
# are needed in order to correctly link, headers are needed in order to find
# prototypes (this is especially important for C++, which has stricter
# prototyping rules than C), and typedefs are needed for those functions which
# use or return types which are not built in.

# Output. 
# 
# This is done by invoking AC_OUTPUT

AC_OUTPUT(Makefile diablodiversity/Makefile test/Makefile diablodiversity.spec diablodiversity.pc)


echo \
"---------------------------------------------------------------------
Configuration:

Source code location:       ${srcdir}
Compiler:                   ${CC}
Compiler flags:             ${CFLAGS}
Host System Type:           ${host}
Install path:               ${prefix}
Data dir:             	    ${datadir}

See diablodiversity/diablodiversity_config.h for further configuration 
information.
---------------------------------------------------------------------"
