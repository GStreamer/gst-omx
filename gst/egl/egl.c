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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#if defined(USE_EGL_RPI) && defined (__GNUC__)

#ifndef __VCCOREVER__
#define __VCCOREVER__ 0x04000000
#endif

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wredundant-decls"
#pragma GCC optimize ("gnu89-inline")
#endif

#define EGL_EGLEXT_PROTOTYPES
#include <gst/egl/egl.h>

#if defined(USE_EGL_RPI) && defined (__GNUC__)
#pragma GCC reset_options
#pragma GCC diagnostic pop
#endif

#define DEBUG_MAGIC_CHECK 0

#if DEBUG_MAGIC_CHECK
#define EGLMEMORY_MAGIC 0x5fce5fce
#define EGLMEMORY_POOL_MAGIC 0x5fce9001

#define EGLMEMORY_POOL_MAGIC_ASSERT(pool)  G_STMT_START {  \
  g_assert (pool->magic == EGLMEMORY_POOL_MAGIC);          \
} G_STMT_END
#define EGLMEMORY_MAGIC_ASSERT(mem)  G_STMT_START {        \
  g_assert (mem->magic == EGLMEMORY_MAGIC);                \
} G_STMT_END
#else
#define EGLMEMORY_POOL_MAGIC_ASSERT(pool)  G_STMT_START{ }G_STMT_END
#define EGLMEMORY_MAGIC_ASSERT(mem)  G_STMT_START{ }G_STMT_END
#endif

struct _GstEGLImageMemory
{
  volatile gint refcount;

#if DEBUG_MAGIC_CHECK
  guint32 magic;
#endif

  EGLClientBuffer client_buffer;
  EGLImageKHR image;

  gpointer user_data;
  GDestroyNotify destroy_data;

  GstEGLImageMemoryPool *pool;
};

#define GST_EGL_IMAGE_MEMORY(mem) ((GstEGLImageMemory *)(mem))

struct _GstEGLImageMemoryPool
{
  volatile gint refcount;

#if DEBUG_MAGIC_CHECK
  guint32 magic;
#endif

  GMutex *lock;
  GCond *cond;

  GstEGLDisplay *display;
  GstEGLImageMemory *memory;

  gint size;
  volatile gint unused;
  volatile gint active;

  gpointer user_data;
  GstDestroyNotifyEGLImageMemoryPool destroy_data;
};

#define GST_EGL_IMAGE_MEMORY_POOL_LOCK(pool) G_STMT_START {           \
  g_mutex_lock (pool->lock);                                          \
} G_STMT_END

#define GST_EGL_IMAGE_MEMORY_POOL_UNLOCK(pool) G_STMT_START {         \
  g_mutex_unlock (pool->lock);                                        \
} G_STMT_END

#define GST_EGL_IMAGE_MEMORY_POOL_WAIT_RELEASED(pool) G_STMT_START {  \
  g_cond_wait (pool->cond, pool->lock);                               \
} G_STMT_END

#define GST_EGL_IMAGE_MEMORY_POOL_SIGNAL_UNUSED(pool) G_STMT_START {  \
  g_cond_signal (pool->cond);                                         \
} G_STMT_END

#define GST_EGL_IMAGE_MEMORY_POOL_IS_FULL(pool) (g_atomic_int_get (&pool->unused) == 0)
#define GST_EGL_IMAGE_MEMORY_POOL_IS_EMPTY(pool) (g_atomic_int_get (&pool->unused) == pool->size)

#define GST_EGL_IMAGE_MEMORY_POOL_SET_ACTIVE(pool,active) \
  g_atomic_int_set (&pool->active, active)
#define GST_EGL_IMAGE_MEMORY_POOL_IS_ACTIVE(pool) (g_atomic_int_get (&pool->active) == TRUE)

struct _GstEGLDisplay
{
  volatile gint refcount;

  EGLDisplay display;

  gpointer user_data;
  GDestroyNotify destroy_data;
};

/**
 * gst_egl_image_memory_ref
 * @mem: a #GstEGLImageMemory
 *
 * Increase the refcount of @mem.
 *
 */
