# initialization
AC_INIT([edje-modules-feedback], [1.0.0], [BUG-REPORT-ADDRESS])
AM_INIT_AUTOMAKE([-Wall foreign])

# Checks for programs.
AC_PROG_CC
AC_DISABLE_STATIC
AC_PROG_LIBTOOL
#AC_PROG_INSTALL

# checks for header files
AC_HEADER_STDC

# checks for typedefs, structures, and compiler characteristics
AC_C_CONST

# checks for pkg-config
PKG_PROG_PKG_CONFIG

requirements="eina, edje, feedback"

PKG_CHECK_MODULES(EDJE_FEEDBACK, [${requirements}])

# Check host system
EDJE_VERSION=`$PKG_CONFIG --modversion edje | awk -F . '{printf("%s.%s",$1,$2);}'`
MODULE_ARCH="v-$EDJE_VERSION"
AC_SUBST(MODULE_ARCH)
AC_DEFINE_UNQUOTED(MODULE_ARCH, "$MODULE_ARCH", "Module architecture")

# Generate files
AC_CONFIG_FILES([Makefile])
AC_OUTPUT
