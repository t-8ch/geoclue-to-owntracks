bin_PROGRAMS = geoclue-to-owntracks
noinst_LTLIBRARIES = libowntracks.la

geoclue_to_owntracks_CFLAGS = $(GEOCLUE2_CFLAGS)
geoclue_to_owntracks_LDFLAGS = $(GEOCLUE2_LIBS)
geoclue_to_owntracks_SOURCES = geoclue-to-owntracks.c battery.c
geoclue_to_owntracks_LDADD = libowntracks.la

libowntracks_la_CFLAGS=$(JSON_C_CFLAGS)
libowntracks_la_LDFLAGS=$(JSON_C_LIBS)
libowntracks_la_SOURCES=libowntracks.c
