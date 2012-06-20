/* GStreamer
 * Copyright (C) <1999> Erik Walthinsen <omega@cse.ogi.edu>
 * Library       <2002> Ronald Bultje <rbultje@ronald.bitfreak.net>
 * Copyright (C) 2007 David A. Schleef <ds@schleef.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <string.h>
#include "video.h"

static GstVideoFormat gst_video_format_from_rgb32_masks (int red_mask,
    int green_mask, int blue_mask);
static GstVideoFormat gst_video_format_from_rgba32_masks (int red_mask,
    int green_mask, int blue_mask, int alpha_mask);
static GstVideoFormat gst_video_format_from_rgb24_masks (int red_mask,
    int green_mask, int blue_mask);
static GstVideoFormat gst_video_format_from_rgb16_masks (int red_mask,
    int green_mask, int blue_mask);


static int fill_planes (GstVideoInfo * info);

typedef struct
{
  guint32 fourcc;
  GstVideoFormatInfo info;
} VideoFormat;

/* depths: bits, n_components, shift, depth */
#define DPTH0            0, 0, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }
#define DPTH8            8, 1, { 0, 0, 0, 0 }, { 8, 0, 0, 0 }
#define DPTH888          8, 3, { 0, 0, 0, 0 }, { 8, 8, 8, 0 }
#define DPTH8888         8, 4, { 0, 0, 0, 0 }, { 8, 8, 8, 8 }
#define DPTH10_10_10     10, 3, { 0, 0, 0, 0 }, { 10, 10, 10, 0 }
#define DPTH16           16, 1, { 0, 0, 0, 0 }, { 16, 0, 0, 0 }
#define DPTH16_16_16     16, 3, { 0, 0, 0, 0 }, { 16, 16, 16, 0 }
#define DPTH16_16_16_16  16, 4, { 0, 0, 0, 0 }, { 16, 16, 16, 16 }
#define DPTH555          16, 3, { 10, 5, 0, 0 }, { 5, 5, 5, 0 }
#define DPTH565          16, 3, { 11, 5, 0, 0 }, { 5, 6, 5, 0 }

/* pixel strides */
#define PSTR0             { 0, 0, 0, 0 }
#define PSTR1             { 1, 0, 0, 0 }
#define PSTR111           { 1, 1, 1, 0 }
#define PSTR1111          { 1, 1, 1, 1 }
#define PSTR122           { 1, 2, 2, 0 }
#define PSTR2             { 2, 0, 0, 0 }
#define PSTR222           { 2, 2, 2, 0 }
#define PSTR244           { 2, 4, 4, 0 }
#define PSTR444           { 4, 4, 4, 0 }
#define PSTR4444          { 4, 4, 4, 4 }
#define PSTR333           { 3, 3, 3, 0 }
#define PSTR488           { 4, 8, 8, 0 }
#define PSTR8888          { 8, 8, 8, 8 }

/* planes */
#define PLANE_NA          0, { 0, 0, 0, 0 }
#define PLANE0            1, { 0, 0, 0, 0 }
#define PLANE011          2, { 0, 1, 1, 0 }
#define PLANE012          3, { 0, 1, 2, 0 }
#define PLANE0123         4, { 0, 1, 2, 3 }
#define PLANE021          3, { 0, 2, 1, 0 }

/* offsets */
#define OFFS0             { 0, 0, 0, 0 }
#define OFFS013           { 0, 1, 3, 0 }
#define OFFS102           { 1, 0, 2, 0 }
#define OFFS1230          { 1, 2, 3, 0 }
#define OFFS012           { 0, 1, 2, 0 }
#define OFFS210           { 2, 1, 0, 0 }
#define OFFS123           { 1, 2, 3, 0 }
#define OFFS321           { 3, 2, 1, 0 }
#define OFFS0123          { 0, 1, 2, 3 }
#define OFFS2103          { 2, 1, 0, 3 }
#define OFFS3210          { 3, 2, 1, 0 }
#define OFFS031           { 0, 3, 1, 0 }
#define OFFS026           { 0, 2, 6, 0 }
#define OFFS001           { 0, 0, 1, 0 }
#define OFFS010           { 0, 1, 0, 0 }
#define OFFS104           { 1, 0, 4, 0 }
#define OFFS2460          { 2, 4, 6, 0 }

/* subsampling */
#define SUB410            { 0, 2, 2, 0 }, { 0, 2, 2, 0 }
#define SUB411            { 0, 2, 2, 0 }, { 0, 0, 0, 0 }
#define SUB420            { 0, 1, 1, 0 }, { 0, 1, 1, 0 }
#define SUB422            { 0, 1, 1, 0 }, { 0, 0, 0, 0 }
#define SUB4              { 0, 0, 0, 0 }, { 0, 0, 0, 0 }
#define SUB444            { 0, 0, 0, 0 }, { 0, 0, 0, 0 }
#define SUB4444           { 0, 0, 0, 0 }, { 0, 0, 0, 0 }
#define SUB4204           { 0, 1, 1, 0 }, { 0, 1, 1, 0 }

