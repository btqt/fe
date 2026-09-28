#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.68])
AC_INIT(remotediag, 0.1, hung5.nguyen@lge.com)
AC_CONFIG_SRCDIR([Remotediag.cpp])
AC_CONFIG_HEADERS([config.h])

AC_CONFIG_AUX_DIR([build-aux])
AM_INIT_AUTOMAKE([-Wall gnu foreign])

LT_INIT([dlopen])

# Checks for programs.
AC_PROG_CXX
AC_PROG_AWK
AC_PROG_CC
AC_PROG_CPP
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET
AC_PROG_RANLIB
AC_PROG_LIBTOOL
AC_LIBTOOL_DLOPEN

# Checks for libraries.

# Checks for header files.
AC_CHECK_HEADERS([limits.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_TYPE_INT32_T
AC_TYPE_INT64_T
AC_TYPE_PID_T
AC_TYPE_UINT32_T

# -------------------------------------------------------------- #
# Checks for library functions.
# -------------------------------------------------------------- #
AC_FUNC_FORK
AC_CHECK_FUNCS([select])

AC_CHECK_LIB(dl, dlopen)
AC_CHECK_FUNCS(dlopen)

# -------------------------------------------------------------- #

# Build all components by default
# To add new components:
# add AS_CASE, AC_ARG_WITH, AM_CONDITIONAL, AC_DEFINE, AC_SUBST.
# -------------------------------------------------------------- #
# add the option "--with-arm"
AC_ARG_WITH(arm,
 AC_HELP_STRING([--with-arm],[select _ARM or not]),
 [with_arm=$withval],
 with_arm=no)
# add the option "--with-nad"
AC_ARG_WITH(nad,
 AC_HELP_STRING([--with-nad],[select _DEVICE_NAD or not]),
 [with_nad=$withval],
 with_nad=no)
# add the option "--with-debug"
AC_ARG_WITH(debug,
 AC_HELP_STRING([--with-debug],[select _DEBUG or not]),
 [with_debug=$withval],
 with_debug=no)
# add the option "--with-more-marshm"
AC_ARG_WITH(more-marshm,
	AC_HELP_STRING([--with-more-marshm],[select _MORE_MARSHM or not]),
	[with_more_marshm=$withval],
	with_more_marshm=no)

add the option "--with-project" for can_cb
AC_ARG_WITH(project,
    AC_HELP_STRING([--with-project],[select project or not]),
    [with_project=$withval],
    with_project=no)

AC_ARG_WITH(project-su,
    AC_HELP_STRING([--with-project-su],[select project su (0/1/2)]),
    [with_project_su=$withval],
    [with_project_su=2])

# ENABLE lge_lxc option
# Default: enable_lge_lxc=no (LXC container feature disabled)
AC_ARG_ENABLE(lge_lxc,
    AC_HELP_STRING([--enable-lge_lxc], [enable LXC container feature]),
    [enable_lge_lxc=$enableval],
    [enable_lge_lxc=no])

# -------------------------------------------------------------- #
# FLAGS section
# -------------------------------------------------------------- #
if test "x$with_arm" == "xyes"; then
 AM_CPPFLAGS="$AM_CPPFLAGS -include arch/linux-arm/OEConfig.h"
else
 AM_CPPFLAGS="$AM_CPPFLAGS -include arch/target_linux-x86/OEConfig.h"
fi

if test "x$with_nad" == "xyes"; then
 AM_CPPFLAGS="$AM_CPPFLAGS -D_DEVICE_NAD=1"
else
 AM_CPPFLAGS="$AM_CPPFLAGS -D_DEVICE_AP=1"
fi

if test "x$with_debug" == "xyes"; then
 AM_CPPFLAGS="$AM_CPPFLAGS -D_DEBUG=1"
else
 AM_CPPFLAGS="$AM_CPPFLAGS -O2"
fi
if test "x$with_more_marshm" == "xyes"; then
	AM_CPPFLAGS="$AM_CPPFLAGS -D_MORE_MARSHM=1"
fi

if test "x$enable_lge_lxc" = "xyes"; then
    AM_CPPFLAGS="$AM_CPPFLAGS -DENABLE_LGE_LXC"
fi

if test "x$with_project" == "xcy"; then
    AM_CPPFLAGS="$AM_CPPFLAGS -DCAN_TYPE_CY -DPF_TYPE_CY"
elif test "x$with_project" == "xlc"; then
    AM_CPPFLAGS="$AM_CPPFLAGS -DCAN_TYPE_LC -DPF_TYPE_LC"
elif test "x$with_project" == "xsu"; then
    AM_CPPFLAGS="$AM_CPPFLAGS -DCAN_TYPE_SU -DPF_TYPE_CY"
else
    AM_CPPFLAGS="$AM_CPPFLAGS -DCAN_TYPE_CY -DPF_TYPE_CY"
fi

# -------------------------------------------------------------- #
# CONDITIONAL section
# -------------------------------------------------------------- #
AM_CONDITIONAL(ARM, test "x${with_arm}" = "xyes")
AM_CONDITIONAL(DEVICE_NAD, test "x${with_nad}" = "xyes")
AM_CONDITIONAL(DEVICE_AP, test "x${with_nad}" = "xno")
AM_CONDITIONAL(DEBUG, test "x${with_debug}" = "xyes")
AM_CONDITIONAL(MORE_MARSHM, test "x${with_more_marshm}" = "xyes")
AM_CONDITIONAL(ENABLE_LGE_LXC, test "x${enable_lge_lxc}" = "xyes")

# -------------------------------------------------------------- #
# AC_SUBST section
# -------------------------------------------------------------- #
AC_SUBST(AM_CPPFLAGS)
AC_SUBST(AM_CXXFLAGS)
AC_SUBST(AM_LDFLAGS)

# -------------------------------------------------------------- #
# CONFIG section
# -------------------------------------------------------------- #
AC_CONFIG_FILES([Makefile])
AC_OUTPUT
