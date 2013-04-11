noinst_PROGRAMS = testegl

testegl_SOURCES = testegl.c

noinst_HEADERS = cube_texture_and_coords.h

testegl_LDADD = \
	$(GST_PLUGINS_BASE_LIBS) \
  -lgstapp-@GST_MAJORMINOR@ \
	$(GST_BASE_LIBS) \
	$(GST_LIBS) \
  $(top_builddir)/gst/egl/libgstegl-$(GST_MAJORMINOR).la

testegl_CFLAGS = \
	$(GST_PLUGINS_BASE_CFLAGS) \
	$(GST_BASE_CFLAGS) \
	$(GST_CFLAGS)