GstEGLImageMemory *
gst_egl_image_memory_ref (GstEGLImageMemory * mem)
{
  g_return_val_if_fail (mem != NULL, NULL);

  EGLMEMORY_MAGIC_ASSERT (mem);

  g_atomic_int_inc (&mem->refcount);

  return mem;
}

static void
gst_egl_image_memory_free (GstEGLImageMemory * mem)
{
  GstEGLImageMemoryPool *pool = mem->pool;

  /* We grab the pool lock as we will make changes to number of unused
   * memories. */
  GST_EGL_IMAGE_MEMORY_POOL_LOCK (pool);

  /* We have one more unused memory */
  g_atomic_int_inc (&pool->unused);

  /* Wake up potential waiters */
  GST_EGL_IMAGE_MEMORY_POOL_SIGNAL_UNUSED (pool);

  GST_EGL_IMAGE_MEMORY_POOL_UNLOCK (pool);

  if (mem->destroy_data) {
    mem->destroy_data (mem->user_data);
  }

  /* Remove our ref to the pool. That could destroy the pool so it is
   * important to do that when we released the lock */
  mem->pool = gst_egl_image_memory_pool_unref (pool);
}

/**
 * gst_egl_image_memory_unref:
 * @mem: a #GstEGLImageMemory
 *
 * Unref @mem and when the refcount reaches 0 frees the resources
 * and return NULL.
 *
 */
GstEGLImageMemory *
gst_egl_image_memory_unref (GstEGLImageMemory * mem)
{
  g_return_val_if_fail (mem != NULL, NULL);

  EGLMEMORY_MAGIC_ASSERT (mem);

  if (g_atomic_int_dec_and_test (&mem->refcount)) {
    gst_egl_image_memory_free (mem);
    mem = NULL;
  }

  return mem;
}

/**
 * gst_egl_image_memory_get_image:
 * @mem: a #GstEGLImageMemory
 *
 * Gives the #EGLImageKHR used by the #GstEGLImageMemory.
 *
 */
EGLImageKHR
gst_egl_image_memory_get_image (GstEGLImageMemory * mem)
{
  return mem->image;
}

/**
 * gst_egl_image_memory_pool_new:
 * @size: a number of memories managed by the pool
 * @display: a #GstEGLDisplay display
 * @user_data: user data passed to the callback
 * @destroy_data: #GstDestroyNotifyEGLImageMemoryPool for user_data
 *
 * Create a new GstEGLImageMemoryPool instance with the provided images.
 *
 * Returns: a new #GstEGLImageMemoryPool
 *
 */
GstEGLImageMemoryPool *
gst_egl_image_memory_pool_new (gint size, GstEGLDisplay * display,
    gpointer user_data, GstDestroyNotifyEGLImageMemoryPool destroy_data)
{
  GstEGLImageMemoryPool *pool;

  pool = g_new0 (GstEGLImageMemoryPool, 1);

  pool->refcount = 1;
  pool->lock = g_mutex_new ();
  pool->cond = g_cond_new ();
  pool->display = gst_egl_display_ref (display);
  pool->memory = (GstEGLImageMemory *) g_new0 (GstEGLImageMemory, size);
  pool->size = pool->unused = size;
  pool->user_data = user_data;
  pool->destroy_data = destroy_data;

#if DEBUG_MAGIC_CHECK
  {
    gint i;
    pool->magic = EGLMEMORY_POOL_MAGIC;
    for (i = 0; i < size; i++) {
      GstEGLImageMemory *mem = &pool->memory[i];
      mem->magic = EGLMEMORY_MAGIC;
    }
  }
#endif

  return pool;
}

/**
 * gst_egl_image_memory_pool_ref:
 * @pool: a #GstEGLImageMemoryPool
 *
 * Increase the refcount of @pool.
 *
 */
GstEGLImageMemoryPool *
gst_egl_image_memory_pool_ref (GstEGLImageMemoryPool * pool)
{
  g_return_val_if_fail (pool != NULL, NULL);

  EGLMEMORY_POOL_MAGIC_ASSERT (pool);

  g_atomic_int_inc (&pool->refcount);

  return pool;
}

