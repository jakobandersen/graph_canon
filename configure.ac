AC_PREREQ([2.68])
# http://stackoverflow.com/questions/8559456/read-a-version-number-from-a-file-in-configure-ac
AC_INIT([graph_canon], m4_esyscmd([tr -d '\n' < VERSION]), [jakob@caput.dk])
AC_LANG([C++])
AC_CONFIG_AUX_DIR([auxConfig])
AC_CONFIG_SRCDIR([include/graph_canon/canonicalization.hpp])
AM_INIT_AUTOMAKE([1.11 foreign silent-rules subdir-objects -Wall -Werror tar-ustar nostdinc])
# this AM_PROG_AR call should not be needed, but see http://debbugs.gnu.org/cgi/bugreport.cgi?bug=11401%3E
AM_PROG_AR

AC_PROG_RANLIB

AC_CONFIG_MACRO_DIR([m4])
m4_include([m4/ax_prefix_config_h.m4])
m4_include([m4/ax_cxx_compile_stdcxx_11.m4])

AC_CONFIG_HEADERS([include/graph_canon/Config.h_])
AX_PREFIX_CONFIG_H([include/graph_canon/Config.h], [PERM_GROUP], [include/graph_canon/Config.h_])

##########################################################################
# Constants and Variable Information
##########################################################################
# search build-generated sources, distributed source, and our Boost files
AM_CPPFLAGS="-I\$(top_srcdir)/include"
AM_CXXFLAGS="-O3 -Wno-deprecated-declarations -std=c++14"
# so we can restore before each test
CPPFLAGS=${CPPFLAGS:-""}
CXXFLAGS=${CXXFLAGS:-""}
LDFLAGS=${LDFLAGS:-""}
LDLIBS=${LDLIBS:-""}
LIBS=${LIBS:-""}
orig_CPPFLAGS=$CPPFLAGS
orig_CXXFLAGS="$CXXFLAGS $AM_CXXFLAGS"
orig_LDFLAGS=$LDFLAGS
orig_LDLIBS=$LDLIBS
orig_LIBS=$LIBS


# Individual checks
##########################################################################
# C++14 ------------------------------------------------------------------
# Boost ------------------------------------------------------------------
boostReqVersion="1.64.0"
BOOST_LDLIBS="-lboost_program_options -lboost_random -lboost_chrono -lboost_system"
AC_SUBST([BOOST_LDLIBS])
# perm_group -------------------------------------------------------------
permGroupReqVersion="v0.2"

##########################################################################
# Options for individual dependencies
##########################################################################

