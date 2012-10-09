#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT(cmfs-tools, 0.1, i@coly.li)
AC_CONFIG_SRCDIR([include/config.h.in])
AC_CONFIG_HEADERS(include/config.h)
AM_INIT_AUTOMAKE
# Checks for programs.
AC_PROG_CC

# Checks for libraries.

# Checks for header files.

# Checks for typedefs, structures, and compiler characteristics.
AC_C_INLINE
AC_TYPE_SIZE_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T

# Checks for library functions.
AC_FUNC_MALLOC
AC_CHECK_FUNCS([getpagesize memset strchr strdup strerror strstr strtoull uname])

AC_OUTPUT([Makefile
	   libcmfs/Makefile
	   mkfs.cmfs/Makefile
	   ])
