noinst_PROGRAMS = testart testuta

bin_SCRIPTS = \
	libart-config

EXTRA_DIST = \
	libart.m4

lib_LTLIBRARIES = libart_lgpl.la

libart_lgpl_la_SOURCES = \
	art_bpath.c \
	art_gray_svp.c \
	art_misc.c \
	art_rect.c \
	art_rect_uta.c \
	art_rgb.c \
	art_rgb_svp.c \
	art_svp.c \
	art_svp_render_aa.c \
	art_svp_vpath.c \
	art_svp_wind.c \
	art_svp_wind.h \
	art_uta.c \
	art_uta_ops.c \
	art_uta_rect.c \
	art_uta_vpath.c \
	art_vpath.c \
	art_vpath_bpath.c

libart_lgpl_la_LDFLAGS = -version-info @LIBART_VERSION_INFO@

libart_lgplincdir = $(includedir)/libart_lgpl
libart_lgplinc_HEADERS = \
	art_bpath.h \
	art_gray_svp.h \
	art_misc.h \
	art_pathcode.h \
	art_point.h \
	art_rect.h \
	art_rect_uta.h \
	art_rgb.h \
	art_rgb_svp.h \
	art_svp.h \
	art_svp_render_aa.h \
	art_svp_vpath.h \
	art_uta.h \
	art_uta_ops.h \
	art_uta_rect.h \
	art_uta_vpath.h \
	art_vpath.h \
	art_vpath_bpath.h

INCLUDES = -I$(top_srcdir)/..

DEPS = $(top_builddir)/libart_lgpl.la
LDADDS = $(top_builddir)/libart_lgpl.la

testart_SOURCES=testart.c
testart_LDFLAGS =
testart_DEPENDENCIES = $(DEPS)
testart_LDADD = $(LDADDS) -lm

testuta_SOURCES=testuta.c
testuta_LDFLAGS =
testuta_DEPENDENCIES = $(DEPS)
testuta_LDADD = $(LDADDS) -lm

tests:	testart testuta

m4datadir = $(datadir)/aclocal
m4data_DATA = libart.m4

## Put `exec' in the name because this should be installed by
## `install-exec', not `install-data'.

confexecdir=$(libdir)
confexec_DATA = libartConf.sh

EXTRA_DIST = libartConf.sh.in

## We create libartConf.sh here and not from configure because we want
## to get the paths expanded correctly.  Macros like srcdir are given
## the value NONE in configure if the user doesn't specify them (this
## is an autoconf feature, not a bug).

libartConf.sh: libartConf.sh.in Makefile
## Use sed and then mv to avoid problems if the user interrupts.
	sed -e 's?\@LIBART_LIBDIR\@?$(LIBART_LIBDIR)?g' \
	    -e 's?\@LIBART_INCLUDEDIR\@?$(LIBART_INCLUDEDIR)?g' \
	    -e 's?\@LIBART_LIBS\@?$(LIBART_LIBS)?g' \
	      < $(srcdir)/libartConf.sh.in > libartConf.tmp \
	  && mv libartConf.tmp libartConf.sh
