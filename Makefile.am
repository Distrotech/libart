noinst_PROGRAMS = testart testuta

lib_LTLIBRARIES = libart_lgpl.la

libart_lgpl_la_SOURCES = \
	art_bpath.c \
	art_bpath.h \
	art_misc.c \
	art_misc.h \
	art_pathcode.h \
	art_point.h \
	art_rect.c \
	art_rect.h \
	art_rect_uta.c \
	art_rect_uta.h \
	art_uta.c \
	art_uta.h \
	art_uta_rect.c \
	art_uta_rect.h \
	art_uta_vpath.c \
	art_uta_vpath.h \
	art_vpath.c \
	art_vpath.h

libart_lgplincdir = $(includedir)/libart
libart_lgplinc = \
	art_bpath.h \
	art_misc.h \
	art_pathcode.h \
	art_point.h \
	art_rect.h \
	art_rect_uta.h \
	art_uta.h \
	art_uta_rect.h \
	art_uta_vpath.h \
	art_vpath.h

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

