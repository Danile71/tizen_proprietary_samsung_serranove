/*
 * evastextoverlay
 *
 * Copyright (c) 2000 - 2011 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Contact: Jawahir Abul <jawahir.abul@samsung.com>,
 *                Manoj Kumar K <manojkumar.k@samsung.com>
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the
 * Free Software Foundation; either version 2.1 of the License, or (at your option)
 * any later version.
 *
 * This library is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public
 * License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library; if not, write to the Free Software Foundation, Inc., 51
 * Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 *
 */

#ifndef __GST_EVASTEXTOVERLAY_H__
#define __GST_EVASTEXTOVERLAY_H__

#include <gst/gst.h>
#include <gst/base/gstbasetransform.h>

G_BEGIN_DECLS

/* #defines don't like whitespacey bits */
#define GST_TYPE_EVASTEXTOVERLAY \
  (gst_evastextoverlay_get_type())
#define GST_EVASTEXTOVERLAY(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_EVASTEXTOVERLAY,Gstevastextoverlay))
#define GST_EVASTEXTOVERLAY_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_EVASTEXTOVERLAY,GstevastextoverlayClass))
#define GST_IS_EVASTEXTOVERLAY(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_EVASTEXTOVERLAY))
#define GST_IS_EVASTEXTOVERLAY_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_EVASTEXTOVERLAY))

typedef struct _Gstevastextoverlay      Gstevastextoverlay;
typedef struct _GstevastextoverlayClass GstevastextoverlayClass;

struct _Gstevastextoverlay
{
  GstElement element;

  GstPad      *sinkpad;
  GstPad      *srcpad;

  Evas        *canvas;
  gint        buffer_depth;
  gint        width;
  gint        height;

  gint        xpos;
  gint        ypos;
  gboolean    enable_overlay;
  gchar       *text;
  gint        size;
  gint        style;
  gchar       *font;
  guint       color;

  Evas_Colorspace       colorspace;
};

struct _GstevastextoverlayClass
{
  GstElementClass parent_class;
};

GType gst_evastextoverlay_get_type (void);

G_END_DECLS
#endif /* __GST_EVASTEXTOVERLAY_H__ */
