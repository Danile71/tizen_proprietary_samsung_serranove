/* GStreamer
 * Copyright (C) <1999> Erik Walthinsen <omega@cse.ogi.edu>
 * Copyright (C) <2003> David Schleef <ds@schleef.org>
 * Copyright (C) <2009> Young-Ho Cha <ganadist@gmail.com>
 * Copyright (c) 2000 - 2014 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Contact: Nam Jeong-yoon <just.nam@samsung.com>
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

#ifndef _GST_SUBRENDER_H_
#define _GST_SUBRENDER_H_

//#define DUMP_IMG

#include <unistd.h>

#include <gst/gst.h>
#include <pango/pangocairo.h>

/* tbm */
#include <tbm_bufmgr.h>

/* headers for drm and gem */
#include <exynos_drm.h>
#include <drm_fourcc.h>
#include <xf86drm.h>
#include <xf86drmMode.h>
#include <X11/Xmd.h>
#include <dri2/dri2.h>
#include <libdrm/drm.h>
#include <fcntl.h>


#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/XShm.h>
#include <X11/extensions/Xv.h>
#include <X11/extensions/Xvlib.h>
#include <X11/extensions/Xvproto.h>

G_BEGIN_DECLS

#define GST_TYPE_SUBRENDER   (gst_subrender_get_type())
#define GST_SUBRENDER(obj)   (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_SUBRENDER,GstSubrender))
#define GST_SUBRENDER_CLASS(klass)   (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_SUBRENDER,GstSubrenderClass))
#define GST_IS_SUBRENDER(obj)   (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_SUBRENDER))
#define GST_IS_SUBRENDER_CLASS(obj)   (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_SUBRENDER))
#define GST_SUBRENDER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GST_TYPE_SUBRENDER, GstSubrenderClass))

#define exit_if_fail(c) {if(!(c)){fprintf (stderr, "'%s' failed.\n",#c);exit (-1);}}

typedef struct _GstSubrender GstSubrender;
typedef struct _GstSubrenderClass GstSubrenderClass;

#define GST_TYPE_SUBRENDER_EDGE_MODE (gst_subrender_edge_mode_get_type())

static GType
gst_subrender_edge_mode_get_type(void)
{
  static GType subrender_edge_mode_type = 0;
  static const GEnumValue edge_mode_type[] = {
    { 0, "No Edge mode", "GST_SUBRENDER_EDGE_NO"},
    { 1, "Edge Raised mode", "GST_SUBRENDER_EDGE_RAISED"},
    { 2, "Edge Depressed mode", "GST_SUBRENDER_EDGE_DEPRESSED"},
    { 3, "Edge Uniform mode", "GST_SUBRENDER_EDGE_UNIFORM"},
    { 4, "Edge Drop shadow mode", "GST_SUBRENDER_EDGE_DROPSHADOW"},
    { 5, NULL, NULL},
  };

  if (!subrender_edge_mode_type) {
    subrender_edge_mode_type = g_enum_register_static("GstSubrenderEdgeModeType", edge_mode_type);
  }

  return subrender_edge_mode_type;
}

/**
 * GstSubrenderVAlign:
 * @GST_SUBRENDER_VALIGN_BASELINE: draw text on the baseline
 * @GST_SUBRENDER_VALIGN_BOTTOM: draw text on the bottom
 * @GST_SUBRENDER_VALIGN_TOP: draw test on top
 *
 * Vertical alignment of the text.
 */
typedef enum {
    GST_SUBRENDER_VALIGN_BASELINE,
    GST_SUBRENDER_VALIGN_BOTTOM,
    GST_SUBRENDER_VALIGN_TOP
} GstSubrenderVAlign;

/**
 * GstSubrenderHAlign:
 * @GST_SUBRENDER_HALIGN_LEFT: align text left
 * @GST_SUBRENDER_HALIGN_CENTER: align text center
 * @GST_SUBRENDER_HALIGN_RIGHT: align text right
 *
 * Horizontal alignment of the text.
 */
typedef enum {
    GST_SUBRENDER_HALIGN_LEFT,
    GST_SUBRENDER_HALIGN_CENTER,
    GST_SUBRENDER_HALIGN_RIGHT
} GstSubrenderHAlign;

/**
 * GstSubrenderLineAlign:
 * @GST_SUBRENDER_LINE_ALIGN_LEFT: lines are left-aligned
 * @GST_SUBRENDER_LINE_ALIGN_CENTER: lines are center-aligned
 * @GST_SUBRENDER_LINE_ALIGN_RIGHT: lines are right-aligned
 *
 * Alignment of text lines relative to each other
 */
typedef enum {
    GST_SUBRENDER_LINE_ALIGN_LEFT = PANGO_ALIGN_LEFT,
    GST_SUBRENDER_LINE_ALIGN_CENTER = PANGO_ALIGN_CENTER,
    GST_SUBRENDER_LINE_ALIGN_RIGHT = PANGO_ALIGN_RIGHT
} GstSubrenderLineAlign;

/**
 * GstSubrenderEdge:
 * @GST_SUBRENDER_EDGE_NO: no edge
 * @GST_SUBRENDER_EDGE_RAISED: edge is raised
 * @GST_SUBRENDER_EDGE_DEPRESSED: edge is depressed
 * @GST_SUBRENDER_EDGE_UNIFORM: edge is uniform
 * @GST_SUBRENDER_EDGE_DROPSHADOW: edge drop shadow
 *
 * Edge Properties of the text.
 */
typedef enum {
    GST_SUBRENDER_EDGE_NO,
    GST_SUBRENDER_EDGE_RAISED,
    GST_SUBRENDER_EDGE_DEPRESSED,
    GST_SUBRENDER_EDGE_UNIFORM,
    GST_SUBRENDER_EDGE_DROPSHADOW
} GstSubrenderEdge;

/**
 * edge color for subtitle
 */
typedef enum {
    EDGE_COLOR_BLACK,
    EDGE_COLOR_WHITE,
    EDGE_COLOR_SHADOW
} EdgeColor;

/**
 * GstSubender:
 *
 * subrender data structure.
 */
struct _GstSubrender
{
  GstElement            base_subrender;

  GstPad               *sinkpad, *srcpad;
  gint                  width;
  gint                  height;
  PangoLayout          *layout;
  gdouble               shadow_offset;
  gdouble               outline_offset;
  guchar               *text_image;
  gint                  image_width;
  gint                  image_height;
  gint                  baseline_y;
  gboolean              use_ARGB;

  GstSubrenderVAlign     valign;
  GstSubrenderHAlign     halign;
  GstSubrenderLineAlign  line_align;

  gint xpad;
  gint ypad;

  /* fourcc colorspace format */
  guint32 format;

  Display *dpy;

  Pixmap pixmap;
  void *pixmap_addr;
  /* for making pixmap buffer only once */
  gboolean is_buffer;

  /* subtitle format */
  gboolean is_subtitle_format;

  /* external display resolution */
  gint external_w;
  gint external_h;

  gboolean is_changed_size;

  /* font setting */
  guint font_color;
  guint font_bg_color;

  gboolean is_nbsp;
  gboolean skip_frame;

  gboolean ignore_markup;

  /* font edge */
  guint edge_mode;
  gdouble custom_edge_outline;
  gboolean use_custom_offset;
  gdouble custom_edge_offset_x;
  gdouble custom_edge_offset_y;
  EdgeColor edge_color;
};

struct _GstSubrenderClass
{
  GstElementClass parent_class;
  PangoContext *pango_context;
};

GType gst_subrender_get_type (void) G_GNUC_CONST;

G_END_DECLS

#endif
/* __GST_SUBRENDER_H */