AM_CPPFLAGS =						\
	-I.						\
	-I$(srcdir) 					\
	$(SYSMENU_APPLET_CFLAGS)			\
	-DEXTRA_XDG_DATA_DIR=\""$(pkgdatadir)"\"	\
	-DGMENU_I_KNOW_THIS_IS_UNSTABLE

libexec_PROGRAMS = sysmenu_applet

sysmenu_applet_SOURCES =	\
        sysmenu.c		\
        menu.c			\
	launch.c		\
        util.c			\
        sysmenu.h		\
        menu.h			\
	launch.h		\
        util.h


sysmenu_applet_LDADD = 		\
	$(SYSMENU_APPLET_LIBS)

appletdir       = $(datadir)/gnome-panel/4.0/applets
applet_in_files = org.gnome.panel.applet.SysMenuApplet.panel-applet.in
applet_DATA     = $(applet_in_files:.panel-applet.in=.panel-applet)

$(applet_in_files): $(applet_in_files).in Makefile
	$(AM_V_GEN)sed \
            -e "s|\@LIBEXECDIR\@|$(libexecdir)|" \
            -e "s|\@VERSION\@|$(PACKAGE_VERSION)|" \
            $< > $@

%.panel-applet: %.panel-applet.in $(INTLTOOL_MERGE) $(wildcard $(top_srcdir)/po/*po) ; $(INTLTOOL_MERGE) $(top_srcdir)/po $< $@ -d -u -c $(top_builddir)/po/.intltool-merge-cache

servicedir       = $(datadir)/dbus-1/services
service_in_files = org.gnome.panel.applet.SysMenuAppletFactory.service.in
service_DATA     = $(service_in_files:.service.in=.service)

org.gnome.panel.applet.SysMenuAppletFactory.service: $(service_in_files)
	$(AM_V_GEN)sed \
            -e "s|\@LIBEXECDIR\@|$(libexecdir)|" \
            $< > $@

menusdir     = $(pkgdatadir)/menus
menus_DATA   = gnome-preferences.menu gnome-settings.menu

deskdirdir   = $(pkgdatadir)/desktop-directories
deskdir_DATA = Preferences.directory System-Settings.directory


CLEANFILES = $(applet_DATA) $(applet_DATA).in $(service_DATA)

EXTRA_DIST = \
	     org.gnome.panel.applet.SysMenuApplet.panel-applet.in.in	\
             $(service_in_files) $(menus_DATA) $(deskdir_DATA)

