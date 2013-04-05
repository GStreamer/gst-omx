plugin_LTLIBRARIES = libgstopenmax.la

VIDEO_BASE_CLASSES_C = \
	video/video.c \
	video/gstvideodecoder.c \
	video/gstvideoencoder.c \
	video/gstvideoutils.c

VIDEO_BASE_CLASSES_H = \
	video/video.h \
	video/gstvideodecoder.h \
	video/gstvideoencoder.h \
	video/gstvideoutils.h

if HAVE_VP8
VP8_C_FILES=gstomxvp8dec.c
VP8_H_FILES=gstomxvp8dec.h
endif

if HAVE_EGL
GST_EGL_LIBS = $(top_builddir)/gst/egl/libgstegl-$(GST_MAJORMINOR).la
endif

libgstopenmax_la_SOURCES = \
	gstomx.c \
	gstomxvideodec.c \
	gstomxvideoenc.c \
	gstomxaudioenc.c \
	gstomxmjpegdec.c \
	gstomxmpeg4videodec.c \
	gstomxmpeg2videodec.c \
	gstomxh264dec.c \
	gstomxh263dec.c \
	$(VP8_C_FILES) \
	gstomxwmvdec.c \
	gstomxmpeg4videoenc.c \
	gstomxh264enc.c \
	gstomxh263enc.c \
	gstomxaacenc.c

if USE_LOCAL_VIDEO_BASE_CLASSES
libgstopenmax_la_SOURCES += $(VIDEO_BASE_CLASSES_C)
endif

noinst_HEADERS = \
	gstomx.h \
	gstomxvideodec.h \
	gstomxvideoenc.h \
	gstomxaudioenc.h \
	gstomxmjpegdec.h \
	gstomxmpeg2videodec.h \
	gstomxmpeg4videodec.h \
	gstomxh264dec.h \
	gstomxh263dec.h \
	$(VP8_H_FILES) \
	gstomxwmvdec.h \
	gstomxmpeg4videoenc.h \
	gstomxh264enc.h \
	gstomxh263enc.h \
	gstomxaacenc.h
	$(VIDEO_BASE_CLASSES_H)

if !HAVE_EXTERNAL_OMX
OMX_INCLUDEPATH = -I$(abs_srcdir)/openmax
endif

libgstopenmax_la_CFLAGS = \
	-DGST_USE_UNSTABLE_API=1 \
	$(OMX_INCLUDEPATH) \
	$(GST_PLUGINS_BASE_CFLAGS) \
	$(GST_BASE_CFLAGS) \
	$(GST_CFLAGS)

libgstopenmax_la_LIBADD = \
	$(GST_PLUGINS_BASE_LIBS) \
	-lgstaudio-@GST_MAJORMINOR@ \
	-lgstpbutils-@GST_MAJORMINOR@ \
	-lgstvideo-@GST_MAJORMINOR@ \
	$(GST_BASE_LIBS) \
	$(GST_LIBS) \
	$(GST_EGL_LIBS)

libgstopenmax_la_LDFLAGS = $(GST_PLUGIN_LDFLAGS)

EXTRA_DIST = \
  openmax \
  $(VIDEO_BASE_CLASSES_C) \
  $(VIDEO_BASE_CLASSES_H) \
  gstomxvp8dec.c \
  gstomxvp8dec.h


Android.mk: Makefile.am $(BUILT_SOURCES)
	androgenizer \
	-:PROJECT libgstopenmax -:SHARED libgstopenmax \
	 -:TAGS eng debug \
         -:REL_TOP $(top_srcdir) -:ABS_TOP $(abs_top_srcdir) \
	 -:SOURCES $(libgstopenmax_la_SOURCES) \
	           $(nodist_libgstopenmax_la_SOURCES) \
	 -:CFLAGS $(DEFS) $(DEFAULT_INCLUDES) $(libgstopenmax_la_CFLAGS) \
	 -:LDFLAGS $(libgstopenmax_la_LDFLAGS) \
	           $(libgstopenmax_la_LIBADD) \
	           -ldl \
	 -:PASSTHROUGH LOCAL_ARM_MODE:=arm \
		       LOCAL_MODULE_PATH:='$$(TARGET_OUT)/lib/gstreamer-$(GST_API_VERSION)' \
	> $@