static void
gst_egl_image_memory_pool_free (GstEGLImageMemoryPool * pool)
{
  if (g_atomic_int_get (&pool->unused) != pool->size) {
    g_critical ("refcounting problem detected, some memory is still in use");
  }

  if (pool->destroy_data) {
    pool->destroy_data (pool, pool->user_data);
  }

  gst_egl_display_unref (pool->display);
  g_mutex_free (pool->lock);
  g_cond_free (pool->cond);

  g_free (pool->memory);
  g_free (pool);
}

/**
 * gst_egl_image_memory_pool_unref:
 * @pool: a #GstEGLImageMemoryPool
 *
 * Unref @pool and when the refcount reaches 0 frees the resources
 * and return NULL.
 *
 */
GstEGLImageMemoryPool *
gst_egl_image_memory_pool_unref (GstEGLImageMemoryPool * pool)
{
  g_return_val_if_fail (pool != NULL, NULL);

  EGLMEMORY_POOL_MAGIC_ASSERT (pool);

  if (g_atomic_int_dec_and_test (&pool->refcount)) {
    gst_egl_image_memory_pool_free (pool);
    pool = NULL;
  }
  return pool;
}

/**
 * gst_egl_image_memory_pool_get_size:
 * @pool: a #GstEGLImageMemoryPool
 *
 * Gives the number of memories that are managed by the pool.
 *
 */
gint
gst_egl_image_memory_pool_get_size (GstEGLImageMemoryPool * pool)
{
  g_return_val_if_fail (pool != NULL, 0);

  return pool->size;
}

/**
 * gst_egl_image_memory_pool_set_resources:
 * @pool: a #GstEGLImageMemoryPool
 * @idx: memory index
 * @client_buffer: an #EGLClientBuffer to store in the pool
 * @image: an #EGLImageKHR to store in the pool.
 *
 * Stores @client_buffer and @image at @idx memory slot.
 *
 */
gboolean
gst_egl_image_memory_pool_set_resources (GstEGLImageMemoryPool * pool, gint idx,
    EGLClientBuffer client_buffer, EGLImageKHR image)
{
  GstEGLImageMemory *mem;

  g_return_val_if_fail (pool != NULL, FALSE);
  g_return_val_if_fail (idx >= 0 && idx < pool->size, FALSE);

  mem = &pool->memory[idx];
  mem->client_buffer = client_buffer;
  mem->image = image;

  return TRUE;
}

/**
 * gst_egl_image_memory_pool_get_resources:
 * @pool: a #GstEGLImageMemoryPool
 * @idx: memory index
 * @client_buffer: (out) (allow-none): the #EGLClientBuffer at @idx 
 * @image: (out) (allow-none): the #EGLImageKHR at @idx
 *
 * Retrieves @client_buffer and @image at @idx memory slot.
 *
 */
gboolean
gst_egl_image_memory_pool_get_resources (GstEGLImageMemoryPool * pool, gint idx,
    EGLClientBuffer * client_buffer, EGLImageKHR * image)
{
  GstEGLImageMemory *mem;

  g_return_val_if_fail (pool != NULL, FALSE);
  g_return_val_if_fail (idx >= 0 && idx < pool->size, FALSE);

  mem = &pool->memory[idx];

  if (client_buffer)
    *client_buffer = mem->client_buffer;

  if (image)
    *image = mem->image;

  return TRUE;
}

/**
 * gst_egl_image_memory_pool_get_display:
 * @pool: a #GstEGLImageMemoryPool
 *
 * Provides a reference to the #GstEGLDisplay used by the pool
 */
GstEGLDisplay *
gst_egl_image_memory_pool_get_display (GstEGLImageMemoryPool * pool)
{
  g_return_val_if_fail (pool != NULL, NULL);

  EGLMEMORY_POOL_MAGIC_ASSERT (pool);

  g_return_val_if_fail (pool->display != NULL, NULL);

  return gst_egl_display_ref (pool->display);
}

