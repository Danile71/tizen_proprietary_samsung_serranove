/*
 * gstmshttpsrc
 *
 * Copyright (c) 2000 - 2014 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Alternatively, the contents of this file may be used under the
 * GNU Lesser General Public License Version 2.1 (the "LGPL"), in
 * which case the following provisions apply instead of the ones
 * mentioned above:
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

#ifndef __GST_MSHTTP_SRC_H__
#define __GST_MSHTTP_SRC_H__

#include <gst/gst.h>
#include <gst/base/gstadapter.h>

G_BEGIN_DECLS
#define GST_TYPE_MSHTTP_SRC \
  (gst_mshttpsrc_get_type())
#define GST_MSHTTP_SRC(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_MSHTTP_SRC,GstMSHTTPSrc))
#define GST_MSHTTP_SRC_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_MSHTTP_SRC,GstMSHTTPSrcClass))
#define GST_IS_MSHTTP_SRC(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_MSHTTP_SRC))
#define GST_IS_MSHTTP_SRC_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_MSHTTP_SRC))

typedef struct _GstMSHTTPSrcClass GstMSHTTPSrcClass;

/* Fragment buffer to be stored in sorted queue */
typedef struct _GstMSHTTPBuffer
{
  guint  index;                       // Fragment index of the file to which this buffer belongs to
  GstBuffer *buffer;                  // Actual buffer of the fragment
}GstMSHTTPBuffer;

/* Fragments fetcher which will be assigned with a fragment index to be downloaded using souphttpsrc */
typedef struct _GstMSHTTPFetcher
{
  /* Pipeline elements */
  GstElement *fetcher;                // Fetcher element
  GstBus *fetcher_bus;                // Fetcher bus
  GstPad *fetcherpad;                 // Fetcher pad
  GMutex *fetcher_lock;               // Fetcher lock to make fetcher thread safe
  GCond *fetcher_cond;                // Fetcher condition to make fetcher thread safe
  GMutex *cond_lock;                  // Fetcher condition lock to make fetcher thread safe
  /* Buffer elements */
  gchar *location;                    // URI of the file to be fetched
  guint64 range_size;                  // Fetcher fragment size to be downloaded
  gint64 range_downloaded;            // Fetcher fragment size which got downloaded upto now
  guint buf_index_cur;                // Fetcher fragment index which is requested currently
  guint buf_index_prev;               // Fetcher fragment index which was requested previously whose downloading is completed
  GstAdapter *download;               // Downloaded fragment buffers will be stored it this adapter
  /* Maintenance flags */
  gboolean fetcher_error;             // Flag to be used for error checking of fetcher
  gboolean stopping_fetcher;          // Flag to be used for checking the state of fetcher
  gboolean cancelled;                 // Flag to be used for closing fetcher properly
  gboolean is_flushed;                // Flag to be used for flush operation
  gboolean end_of_sequence;           // EOS flag of the fetcher
  gboolean is_active;                 // Is active flag which will be set by update thread based on current fragment index

  gpointer *parent;                   // Pointer to Parent(GstMSHTTPSrc)
}GstMSHTTPFetcher;

/**
 * GstMSHTTPSrc:
 *
 * Opaque #GstMSHTTPSrc data structure.
 */
typedef struct _GstMSHTTPSrc
{
  GstElement parent;                  // Pointer to Parent
  GstMSHTTPFetcher *fetcher;          // Array of fetcher elements
  GstPad *srcpad;                     // Src pad
  /* Properties */
  gchar *user_agent;                  // User-Agent HTTP header
  gboolean automatic_redirect;        // Follow redirects
  gchar *proxy;                       // HTTP proxy URI
  gchar *user_id;                     // Authentication user id for location URI
  gchar *user_pw;                     // Authentication user password for location URI
  gchar *proxy_id;                    // Authentication user id for proxy URI
  gchar *proxy_pw;                    // Authentication user password for proxy URI
  gchar **cookies;                    // HTTP request cookies
  gboolean iradio_mode;               // Internet radio mode
  gboolean is_live;                   // Is live mode
  GstStructure *extra_headers;        // Extra HTTP headers
  guint timeout;                      // Time out for any HTTP response
  guint uri_count;                    // URI count if multiple URI are available
  GList *uri_list;                    // URI list
  /* Updates thread */
  GThread *updates_thread;            // Thread for checking the available data in fetcher adapters and pushing them into queue
  GMutex *thread_lock;                // Thread lock to make it thread safe
  GCond *thread_cond;                 // Signals the thread to quit from lock
  gboolean thread_return;             // Instructs the task function to return after the thread is returned
  GMutex *index_lock;                 // Mutex lock to make index thread safe
  /* Task elements */
  GstTask *task;                      // Task element for running a task function running on srcpad
  GStaticRecMutex task_lock;          // RecMutex to make task function thread safe
  GQueue *queue;                      // Queue storing the fetched fragments
  GMutex *queue_lock;                 // Mutex lock to make queue thread safe
  /* File elements */
  guint64 total_file_size;            // Total file size (in Bytes) which is currently getting downloaded
  guint64 total_download;             // File size got downloaded upto now
  guint session_count;                // number of sessions to be made for buffering
  guint64 range_size;                  // Fetcher fragment size to be downloaded
  gint buf_index_req;                 // Fragment index requested currently
  gint buf_index_push;                // Fragment index pushed currently
  /* Maintenance flags */
  gboolean end_of_playlist;           // EOS flag if all the fragments got downloaded
  gint eos_count;                     // EOS count of the fetchers, all the fetchers should give EOS before setting end_of_playlist
  gboolean is_flushed;                // Flag to be used for flush operation

  GstEvent *seek_event;               // Current seek event is stored to check for duplicate seek events

}GstMSHTTPSrc;

struct _GstMSHTTPSrcClass
{
  GstElementClass parent_class;
};

GType gst_mshttpsrc_get_type (void);

G_END_DECLS
#endif /* __GST_MSHTTP_SRC_H__ */
