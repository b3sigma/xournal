## Process this file with automake to produce Makefile.in

if WIN32
  SUBDIRS = ttsubset win32
else
  SUBDIRS = ttsubset
endif

INCLUDES = \
	-DPACKAGE_DATA_DIR=\""$(datadir)"\" \
	-DPACKAGE_LOCALE_DIR=\""$(prefix)/$(DATADIRNAME)/locale"\" \
	@PACKAGE_CFLAGS@

bin_PROGRAMS = xournal

xournal_SOURCES = \
	main.c xournal.h \
	xo-misc.c xo-misc.h \
	xo-file.c xo-file.h \
	xo-paint.c xo-paint.h \
	xo-selection.c xo-selection.h \
	xo-clipboard.c xo-clipboard.h \
	xo-image.c xo-image.h \
	xo-print.c xo-print.h \
	xo-support.c xo-support.h \
	xo-interface.c xo-interface.h \
	xo-callbacks.c xo-callbacks.h \
	xo-shapes.c xo-shapes.h \
	xo-scribble.c xo-scribble.h

if WIN32
#  xournal_LDFLAGS = -mwindows
#  xournal_LDADD = win32/xournal.res ttsubset/libttsubset.a @PACKAGE_LIBS@ $(INTLLIBS) -lz
else
## EXTRA_LIB_DIRS = -L../libscribble/build/ @EXTRA_LIB_DIRS@
## EXTRA_LIBS = -lz -lm  -lscribble -lpthread -lharfbuzz @EXTRA_LIBS@

	if LINUX
##		xournal_LDADD = ttsubset/libttsubset.a @PACKAGE_LIBS@ $(INTLLIBS) -lX11 -lz -lm
##		xournal_LDADD = ttsubset/libttsubset.a @PACKAGE_LIBS@ $(INTLLIBS) -lX11 $(EXTRA_LIB_DIRS) $(EXTRA_LIBS)
	else
##		xournal_LDADD = ttsubset/libttsubset.a @PACKAGE_LIBS@ $(INTLLIBS) -lz -lm
##		xournal_LDADD = ttsubset/libttsubset.a @PACKAGE_LIBS@ $(INTLLIBS) $(EXTRA_LIB_DIRS) $(EXTRA_LIBS)
	endif
endif