/**
 * gst_egl_image_memory_pool_get_images:
 * @pool: a #GstEGLImageMemoryPool
 *
 * Provides a #GList of EGL images
 */
GList *
gst_egl_image_memory_pool_get_images (GstEGLImageMemoryPool * pool)
{
  gint i;
  GList *images = NULL;

  g_return_val_if_fail (pool != NULL, NULL);

  for (i = 0; i < pool->size; i++) {
    GstEGLImageMemory *mem = &pool->memory[i];
    images = g_list_append (images, mem->image);
  }

  return images;
}

/**
 * gst_egl_image_memory_pool_set_active:
 * @pool: a #GstEGLImageMemoryPool
 * @active: a #gboolean that indicates the pool is active
 *
 * Change @pool active state and unblocks any wait condition if needed.
 *
 */
void
gst_egl_image_memory_pool_set_active (GstEGLImageMemoryPool * pool,
    gboolean active)
{
  g_return_if_fail (pool != NULL);

  EGLMEMORY_POOL_MAGIC_ASSERT (pool);

  GST_EGL_IMAGE_MEMORY_POOL_SET_ACTIVE (pool, active);

  if (!active) {
    GST_EGL_IMAGE_MEMORY_POOL_SIGNAL_UNUSED (pool);
  }
}

/**
 * gst_egl_image_memory_pool_wait_released:
 * @pool: a #GstEGLImageMemoryPool
 *
 * Waits until none of the memory is in use.
 *
 */
void
gst_egl_image_memory_pool_wait_released (GstEGLImageMemoryPool * pool)
{
  g_return_if_fail (pool != NULL);

  EGLMEMORY_POOL_MAGIC_ASSERT (pool);

  GST_EGL_IMAGE_MEMORY_POOL_LOCK (pool);
  while (!GST_EGL_IMAGE_MEMORY_POOL_IS_EMPTY (pool)) {
    GST_EGL_IMAGE_MEMORY_POOL_WAIT_RELEASED (pool);
  }
  GST_EGL_IMAGE_MEMORY_POOL_UNLOCK (pool);
}

/**
 * gst_egl_image_memory_pool_acquire_buffer:
 * @pool: a #GstEGLImageMemoryPool
 * @idx: ordinal that specifies a #GstEGLImageMemory in the pool
 * @user_data: user data passed to the callback
 * @destroy_data: #GDestroyNotify for user_data
 * 
 * Provides an specified #GstEGLImageMemory wrapped by a #GstBuffer.
 *
 */
GstBuffer *
gst_egl_image_memory_pool_acquire_buffer (GstEGLImageMemoryPool * pool,
    gint idx, gpointer user_data, GDestroyNotify destroy_data)
{
  GstBuffer *ret;
  GstEGLImageMemory *mem;

  g_return_val_if_fail (pool != NULL, NULL);
  g_return_val_if_fail (idx >= 0 && idx < pool->size, NULL);

  GST_EGL_IMAGE_MEMORY_POOL_LOCK (pool);
  if (!GST_EGL_IMAGE_MEMORY_POOL_IS_ACTIVE (pool)) {
    GST_EGL_IMAGE_MEMORY_POOL_UNLOCK (pool);
    return NULL;
  }

  mem = &pool->memory[idx];

  g_atomic_int_add (&pool->unused, -1);
  g_atomic_int_set (&mem->refcount, 1);
  mem->pool = gst_egl_image_memory_pool_ref (pool);
  mem->user_data = user_data;
  mem->destroy_data = destroy_data;

  ret = gst_buffer_new ();
  GST_BUFFER_MALLOCDATA (ret) = GST_BUFFER_DATA (ret) = (guint8 *) mem;
  GST_BUFFER_FREE_FUNC (ret) = (GFreeFunc) gst_egl_image_memory_unref;
  GST_BUFFER_SIZE (ret) = sizeof (GstEGLImageMemory);

  GST_EGL_IMAGE_MEMORY_POOL_UNLOCK (pool);

  return ret;
}