#define MAKE_YUV_FORMAT(name, desc, fourcc, depth, pstride, plane, offs, sub ) \
 { fourcc, {GST_VIDEO_FORMAT_ ##name, G_STRINGIFY(name), desc, GST_VIDEO_FORMAT_FLAG_YUV, depth, pstride, plane, offs, sub } }
#define MAKE_YUVA_FORMAT(name, desc, fourcc, depth, pstride, plane, offs, sub) \
 { fourcc, {GST_VIDEO_FORMAT_ ##name, G_STRINGIFY(name), desc, GST_VIDEO_FORMAT_FLAG_YUV | GST_VIDEO_FORMAT_FLAG_ALPHA, depth, pstride, plane, offs, sub } }

#define MAKE_YUVA_LE_FORMAT(name, desc, fourcc, depth, pstride, plane, offs, sub) \
 { fourcc, {GST_VIDEO_FORMAT_ ##name, G_STRINGIFY(name), desc, GST_VIDEO_FORMAT_FLAG_YUV | GST_VIDEO_FORMAT_FLAG_ALPHA | GST_VIDEO_FORMAT_FLAG_LE, depth, pstride, plane, offs, sub } }

#define MAKE_RGB_FORMAT(name, desc, depth, pstride, plane, offs, sub) \
 { 0x00000000, {GST_VIDEO_FORMAT_ ##name, G_STRINGIFY(name), desc, GST_VIDEO_FORMAT_FLAG_RGB, depth, pstride, plane, offs, sub } }
#define MAKE_RGB_LE_FORMAT(name, desc, depth, pstride, plane, offs, sub) \
 { 0x00000000, {GST_VIDEO_FORMAT_ ##name, G_STRINGIFY(name), desc, GST_VIDEO_FORMAT_FLAG_RGB | GST_VIDEO_FORMAT_FLAG_LE, depth, pstride, plane, offs, sub } }
#define MAKE_RGBA_FORMAT(name, desc, depth, pstride, plane, offs, sub) \
 { 0x00000000, {GST_VIDEO_FORMAT_ ##name, G_STRINGIFY(name), desc, GST_VIDEO_FORMAT_FLAG_RGB | GST_VIDEO_FORMAT_FLAG_ALPHA, depth, pstride, plane, offs, sub } }
#define MAKE_RGBA_LE_FORMAT(name, desc, depth, pstride, plane, offs, sub) \
 { 0x00000000, {GST_VIDEO_FORMAT_ ##name, G_STRINGIFY(name), desc, GST_VIDEO_FORMAT_FLAG_RGB | GST_VIDEO_FORMAT_FLAG_ALPHA | GST_VIDEO_FORMAT_FLAG_LE, depth, pstride, plane, offs, sub } }

#define MAKE_GRAY_FORMAT(name, desc, depth, pstride, plane, offs, sub) \
 { 0x00000000, {GST_VIDEO_FORMAT_ ##name, G_STRINGIFY(name), desc, GST_VIDEO_FORMAT_FLAG_GRAY, depth, pstride, plane, offs, sub } }
#define MAKE_GRAY_LE_FORMAT(name, desc, depth, pstride, plane, offs, sub) \
 { 0x00000000, {GST_VIDEO_FORMAT_ ##name, G_STRINGIFY(name), desc, GST_VIDEO_FORMAT_FLAG_GRAY | GST_VIDEO_FORMAT_FLAG_LE, depth, pstride, plane, offs, sub } }

static VideoFormat formats[] = {
  {0x00000000, {GST_VIDEO_FORMAT_UNKNOWN, "UNKNOWN", "unknown video", 0, DPTH0,
              PSTR0, PLANE_NA,
          OFFS0}},
  MAKE_YUV_FORMAT (I420, "raw video", GST_MAKE_FOURCC ('I', '4', '2', '0'),
      DPTH888, PSTR111,
      PLANE012, OFFS0, SUB420),
  MAKE_YUV_FORMAT (YV12, "raw video", GST_MAKE_FOURCC ('Y', 'V', '1', '2'),
      DPTH888, PSTR111,
      PLANE021, OFFS0, SUB420),
  MAKE_YUV_FORMAT (YUY2, "raw video", GST_MAKE_FOURCC ('Y', 'U', 'Y', '2'),
      DPTH888, PSTR244,
      PLANE0, OFFS013, SUB422),
  MAKE_YUV_FORMAT (UYVY, "raw video", GST_MAKE_FOURCC ('U', 'Y', 'V', 'Y'),
      DPTH888, PSTR244,
      PLANE0, OFFS102, SUB422),
  MAKE_YUVA_FORMAT (AYUV, "raw video", GST_MAKE_FOURCC ('A', 'Y', 'U', 'V'),
      DPTH8888,
      PSTR4444, PLANE0, OFFS1230, SUB4444),
  MAKE_RGB_FORMAT (RGBx, "raw video", DPTH888, PSTR444, PLANE0, OFFS012,
      SUB444),
  MAKE_RGB_FORMAT (BGRx, "raw video", DPTH888, PSTR444, PLANE0, OFFS210,
      SUB444),
  MAKE_RGB_FORMAT (xRGB, "raw video", DPTH888, PSTR444, PLANE0, OFFS123,
      SUB444),
  MAKE_RGB_FORMAT (xBGR, "raw video", DPTH888, PSTR444, PLANE0, OFFS321,
      SUB444),
  MAKE_RGBA_FORMAT (RGBA, "raw video", DPTH8888, PSTR4444, PLANE0, OFFS0123,
      SUB4444),
  MAKE_RGBA_FORMAT (BGRA, "raw video", DPTH8888, PSTR4444, PLANE0, OFFS2103,
      SUB4444),
  MAKE_RGBA_FORMAT (ARGB, "raw video", DPTH8888, PSTR4444, PLANE0, OFFS1230,
      SUB4444),
  MAKE_RGBA_FORMAT (ABGR, "raw video", DPTH8888, PSTR4444, PLANE0, OFFS3210,
      SUB4444),
  MAKE_RGB_FORMAT (RGB, "raw video", DPTH888, PSTR333, PLANE0, OFFS012, SUB444),
  MAKE_RGB_FORMAT (BGR, "raw video", DPTH888, PSTR333, PLANE0, OFFS210, SUB444),

  MAKE_YUV_FORMAT (Y41B, "raw video", GST_MAKE_FOURCC ('Y', '4', '1', 'B'),
      DPTH888, PSTR111,
      PLANE012, OFFS0, SUB411),
  MAKE_YUV_FORMAT (Y42B, "raw video", GST_MAKE_FOURCC ('Y', '4', '2', 'B'),
      DPTH888, PSTR111,
      PLANE012, OFFS0, SUB422),
  MAKE_YUV_FORMAT (YVYU, "raw video", GST_MAKE_FOURCC ('Y', 'V', 'Y', 'U'),
      DPTH888, PSTR244,
      PLANE0, OFFS031, SUB422),
  MAKE_YUV_FORMAT (Y444, "raw video", GST_MAKE_FOURCC ('Y', '4', '4', '4'),
      DPTH888, PSTR111,
      PLANE012, OFFS0, SUB444),
  MAKE_YUV_FORMAT (v210, "raw video", GST_MAKE_FOURCC ('v', '2', '1', '0'),
      DPTH10_10_10,
      PSTR0, PLANE0, OFFS0, SUB422),
  MAKE_YUV_FORMAT (v216, "raw video", GST_MAKE_FOURCC ('v', '2', '1', '6'),
      DPTH16_16_16,
      PSTR488, PLANE0, OFFS026, SUB422),
  MAKE_YUV_FORMAT (NV12, "raw video", GST_MAKE_FOURCC ('N', 'V', '1', '2'),
      DPTH888, PSTR122,
      PLANE011, OFFS001, SUB420),
  MAKE_YUV_FORMAT (NV21, "raw video", GST_MAKE_FOURCC ('N', 'V', '2', '1'),
      DPTH888, PSTR122,
      PLANE011, OFFS010, SUB420),

  MAKE_GRAY_FORMAT (GRAY8, "raw video", DPTH8, PSTR1, PLANE0, OFFS0, SUB4),
  MAKE_GRAY_FORMAT (GRAY16_BE, "raw video", DPTH16, PSTR2, PLANE0, OFFS0, SUB4),
  MAKE_GRAY_LE_FORMAT (GRAY16_LE, "raw video", DPTH16, PSTR2, PLANE0, OFFS0,
      SUB4),

  MAKE_YUV_FORMAT (v308, "raw video", GST_MAKE_FOURCC ('v', '3', '0', '8'),
      DPTH888, PSTR333,
      PLANE0, OFFS012, SUB444),
  MAKE_YUV_FORMAT (Y800, "raw video", GST_MAKE_FOURCC ('Y', '8', '0', '0'),
      DPTH8, PSTR1,
      PLANE0, OFFS0, SUB4),
  MAKE_YUV_FORMAT (Y16, "raw video", GST_MAKE_FOURCC ('Y', '1', '6', ' '),
      DPTH16, PSTR2,
      PLANE0, OFFS0, SUB4),

#if G_BYTE_ORDER == G_LITTLE_ENDIAN
  MAKE_RGB_LE_FORMAT (RGB16, "raw video", DPTH565, PSTR222, PLANE0, OFFS0,
      SUB444),
  MAKE_RGB_LE_FORMAT (BGR16, "raw video", DPTH565, PSTR222, PLANE0, OFFS0,
      SUB444),
  MAKE_RGB_LE_FORMAT (RGB15, "raw video", DPTH555, PSTR222, PLANE0, OFFS0,
      SUB444),
  MAKE_RGB_LE_FORMAT (BGR15, "raw video", DPTH555, PSTR222, PLANE0, OFFS0,
      SUB444),
#else
  MAKE_RGB_FORMAT (RGB16, "raw video", DPTH565, PSTR222, PLANE0, OFFS0, SUB444),
  MAKE_RGB_FORMAT (BGR16, "raw video", DPTH565, PSTR222, PLANE0, OFFS0, SUB444),
  MAKE_RGB_FORMAT (RGB15, "raw video", DPTH555, PSTR222, PLANE0, OFFS0, SUB444),
  MAKE_RGB_FORMAT (BGR15, "raw video", DPTH555, PSTR222, PLANE0, OFFS0, SUB444),
#endif

  MAKE_YUV_FORMAT (UYVP, "raw video", GST_MAKE_FOURCC ('U', 'Y', 'V', 'P'),
      DPTH10_10_10,
      PSTR0, PLANE0, OFFS0, SUB422),
  MAKE_YUVA_FORMAT (A420, "raw video", GST_MAKE_FOURCC ('A', '4', '2', '0'),
      DPTH8888,
      PSTR1111, PLANE0123, OFFS0, SUB4204),
  MAKE_RGBA_FORMAT (RGB8_PALETTED, "raw video", DPTH8888, PSTR1111, PLANE0,
      OFFS0, SUB4444),
  MAKE_YUV_FORMAT (YUV9, "raw video", GST_MAKE_FOURCC ('Y', 'U', 'V', '9'),
      DPTH888, PSTR111,
      PLANE012, OFFS0, SUB410),
  MAKE_YUV_FORMAT (YVU9, "raw video", GST_MAKE_FOURCC ('Y', 'V', 'U', '9'),
      DPTH888, PSTR111,
      PLANE021, OFFS0, SUB410),
  MAKE_YUV_FORMAT (IYU1, "raw video", GST_MAKE_FOURCC ('I', 'Y', 'U', '1'),
      DPTH888, PSTR0,
      PLANE0, OFFS104, SUB411),
#if G_BYTE_ORDER == G_LITTLE_ENDIAN
  MAKE_RGBA_LE_FORMAT (ARGB64, "raw video", DPTH16_16_16_16, PSTR8888,
      PLANE0, OFFS2460, SUB444),
  MAKE_YUVA_LE_FORMAT (AYUV64, "raw video", 0x00000000, DPTH16_16_16_16,
      PSTR8888, PLANE0, OFFS2460, SUB444),
#else
  MAKE_RGBA_FORMAT (ARGB64, "raw video", DPTH16_16_16_16, PSTR8888, PLANE0,
      OFFS2460,
      SUB444),
  MAKE_YUVA_FORMAT (AYUV64, "raw video", 0x00000000, DPTH16_16_16_16, PSTR8888,
      PLANE0, OFFS2460, SUB444),
#endif
  MAKE_RGB_FORMAT (r210, "raw video", DPTH10_10_10, PSTR444, PLANE0, OFFS0,
      SUB444),
  {0x00000000, {GST_VIDEO_FORMAT_ENCODED, "ENCODED", "encoded video",
          GST_VIDEO_FORMAT_FLAG_COMPLEX, DPTH0, PSTR0, PLANE_NA, OFFS0}},
};

/*
 * gst_video_format_from_rgb32_masks:
 * @red_mask: red bit mask
 * @green_mask: green bit mask
 * @blue_mask: blue bit mask
 *
 * Converts red, green, blue bit masks into the corresponding
 * #GstVideoFormat.
 *
 * Since: 0.10.16
 *
 * Returns: the #GstVideoFormat corresponding to the bit masks
 */
static GstVideoFormat
gst_video_format_from_rgb32_masks (int red_mask, int green_mask, int blue_mask)
{
  if (red_mask == 0xff000000 && green_mask == 0x00ff0000 &&
      blue_mask == 0x0000ff00) {
    return GST_VIDEO_FORMAT_RGBx;
  }
  if (red_mask == 0x0000ff00 && green_mask == 0x00ff0000 &&
      blue_mask == 0xff000000) {
    return GST_VIDEO_FORMAT_BGRx;
  }
  if (red_mask == 0x00ff0000 && green_mask == 0x0000ff00 &&
      blue_mask == 0x000000ff) {
    return GST_VIDEO_FORMAT_xRGB;
  }
  if (red_mask == 0x000000ff && green_mask == 0x0000ff00 &&
      blue_mask == 0x00ff0000) {
    return GST_VIDEO_FORMAT_xBGR;
  }

  return GST_VIDEO_FORMAT_UNKNOWN;
}

static GstVideoFormat
gst_video_format_from_rgba32_masks (int red_mask, int green_mask,
    int blue_mask, int alpha_mask)
{
  if (red_mask == 0xff000000 && green_mask == 0x00ff0000 &&
      blue_mask == 0x0000ff00 && alpha_mask == 0x000000ff) {
    return GST_VIDEO_FORMAT_RGBA;
  }
  if (red_mask == 0x0000ff00 && green_mask == 0x00ff0000 &&
      blue_mask == 0xff000000 && alpha_mask == 0x000000ff) {
    return GST_VIDEO_FORMAT_BGRA;
  }
  if (red_mask == 0x00ff0000 && green_mask == 0x0000ff00 &&
      blue_mask == 0x000000ff && alpha_mask == 0xff000000) {
    return GST_VIDEO_FORMAT_ARGB;
  }
  if (red_mask == 0x000000ff && green_mask == 0x0000ff00 &&
      blue_mask == 0x00ff0000 && alpha_mask == 0xff000000) {
    return GST_VIDEO_FORMAT_ABGR;
  }

  return GST_VIDEO_FORMAT_UNKNOWN;
}

static GstVideoFormat
gst_video_format_from_rgb24_masks (int red_mask, int green_mask, int blue_mask)
{
  if (red_mask == 0xff0000 && green_mask == 0x00ff00 && blue_mask == 0x0000ff) {
    return GST_VIDEO_FORMAT_RGB;
  }
  if (red_mask == 0x0000ff && green_mask == 0x00ff00 && blue_mask == 0xff0000) {
    return GST_VIDEO_FORMAT_BGR;
  }

  return GST_VIDEO_FORMAT_UNKNOWN;
}

static GstVideoFormat
gst_video_format_from_rgb16_masks (int red_mask, int green_mask, int blue_mask)
{
  if (red_mask == GST_VIDEO_COMP1_MASK_16_INT
      && green_mask == GST_VIDEO_COMP2_MASK_16_INT
      && blue_mask == GST_VIDEO_COMP3_MASK_16_INT) {
    return GST_VIDEO_FORMAT_RGB16;
  }
  if (red_mask == GST_VIDEO_COMP3_MASK_16_INT
      && green_mask == GST_VIDEO_COMP2_MASK_16_INT
      && blue_mask == GST_VIDEO_COMP1_MASK_16_INT) {
    return GST_VIDEO_FORMAT_BGR16;
  }
  if (red_mask == GST_VIDEO_COMP1_MASK_15_INT
      && green_mask == GST_VIDEO_COMP2_MASK_15_INT
      && blue_mask == GST_VIDEO_COMP3_MASK_15_INT) {
    return GST_VIDEO_FORMAT_RGB15;
  }
  if (red_mask == GST_VIDEO_COMP3_MASK_15_INT
      && green_mask == GST_VIDEO_COMP2_MASK_15_INT
      && blue_mask == GST_VIDEO_COMP1_MASK_15_INT) {
    return GST_VIDEO_FORMAT_BGR15;
  }

  return GST_VIDEO_FORMAT_UNKNOWN;
}

/**
 * gst_video_format_from_masks:
 * @depth: the amount of bits used for a pixel
 * @bpp: the amount of bits used to store a pixel. This value is bigger than
 *   @depth
 * @endianness: the endianness of the masks
 * @red_mask: the red mask
 * @green_mask: the green mask
 * @blue_mask: the blue mask
 * @alpha_mask: the optional alpha mask
 *
 * Find the #GstVideoFormat for the given parameters.
 *
 * Returns: a #GstVideoFormat or GST_VIDEO_FORMAT_UNKNOWN when the parameters to
 * not specify a known format.
 */
static GstVideoFormat
gst_video_format_from_masks (gint depth, gint bpp, gint endianness,
    gint red_mask, gint green_mask, gint blue_mask, gint alpha_mask)
{
  GstVideoFormat format;

  /* our caps system handles 24/32bpp RGB as big-endian. */
  if ((bpp == 24 || bpp == 32) && endianness == G_LITTLE_ENDIAN) {
    red_mask = GUINT32_TO_BE (red_mask);
    green_mask = GUINT32_TO_BE (green_mask);
    blue_mask = GUINT32_TO_BE (blue_mask);
    endianness = G_BIG_ENDIAN;
    if (bpp == 24) {
      red_mask >>= 8;
      green_mask >>= 8;
      blue_mask >>= 8;
    }
  }

  if (depth == 30 && bpp == 32) {
    format = GST_VIDEO_FORMAT_r210;
  } else if (depth == 24 && bpp == 32) {
    format = gst_video_format_from_rgb32_masks (red_mask, green_mask,
        blue_mask);
  } else if (depth == 32 && bpp == 32 && alpha_mask) {
    format = gst_video_format_from_rgba32_masks (red_mask, green_mask,
        blue_mask, alpha_mask);
  } else if (depth == 24 && bpp == 24) {
    format = gst_video_format_from_rgb24_masks (red_mask, green_mask,
        blue_mask);
  } else if ((depth == 15 || depth == 16) && bpp == 16 &&
      endianness == G_BYTE_ORDER) {
    format = gst_video_format_from_rgb16_masks (red_mask, green_mask,
        blue_mask);
  } else if (depth == 8 && bpp == 8) {
    format = GST_VIDEO_FORMAT_RGB8_PALETTED;
  } else if (depth == 64 && bpp == 64) {
    format = gst_video_format_from_rgba32_masks (red_mask, green_mask,
        blue_mask, alpha_mask);
    if (format == GST_VIDEO_FORMAT_ARGB) {
      format = GST_VIDEO_FORMAT_ARGB64;
    } else {
      format = GST_VIDEO_FORMAT_UNKNOWN;
    }
  } else {
    format = GST_VIDEO_FORMAT_UNKNOWN;
  }
  return format;
}

static GstCaps *
gst_video_format_new_caps_raw (GstVideoFormat format)
{
  g_return_val_if_fail (format != GST_VIDEO_FORMAT_UNKNOWN, NULL);

  if (gst_video_format_is_yuv (format)) {
    return gst_caps_new_simple ("video/x-raw-yuv",
        "format", GST_TYPE_FOURCC, gst_video_format_to_fourcc (format), NULL);
  }
  if (gst_video_format_is_rgb (format)) {
    GstCaps *caps;
    int red_mask = 0;
    int blue_mask = 0;
    int green_mask = 0;
    int alpha_mask;
    int depth;
    int bpp;
    gboolean have_alpha;
    unsigned int mask = 0;

    switch (format) {
      case GST_VIDEO_FORMAT_RGBx:
      case GST_VIDEO_FORMAT_BGRx:
      case GST_VIDEO_FORMAT_xRGB:
      case GST_VIDEO_FORMAT_xBGR:
        bpp = 32;
        depth = 24;
        have_alpha = FALSE;
        break;
      case GST_VIDEO_FORMAT_RGBA:
      case GST_VIDEO_FORMAT_BGRA:
      case GST_VIDEO_FORMAT_ARGB:
      case GST_VIDEO_FORMAT_ABGR:
        bpp = 32;
        depth = 32;
        have_alpha = TRUE;
        break;
      case GST_VIDEO_FORMAT_RGB:
      case GST_VIDEO_FORMAT_BGR:
        bpp = 24;
        depth = 24;
        have_alpha = FALSE;
        break;
      case GST_VIDEO_FORMAT_RGB16:
      case GST_VIDEO_FORMAT_BGR16:
        bpp = 16;
        depth = 16;
        have_alpha = FALSE;
        break;
      case GST_VIDEO_FORMAT_RGB15:
      case GST_VIDEO_FORMAT_BGR15:
        bpp = 16;
        depth = 15;
        have_alpha = FALSE;
        break;
      case GST_VIDEO_FORMAT_RGB8_PALETTED:
        bpp = 8;
        depth = 8;
        have_alpha = FALSE;
        break;
      case GST_VIDEO_FORMAT_ARGB64:
        bpp = 64;
        depth = 64;
        have_alpha = TRUE;
        break;
      case GST_VIDEO_FORMAT_r210:
        bpp = 32;
        depth = 30;
        have_alpha = FALSE;
        break;
      default:
        return NULL;
    }
    if (bpp == 32 && depth == 30) {
      red_mask = 0x3ff00000;
      green_mask = 0x000ffc00;
      blue_mask = 0x000003ff;
      have_alpha = FALSE;
    } else if (bpp == 32 || bpp == 24 || bpp == 64) {
      if (bpp == 32) {
        mask = 0xff000000;
      } else {
        mask = 0xff0000;
      }
      red_mask =
          mask >> (8 * gst_video_format_get_component_offset (format, 0, 0, 0));
      green_mask =
          mask >> (8 * gst_video_format_get_component_offset (format, 1, 0, 0));
      blue_mask =
          mask >> (8 * gst_video_format_get_component_offset (format, 2, 0, 0));
    } else if (bpp == 16) {
      switch (format) {
        case GST_VIDEO_FORMAT_RGB16:
          red_mask = GST_VIDEO_COMP1_MASK_16_INT;
          green_mask = GST_VIDEO_COMP2_MASK_16_INT;
          blue_mask = GST_VIDEO_COMP3_MASK_16_INT;
          break;
        case GST_VIDEO_FORMAT_BGR16:
          red_mask = GST_VIDEO_COMP3_MASK_16_INT;
          green_mask = GST_VIDEO_COMP2_MASK_16_INT;
          blue_mask = GST_VIDEO_COMP1_MASK_16_INT;
          break;
          break;
        case GST_VIDEO_FORMAT_RGB15:
          red_mask = GST_VIDEO_COMP1_MASK_15_INT;
          green_mask = GST_VIDEO_COMP2_MASK_15_INT;
          blue_mask = GST_VIDEO_COMP3_MASK_15_INT;
          break;
        case GST_VIDEO_FORMAT_BGR15:
          red_mask = GST_VIDEO_COMP3_MASK_15_INT;
          green_mask = GST_VIDEO_COMP2_MASK_15_INT;
          blue_mask = GST_VIDEO_COMP1_MASK_15_INT;
          break;
        default:
          g_assert_not_reached ();
      }
    } else if (bpp != 8) {
      g_assert_not_reached ();
    }

    caps = gst_caps_new_simple ("video/x-raw-rgb",
        "bpp", G_TYPE_INT, bpp, "depth", G_TYPE_INT, depth, NULL);

    if (bpp != 8) {
      gst_caps_set_simple (caps,
          "endianness", G_TYPE_INT, bpp == 16 ? G_BYTE_ORDER : G_BIG_ENDIAN,
          "red_mask", G_TYPE_INT, red_mask,
          "green_mask", G_TYPE_INT, green_mask,
          "blue_mask", G_TYPE_INT, blue_mask, NULL);
    }

    if (have_alpha) {
      alpha_mask =
          mask >> (8 * gst_video_format_get_component_offset (format, 3, 0, 0));
      gst_caps_set_simple (caps, "alpha_mask", G_TYPE_INT, alpha_mask, NULL);
    }
    return caps;
  }

  if (gst_video_format_is_gray (format)) {
    GstCaps *caps;
    int bpp;
    int depth;
    int endianness;

    switch (format) {
      case GST_VIDEO_FORMAT_GRAY8:
        bpp = depth = 8;
        endianness = G_BIG_ENDIAN;
        break;
      case GST_VIDEO_FORMAT_GRAY16_BE:
        bpp = depth = 16;
        endianness = G_BIG_ENDIAN;
        break;
      case GST_VIDEO_FORMAT_GRAY16_LE:
        bpp = depth = 16;
        endianness = G_LITTLE_ENDIAN;
        break;
      default:
        return NULL;
        break;
    }

    if (bpp <= 8) {
      caps = gst_caps_new_simple ("video/x-raw-gray",
          "bpp", G_TYPE_INT, bpp, "depth", G_TYPE_INT, depth, NULL);
    } else {
      caps = gst_caps_new_simple ("video/x-raw-gray",
          "bpp", G_TYPE_INT, bpp,
          "depth", G_TYPE_INT, depth,
          "endianness", G_TYPE_INT, endianness, NULL);
    }

    return caps;
  }

  return NULL;
}

/**
 * gst_video_format_to_string:
 * @format: a #GstVideoFormat video format
 *
 * Returns: a short string that describes @format, or #NULL
 *
 * Since: 0.10.37
 */
const gchar *
gst_video_format_to_string (GstVideoFormat format)
{
  if (format >= G_N_ELEMENTS (formats))
    return NULL;

  return GST_VIDEO_FORMAT_INFO_NAME (&formats[format].info);
}

/**
 * gst_video_format_get_info:
 * @format: a #GstVideoFormat
 *
 * Get the #GstVideoFormatInfo for @format
 *
 * Returns: The #GstVideoFormatInfo for @format.
 *
 * Since: 0.10.37
 */
const GstVideoFormatInfo *
gst_video_format_get_info (GstVideoFormat format)
{
  g_return_val_if_fail (format != GST_VIDEO_FORMAT_UNKNOWN, NULL);
  g_return_val_if_fail (format < G_N_ELEMENTS (formats), NULL);

  return &formats[format].info;
}

/**
 * gst_video_info_init:
 * @info: a #GstVideoInfo
 *
 * Initialize @info with default values.
 *
 * Since: 0.10.37
 */
void
gst_video_info_init (GstVideoInfo * info)
{
  g_return_if_fail (info != NULL);

  memset (info, 0, sizeof (GstVideoInfo));

  info->finfo = &formats[GST_VIDEO_FORMAT_UNKNOWN].info;

  /* arrange for sensible defaults, e.g. if turned into caps */
  info->fps_n = 0;
  info->fps_d = 1;
  info->par_n = 1;
  info->par_d = 1;
}

/**
 * gst_video_info_set_format:
 * @info: a #GstVideoInfo
 * @format: the format
 * @width: a width
 * @height: a height
 *
 * Set the default info for a video frame of @format and @width and @height.
 *
 * Since: 0.10.37
 */
void
gst_video_info_set_format (GstVideoInfo * info, GstVideoFormat format,
    guint width, guint height)
{
  const GstVideoFormatInfo *finfo;

  g_return_if_fail (info != NULL);
  g_return_if_fail (format != GST_VIDEO_FORMAT_UNKNOWN);

  finfo = &formats[format].info;

  info->flags = 0;
  info->finfo = finfo;
  info->width = width;
  info->height = height;

  fill_planes (info);
}

#if 0
static const gchar *interlace_mode[] = {
  "progressive",
  "interleaved",
  "mixed",
  "fields"
};

static const gchar *
gst_interlace_mode_to_string (GstVideoInterlaceMode mode)
{
  if (mode < 0 || mode >= G_N_ELEMENTS (interlace_mode))
    return NULL;

  return interlace_mode[mode];
}

static GstVideoInterlaceMode
gst_interlace_mode_from_string (const gchar * mode)
{
  gint i;
  for (i = 0; i < G_N_ELEMENTS (interlace_mode); i++) {
    if (g_str_equal (interlace_mode[i], mode))
      return i;
  }
  return GST_VIDEO_INTERLACE_MODE_PROGRESSIVE;
}
#endif

typedef struct
{
  const gchar *name;
  GstVideoChromaSite site;
} ChromaSiteInfo;

static const ChromaSiteInfo chromasite[] = {
  {"jpeg", GST_VIDEO_CHROMA_SITE_JPEG},
  {"mpeg2", GST_VIDEO_CHROMA_SITE_MPEG2},
  {"dv", GST_VIDEO_CHROMA_SITE_DV}
};

static GstVideoChromaSite
gst_video_chroma_from_string (const gchar * s)
{
  gint i;
  for (i = 0; i < G_N_ELEMENTS (chromasite); i++) {
    if (g_str_equal (chromasite[i].name, s))
      return chromasite[i].site;
  }
  return GST_VIDEO_CHROMA_SITE_UNKNOWN;
}

static const gchar *
gst_video_chroma_to_string (GstVideoChromaSite site)
{
  gint i;
  for (i = 0; i < G_N_ELEMENTS (chromasite); i++) {
    if (chromasite[i].site == site)
      return chromasite[i].name;
  }
  return NULL;
}

typedef struct
{
  const gchar *name;
  GstVideoColorimetry color;
} ColorimetryInfo;

#define MAKE_COLORIMETRY(n,r,m,t,p) { GST_VIDEO_COLORIMETRY_ ##n, \
  { GST_VIDEO_COLOR_RANGE ##r, GST_VIDEO_COLOR_MATRIX_ ##m, \
  GST_VIDEO_TRANSFER_ ##t, GST_VIDEO_COLOR_PRIMARIES_ ##p } }

static const ColorimetryInfo colorimetry[] = {
  MAKE_COLORIMETRY (BT601, _16_235, BT601, BT709, BT470M),
  MAKE_COLORIMETRY (BT709, _16_235, BT709, BT709, BT709),
  MAKE_COLORIMETRY (SMPTE240M, _16_235, SMPTE240M, SMPTE240M, SMPTE240M),
};

static const ColorimetryInfo *
gst_video_get_colorimetry (const gchar * s)
{
  gint i;

  for (i = 0; i < G_N_ELEMENTS (colorimetry); i++) {
    if (g_str_equal (colorimetry[i].name, s))
      return &colorimetry[i];
  }
  return NULL;
}

#define IS_EQUAL(ci,i) (((ci)->color.range == (i)->range) && \
                        ((ci)->color.matrix == (i)->matrix) && \
                        ((ci)->color.transfer == (i)->transfer) && \
                        ((ci)->color.primaries == (i)->primaries))


/**
 * gst_video_colorimetry_from_string
 * @cinfo: a #GstVideoColorimetry
 * @color: a colorimetry string
 *
 * Parse the colorimetry string and update @cinfo with the parsed
 * values.
 *
 * Returns: #TRUE if @color points to valid colorimetry info.
 *
 * Since: 0.10.37
 */
gboolean
gst_video_colorimetry_from_string (GstVideoColorimetry * cinfo,
    const gchar * color)
{
  const ColorimetryInfo *ci;

  if ((ci = gst_video_get_colorimetry (color))) {
    *cinfo = ci->color;
  } else {
    /* FIXME, split and parse */
    cinfo->range = GST_VIDEO_COLOR_RANGE_16_235;
    cinfo->matrix = GST_VIDEO_COLOR_MATRIX_BT601;
    cinfo->transfer = GST_VIDEO_TRANSFER_BT709;
    cinfo->primaries = GST_VIDEO_COLOR_PRIMARIES_BT709;
  }
  return TRUE;
}

static void
gst_video_caps_set_colorimetry (GstCaps * caps, GstVideoColorimetry * cinfo)
{
  gint i;

  for (i = 0; i < G_N_ELEMENTS (colorimetry); i++) {
    if (IS_EQUAL (&colorimetry[i], cinfo)) {
      gst_caps_set_simple (caps, "colorimetry", G_TYPE_STRING,
          colorimetry[i].name, NULL);
      return;
    }
  }
  /* FIXME, construct colorimetry */
}

/**
 * gst_video_colorimetry_matches:
 * @cinfo: a #GstVideoInfo
 * @color: a colorimetry string
 *
 * Check if the colorimetry information in @cinfo matches that of the
 * string @color.
 *
 * Returns: #TRUE if @color conveys the same colorimetry info as the color
 * information in @info.
 *
 * Since: 0.10.37
 */
gboolean
gst_video_colorimetry_matches (GstVideoColorimetry * cinfo, const gchar * color)
{
  const ColorimetryInfo *ci;

  if ((ci = gst_video_get_colorimetry (color)))
    return IS_EQUAL (ci, cinfo);

  return FALSE;
}

/**
 * gst_video_info_from_caps:
 * @info: a #GstVideoInfo
 * @caps: a #GstCaps
 *
 * Parse @caps and update @info.
 *
 * Returns: TRUE if @caps could be parsed
 *
 * Since: 0.10.37
 */
gboolean
gst_video_info_from_caps (GstVideoInfo * info, const GstCaps * caps)
{
  GstStructure *structure;
  const gchar *s;
  GstVideoFormat format = GST_VIDEO_FORMAT_UNKNOWN;
  gint width = 0, height = 0;
  gint fps_n, fps_d;
  gint par_n, par_d;
  gboolean interlaced;

  g_return_val_if_fail (info != NULL, FALSE);
  g_return_val_if_fail (caps != NULL, FALSE);
  g_return_val_if_fail (gst_caps_is_fixed (caps), FALSE);

  GST_DEBUG ("parsing caps %" GST_PTR_FORMAT, caps);

  structure = gst_caps_get_structure (caps, 0);

  if (gst_structure_has_name (structure, "video/x-raw-yuv")) {
    guint32 fourcc;

    if (!gst_structure_get_fourcc (structure, "format", &fourcc))
      goto no_format;

    format = gst_video_format_from_fourcc (fourcc);
  } else if (gst_structure_has_name (structure, "video/x-raw-rgb")) {
    int depth;
    int bpp;
    int endianness = 0;
    int red_mask = 0;
    int green_mask = 0;
    int blue_mask = 0;
    int alpha_mask = 0;

    if (!gst_structure_get_int (structure, "depth", &depth) ||
        !gst_structure_get_int (structure, "bpp", &bpp))
      goto no_bpp_depth;

    if (bpp != 8) {
      gst_structure_get_int (structure, "endianness", &endianness);
      gst_structure_get_int (structure, "red_mask", &red_mask);
      gst_structure_get_int (structure, "green_mask", &green_mask);
      gst_structure_get_int (structure, "blue_mask", &blue_mask);
    }
    gst_structure_get_int (structure, "alpha_mask", &alpha_mask);
    format = gst_video_format_from_masks (depth, bpp, endianness,
        red_mask, green_mask, blue_mask, alpha_mask);
  } else if (gst_structure_has_name (structure, "video/x-raw-gray")) {
    int depth;
    int bpp;
    int endianness;

    if (!gst_structure_get_int (structure, "depth", &depth) ||
        !gst_structure_get_int (structure, "bpp", &bpp))
      goto no_bpp_depth;

    /* endianness is mandatory for bpp > 8 */
    if (bpp > 8 &&
        !gst_structure_get_int (structure, "endianness", &endianness))
      goto no_endianess;

    if (depth == 8 && bpp == 8) {
      format = GST_VIDEO_FORMAT_GRAY8;
    } else if (depth == 16 && bpp == 16 && endianness == G_BIG_ENDIAN) {
      format = GST_VIDEO_FORMAT_GRAY16_BE;
    } else if (depth == 16 && bpp == 16 && endianness == G_LITTLE_ENDIAN) {
      format = GST_VIDEO_FORMAT_GRAY16_LE;
    }
  } else if (g_str_has_prefix (gst_structure_get_name (structure), "video/") ||
      g_str_has_prefix (gst_structure_get_name (structure), "image/"))
    format = GST_VIDEO_FORMAT_ENCODED;

  if (format == GST_VIDEO_FORMAT_UNKNOWN)
    goto unknown_format;

  /* width and height are mandatory, except for non-raw-formats */
  if (!gst_structure_get_int (structure, "width", &width) &&
      format != GST_VIDEO_FORMAT_ENCODED)
    goto no_width;
  if (!gst_structure_get_int (structure, "height", &height) &&
      format != GST_VIDEO_FORMAT_ENCODED)
    goto no_height;

  gst_video_info_set_format (info, format, width, height);

  if (gst_structure_get_fraction (structure, "framerate", &fps_n, &fps_d)) {
    if (fps_n == 0) {
      /* variable framerate */
      info->flags |= GST_VIDEO_FLAG_VARIABLE_FPS;
      /* see if we have a max-framerate */
      gst_structure_get_fraction (structure, "max-framerate", &fps_n, &fps_d);
    }
    info->fps_n = fps_n;
    info->fps_d = fps_d;
  } else {
    /* unspecified is variable framerate */
    info->fps_n = 0;
    info->fps_d = 1;
  }

  if (gst_structure_get_boolean (structure, "interlaced", &interlaced)
      && interlaced)
    info->interlace_mode = GST_VIDEO_INTERLACE_MODE_INTERLEAVED;
  else
    info->interlace_mode = GST_VIDEO_INTERLACE_MODE_PROGRESSIVE;

  if ((s = gst_structure_get_string (structure, "chroma-site")))
    info->chroma_site = gst_video_chroma_from_string (s);
  else
    info->chroma_site = GST_VIDEO_CHROMA_SITE_UNKNOWN;

  if ((s = gst_structure_get_string (structure, "colorimetry")))
    gst_video_colorimetry_from_string (&info->colorimetry, s);
  else
    memset (&info->colorimetry, 0, sizeof (GstVideoColorimetry));

  if (gst_structure_get_fraction (structure, "pixel-aspect-ratio",
          &par_n, &par_d)) {
    info->par_n = par_n;
    info->par_d = par_d;
  } else {
    info->par_n = 1;
    info->par_d = 1;
  }
  return TRUE;

  /* ERROR */
no_format:
  {
    GST_ERROR ("no format given");
    return FALSE;
  }
unknown_format:
  {
    GST_ERROR ("unknown format");
    return FALSE;
  }
no_width:
  {
    GST_ERROR ("no width property given");
    return FALSE;
  }
no_height:
  {
    GST_ERROR ("no height property given");
    return FALSE;
  }

no_bpp_depth:
  {
    GST_ERROR ("no bpp or depth given");
    return FALSE;
  }

no_endianess:
  {
    GST_ERROR ("no endianness given");
    return FALSE;
  }
}

/**
 * gst_video_info_is_equal:
 * @info: a #GstVideoInfo
 * @other: a #GstVideoInfo
 *
 * Compares two #GstVideoInfo and returns whether they are equal or not
 *
 * Returns: %TRUE if @info and @other are equal, else %FALSE.
 *
 * Since: 0.10.37
 */
gboolean
gst_video_info_is_equal (const GstVideoInfo * info, const GstVideoInfo * other)
{
  if (GST_VIDEO_INFO_FORMAT (info) != GST_VIDEO_INFO_FORMAT (other))
    return FALSE;
  if (GST_VIDEO_INFO_INTERLACE_MODE (info) !=
      GST_VIDEO_INFO_INTERLACE_MODE (other))
    return FALSE;
  if (GST_VIDEO_INFO_FLAGS (info) != GST_VIDEO_INFO_FLAGS (other))
    return FALSE;
  if (GST_VIDEO_INFO_WIDTH (info) != GST_VIDEO_INFO_WIDTH (other))
    return FALSE;
  if (GST_VIDEO_INFO_HEIGHT (info) != GST_VIDEO_INFO_HEIGHT (other))
    return FALSE;
  if (GST_VIDEO_INFO_SIZE (info) != GST_VIDEO_INFO_SIZE (other))
    return FALSE;
  if (GST_VIDEO_INFO_PAR_N (info) != GST_VIDEO_INFO_PAR_N (other))
    return FALSE;
  if (GST_VIDEO_INFO_PAR_D (info) != GST_VIDEO_INFO_PAR_D (other))
    return FALSE;
  if (GST_VIDEO_INFO_FPS_N (info) != GST_VIDEO_INFO_FPS_N (other))
    return FALSE;
  if (GST_VIDEO_INFO_FPS_D (info) != GST_VIDEO_INFO_FPS_D (other))
    return FALSE;
  return TRUE;
}

/**
 * gst_video_info_to_caps:
 * @info: a #GstVideoInfo
 *
 * Convert the values of @info into a #GstCaps.
 *
 * Returns: a new #GstCaps containing the info of @info.
 *
 * Since: 0.10.37
 */
GstCaps *
gst_video_info_to_caps (GstVideoInfo * info)
{
  GstCaps *caps;

  g_return_val_if_fail (info != NULL, NULL);
  g_return_val_if_fail (info->finfo != NULL, NULL);
  g_return_val_if_fail (info->finfo->format != GST_VIDEO_FORMAT_UNKNOWN, NULL);

  caps = gst_video_format_new_caps_raw (GST_VIDEO_INFO_FORMAT (info));

  gst_caps_set_simple (caps,
      "width", G_TYPE_INT, info->width,
      "height", G_TYPE_INT, info->height,
      "pixel-aspect-ratio", GST_TYPE_FRACTION, info->par_n, info->par_d, NULL);

  gst_caps_set_simple (caps, "interlaced", G_TYPE_BOOLEAN,
      GST_VIDEO_INFO_IS_INTERLACED (info), NULL);

  if (info->chroma_site != GST_VIDEO_CHROMA_SITE_UNKNOWN)
    gst_caps_set_simple (caps, "chroma-site", G_TYPE_STRING,
        gst_video_chroma_to_string (info->chroma_site), NULL);

  gst_video_caps_set_colorimetry (caps, &info->colorimetry);

  if (info->flags & GST_VIDEO_FLAG_VARIABLE_FPS && info->fps_n != 0) {
    /* variable fps with a max-framerate */
    gst_caps_set_simple (caps, "framerate", GST_TYPE_FRACTION, 0, 1,
        "max-framerate", GST_TYPE_FRACTION, info->fps_n, info->fps_d, NULL);
  } else {
    /* no variable fps or no max-framerate */
    gst_caps_set_simple (caps, "framerate", GST_TYPE_FRACTION,
        info->fps_n, info->fps_d, NULL);
  }

  return caps;
}


static int
fill_planes (GstVideoInfo * info)
{
  gint width, height;

  width = info->width;
  height = info->height;

  switch (info->finfo->format) {
    case GST_VIDEO_FORMAT_YUY2:
    case GST_VIDEO_FORMAT_YVYU:
    case GST_VIDEO_FORMAT_UYVY:
      info->stride[0] = GST_ROUND_UP_4 (width * 2);
      info->offset[0] = 0;
      info->size = info->stride[0] * height;
      break;
    case GST_VIDEO_FORMAT_AYUV:
    case GST_VIDEO_FORMAT_RGBx:
    case GST_VIDEO_FORMAT_RGBA:
    case GST_VIDEO_FORMAT_BGRx:
    case GST_VIDEO_FORMAT_BGRA:
    case GST_VIDEO_FORMAT_xRGB:
    case GST_VIDEO_FORMAT_ARGB:
    case GST_VIDEO_FORMAT_xBGR:
    case GST_VIDEO_FORMAT_ABGR:
    case GST_VIDEO_FORMAT_r210:
      info->stride[0] = width * 4;
      info->offset[0] = 0;
      info->size = info->stride[0] * height;
      break;
    case GST_VIDEO_FORMAT_RGB16:
    case GST_VIDEO_FORMAT_BGR16:
    case GST_VIDEO_FORMAT_RGB15:
    case GST_VIDEO_FORMAT_BGR15:
      info->stride[0] = GST_ROUND_UP_4 (width * 2);
      info->offset[0] = 0;
      info->size = info->stride[0] * height;
      break;
    case GST_VIDEO_FORMAT_RGB:
    case GST_VIDEO_FORMAT_BGR:
    case GST_VIDEO_FORMAT_v308:
      info->stride[0] = GST_ROUND_UP_4 (width * 3);
      info->offset[0] = 0;
      info->size = info->stride[0] * height;
      break;
    case GST_VIDEO_FORMAT_v210:
      info->stride[0] = ((width + 47) / 48) * 128;
      info->offset[0] = 0;
      info->size = info->stride[0] * height;
      break;
    case GST_VIDEO_FORMAT_v216:
      info->stride[0] = GST_ROUND_UP_8 (width * 4);
      info->offset[0] = 0;
      info->size = info->stride[0] * height;
      break;
    case GST_VIDEO_FORMAT_GRAY8:
    case GST_VIDEO_FORMAT_Y800:
      info->stride[0] = GST_ROUND_UP_4 (width);
      info->offset[0] = 0;
      info->size = info->stride[0] * height;
      break;
    case GST_VIDEO_FORMAT_GRAY16_BE:
    case GST_VIDEO_FORMAT_GRAY16_LE:
    case GST_VIDEO_FORMAT_Y16:
      info->stride[0] = GST_ROUND_UP_4 (width * 2);
      info->offset[0] = 0;
      info->size = info->stride[0] * height;
      break;
    case GST_VIDEO_FORMAT_UYVP:
      info->stride[0] = GST_ROUND_UP_4 ((width * 2 * 5 + 3) / 4);
      info->offset[0] = 0;
      info->size = info->stride[0] * height;
      break;
    case GST_VIDEO_FORMAT_RGB8_PALETTED:
      info->stride[0] = GST_ROUND_UP_4 (width);
      info->offset[0] = 0;
      info->size = info->stride[0] * height;
      break;
    case GST_VIDEO_FORMAT_IYU1:
      info->stride[0] = GST_ROUND_UP_4 (GST_ROUND_UP_4 (width) +
          GST_ROUND_UP_4 (width) / 2);
      info->offset[0] = 0;
      info->size = info->stride[0] * height;
      break;
    case GST_VIDEO_FORMAT_ARGB64:
    case GST_VIDEO_FORMAT_AYUV64:
      info->stride[0] = width * 8;
      info->offset[0] = 0;
      info->size = info->stride[0] * height;
      break;
    case GST_VIDEO_FORMAT_I420:
    case GST_VIDEO_FORMAT_YV12:        /* same as I420, but plane 1+2 swapped */
      info->stride[0] = GST_ROUND_UP_4 (width);
      info->stride[1] = GST_ROUND_UP_4 (GST_ROUND_UP_2 (width) / 2);
      info->stride[2] = info->stride[1];
      info->offset[0] = 0;
      info->offset[1] = info->stride[0] * GST_ROUND_UP_2 (height);
      info->offset[2] = info->offset[1] +
          info->stride[1] * (GST_ROUND_UP_2 (height) / 2);
      info->size = info->offset[2] +
          info->stride[2] * (GST_ROUND_UP_2 (height) / 2);
      break;
    case GST_VIDEO_FORMAT_Y41B:
      info->stride[0] = GST_ROUND_UP_4 (width);
      info->stride[1] = GST_ROUND_UP_16 (width) / 4;
      info->stride[2] = info->stride[1];
      info->offset[0] = 0;
      info->offset[1] = info->stride[0] * height;
      info->offset[2] = info->offset[1] + info->stride[1] * height;
      /* simplification of ROUNDUP4(w)*h + 2*((ROUNDUP16(w)/4)*h */
      info->size = (info->stride[0] + (GST_ROUND_UP_16 (width) / 2)) * height;
      break;
    case GST_VIDEO_FORMAT_Y42B:
      info->stride[0] = GST_ROUND_UP_4 (width);
      info->stride[1] = GST_ROUND_UP_8 (width) / 2;
      info->stride[2] = info->stride[1];
      info->offset[0] = 0;
      info->offset[1] = info->stride[0] * height;
      info->offset[2] = info->offset[1] + info->stride[1] * height;
      /* simplification of ROUNDUP4(w)*h + 2*(ROUNDUP8(w)/2)*h */
      info->size = (info->stride[0] + GST_ROUND_UP_8 (width)) * height;
      break;
    case GST_VIDEO_FORMAT_Y444:
      info->stride[0] = GST_ROUND_UP_4 (width);
      info->stride[1] = info->stride[0];
      info->stride[2] = info->stride[0];
      info->offset[0] = 0;
      info->offset[1] = info->stride[0] * height;
      info->offset[2] = info->offset[1] * 2;
      info->size = info->stride[0] * height * 3;
      break;
    case GST_VIDEO_FORMAT_NV12:
    case GST_VIDEO_FORMAT_NV21:
      info->stride[0] = GST_ROUND_UP_4 (width);
      info->stride[1] = info->stride[0];
      info->offset[0] = 0;
      info->offset[1] = info->stride[0] * GST_ROUND_UP_2 (height);
      info->size = info->stride[0] * GST_ROUND_UP_2 (height) * 3 / 2;
      break;
    case GST_VIDEO_FORMAT_A420:
      info->stride[0] = GST_ROUND_UP_4 (width);
      info->stride[1] = GST_ROUND_UP_4 (GST_ROUND_UP_2 (width) / 2);
      info->stride[2] = info->stride[1];
      info->stride[3] = info->stride[0];
      info->offset[0] = 0;
      info->offset[1] = info->stride[0] * GST_ROUND_UP_2 (height);
      info->offset[2] = info->offset[1] +
          info->stride[1] * (GST_ROUND_UP_2 (height) / 2);
      info->offset[3] = info->offset[2] +
          info->stride[2] * (GST_ROUND_UP_2 (height) / 2);
      info->size = info->offset[3] + info->stride[0];
      break;
    case GST_VIDEO_FORMAT_YUV9:
    case GST_VIDEO_FORMAT_YVU9:
      info->stride[0] = GST_ROUND_UP_4 (width);
      info->stride[1] = GST_ROUND_UP_4 (GST_ROUND_UP_4 (width) / 4);
      info->stride[2] = info->stride[1];
      info->offset[0] = 0;
      info->offset[1] = info->stride[0] * height;
      info->offset[2] = info->offset[1] +
          info->stride[1] * (GST_ROUND_UP_4 (height) / 4);
      info->size = info->offset[2] +
          info->stride[2] * (GST_ROUND_UP_4 (height) / 4);
      break;
    default:
      if (GST_VIDEO_FORMAT_INFO_IS_COMPLEX (info->finfo))
        break;
      GST_ERROR ("invalid format");
      g_warning ("invalid format");
      break;
  }
  return 0;
}
