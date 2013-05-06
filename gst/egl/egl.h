/*
 * GStreamer EGL Library 
 * Copyright (C) 2013 Fluendo S.A.
 *   @author: Josep Torra <josep@fluendo.com>
 * *
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
 * Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#ifndef __GST_EGL_H__
#define __GST_EGL_H__

#include <gst/gst.h>

#include <EGL/egl.h>
#include <EGL/eglext.h>

G_BEGIN_DECLS

typedef struct _GstEGLImageMemory GstEGLImageMemory;
typedef struct _GstEGLImageMemoryPool GstEGLImageMemoryPool;
typedef struct _GstEGLDisplay GstEGLDisplay;
typedef void (*GstDestroyNotifyEGLImageMemoryPool) (GstEGLImageMemoryPool *
    pool, gpointer user_data);

/* GstEGLImageMemory handling */
GstEGLImageMemory *gst_egl_image_memory_ref (GstEGLImageMemory * mem);
GstEGLImageMemory *gst_egl_image_memory_unref (GstEGLImageMemory * mem);
EGLImageKHR gst_egl_image_memory_get_image (GstEGLImageMemory * mem);

/* GstEGLImageMemoryPool handling */
#define GST_TYPE_EGL_IMAGE_MEMORY_POOL (gst_egl_image_memory_pool_get_type())
GType gst_egl_image_memory_pool_get_type (void);

GstEGLImageMemoryPool *gst_egl_image_memory_pool_new (gint size,
    GstEGLDisplay * display, gpointer user_data,
    GstDestroyNotifyEGLImageMemoryPool destroy_data);
GstEGLImageMemoryPool *gst_egl_image_memory_pool_ref (GstEGLImageMemoryPool *
    pool);
GstEGLImageMemoryPool *gst_egl_image_memory_pool_unref (GstEGLImageMemoryPool *
    pool);
gint gst_egl_image_memory_pool_get_size (GstEGLImageMemoryPool * pool);

gboolean gst_egl_image_memory_pool_set_resources (GstEGLImageMemoryPool * pool,
    gint idx, EGLClientBuffer client_buffer, EGLImageKHR image);

gboolean gst_egl_image_memory_pool_get_resources (GstEGLImageMemoryPool * pool,
    gint idx, EGLClientBuffer * client_buffer, EGLImageKHR * image);

GstEGLDisplay *gst_egl_image_memory_pool_get_display (GstEGLImageMemoryPool *
    pool);
GList *gst_egl_image_memory_pool_get_images (GstEGLImageMemoryPool * pool);
void gst_egl_image_memory_pool_set_active (GstEGLImageMemoryPool * pool,
    gboolean active);
void gst_egl_image_memory_pool_wait_released (GstEGLImageMemoryPool * pool);
GstBuffer *gst_egl_image_memory_pool_acquire_buffer (GstEGLImageMemoryPool *
    pool, gint idx, gpointer user_data, GDestroyNotify destroy_data);

/* EGLDisplay wrapper with refcount, connection is closed after last ref is gone */
GstEGLDisplay * gst_egl_display_new (EGLDisplay display, gpointer user_data,
    GDestroyNotify destroy_data);
GstEGLDisplay *gst_egl_display_ref (GstEGLDisplay * display);
void gst_egl_display_unref (GstEGLDisplay * display);
EGLDisplay gst_egl_display_get (GstEGLDisplay * display);

/* Helper functions to manage pool requests */
GstMessage *gst_message_new_need_egl_pool (GstObject * src, gint size,
    gint width, gint height);
void gst_message_parse_need_egl_pool (GstMessage * message, gint * size,
    gint * width, gint * height);

G_END_DECLS

#endif /* __GST_EGL_H__ */