G_DEFINE_BOXED_TYPE (GstEGLImageMemoryPool, gst_egl_image_memory_pool,
    (GBoxedCopyFunc) gst_egl_image_memory_pool_ref,
    (GBoxedFreeFunc) gst_egl_image_memory_pool_unref);

/**
 * gst_egl_display_new:
 * @display: a #EGLDisplay display
 * @user_data: user data passed to the callback
 * @destroy_data: #GDestroyNotify for user_data
 *
 * Create a new #GstEGLDisplay that wraps and refcount @display.
 *
 * Returns: a new #GstEGLDisplay
 *
 */
GstEGLDisplay *
gst_egl_display_new (EGLDisplay display, gpointer user_data,
    GDestroyNotify destroy_data)
{
  GstEGLDisplay *gdisplay;

  gdisplay = g_slice_new (GstEGLDisplay);
  gdisplay->refcount = 1;
  gdisplay->display = display;
  gdisplay->user_data = user_data;
  gdisplay->destroy_data = destroy_data;

  return gdisplay;
}

/**
 * gst_egl_display_ref
 * @display: a #GstEGLDisplay
 *
 * Increase the refcount of @display.
 *
 */
GstEGLDisplay *
gst_egl_display_ref (GstEGLDisplay * display)
{
  g_return_val_if_fail (display != NULL, NULL);

  g_atomic_int_inc (&display->refcount);

  return display;
}

/**
 * gst_egl_display_unref:
 * @display: a #GstEGLDisplay
 *
 * Decrease the refcount of @display and calls provided destroy function on
 * last reference.
 *
 */
void
gst_egl_display_unref (GstEGLDisplay * display)
{
  g_return_if_fail (display != NULL);

  if (g_atomic_int_dec_and_test (&display->refcount)) {
    if (display->destroy_data) {
      display->destroy_data (display->user_data);
    }
    g_slice_free (GstEGLDisplay, display);
  }
}

/**
 * gst_egl_display_get
 * @display: a #GstEGLDisplay
 *
 * Gives the #EGLDisplay wrapped.
 *
 */
EGLDisplay
gst_egl_display_get (GstEGLDisplay * display)
{
  g_return_val_if_fail (display != NULL, EGL_NO_DISPLAY);

  return display->display;
}

/**
 * gst_message_new_need_egl_pool:
 * @src: (transfer none): The object originating the message.
 * @size: number of required images
 * @width: width in pixels of the images
 * @height: height in pixels of the images
 *
 * Create a new need egl pool message. This message is posted to 
 * require the application to provide a #GstEGLImageMemoryPool through
 * the pool property in the @src element.
 *
 * Returns: (transfer full): The new need egl pool message.
 *
 */
GstMessage *
gst_message_new_need_egl_pool (GstObject * src, gint size, gint width,
    gint height)
{
  GstStructure *structure;

  structure = gst_structure_new ("need-egl-pool",
      "size", G_TYPE_INT, size, "width", G_TYPE_INT, width,
      "height", G_TYPE_INT, height, NULL);

  return gst_message_new_custom (GST_MESSAGE_ELEMENT, src, structure);
}

/**
 * gst_message_parse_need_egl_pool:
 * @message: A valid need_egl_pool #GstMessage.
 * @size: (out) (allow-none): number of required images
 * @width: (out) (allow-none): width in pixels of the images
 * @height: (out) (allow-none): height in pixels of the images
 *
 * Extracts the details from the GstMessage. see also
 * gst_message_new_need_egl_pool().
 *
 */
void
gst_message_parse_need_egl_pool (GstMessage * message, gint * size,
    gint * width, gint * height)
{
  g_return_if_fail (GST_IS_MESSAGE (message));
  g_return_if_fail (GST_MESSAGE_TYPE (message) == GST_MESSAGE_ELEMENT);
  g_return_if_fail (gst_structure_has_name (message->structure,
          "need-egl-pool"));

  if (size)
    *size =
        g_value_get_int (gst_structure_get_value (message->structure, "size"));

  if (width)
    *width =
        g_value_get_int (gst_structure_get_value (message->structure, "width"));

  if (height)
    *height =
        g_value_get_int (gst_structure_get_value (message->structure,
            "height"));
}