Ipaths=()
Lpaths=()
function printPaths {
	if test ${#Ipaths[@]} > 0; then
		AC_MSG_NOTICE([Specified I paths are:])
		for p in ${Ipaths[[@]]}; do
			AC_MSG_NOTICE([   $p])
		done
	fi
	if test ${#Lpaths[@]} > 0; then
		AC_MSG_NOTICE([Specified L paths are:])
		for p in ${Lpaths[[@]]}; do
			AC_MSG_NOTICE([   $p])
		done
	fi
}
doPathSort="no"
function restoreFlags {
	CPPFLAGS=$orig_CPPFLAGS
	CXXFLAGS=$orig_CXXFLAGS
	LDFLAGS=$orig_LDFLAGS
	LDLIBS=$orig_LDLIBS
	LIBS=$orig_LIBS
	if test "$doPathSort" = "yes"; then
		doPathSort="no"
		Ipaths=($(for p in ${Ipaths[[@]]}; do echo $p; done | sort | uniq))
		Lpaths=($(for p in ${Lpaths[[@]]}; do echo $p; done | sort | uniq))
	fi
	for p in ${Ipaths[[@]]}; do
		CPPFLAGS+=" -I$p"
	done
	for p in ${Lpaths[[@]]}; do
		LDFLAGS+=" -L$p -Wl,-rpath -Wl,$p"
	done
}
function addIPath {
	Ipaths+=("$1")
	doPathSort="yes"
}
function addLPath {
	Lpaths+=("$1")
	doPathSort="yes"
	PKG_CONFIG_PATH+=":$1/pkgconfig"
	export PKG_CONFIG_PATH
}

# Boost ------------------------------------------------------------------
AC_ARG_WITH([boost],
	[AS_HELP_STRING([--with-boost=arg],
		[yes: enable Boost, <path>: add specified location for dependency searching, no: not an option, you need Boost.])],
	[], [with_boost=yes])
if test "$with_boost" != "yes" -a "$with_boost" != "no"; then
	addIPath "$with_boost/include"
	addLPath "$with_boost/lib"
fi

# perm_group -------------------------------------------------------------
AC_ARG_WITH([perm_group],
	[AS_HELP_STRING([--with-perm_group=arg],
		[yes: enable PermGroup, <path>: add specified location for dependency searching, no: not an option, you need PermGroup.])],
	[], [with_perm_group=yes])
if test "$with_perm_group" != "yes" -a "$with_perm_group" != "no"; then
	addIPath "$with_perm_group/include"
	addLPath "$with_perm_group/lib"
fi

##########################################################################
# Check Stuff
##########################################################################

# Compiler Stuff ---------------------------------------------------------
AC_PROG_CXXCPP
AC_PROG_CXX
AM_CXXFLAGS="$AM_CXXFLAGS $CXXFLAGS"
CXXFLAGS=""
AC_LANG([C++])

# Boost ------------------------------------------------------------------
restoreFlags
if test "$with_boost" = "no"; then
	AC_MSG_ERROR([You have to want Boost libraries. --with-boost=no is not an option.])
else
	AC_CHECK_HEADER([boost/version.hpp],
		[],
		[AC_MSG_NOTICE([Boost not found.])
		 printPaths
		 if test $with_boost != "yes"; then
			AC_MSG_ERROR([Is the path "$with_boost" correct?])
		 else
			AC_MSG_ERROR([Use --with-boost=<path> to specify where Boost is installed.])
		 fi
	])

	AC_MSG_CHECKING(for Boost >= $boostReqVersion)
	# these checks are based on the ax_boost_base.m4 macros
	expr "$boostReqVersion" : '[[0-9]]\{1,\}\.[[0-9]]\{1,\}\.[[0-9]]\{1,\}$' &> /dev/null
	if test $? -ne 0; then
		AC_MSG_ERROR([Boost version "$boostReqVersion" is invalid. It most be on the form MAJOR.MINOR.PATCH])
	fi
	major=$(expr "$boostReqVersion" : '\([[0-9]]\{1,\}\)\.[[0-9]]\{1,\}\.[[0-9]]\{1,\}$')
	minor=$(expr "$boostReqVersion" : '[[0-9]]\{1,\}\.\([[0-9]]\{1,\}\)\.[[0-9]]\{1,\}$')
	patch=$(expr "$boostReqVersion" : '[[0-9]]\{1,\}\.[[0-9]]\{1,\}\.\([[0-9]]\{1,\}\)$')
	reqVersion=$(expr \( $major \* 1000 \+ $minor \) \* 100 + $patch)
	AC_COMPILE_IFELSE([AC_LANG_SOURCE([[
		#include <boost/version.hpp>
		#if BOOST_VERSION >= $reqVersion
		// yay
		#else
		#error Too old
		#endif
	]])], [versionOk="yes"], [versionOk="no"])
	AC_MSG_RESULT($versionOk)
	if test $versionOk = "no"; then
		AC_MSG_ERROR([Boost version too old.])
	fi
fi

restoreFlags
LIBS+="$BOOST_LDLIBS"
AC_MSG_CHECKING([for needed Boost libraries: $BOOST_LDLIBS])
AC_LINK_IFELSE([
	AC_LANG_SOURCE([[int main() {}]])],
	[AC_MSG_RESULT([yes])],
	[AC_MSG_RESULT([no])
	AC_MSG_ERROR([Could not link test program using Boost libraries.])
	])

# perm_group -------------------------------------------------------------
restoreFlags
if test "$with_perm_group" = "no"; then
	AC_MSG_ERROR([You have to want the PermGroup library. --with-perm_group=no is not an option.])
else
	PKG_CHECK_MODULES([perm_group], [perm-group >= $permGroupReqVersion], [], [
		AC_MSG_NOTICE([$perm_group_PKG_ERRORS])
		AC_MSG_NOTICE([PKG_CONFIG_PATH=$PKG_CONFIG_PATH])
		printPaths
		AC_MSG_ERROR([Use --with-perm_group=prefix, or set PKG_CONFIG_PATH for non-standard install prefix.])])
	addIPath "$(pkg-config perm-group --variable=pkgincludedir)"
fi

# Sphinx -----------------------------------------------------------------
restoreFlags
AC_CHECK_PROGS([SPHINXBUILD], [sphinx-build sphinx-build3 sphinx-build-3 sphinx-build2], [no])
if test "$SPHINXBUILD" = "no"; then
    if test "$enable_doc_checks" = "yes"; then
        AC_MSG_ERROR([sphinx-build is required for building the documentation.
This error can be suppressed with '--disable-doc-checks'])
    else
        AC_MSG_WARN([sphinx-build is required for building the documentation.
This warning will be an error without '--disable-doc-checks'])
    fi
fi
AM_CONDITIONAL([ENABLE_SPHINX], [test "$SPHINXBUILD" != "no"])


##########################################################################
# pkg-config file
##########################################################################
restoreFlags
PKG_INCLUDES=""
for p in ${Ipaths[[@]]}; do
	PKG_INCLUDES+=" -I$p"
done

AC_SUBST([PKG_INCLUDES])
AC_SUBST([PKG_LIBS])

##########################################################################
# Finalize
##########################################################################
restoreFlags
AC_SUBST([AM_CPPFLAGS])
AC_SUBST([AM_CXXFLAGS])
AC_SUBST([AM_LDFLAGS])
AC_SUBST([AM_LDLIBS])

AC_CONFIG_FILES([
	Makefile
	include/Makefile
	doc/Makefile
	test/Makefile
	lib/pkgconfig/graph-canon.pc
])
AC_OUTPUT
