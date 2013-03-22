lib_LTLIBRARIES = libgstegl-@GST_MAJORMINOR@.la

pcfiles = gstreamer-egl-@GST_MAJORMINOR@.pc

libgstegl_@GST_MAJORMINOR@_la_SOURCES = egl.c

libgstegl_@GST_MAJORMINOR@includedir = \
	$(includedir)/gstreamer-@GST_MAJORMINOR@/gst/egl

libgstegl_@GST_MAJORMINOR@include_HEADERS = egl.h

libgstegl_@GST_MAJORMINOR@_la_CFLAGS = \
	$(GST_CFLAGS) \
	$(EGL_CFLAGS)

libgstegl_@GST_MAJORMINOR@_la_LIBADD = \
	$(GST_LIBS) \
	$(EGL_LIBS)

libgstegl_@GST_MAJORMINOR@_la_LDFLAGS = \
	$(GST_LIB_LDFLAGS) \
	$(GST_ALL_LDFLAGS) \
	$(GST_LT_LDFLAGS)

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = $(pcfiles)

EXTRA_DIST = $(pcfiles)

