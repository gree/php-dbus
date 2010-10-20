PHP_ARG_ENABLE(dbus, whether to enable dbus support,
[  --enable-dbus              enable dbus support])

if test "$PHP_DBUS" = "yes"; then
	AC_PATH_PROG(PKG_CONFIG, pkg-config, no)
	if test -x "$PKG_CONFIG"; then
		AC_MSG_CHECKING([for dbus installation])
		if $PKG_CONFIG --exists dbus-1; then
			PHP_DBUS_CFLAGS=`$PKG_CONFIG --cflags dbus-1`
			AC_MSG_RESULT([yes])
		else
			AC_MSG_ERROR([dbus not found])
		fi
	else
		AC_MSG_ERROR([pkg-config not found])
	fi

	AC_DEFINE(HAVE_DBUS, 1, [Whether you have dbus or not])
	PHP_ADD_LIBRARY_WITH_PATH(dbus-1, , DBUS_SHARED_LIBADD)
	PHP_NEW_EXTENSION(dbus, dbus.c, $ext_shared,, $PHP_DBUS_CFLAGS)
	PHP_SUBST(DBUS_SHARED_LIBADD)
fi
