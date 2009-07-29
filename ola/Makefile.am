
INCLUDES = -I$(top_builddir) -I$(top_srcdir) -I$(top_builddir)/include

HEADER_FILES = OlaClient.h OlaDevice.h SimpleClient.h common.h

if HAVE_PTHREAD
THREAD_LIBS = -lpthread
endif

lib_LTLIBRARIES = libola.la
libola_la_SOURCES = OlaClient.cpp \
                    OlaClientServiceImpl.cpp OlaClientCore.cpp \
                    SimpleClient.cpp
libola_la_LDFLAGS = -version-info 1:1:0 $(THREAD_LIBS)
libola_la_LIBADD = $(top_builddir)/common/libolacommon.la
libola_la_CXXFLAGS = -Wall -Wformat

pkgincludedir = $(includedir)/ola
pkginclude_HEADERS = $(HEADER_FILES)

EXTRA_DIST = $(HEADER_FILES) OlaClientServiceImpl.h OlaClientCore.h \
             common-h.in


## @end 1
## @start 2
# Regenerate common.h with config.status whenever common-h.in changes.
common.h: stamp-common
	@:
stamp-common: $(srcdir)/common-h.in $(top_builddir)/config.status
	cd $(top_builddir) \
	  && CONFIG_FILES= CONFIG_HEADERS= CONFIG_OTHER=sic/common.h \
	  $(SHELL) ./config.status
	echo timestamp > $@
## @end 2

# Don't distribute common.h, since it is build host dependent!
dist-hook:
	rm -f $(distdir)/common.h