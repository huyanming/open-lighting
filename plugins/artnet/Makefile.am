
libdir = $(plugindir)

if HAVE_ARTNET

lib_LTLIBRARIES = libllaartnet.la


INCLUDES = -I$(top_builddir) -I$(top_srcdir)
AM_CFLAGS = -Wall -Werror -Wformat -W


libllaartnet_la_SOURCES = artnetplugin.cpp artnetplugin.h artnetdevice.cpp\
						 artnetdevice.h	artnetport.cpp artnetport.h
libllaartnet_la_LIBADD = @LIBS@  -lartnet

endif