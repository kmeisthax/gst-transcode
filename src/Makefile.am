# Note: plugindir is set in configure

plugin_LTLIBRARIES = libgsttranscode-0.10.la

# sources used to compile this plug-in
libgsttranscode_0_10_la_SOURCES = plugin_defs.c gsttranscodebin.c gsttranscodebin.h

# compiler and linker flags used to compile this plugin, set in configure.ac
libgsttranscode_0_10_la_CFLAGS = $(GST_CFLAGS)
libgsttranscode_0_10_la_LIBADD = $(GST_LIBS)
libgsttranscode_0_10_la_LDFLAGS = $(GST_PLUGIN_LDFLAGS)
libgsttranscode_0_10_la_LIBTOOLFLAGS = --tag=disable-static

noinst_HEADERS = gsttranscodebin.h
