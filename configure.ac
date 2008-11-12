
AC_INIT(vamp-plugin-sdk, 2.0, cannam@all-day-breakfast.com)

AC_CONFIG_SRCDIR(vamp/vamp.h)
AC_PROG_CXX
AC_HEADER_STDC
AC_C_BIGENDIAN

if pkg-config --modversion vamp-sdk >/dev/null 2>&1; then
  echo "WARNING: A version of the Vamp plugin SDK is already installed."
  echo "         Expect worries and sorrows if you install a new version"
  echo "         without removing the old one first.  (Continuing)"
fi

AC_SEARCH_LIBS([dlopen],[dl])

PKG_CHECK_MODULES([SNDFILE],[sndfile])
AC_SUBST(SNDFILE_CFLAGS)
AC_SUBST(SNDFILE_LIBS)

changequote(,)dnl
if test "x$GCC" = "xyes"; then
  case " $CXXFLAGS " in
    *[\ \	]-fPIC\ -Wall[\ \	]*) ;;
    *) CXXFLAGS="$CXXFLAGS -fPIC -Wall" ;;
  esac
fi
changequote([,])dnl

AC_OUTPUT([Makefile])

