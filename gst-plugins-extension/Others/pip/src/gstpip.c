/*
 * Pip
 *
 * Copyright (c) 2000 - 2014 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Contact: Jian He <rocket.he@samsung.com>
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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <stdio.h>
#include <gst/gst.h>
#include "gstpip.h"

//#define ENABLE_PIP_DUMP_BUFFER
#ifdef ENABLE_PIP_DUMP_BUFFER
#include <stdio.h>
static int
pip_data_dump (char *filename, void *data, int size){
    FILE *fp = NULL;
    fp = fopen (filename, "a+");
    if (!fp)
        return FALSE;
    fwrite (data, 1, size, fp);
    fclose (fp);
    return TRUE;
}
#endif


GST_DEBUG_CATEGORY (gst_pip_debug);
#define GST_CAT_DEFAULT gst_pip_debug

/*property for controlling GLM options*/
#define INSET_WINDOW_X_MAX 1920
#define INSET_WINDOW_X_MIN 0
#define INSET_WINDOW_X_DEFAULT 50
#define INSET_WINDOW_Y_MAX 1920
#define INSET_WINDOW_Y_MIN 0
#define INSET_WINDOW_Y_DEFAULT 50
#define INSET_WINDOW_W_MAX 1920
#define INSET_WINDOW_W_MIN 0
#define INSET_WINDOW_W_DEFAULT 320
#define INSET_WINDOW_H_MAX 1920
#define INSET_WINDOW_H_MIN 0
#define INSET_WINDOW_H_DEFAULT 240
#define INSET_WINDOW_PREVIEW_CROP_X_MAX 1920
#define INSET_WINDOW_PREVIEW_CROP_X_MIN 0
#define INSET_WINDOW_PREVIEW_CROP_X_DEFAULT 0
#define INSET_WINDOW_PREVIEW_CROP_Y_MAX 1920
#define INSET_WINDOW_PREVIEW_CROP_Y_MIN 0
#define INSET_WINDOW_PREVIEW_CROP_Y_DEFAULT 0
#define INSET_WINDOW_PREVIEW_CROP_W_MAX 1920
#define INSET_WINDOW_PREVIEW_CROP_W_MIN 0
#define INSET_WINDOW_PREVIEW_CROP_W_DEFAULT 640
#define INSET_WINDOW_PREVIEW_CROP_H_MAX 1920
#define INSET_WINDOW_PREVIEW_CROP_H_MIN 0
#define INSET_WINDOW_PREVIEW_CROP_H_DEFAULT 480
#define STYLE_TYPE_DEFAULT FILTER_VALUE_GD_NORMAL_PIP
#define ORIENTATION_TYPE_DEFAULT ORIENTATION_UP
#define INSET_WINDOW_DEVICE_DEFAULT 0
#define CAMERA_MODE_DEFAULT DUAL_CAMERA  /*To be change*/
#define EFFECT_TYPE_DEFAULT FILTER_VALUE_GS_BW  /*To be change*/
#define DOWNLOAD_EFFECT_TYPE_DEFAULT ""  /*To be change*/
#define DEFAULT_PIP_SIGNAL_STILL_CAPTURE           FALSE
#define DEFAULT_HIDE_INSET_WINDOW           FALSE
#define DEFAULT_ENABLE_FADING               TRUE
#define DEFAULT_PIP_CAPTURE_WIDTH 1920
#define DEFAULT_PIP_CAPTURE_HEIGHT 1080
#define GST_TYPE_PIP_FILTER (gst_pip_get_style_type ())
#define GST_TYPE_PIP_ORIENTATION (gst_pip_get_orientation_type ())
#define GST_TYPE_PIP_INSET_WINDOW_DEVICE (gst_pip_get_inset_window_device_type ())
#define GST_TYPE_PIP_CAMERA_MODE (gst_pip_get_camera_mode_type ())
#define GST_TYPE_PIP_EFFECT (gst_pip_get_effect_type ())

#define GST_TYPE_PIP_BUFFER               (gst_pip_buffer_get_type())
#define GST_IS_PIP_BUFFER(obj)            (G_TYPE_CHECK_INSTANCE_TYPE((obj), GST_TYPE_PIP_BUFFER))
#define GST_PIP_BUFFER(obj)               (G_TYPE_CHECK_INSTANCE_CAST((obj), GST_TYPE_PIP_BUFFER, GstPipBuffer))
#define HAS_SETTED_INPUT_BUF_PROP(obj)    (obj->appdata->has_setted_ibp)
#define HAS_CREATED_GLM(obj)              (obj->appdata->has_created_glm)
#define HAS_CREATED_CAPTURE_GLM(obj)      (obj->appdata->has_created_capture_glm)

#define SET_INT_PROPERTY(property, value, change_id) do { \
	gint v_int = g_value_get_int (value); \
	if (property != v_int && !pip->appdata->property_change[change_id]) { \
		IF_G_MUTEX_LOCK (pip->appdata->thread_mutex[MU_PRO_CHANGE]); \
		pip->appdata->property_change[change_id] = TRUE; \
		IF_G_MUTEX_UNLOCK (pip->appdata->thread_mutex[MU_PRO_CHANGE]); \
	} \
	property = v_int; \
} while (0)

#define SET_ENUM_PROPERTY(property, value, change_id) do { \
	gint v_enum = g_value_get_enum (value); \
	if (property != v_enum && !pip->appdata->property_change[change_id]) { \
		IF_G_MUTEX_LOCK (pip->appdata->thread_mutex[MU_PRO_CHANGE]); \
		pip->appdata->property_change[change_id] = TRUE;  \
		IF_G_MUTEX_UNLOCK (pip->appdata->thread_mutex[MU_PRO_CHANGE]); \
	} \
	property = v_enum; \
} while (0)

#define SET_STRING_PROPERTY(property, value, change_id) do { \
			gchar *v_string = g_strdup(g_value_get_string (value)); \
			if (g_strcmp0(property, v_string) && !pip->appdata->property_change[change_id]) { \
				IF_G_MUTEX_LOCK (pip->appdata->thread_mutex[MU_PRO_CHANGE]); \
				pip->appdata->property_change[change_id] = TRUE;  \
				IF_G_MUTEX_UNLOCK (pip->appdata->thread_mutex[MU_PRO_CHANGE]); \
			} \
			property = g_strdup(v_string); \
			g_free(v_string); \
		} while (0)

#define SET_BOOLEAN_PROPERTY(property, value, change_id) do { \
			gboolean v_boolean = g_value_get_boolean (value); \
			if (property != v_boolean && !pip->appdata->property_change[change_id]) { \
				IF_G_MUTEX_LOCK (pip->appdata->thread_mutex[MU_PRO_CHANGE]); \
				pip->appdata->property_change[change_id] = TRUE; \
				IF_G_MUTEX_UNLOCK (pip->appdata->thread_mutex[MU_PRO_CHANGE]); \
			} \
			property = v_boolean; \
		} while (0)

enum {
	ARG_0,

	/* GLM options: inset windows --for the window position when user use drags or pinches */
	ARG_INSET_WINDOW_X,
	ARG_INSET_WINDOW_Y,
	ARG_INSET_WINDOW_W,
	ARG_INSET_WINDOW_H,

	/* GLM options: inset windows --for the preview crop for inset window,
	   user can set value specific preview location to display in  inset window */
	ARG_INSET_WINDOW_PREVIEW_CROP_X,
	ARG_INSET_WINDOW_PREVIEW_CROP_Y,
	ARG_INSET_WINDOW_PREVIEW_CROP_W,
	ARG_INSET_WINDOW_PREVIEW_CROP_H,

	/* GLM options: style */
	ARG_STYLE_TYPE,

	/* GLM options: orientation */
	ARG_ORIENTATION_TYPE,

	/* GLM options: orientation */
	ARG_INSET_WINDOW_DEVICE,

	/* camera mode -- rear,front,dual camera */
	ARG_CAMERA_MODE,

	/* single camera effect */
	ARG_EFFECT_TYPE,
	ARG_DOWANLOAD_EFFECT_TYPE,

	/* Signal */
	ARG_PIP_SIGNAL_STILLCAPTURE,

	/*Hide inset window*/
	ARG_HIDE_INSET_WINDOW,

	/* Enable fading */
	ARG_ENABLE_FADING,

	/* Capture Size */
	ARG_CAPTURE_WIDTH,
	ARG_CAPTURE_HEIGHT,

	/* Ready to Stop */
	ARG_READY_TO_STOP,

	/* Output size */
	ARG_OUTPUT_WIDTH,
	ARG_OUTPUT_HEIGHT,

	ARG_SET_ROI_INFO,

	ARG_LAST
};

/* Enumerations */
enum {
	/*signal*/
	SIGNAL_PIP_STILL_CAPTURE,
	SIGNAL_LAST
};

/* signal for pip */
static guint gst_pip_signals[SIGNAL_LAST] = { 0 };

static void gst_pip_set_property (GObject *object, guint prop_id,
                                  const GValue *value, GParamSpec *pspec);
static void gst_pip_get_property (GObject *object, guint prop_id,
                                  GValue *value, GParamSpec *pspec);

static gboolean gst_pip_set_caps (GstPad *pad, GstCaps *caps);

static GstFlowReturn gst_pip_do_buffer (GstCollectPads *pads, gpointer user_data);
static GstPad* gst_pip_request_new_pad (GstElement *element,
                                        GstPadTemplate *templ, const gchar *name,const GstCaps * caps);
static void gst_pip_release_pad (GstElement *element, GstPad *pad);
static GstStateChangeReturn gst_pip_change_state (GstElement *element, GstStateChange transition);
static GstCaps *gst_set_srcpad_caps(gint width, gint height, gint rate, gint scale);

static MMVideoBuffer *gst_pip_make_tbm_buffer(OutputData *output);

static guint32 convert_string_to_fourcc_value(const gchar* format_name);

static int padnum = 1;

static guint32 convert_string_to_fourcc_value(const gchar* format_name)
{
    return format_name[0] | (format_name[1] << 8) | (format_name[2] << 16) | (format_name[3] << 24);
}

static GstStaticPadTemplate sink_factory =
	GST_STATIC_PAD_TEMPLATE ("camera_%d",
	                         GST_PAD_SINK,
	                         GST_PAD_REQUEST,
	                         GST_STATIC_CAPS ("video/x-raw, "
	                                          "format = (string) { S420, I420, SN12, ITLV }, "
	                                          "width = (int) [ 16, 4096 ], "
	                                          "height = (int) [ 16, 4096 ], "
	                                          "framerate = (fraction) [ 0, MAX ]; ")
	);

static GstStaticPadTemplate src_factory =
	GST_STATIC_PAD_TEMPLATE ("src",
	                         GST_PAD_SRC,
	                         GST_PAD_ALWAYS,
	                         GST_STATIC_CAPS ("video/x-raw,"
	                                          "format = (string) { SR32 }, "
	                                          "bpp = (int) { 32 }, "
	                                          "depth = (int) { 24 }, "
	                                          "endianness = (int) { 4321 }, "
	                                          "red_mask = (int) { 65280 }, "
	                                          "green_mask = (int) { 16711680 }, "
	                                          "blue_mask = (int) { -16777216 }, "
	                                          "width = (int) [ 1, 4096 ], "
	                                          "height = (int) [ 1, 4096 ], "
	                                          "framerate = (fraction) [ 0, MAX ]; ")
	);


static GstElementClass *parent_class = NULL;
static void gst_pip_base_init (gpointer gclass);
static void gst_pip_class_init (GstPipClass * klass);
static void gst_pip_init (GstPip * pip,GstPipClass * gclass);
static void gst_pip_pad_free (GstPad * collect_pad);



GType gst_pip_get_type (void)
{
	startfunc

	static GType pip_type = 0;

	if (!pip_type) {
		static const GTypeInfo pip_info = {
			sizeof (GstPipClass),
			gst_pip_base_init,
			NULL,
			(GClassInitFunc) gst_pip_class_init,
			NULL,
			NULL,
			sizeof (GstPip),
			0,
			(GInstanceInitFunc) gst_pip_init,
		};
		static const GInterfaceInfo tag_setter_info = {
			NULL,
			NULL,
			NULL
		};

		pip_type = g_type_register_static (GST_TYPE_ELEMENT, "GstPip", &pip_info, 0);
		g_type_add_interface_static (pip_type, GST_TYPE_TAG_SETTER, &tag_setter_info);
	}

	endfunc

        return pip_type;
}

static GType gst_pip_get_style_type (void)
{
	static GType style_type = 0;
	static GEnumValue style_types[] = {
		{FILTER_VALUE_GD_NORMAL_PIP, "NORMAL_PIP", "NORMAL_PIP"},
		{FILTER_VALUE_GD_CUBISM, "CUBISM", "CUBISM"},
		{FILTER_VALUE_GD_POSTCARD, "POSTCARD", "POSTCARD"},
		{FILTER_VALUE_GD_SIGNATURE, "SIGNATURE", "SIGNATURE"},
		{FILTER_VALUE_GD_OVAL_BLUR, "OVAL_BLUR", "OVAL_BLUR"},
		{FILTER_VALUE_GD_SHINY, "SHINY", "SHINY"},
		{FILTER_VALUE_GD_SHAPE_HEART, "SHAPE_HEART", "SHAPE_HEART"},
		{FILTER_VALUE_GD_EXPOSER_OVERLAY, "EXPOSER_OVERLAY", "EXPOSER_OVERLAY"},
		{FILTER_VALUE_GD_SPLIT_VIEW, "SPLIT_VIEW", "SPLIT_VIEW"},
		{FILTER_VALUE_GD_POLAROID, "POLAROID", "POLAROID"},
		{FILTER_VALUE_GD_PIP_FISHEYE, "PIP_FISHEYE", "PIP_FISHEYE"},
		{FILTER_VALUE_GD_FRAME_OVERLAY, "FRAME_OVERLAY", "FRAME_OVERLAY"},
		{FILTER_VALUE_GD_TRACKING,"TRACKING","TRACKING"},
		{FILTER_VALUE_GD_MAX, NULL, NULL},
	};

	if (!style_type) {
		style_type = g_enum_register_static ("GstPipStyleType", style_types);
	}

	return style_type;
}

static GType gst_pip_get_orientation_type (void)
{
	static GType orientation_type = 0;
	static GEnumValue orientation_types[] = {
		{ORIENTATION_LEFT, "orientation left", "left"},
		{ORIENTATION_DOWN, "orientation down", "down"},
		{ORIENTATION_RIGHT, "orientation right", "right"},
		{ORIENTATION_UP, "orientation up", "up"},
		{ORIENTATION_MAX, NULL, NULL},
	};

	if (!orientation_type) {
		orientation_type = g_enum_register_static ("GstPipOrientationType", orientation_types);
	}

	return orientation_type;
}

static GType gst_pip_get_inset_window_device_type (void)
{
	static GType inset_window_device_type = 0;
	static GEnumValue inset_window_device_types[] = {
		{IWD_FRONT, "inset window device is front", "front"},
		{IWD_REAR, "inset window device is rear", "rear"},
		{IWD_MAX, NULL, NULL},
	};

	if (!inset_window_device_type) {
		inset_window_device_type = g_enum_register_static ("GstPipInsetWindowDeviceType", inset_window_device_types);
	}

	return inset_window_device_type;
}

static GType gst_pip_get_camera_mode_type (void)
{
	static GType camera_mode_type = 0;
	static GEnumValue camera_mode_types[] = {
		{REAR_CAMERA, "camera mode is single camera", "rear"},
		{FRONT_CAMERA, "camera mode is single camera", "front"},
		{DUAL_CAMERA, "camera mode is dual camera", "dual camera"},
		{CAMERA_MODE_MAX, NULL, NULL},
	};

	if (!camera_mode_type) {
		camera_mode_type = g_enum_register_static ("GstPipCameraModeType", camera_mode_types);
	}

	return camera_mode_type;
}

static GType gst_pip_get_effect_type (void)
{
	static GType effect_type = 0;
	static GEnumValue effect_types[] = {
		{FILTER_VALUE_GS_NO_EFFECT, "NO EFFECT", "NO EFFECT"},
		{FILTER_VALUE_GS_SEPIA, "SEPIA", "SEPIA"},
		{FILTER_VALUE_GS_BW, "BLACKWHITE", "BLACKWHITE"},
		{FILTER_VALUE_GS_NEGATIVE, "NEGATIVE", "NEGATIVE"},
		{FILTER_VALUE_GS_OLDPHOTO, "OLDPHOTO", "OLDPHOTO"},
		{FILTER_VALUE_GS_SUNSHINE, "SUNSHINE", "SUNSHINE"},
		{FILTER_VALUE_GS_VINTAGE, "VINTAGE", "VINTAGE"},
		{FILTER_VALUE_GS_RETRO, "RETRO", "RETRO"},
		{FILTER_VALUE_GS_FADEDCOLOR, "FADEDCOLOR", "FADEDCOLOR"},
		{FILTER_VALUE_GS_NOSTALGIA, "NOSTALGIA", "NOSTALGIA"},
		{FILTER_VALUE_GS_COMIC, "COMIC", "COMIC"},
		{FILTER_VALUE_GS_PASTEL_SKETCH, "PASTEL SKETCH", "PASTEL SKETCH"},
		{FILTER_VALUE_GS_GOTHIC_NOIR, "GOTHIC NOIR", "GOTHIC NOIR"},
		{FILTER_VALUE_GS_IMPRESSIONIST, "IMPRESSIONIST", "IMPRESSIONIST"},
		{FILTER_VALUE_GS_SANDSTONE, "SANDSTONE", "SANDSTONE"},
		{FILTER_VALUE_GS_RAINBOW, "RAINBOW", "RAINBOW"},
		{FILTER_VALUE_GS_INSTAGRAM_NASHVILLE, "INSTAGRAM NASHVILLE", "INSTAGRAM NASHVILLE"},
		{FILTER_VALUE_GS_FISHEYE, "FISHEYE", "FISHEYE"},
		{FILTER_VALUE_GS_FOR_REAL, "FOR REAL", "FOR REAL"},
		{FILTER_VALUE_GS_STUCCHEVOLE, "STUCCHEVOLE", "STUCCHEVOLE"},
		{FILTER_VALUE_GS_NOIR_NOTE, "NOIR NOTE", "NOIR NOTE"},
		{FILTER_VALUE_GS_VINCENT, "VINCENT", "VINCENT"},
		{FILTER_VALUE_GS_VIGNETTE, "VIGNETTE", "VIGNETTE"},
		{FILTER_VALUE_GS_DOWNLOAD_EFFECT, "DOWNLOAD EFFECT", "DOWNLOAD EFFECT"},
		{FILTER_VALUE_GS_MAX, NULL, NULL},
	};

	if (!effect_type) {
		effect_type = g_enum_register_static ("GstPipEffectType", effect_types);
	}

	return effect_type;
}

/* VOID:OBJECT,OBJECT,OBJECT (marshaller.list:1) */
#define g_marshal_value_peek_object(v)   (v)->data[0].v_pointer

static void
g_pip_cclosure_user_marshal_VOID__OBJECT_OBJECT_OBJECT (GClosure *closure,
                                                        GValue *return_value G_GNUC_UNUSED,
                                                        guint n_param_values,
                                                        const GValue *param_values,
                                                        gpointer invocation_hint G_GNUC_UNUSED,
                                                        gpointer marshal_data)
{
	typedef void (*GMarshalFunc_VOID__OBJECT_OBJECT_OBJECT) (gpointer data1,
	                                                         gpointer arg_1,
	                                                         gpointer arg_2,
	                                                         gpointer arg_3,
	                                                         gpointer data2);
	register GMarshalFunc_VOID__OBJECT_OBJECT_OBJECT callback;
	register GCClosure *cc = (GCClosure*) closure;
	register gpointer data1, data2;

	g_return_if_fail (n_param_values == 4);

	if (G_CCLOSURE_SWAP_DATA (closure)) {
		data1 = closure->data;
		data2 = g_value_peek_pointer (param_values + 0);
	} else {
		data1 = g_value_peek_pointer (param_values + 0);
		data2 = closure->data;
	}

	callback = (GMarshalFunc_VOID__OBJECT_OBJECT_OBJECT) (marshal_data ? marshal_data : cc->callback);

	callback (data1,
	          g_marshal_value_peek_object (param_values + 1),
	          g_marshal_value_peek_object (param_values + 2),
	          g_marshal_value_peek_object (param_values + 3),
	          data2);

	return;
}


static void gst_pip_buffer_finalize(GstPipBuffer *buffer)
{
	GST_LOG("gst_pip_buffer_finalize");

	if(!buffer) {
		GST_ERROR("GstPipBuffer is null");
		return;
	}

	if (buffer->pip->appdata->has_created_glm) {
		if (buffer->surface) {
			GST_LOG("output native buffer %p", buffer->surface);
			TizenGLManager_setNativeBufferSurface(buffer->pip->appdata->manager, buffer->surface);
		}

		if (buffer->pixmap_id >= 0 && buffer->pixmap_id < SURFACE_COUNT) {
			IF_G_MUTEX_LOCK(buffer->pip->appdata->thread_mutex[MU_PIXMAP]);
			GST_LOG("pixmap id[%d] is finalized, send signal", buffer->pixmap_id);
			buffer->pip->appdata->surface_using[buffer->pixmap_id] = FALSE;
			IF_G_COND_SIGNAL(buffer->pip->appdata->thread_cond[MU_PIXMAP]);
			IF_G_MUTEX_UNLOCK(buffer->pip->appdata->thread_mutex[MU_PIXMAP]);
		} else {
			GST_ERROR("invalid pixmap_id %d", buffer->pixmap_id);
		}
	} else {
		GST_WARNING("GLManager was destroyed");
	}
	gst_object_unref(buffer->pip);
	free(buffer);
	return;
}


static void gst_pip_buffer_class_init(gpointer g_class, gpointer class_data)
{
	startfunc

	endfunc
}

static GstPipBuffer *gst_pip_buffer_new(GstPip *pip)
{
	GstPipBuffer *ret = NULL;
	ret = (GstPipBuffer *)malloc(sizeof(GstPipBuffer));
	ret->buffer = gst_buffer_new();

	GST_LOG_OBJECT(pip, "creating buffer : %p", ret);

	ret->pip = gst_object_ref(GST_OBJECT(pip));
	ret->pixmap_id = -1;

	return ret;
}


static void gst_pip_base_init (gpointer gclass)
{
	startfunc

	endfunc
	return;
}

/* reset pad to initial state
 * free - if true, release all, not only stream related, data */
static void gst_pip_pad_reset (GstPipPad * pippad, gboolean free)
{
	startfunc

	endfunc
}

static void gst_pip_finalize(GObject *object)
{
	startfunc

	GstPip *pip = GST_PIP(object);

	/* completely free each sinkpad */
	GSList *node;
	node = pip->sinkpads;
	while (node) {
		GstPipPad *pippad = (GstPipPad *) node->data;
		node = node->next;
		gst_pip_pad_reset (pippad, TRUE);
		g_free (pippad);
	}
	g_slist_free (pip->sinkpads);
	pip->sinkpads = NULL;

	/*destroy app_data*/
	if (pip->appdata) {
		destroy_app_data(pip->appdata);
		pip->appdata = NULL;
	}

	gst_object_unref (pip->collect);
	G_OBJECT_CLASS(parent_class)->finalize(object);

	endfunc
}

/* initialize the plugin's class */
static void gst_pip_class_init (GstPipClass * klass)
{
	startfunc

	GObjectClass *gobject_class;
	GstElementClass *gstelement_class;

	gobject_class = (GObjectClass *) klass;
	gstelement_class = (GstElementClass *) klass;
	parent_class = g_type_class_peek_parent (klass);

	gst_element_class_add_pad_template (gstelement_class, gst_static_pad_template_get (&sink_factory));
	gst_element_class_add_pad_template (gstelement_class, gst_static_pad_template_get (&src_factory));
	gst_element_class_set_static_metadata(gstelement_class,
	                                     "pip",
	                                     "Picture In Picture",
	                                     "Support Picture In Picture Effect",
	                                     "Ravi Patil <ravi.spatil@samsung.com>");

	GST_DEBUG_CATEGORY_INIT (gst_pip_debug, "pip", 0, "Muxer for Video streams");


	gobject_class->set_property = gst_pip_set_property;
	gobject_class->get_property = gst_pip_get_property;
	gobject_class->finalize = gst_pip_finalize;

	g_object_class_install_property (gobject_class, ARG_INSET_WINDOW_X,
	                                 g_param_spec_int ("inset-window-x",
	                                                   "Set start x of inset window",
	                                                   "start x  of inset window",
	                                                   INSET_WINDOW_X_MIN,
	                                                   INSET_WINDOW_X_MAX,
	                                                   INSET_WINDOW_X_DEFAULT,
	                                                   G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	g_object_class_install_property (gobject_class, ARG_INSET_WINDOW_Y,
	                                 g_param_spec_int ("inset-window-y",
	                                                   "Set start y of inset window",
	                                                   "start y  of inset window",
	                                                   INSET_WINDOW_Y_MIN,
	                                                   INSET_WINDOW_Y_MAX,
	                                                   INSET_WINDOW_Y_DEFAULT,
	                                                   G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	g_object_class_install_property (gobject_class, ARG_INSET_WINDOW_W,
	                                 g_param_spec_int ("inset-window-width",
	                                                   "Set width of inset window",
	                                                   "width  of inset window",
	                                                   INSET_WINDOW_W_MIN,
	                                                   INSET_WINDOW_W_MAX,
	                                                   INSET_WINDOW_W_DEFAULT,
	                                                   G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	g_object_class_install_property (gobject_class, ARG_INSET_WINDOW_H,
	                                 g_param_spec_int ("inset-window-height",
	                                                   "Set height of inset window",
	                                                   "height  of inset window",
	                                                   INSET_WINDOW_H_MIN,
	                                                   INSET_WINDOW_H_MAX,
	                                                   INSET_WINDOW_H_DEFAULT,
	                                                   G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	g_object_class_install_property (gobject_class, ARG_INSET_WINDOW_PREVIEW_CROP_X,
	                                 g_param_spec_int ("inset-window-preview-crop-x",
	                                                   "Set start x of inset window preview crop",
	                                                   "start x of inset window preview crop",
	                                                   INSET_WINDOW_PREVIEW_CROP_X_MIN,
	                                                   INSET_WINDOW_PREVIEW_CROP_X_MAX,
	                                                   INSET_WINDOW_PREVIEW_CROP_X_DEFAULT,
	                                                   G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	g_object_class_install_property (gobject_class, ARG_INSET_WINDOW_PREVIEW_CROP_Y,
	                                 g_param_spec_int ("inset-window-preview-crop-y",
	                                                   "Set start y of inset window preview crop",
	                                                   "start y of inset window preview crop",
	                                                   INSET_WINDOW_PREVIEW_CROP_Y_MIN,
	                                                   INSET_WINDOW_PREVIEW_CROP_Y_MAX,
	                                                   INSET_WINDOW_PREVIEW_CROP_Y_DEFAULT,
	                                                   G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	g_object_class_install_property (gobject_class, ARG_INSET_WINDOW_PREVIEW_CROP_W,
	                                 g_param_spec_int ("inset-window-preview-crop-width",
	                                                   "Set width of inset window preview crop",
	                                                   "width of inset window preview crop",
	                                                   INSET_WINDOW_PREVIEW_CROP_W_MIN,
	                                                   INSET_WINDOW_PREVIEW_CROP_W_MAX,
	                                                   INSET_WINDOW_PREVIEW_CROP_W_DEFAULT,
	                                                   G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	g_object_class_install_property (gobject_class, ARG_INSET_WINDOW_PREVIEW_CROP_H,
	                                 g_param_spec_int ("inset-window-preview-crop-height",
	                                                   "Set height of inset window preview crop",
	                                                   "height of inset window preview crop",
	                                                   INSET_WINDOW_PREVIEW_CROP_H_MIN,
	                                                   INSET_WINDOW_PREVIEW_CROP_H_MAX,
	                                                   INSET_WINDOW_PREVIEW_CROP_H_DEFAULT,
	                                                   G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	g_object_class_install_property (gobject_class, ARG_STYLE_TYPE,
	                                 g_param_spec_enum ("style",
	                                                    "Set style type",
	                                                    "style type",
	                                                    GST_TYPE_PIP_FILTER,
	                                                    STYLE_TYPE_DEFAULT,
	                                                    G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	g_object_class_install_property (gobject_class, ARG_ORIENTATION_TYPE,
	                                 g_param_spec_enum ("orientation",
	                                                    "Set orientation type",
	                                                    "orientation type",
	                                                    GST_TYPE_PIP_ORIENTATION,
	                                                    ORIENTATION_TYPE_DEFAULT,
	                                                    G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	g_object_class_install_property (gobject_class, ARG_INSET_WINDOW_DEVICE,
	                                 g_param_spec_enum ("inset-window-device",
	                                                    "Set inset-window-device type",
	                                                    "inset-window-device type",
	                                                    GST_TYPE_PIP_INSET_WINDOW_DEVICE,
	                                                    INSET_WINDOW_DEVICE_DEFAULT,
	                                                    G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	g_object_class_install_property (gobject_class, ARG_CAMERA_MODE,
	                                 g_param_spec_enum ("camera-mode",
	                                                    "Set camera mode type",
	                                                    "camera mode type",
	                                                    GST_TYPE_PIP_CAMERA_MODE,
	                                                    CAMERA_MODE_DEFAULT,
	                                                    G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	g_object_class_install_property (gobject_class, ARG_EFFECT_TYPE,
	                                 g_param_spec_enum ("effect",
	                                                    "Set single camera effect type",
	                                                    "single camera effect type",
	                                                    GST_TYPE_PIP_EFFECT,
	                                                    EFFECT_TYPE_DEFAULT,
	                                                    G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	g_object_class_install_property (gobject_class, ARG_DOWANLOAD_EFFECT_TYPE,
	                                 g_param_spec_string ("download-effect",
	                                                    "Set single camera download effect type",
	                                                    "single camera downlad effect type",
	                                                    DOWNLOAD_EFFECT_TYPE_DEFAULT,
	                                                    G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	g_object_class_install_property(gobject_class, ARG_PIP_SIGNAL_STILLCAPTURE,
	                                g_param_spec_boolean ("still-capture",
	                                                      "Signal still capture",
	                                                      "Send a signal before pushing the buffer",
	                                                      DEFAULT_PIP_SIGNAL_STILL_CAPTURE,
	                                                      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	g_object_class_install_property(gobject_class, ARG_HIDE_INSET_WINDOW,
	                                g_param_spec_boolean ("hide-inset-window",
	                                                      "hide inset window",
	                                                      "Hide the inset window",
	                                                      DEFAULT_HIDE_INSET_WINDOW,
	                                                      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	g_object_class_install_property(gobject_class, ARG_ENABLE_FADING,
	                                g_param_spec_boolean ("enable-fading",
	                                                      "Enable fading",
	                                                      "Enable fading when starts",
	                                                      DEFAULT_ENABLE_FADING,
	                                                      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	g_object_class_install_property(gobject_class, ARG_SET_ROI_INFO,
	                                g_param_spec_pointer ("set-roi-info",
	                                                      "Set ROI info",
	                                                      "Set roi information when starts",
	                                                      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	g_object_class_install_property (gobject_class, ARG_CAPTURE_WIDTH,
	                                 g_param_spec_int ("capture-width",
	                                                   "Set width of capture image",
	                                                   "width of capture image",
	                                                   0, G_MAXINT, DEFAULT_PIP_CAPTURE_WIDTH,
	                                                   G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	g_object_class_install_property (gobject_class, ARG_CAPTURE_HEIGHT,
	                                 g_param_spec_int ("capture-height",
	                                                   "Set height of capture image",
	                                                   "height of capture image",
	                                                   0, G_MAXINT, DEFAULT_PIP_CAPTURE_HEIGHT,
	                                                   G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	g_object_class_install_property(gobject_class, ARG_READY_TO_STOP,
	                                g_param_spec_boolean ("ready-to-stop",
	                                                      "Ready to stop",
	                                                      "Ready to stop",
	                                                      FALSE,
	                                                      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	g_object_class_install_property (gobject_class, ARG_OUTPUT_WIDTH,
	                                 g_param_spec_int ("output-width",
	                                                   "Set width of output buffer",
	                                                   "width of output buffer",
	                                                   0, 3820, 1280,
	                                                   G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	g_object_class_install_property (gobject_class, ARG_OUTPUT_HEIGHT,
	                                 g_param_spec_int ("output-height",
	                                                   "Set height of output buffer",
	                                                   "height of output buffer",
	                                                   0, 2160, 720,
	                                                   G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));


	/**
	* GstPip::still-capture:
	* @buffer: the buffer that will be pushed - Main
	* @buffer: the buffer that will be pushed - Thumbnail
	* @buffer: the buffer that will be pushed - Screennail
	*
	* This signal gets emitted before sending the buffer.
	*/
	gst_pip_signals[SIGNAL_PIP_STILL_CAPTURE] =
		g_signal_new("still-capture",
		             G_TYPE_FROM_CLASS(klass),
		             G_SIGNAL_RUN_LAST,
		             G_STRUCT_OFFSET(GstPipClass, still_capture),
		             NULL,
		             NULL,
		             g_pip_cclosure_user_marshal_VOID__OBJECT_OBJECT_OBJECT,
		             G_TYPE_NONE,
		             3, /* Number of parameter */
		             GST_TYPE_SAMPLE,  /* Main image buffer */
		             GST_TYPE_SAMPLE,  /* Thumbnail image buffer */
		             GST_TYPE_SAMPLE); /* Screennail image buffer */

	gstelement_class->request_new_pad = GST_DEBUG_FUNCPTR (gst_pip_request_new_pad);
	gstelement_class->release_pad = GST_DEBUG_FUNCPTR (gst_pip_release_pad);
	gstelement_class->change_state = GST_DEBUG_FUNCPTR (gst_pip_change_state);

	endfunc

	return;
}

static void gst_pip_reset (GstPip * pip)
{
	startfunc
	GSList *node, *newlist = NULL;

	/* free and reset each sinkpad */
	node = pip->sinkpads;
	while (node) {
		GstPipPad *pippad = (GstPipPad *) node->data;
		node = node->next;
		gst_pip_pad_reset (pippad, FALSE);
		/* if this pad has collectdata, keep it, otherwise dump it completely */
		if (pippad->collect) {
			newlist = g_slist_append (newlist, pippad);
		} else {
			gst_pip_pad_reset (pippad, TRUE);
			g_free (pippad);
		}
	}

	/* free the old list of sinkpads, only keep the real collecting ones */
	g_slist_free (pip->sinkpads);
	pip->sinkpads = newlist;

	pip->camera_pads = 0;
	/* tags */
	gst_tag_setter_reset_tags (GST_TAG_SETTER (pip));
	endfunc
}

static void gst_pip_pad_free (GstPad * collect_pad)
{
  gst_pip_pad_reset ((GstPipPad *) collect_pad, TRUE);
}

static void gst_pip_reset_property(GstPip * pip)
{
	int index = 0;

	if(!pip->appdata) {
		GST_LOG("appdata is null");
		return;
	}

	pip->appdata->inset_window_x = INSET_WINDOW_X_DEFAULT;
	pip->appdata->inset_window_y = INSET_WINDOW_Y_DEFAULT;
	pip->appdata->inset_window_w = INSET_WINDOW_W_DEFAULT;
	pip->appdata->inset_window_h = INSET_WINDOW_H_DEFAULT;
	pip->appdata->inset_window_preview_crop_x = INSET_WINDOW_PREVIEW_CROP_X_DEFAULT;
	pip->appdata->inset_window_preview_crop_y = INSET_WINDOW_PREVIEW_CROP_Y_DEFAULT;
	pip->appdata->inset_window_preview_crop_w = INSET_WINDOW_PREVIEW_CROP_W_DEFAULT;
	pip->appdata->inset_window_preview_crop_h = INSET_WINDOW_PREVIEW_CROP_H_DEFAULT;
	pip->appdata->style_type = STYLE_TYPE_DEFAULT;
	pip->appdata->orientation = ORIENTATION_TYPE_DEFAULT;
	pip->appdata->inset_window_device = INSET_WINDOW_DEVICE_DEFAULT;
	pip->appdata->camera_mode = CAMERA_MODE_DEFAULT;
	pip->appdata->single_camera_effect = EFFECT_TYPE_DEFAULT;
	pip->appdata->single_camera_download_effect = DOWNLOAD_EFFECT_TYPE_DEFAULT;
	pip->appdata->signal_still_capture = DEFAULT_PIP_SIGNAL_STILL_CAPTURE;
	pip->appdata->hide_inset_window = DEFAULT_HIDE_INSET_WINDOW;
	pip->appdata->enable_fading = DEFAULT_ENABLE_FADING;
	pip->appdata->capture_width = DEFAULT_PIP_CAPTURE_WIDTH;
	pip->appdata->capture_height = DEFAULT_PIP_CAPTURE_HEIGHT;
	pip->appdata->property_updated = FALSE;

	for(index = PROC_IW; index < PROC_MAX; index++) {
		pip->appdata->property_change[index] = FALSE;
	}

	return;
}


static gboolean emit_capture_signal(GstPip *pip,
                                    pip_buffer_t *main,
                                    pip_buffer_t *thumb,
                                    pip_buffer_t *scrnl)
{
	/* GstBuffers for application */
	GstBuffer *buf_cap_signal_main = NULL;
	GstBuffer *buf_cap_signal_thumb = NULL;
	GstBuffer *buf_cap_signal_scrnl = NULL;
	GstSample *sample_cap_signal_main = NULL;
	GstSample *sample_cap_signal_thumb = NULL;
	GstSample *sample_cap_signal_scrnl = NULL;

	AppData *app_data = NULL;

	if (pip == NULL) {
		GST_ERROR("");
		return FALSE;
	}

	app_data = pip->appdata;

	if (main == NULL || thumb == NULL || scrnl == NULL || app_data == NULL) {
		GST_ERROR("[main:%p, thumb:%p, scrnl:%p] something is NULL", main, thumb, scrnl);
		return FALSE;
	}

	/* make GstBuffers with captured data */
	if (main->start) {
		buf_cap_signal_main = gst_buffer_new();
		if(buf_cap_signal_main != NULL){
			GstMapInfo map;
			GstCaps *buffer_caps = NULL;
			gst_buffer_map(buf_cap_signal_main,&map,GST_MAP_WRITE);
			map.data = main->start;
			map.size = main->size;
			buffer_caps = gst_caps_new_simple(_PIP_MIME_TYPE_IMAGE_JPEG,
			                                                           "width", G_TYPE_INT, main->width,
			                                                           "height", G_TYPE_INT, main->height,
			                                                           NULL);
			sample_cap_signal_main = gst_sample_new(buf_cap_signal_main, buffer_caps, NULL, NULL);
			gst_buffer_unmap(buf_cap_signal_main, &map);
		}
	}
	GST_INFO("main %p, %d", main->start, main->size);

	/* thumbnail */
	if (thumb->start) {
		buf_cap_signal_thumb = gst_buffer_new();
		if(buf_cap_signal_thumb != NULL){
			GstMapInfo map;
			GstCaps *buffer_caps = NULL;
			gst_buffer_map(buf_cap_signal_thumb,&map,GST_MAP_WRITE);
			map.data = thumb->start;
			map.size = thumb->size;
			buffer_caps = gst_caps_new_simple(_PIP_MIME_TYPE_IMAGE_JPEG,
			                                                           "width", G_TYPE_INT, thumb->width,
			                                                           "height", G_TYPE_INT, thumb->height,
			                                                           NULL);
			sample_cap_signal_thumb = gst_sample_new(buf_cap_signal_thumb, buffer_caps, NULL, NULL);
			gst_buffer_unmap(buf_cap_signal_thumb, &map);
		}
	}

	GST_INFO("thumb %p, %d", thumb->start, thumb->size);

	/* screennail (postview) */
	if (scrnl->start) {
		buf_cap_signal_scrnl = gst_buffer_new();
		if (buf_cap_signal_scrnl != NULL) {
			GstMapInfo map;
			GstCaps *buffer_caps = NULL;
			gst_buffer_map(buf_cap_signal_scrnl,&map,GST_MAP_WRITE);
			map.data = scrnl->start;
			map.size = scrnl->size;
			buffer_caps = gst_caps_new_simple(_PIP_MIME_TYPE_VIDEO_RGB888,
			                                                            "width", G_TYPE_INT, scrnl->width,
			                                                            "height", G_TYPE_INT, scrnl->height,
			                                                            NULL);
			sample_cap_signal_scrnl = gst_sample_new(buf_cap_signal_scrnl, buffer_caps, NULL, NULL);
			gst_buffer_unmap(buf_cap_signal_scrnl, &map);
		}
	}

	GST_INFO("scrnl %p, %d", scrnl->start, scrnl->size);

	/* emit capture signal */
	GST_INFO("Call Capture SIGNAL");
	g_signal_emit(G_OBJECT (pip),
	              gst_pip_signals[SIGNAL_PIP_STILL_CAPTURE],
	              0,
	              sample_cap_signal_main,
	              sample_cap_signal_thumb,
	              sample_cap_signal_scrnl);

	GST_INFO("Return Capture SIGNAL");

	return TRUE;
}

static gboolean capture_queue_is_empty(GstPip *pip)
{
	AppData *app_data = pip->appdata;

	if (!app_data) {
		GST_ERROR("AppData is null");
		return TRUE;
	}

	if (is_dual_camera_mode(app_data)) {
		return (g_queue_is_empty(app_data->capture_buffer_list[BUF_0]) &&
	        g_queue_is_empty(app_data->capture_buffer_list[BUF_1]));
	} else {
		return (g_queue_is_empty(app_data->capture_buffer_list[BUF_0]));
	}
}


static gpointer gst_pip_handle_outbuf_thread_cb(gpointer data)
{
	startfunc

	GstPip *pip = (GstPip *)data;
	AppData *app_data = NULL;

	if (!pip) {
		GST_ERROR("pip is null");
		return NULL;
	}

	app_data = pip->appdata;
	if (!app_data) {
		GST_ERROR("AppData is null");
		return NULL;
	}

	IF_G_MUTEX_LOCK (app_data->thread_mutex[MU_OUTPUT_QUEUE]);

	while (!app_data->exit_loop[TH_HANDLE_OUT_BUF]) {
		GstFlowReturn ret = 0;
		OutputData *curOutput = NULL;
		tbm_surface_h surface = NULL;
		MMVideoBuffer *psimgb = NULL;
		GstPipBuffer *pipBuf = NULL;
		GstBuffer *newbuf = NULL;

		if (g_queue_is_empty(app_data->output)) {
			GST_WARNING("Outer buffer is empty");
			GTimeVal abstimeout;
			g_get_current_time(&abstimeout);
			g_time_val_add(&abstimeout, PIP_WAIT_TIMEOUT);
			GST_WARNING("output buffer list is empty. wait capture signal...");
			if(!g_cond_timed_wait(app_data->thread_cond[MU_OUTPUT_QUEUE],
						app_data->thread_mutex[MU_OUTPUT_QUEUE],&abstimeout)) {
				GST_WARNING("output buffer signal time out , 1 sec");
			} else {
				GST_WARNING("output buffer signal received");
			}
			continue;
		}

		/*handle output buffer*/
		GST_WARNING("Buffer not empty: count=%d",g_queue_get_length(app_data->output));
		curOutput = (OutputData *)g_queue_pop_head(pip->appdata->output);;
		if (!curOutput) {
			GST_WARNING("output buffer is NULL");
			continue;
		}

		surface = curOutput->surface;
		if(!surface){
			destroy_OutputData(curOutput);
			GST_WARNING("Get native buffer failed");
			continue;
		}

		psimgb = gst_pip_make_tbm_buffer(curOutput);
		if(!psimgb){
			destroy_OutputData(curOutput);
			GST_WARNING("gst_pip_make_tbm_buffer failed");
			continue;
		}

		pipBuf = gst_pip_buffer_new(pip);
		if (!pipBuf) {
			destroy_OutputData(curOutput);
			free(psimgb);
			GST_WARNING("gst_pip_buffer_new failed");
			continue;
		}

		IF_G_MUTEX_UNLOCK(app_data->thread_mutex[MU_OUTPUT_QUEUE]);

		GST_WARNING("output buffer thread run");

		newbuf = (GstBuffer *)pipBuf->buffer;
		pipBuf->pixmap_id = curOutput->pixmap_id;
		pipBuf->surface = surface;
		gst_buffer_append_memory(newbuf,
                                gst_memory_new_wrapped(GST_MEMORY_FLAG_READONLY,
                                                        psimgb->data[0],
                                                        tbm_surface_internal_get_size (surface),
                                                        0,
                                                        tbm_surface_internal_get_size (surface),
                                                        pipBuf,
                                                        gst_pip_buffer_finalize)
        );

		gst_buffer_append_memory(newbuf,
                                gst_memory_new_wrapped(GST_MEMORY_FLAG_READONLY,
                                                        psimgb,
                                                        sizeof(*psimgb),
                                                        0,
                                                        sizeof(*psimgb),
                                                        psimgb,
                                                        free)
        );
		GST_BUFFER_PTS(newbuf) = curOutput->timestamp;
		GST_BUFFER_DURATION(newbuf) = curOutput->duration;
		GST_LOG("native_buf->surface : %p, bo : %p, size: %u", surface, psimgb->handle.bo[0], tbm_surface_internal_get_size(surface));

#ifdef ENABLE_PIP_DUMP_BUFFER
		GstMapInfo map;
		gst_buffer_map(newbuf,&map,GST_MAP_READ);
		GST_WARNING("Vaddr %p size %d", map.data, map.size);
		pip_data_dump("/opt/usr/media/pip_output_dump.rgb", map.data,map.size);
		gst_buffer_unmap(newbuf,&map);
		/* BGRA32 format */
#endif

		GST_WARNING("Pushing caps event to source pad");
		if (!gst_pad_has_current_caps(pip->srcpad)) {
			GstEvent *caps_event = gst_event_new_caps(gst_set_srcpad_caps(tbm_surface_get_width(surface), tbm_surface_get_height(surface), pip->appdata->rate, pip->appdata->scale));
			gst_pad_push_event (pip->srcpad, caps_event);
		}
		/* set outputbuffer caps */
		if (!pip->push_possible) {
			gst_buffer_unref(newbuf);
			GST_WARNING("push impossible, so unref the buffer and skip");
		} else {
			if (app_data->ready_to_stop) {
				MMVideoBuffer*imgb = NULL;
				GstMapInfo mapInfo;
				GstMemory *img_buf_memory = NULL;
				if (newbuf && gst_buffer_n_memory(newbuf) > 1){
					img_buf_memory = gst_buffer_peek_memory(newbuf,1);
					gst_memory_map(img_buf_memory,&mapInfo,GST_MAP_READ);
					imgb = (MMVideoBuffer *)mapInfo.data;
					gst_memory_unmap(img_buf_memory,&mapInfo);
				}
				if (app_data->buffer_flush_done == FALSE) {
					if (imgb) {
						GST_WARNING_OBJECT(pip, "make flush buffer");
						app_data->buffer_flush_done = TRUE;
//                                       imgb->type = MM_VIDEO_BUFFER_TYPE_TBM_BO;
//TODO:
					} else {
						GST_WARNING_OBJECT(pip, "imgb is NULL");
					}
				} else {
					GST_WARNING_OBJECT(pip, "flush buffer is already pushed, skip this buffer");
				}
			}

			/*Push buffer*/
			ret = gst_pad_push (pip->srcpad, newbuf);
			if(ret != GST_FLOW_OK) {
				GST_WARNING_OBJECT(pip, "buffer push is fail, so sending  eos event");
				gst_pad_push_event (pip->srcpad, gst_event_new_eos ());
			}
		}
		destroy_OutputData(curOutput);

		IF_G_MUTEX_LOCK(app_data->thread_mutex[MU_OUTPUT_QUEUE]);
	}

	/*release remained buffer*/
	GST_WARNING("release remained buffer");
	g_queue_free_full (app_data->output, destroy_OutputData);
	app_data->output = g_queue_new();

	IF_G_MUTEX_UNLOCK(app_data->thread_mutex[MU_OUTPUT_QUEUE]);

	endfunc

	return NULL;
}


static gpointer gst_pip_capture_thread_cb(gpointer data)
{
	startfunc
	gint j = 0;
	GstPip *pip = (GstPip *)data;
	AppData *app_data = NULL;
	tbm_surface_info_s info;
	if (!pip) {
		GST_ERROR("pip is null");
		return NULL;
	}

	app_data = pip->appdata;
	if (!app_data) {
		GST_ERROR("AppData is null");
		return NULL;
	}

	/* buffers for captured data */
	pip_buffer_t main_buf = {0, NULL, 0, 0};
	pip_buffer_t thumb_buf = {0, NULL, 0, 0};
	pip_buffer_t scrnl_buf = {0, NULL, 0, 0};

	if(app_data->camera_mode >= CAMERA_MODE_MAX)
		return NULL;

	IF_G_MUTEX_LOCK (app_data->thread_mutex[MU_CAPTURE_QUEUE]);

	while (!app_data->exit_loop[TH_CAPTURE]) {
		int ret = 0;
		mm_util_jpeg_yuv_data *decoded_data = NULL;
		tbm_surface_h output_surface = NULL;
		CaptureJpegData *jpegData = NULL;
		CaptureRawData *rawData = NULL;
		CaptureRawData *rawData2 = NULL;
		CaptureData *capture_data = NULL;
		tbm_bo_handle addr = {NULL};
		void *output_jpeg = NULL;
		tbm_format buf_format = TBM_FORMAT_NV12;

		if (capture_queue_is_empty(pip)) {
			GTimeVal abstimeout;
			g_get_current_time(&abstimeout);
			g_time_val_add(&abstimeout, PIP_WAIT_TIMEOUT);
			GST_LOG("capture list is empty. wait capture signal...");
			if(!g_cond_timed_wait(app_data->thread_cond[MU_CAPTURE_QUEUE],
						app_data->thread_mutex[MU_CAPTURE_QUEUE],&abstimeout)) {
				GST_LOG("capture buffer signal time out , 1 sec");
			} else {
				GST_LOG("capture signal received");
			}
			continue;
		}

		GST_WARNING("CAPTURE THREAD RUN");

		if (HAS_CREATED_CAPTURE_GLM(pip)) {
			if (app_data->property_updated) {
				destroy_capture_glm(app_data);
				app_data->property_updated = FALSE;
			}
		} else {
			app_data->property_updated = FALSE;
		}

		switch (app_data->fourcc) {
			case GST_MAKE_FOURCC('S', 'N', '1', '2') :
				buf_format = TBM_FORMAT_NV12;
				break;
			case GST_MAKE_FOURCC('S', '4', '2', '0') :
				buf_format = TBM_FORMAT_YUV420;
				break;
			default :
				break;
		}

		/*JPEG DATA from camerasrc capture jpeg*/
		if (is_dual_camera_mode(app_data)) {
			if (app_data->inset_window_device == FRONT_CAMERA) {
				jpegData = (CaptureJpegData *)g_queue_pop_head(app_data->capture_buffer_list[BUF_0]);
			} else {
				rawData2 = (CaptureRawData *)g_queue_pop_head(app_data->capture_buffer_list[BUF_0]);
			}
			rawData = (CaptureRawData *)g_queue_pop_head(app_data->capture_buffer_list[BUF_1]);
			if ((!jpegData && !rawData2)|| !rawData) {
				GST_WARNING("jpegData[%p] or rawData[%p] or rawData2[%p] is null",
				            jpegData, rawData, rawData2);
				GST_ELEMENT_ERROR (pip, RESOURCE, FAILED, ("capture data alloc fail"), GST_ERROR_SYSTEM);
				goto FREE_DATA;
			}
		} else {
			capture_data = (CaptureData *)g_queue_pop_head(app_data->capture_buffer_list[BUF_0]);
			if (capture_data) {
				if (capture_data->type == CAPTURE_DATA_JPEG) {
					jpegData = (CaptureJpegData *)capture_data;
				} else {
					rawData = (CaptureRawData *)capture_data;
				}
			} else {
				GST_ELEMENT_ERROR (pip, RESOURCE, FAILED, ("capture data alloc fail"), GST_ERROR_SYSTEM);
				goto FREE_DATA;
			}
		}

		IF_G_MUTEX_UNLOCK(app_data->thread_mutex[MU_CAPTURE_QUEUE]);

		if (jpegData || rawData2) {
			if (jpegData) {
				if (jpegData->fourcc == GST_MAKE_FOURCC ('J', 'P', 'E', 'G')) {
					GST_WARNING("decode JPEG data");

					decoded_data = (mm_util_jpeg_yuv_data *)malloc(sizeof(mm_util_jpeg_yuv_data));;
					if(!decoded_data) {
						GST_ERROR("Allocate memory failed");
						goto FREE_DATA;
					}

					/* decode original JPEG */
					memset(decoded_data, 0x0, sizeof(mm_util_jpeg_yuv_data));
					mm_util_jpeg_yuv_format dec_fmt = MM_UTIL_JPEG_FMT_YUV420;
					if(jpegData == NULL || jpegData->data == NULL || jpegData->size == 0){
						GST_ERROR("mm_util_decode_from_jpeg_memory failed");
						free(decoded_data);
						decoded_data = NULL;
						goto FREE_DATA;
					}
					{
						int jpeg_ret = 0;
						jpeg_ret = mm_util_decode_from_jpeg_memory(decoded_data,
					                                jpegData->data,
					                                jpegData->size,
					                                dec_fmt);
						if(jpeg_ret){
							GST_WARNING("decode ERROR");
						}
						else{
							GST_WARNING("decode DONE");
						}
					}
					/* release jpeg data */
					IF_FREE(jpegData->data);
					IF_FREE(jpegData);
#if 0
					{
						FILE *fp = fopen("/opt/capture_input0.yuv", "w");
						GST_ERROR("/opt/capture_input0.yuv fopen %p", fp);
						fwrite(decoded_data->data, decoded_data->size, 1, fp);
						GST_ERROR("fwrite done");
						fclose(fp);
						fp = NULL;
					}
#endif
					if (!decoded_data->data) {
						GST_ERROR("mm_util_decode_from_jpeg_memory failed");
						IF_FREE(decoded_data);
						goto FREE_DATA;
					}

					/* alloc capture bo and copy decoded data */
					GST_WARNING("alloc tbm bo[0]. [decoded data %dx%d, size %d]",
					            decoded_data->width, decoded_data->height, decoded_data->size);
					if (app_data->input_bo[0] == NULL) {
						app_data->input_bo[0] = tbm_bo_alloc (app_data->bufmgr, decoded_data->size, TBM_BO_DEFAULT);
					}
				} else {
					/* alloc capture bo and copy raw data */
					GST_WARNING("alloc tbm bo[0]. [jpegData data %p, size %d]",
								jpegData->data, jpegData->size);
					if (app_data->input_bo[0] == NULL) {
						app_data->input_bo[0] = tbm_bo_alloc (app_data->bufmgr, jpegData->size, TBM_BO_DEFAULT);
					}
				}
			} else if (rawData2) {
				/* alloc capture bo and copy raw data */
				GST_WARNING("alloc tbm bo[0]. [raw data %p, size %d]",rawData2->data, rawData2->size);
				if (app_data->input_bo[0] == NULL) {
					app_data->input_bo[0] = tbm_bo_alloc (app_data->bufmgr, rawData2->size, TBM_BO_DEFAULT);
				}
			}

			if (app_data->input_bo[0]) {
				addr = tbm_bo_map (app_data->input_bo[0], TBM_DEVICE_CPU, TBM_OPTION_READ|TBM_OPTION_WRITE);
				if(addr.ptr == NULL){
					tbm_bo_unmap(app_data->input_bo[0]);
					tbm_bo_unref(app_data->input_bo[0]);
					app_data->input_bo[0] = NULL;
					goto FREE_DATA;
				}

				if (decoded_data) {
					GST_WARNING("memcpy src %p dst %p size %d", decoded_data->data, addr.ptr, decoded_data->size);
					memcpy(addr.ptr, decoded_data->data, decoded_data->size);
				} else if (rawData2) {
					GST_WARNING("memcpy src %p dst %p size %d", rawData2->data, addr.ptr, rawData2->size);
					memcpy(addr.ptr, rawData2->data, rawData2->size);
				} else if (jpegData) {
					GST_WARNING("memcpy src %p dst %p size %d", jpegData->data, addr.ptr, jpegData->size);
					memcpy(addr.ptr, jpegData->data, jpegData->size);
				}
				GST_WARNING("memcpy done");
				tbm_bo_unmap (app_data->input_bo[0]);
				addr.ptr = NULL;
			} else {
				GST_ERROR("failed to alloc tbm bo[0]. size");
				goto FREE_DATA;
			}

			GST_WARNING("tbm_surface_internal_create_with_bos");

			/* make native buffer */
			if (decoded_data) {
				if (app_data->surface[0] == NULL) {
					info.height = decoded_data->height;
					info.width = decoded_data->width;
					info.format = TBM_FORMAT_YUV420;
					info.bpp = tbm_surface_internal_get_bpp(info.format);
					info.num_planes = tbm_surface_internal_get_num_planes(info.format);
					info.planes[0].offset = 0;
					info.planes[0].size = info.height * info.width;
					info.planes[0].stride = info.width;
					info.size = info.planes[0].size;
					info.planes[1].size = info.height * info.width* (1/4);
					info.planes[1].offset =info.planes[0].size;
					info.planes[1].stride = info.width;
					info.planes[2].size = info.height * info.width* (1/4);
					info.planes[2].offset =info.planes[1].size;
					info.planes[2].stride = info.width;
					info.size = info.size + info.planes[1].size + info.planes[2].size;
					app_data->surface[0] = tbm_surface_internal_create_with_bos (&info,&app_data->input_bo[0], 1);
				}

				/*release decoded data*/
				IF_FREE(decoded_data->data);
			} else if (rawData2) {
				if (app_data->surface[0] == NULL) {
					info.height = app_data->height[0];
					info.width = app_data->width[0];
					info.format = buf_format;
					info.bpp = tbm_surface_internal_get_bpp(info.format);
					info.num_planes = tbm_surface_internal_get_num_planes(info.format);
					info.planes[0].offset = 0;
					info.planes[0].size = info.height * info.width;
					info.planes[0].stride = info.width;
					info.size = info.planes[0].size;
					if(info.num_planes == 2) {
						info.planes[1].size = info.height * info.width* (1/2);
						info.planes[1].offset =info.planes[0].size;
						info.planes[1].stride = info.width;
						info.size = info.size + info.planes[1].size;}
					else if (info.num_planes == 3) {
						info.planes[1].size = info.height * info.width* (1/4);
						info.planes[1].offset =info.planes[0].size;
						info.planes[1].stride = info.width;
						info.planes[2].size = info.height * info.width* (1/4);
						info.planes[2].offset =info.planes[1].size;
						info.planes[2].stride = info.width;
						info.size = info.size + info.planes[1].size + info.planes[2].size;}
					app_data->surface[0] = tbm_surface_internal_create_with_bos (&info,&app_data->input_bo[0],1);
				}
			} else if (jpegData) {
				if (app_data->surface[0] == NULL) {
					info.height = jpegData->height;
					info.width = jpegData->width;
					info.format = buf_format;
					info.bpp = tbm_surface_internal_get_bpp(info.format);
					info.num_planes = tbm_surface_internal_get_num_planes(info.format);
					info.planes[0].offset = 0;
					info.planes[0].size = info.height * info.width;
					info.planes[0].stride = info.width;
					info.size = info.planes[0].size;
					if(info.num_planes == 2) {
						info.planes[1].size = info.height * info.width* (1/2);
						info.planes[1].offset =info.planes[0].size;
						info.planes[1].stride = info.width;
						info.size = info.size + info.planes[1].size;}
					else if (info.num_planes == 3) {
						info.planes[1].size = info.height * info.width* (1/4);
						info.planes[1].offset =info.planes[0].size;
						info.planes[1].stride = info.width;
						info.planes[2].size = info.height * info.width* (1/4);
						info.planes[2].offset =info.planes[1].size;
						info.planes[2].stride = info.width;
						info.size = info.size + info.planes[1].size + info.planes[2].size;}
					app_data->surface[0] = tbm_surface_internal_create_with_bos (&info,&app_data->input_bo[0],1);
				}
			}

			GST_WARNING("tbm_surface_internal_create_with_bos[0] done");

			if (!app_data->surface[0]) {
				GST_ERROR("tbm_surface_internal_create_with_bos[0] failed");
				goto FREE_DATA;
			}

			if (!HAS_CREATED_CAPTURE_GLM(pip)) {
				GST_WARNING("create Capture GLM");
				if (decoded_data) {
					ret = create_capture_glm(app_data, decoded_data->width, decoded_data->height);
				} else if (rawData2) {
					ret = create_capture_glm(app_data, app_data->width[1], app_data->height[1]);
				} else if (jpegData) {
					ret = create_capture_glm(app_data, jpegData->width, jpegData->height);
				}
				if (!ret) {
					GST_ERROR("create_capture_glm failed");
					goto FREE_DATA;
				}
			}
		} else {
			if(rawData == NULL || rawData->data == NULL || rawData->size == 0){
				GST_ERROR(" rawData is null");
				goto FREE_DATA;
			}

			GST_WARNING("alloc tbm bo[0]. size %d", rawData->size);

			if (app_data->input_bo[0] == NULL) {
				app_data->input_bo[0] = tbm_bo_alloc (app_data->bufmgr, rawData->size, TBM_BO_DEFAULT);
			}
			if (app_data->input_bo[0]) {
				addr = tbm_bo_map (app_data->input_bo[0], TBM_DEVICE_CPU, TBM_OPTION_READ|TBM_OPTION_WRITE);
				if(addr.ptr == NULL){
					tbm_bo_unmap(app_data->input_bo[0]);
					tbm_bo_unref(app_data->input_bo[0]);
					app_data->input_bo[0] = NULL;
					goto FREE_DATA;
				}
				GST_WARNING("memcpy src %p dst %p size %d", rawData->data, addr.ptr, rawData->size);
				memcpy(addr.ptr, rawData->data, rawData->size);
				GST_WARNING("memcpy done");
				tbm_bo_unmap (app_data->input_bo[0]);
				addr.ptr = NULL;
			} else {
				GST_ERROR("failed to alloc tbm bo[0]. size %d", rawData->size);
				goto FREE_DATA;
			}

			/*release rawData*/
			IF_FREE(rawData->data);

			GST_WARNING("tbm_surface_internal_create_with_bos[0]");

			if (app_data->surface[0] == NULL) {
				info.height = app_data->height[0];
				info.width = app_data->width[0];
				info.format = buf_format;
				info.bpp = tbm_surface_internal_get_bpp(info.format);
				info.num_planes = tbm_surface_internal_get_num_planes(info.format);
				info.planes[0].offset = 0;
				info.planes[0].size = info.height * info.width;
				info.planes[0].stride = info.width;
				info.size = info.planes[0].size;
				if(info.num_planes == 2){
					info.planes[1].size = info.height * info.width* (1/2);
					info.planes[1].offset =info.planes[0].size;
					info.planes[1].stride = info.width;
					info.size = info.size + info.planes[1].size;
				}
				else if(info.num_planes == 3){
					info.planes[1].size = info.height * info.width* (1/4);
					info.planes[1].offset =info.planes[0].size;
					info.planes[1].stride = info.width;
					info.planes[2].size = info.height * info.width* (1/4);
					info.planes[2].offset =info.planes[1].size;
					info.planes[2].stride = info.width;
					info.size = info.size + info.planes[1].size + info.planes[2].size;
					}
				app_data->surface[0] = tbm_surface_internal_create_with_bos (&info,&app_data->input_bo[0], 1);
			}
			IF_FREE(rawData);
			GST_WARNING("tbm_surface_internal_create_with_bos[0] done");
			if (!app_data->surface[0]) {
				GST_ERROR("tbm_surface_internal_create_with_bos[0] failed");
				goto FREE_DATA;
			}

			if (!HAS_CREATED_CAPTURE_GLM(pip)) {
				GST_WARNING("create Capture GLM");
				ret = create_capture_glm(app_data, tbm_surface_get_width(app_data->surface[0]), tbm_surface_get_height(app_data->surface[0]));
				if (!ret) {
					GST_ERROR("create_capture_glm failed");
					goto FREE_DATA;
				}
			}
		}

		GST_WARNING("TizenGLManager_setNativeBufferTexture[0]");

		if(app_data->capture_manager == NULL){
			GST_ERROR("capture_manager is null");
			goto FREE_DATA;
		}

		ret = TizenGLManager_setNativeBufferTexture(app_data->capture_manager,
		                                            app_data->surface[0],
		                                            BUF_0);
		if(ret != 0) {
			GST_ERROR("TizenGLManager_setNativeBufferTexture jpeg date failed");
			goto FREE_DATA;
		}

		if (is_dual_camera_mode(app_data)) {
			GST_WARNING("alloc tbm bo[1]. size %d", rawData->size);

			if (app_data->input_bo[1] == NULL) {
				app_data->input_bo[1] = tbm_bo_alloc (app_data->bufmgr, rawData->size, TBM_BO_DEFAULT);
			}
			if (app_data->input_bo[1]) {
				addr = tbm_bo_map (app_data->input_bo[1], TBM_DEVICE_CPU, TBM_OPTION_READ|TBM_OPTION_WRITE);
				if(addr.ptr == NULL){
					tbm_bo_unmap(app_data->input_bo[1]);
					tbm_bo_unref(app_data->input_bo[1]);
					app_data->input_bo[1] = NULL;
					goto FREE_DATA;
				}
				GST_WARNING("memcpy src %p dst %p size %d", rawData->data, addr.ptr, rawData->size);
				memcpy(addr.ptr, rawData->data, rawData->size);
				GST_WARNING("memcpy done");
				tbm_bo_unmap (app_data->input_bo[1]);
				addr.ptr = NULL;
			} else {
				GST_ERROR("failed to alloc tbm bo[1]. size %d", rawData->size);
				goto FREE_DATA;
			}
#if 0
			{
				FILE *fp = fopen("/opt/capture_input1.yuv", "w");
				GST_ERROR("/opt/capture_input1.yuv fopen %p", fp);
				fwrite(rawData->data, rawData->size, 1, fp);
				GST_ERROR("fwrite done");
				fclose(fp);
				fp = NULL;
			}
#endif
			/*release rawData*/
			if (rawData->data) {
				free(rawData->data);
				rawData->data = NULL;
			}

			GST_WARNING("tbm_surface_internal_create_with_bos[1]");

			if (app_data->surface[1] == NULL) {
				info.height = app_data->height[1];
				info.width = app_data->width[1];
				info.format = buf_format;
				info.bpp = tbm_surface_internal_get_bpp(info.format);
				info.num_planes = tbm_surface_internal_get_num_planes(info.format);
				info.planes[0].offset = 0;
				info.planes[0].size = info.height * info.width;
				info.planes[0].stride = info.width;
				info.size = info.planes[0].size;
				if(info.num_planes == 2){
					info.planes[1].size = info.height * info.width* (1/2);
					info.planes[1].offset =info.planes[0].size;
					info.planes[1].stride = info.width;
					info.size = info.size + info.planes[1].size;
				}
				else if(info.num_planes == 3){
					info.planes[1].size = info.height * info.width* (1/4);
					info.planes[1].offset =info.planes[0].size;
					info.planes[1].stride = info.width;
					info.planes[2].size = info.height * info.width* (1/4);
					info.planes[2].offset =info.planes[1].size;
					info.planes[2].stride = info.width;
					info.size = info.size + info.planes[1].size + info.planes[2].size;
				}

				app_data->surface[1] = tbm_surface_internal_create_with_bos (&info,&app_data->input_bo[1],1);
			}

			free(rawData);
			rawData = NULL;

			GST_WARNING("tbm_surface_internal_create_with_bos[1] done");

			if (!app_data->surface[1]) {
				GST_ERROR("tbm_surface_internal_create_with_bos[1] failed");
				goto FREE_DATA;
			}

			GST_WARNING("TizenGLManager_setNativeBufferTexture[1]");

			ret = TizenGLManager_setNativeBufferTexture(app_data->capture_manager,
			                                            app_data->surface[1],
			                                            BUF_1);
			if (ret != 0) {
				GST_ERROR("TizenGLManager_setNativeBufferTexture id:1 failed");
				goto FREE_DATA;
			}
		}

		GST_WARNING("CAPTURE TizenGLManager_render START ======");
		ret = TizenGLManager_render(app_data->capture_manager);
		GST_WARNING("CAPTURE TizenGLManager_render END status: %d ======", ret);

		output_surface = TizenGLManager_getPixmapBufferSurface (app_data->capture_manager);
		if(!output_surface) {
			GST_ERROR("Get output buffer from GLM failed");
			goto FREE_DATA;
		}

		GST_WARNING("get input buffer");
		TizenGLManager_getNativeBufferTexture(app_data->capture_manager, BUF_0);
		if (is_dual_camera_mode(app_data)) {
			TizenGLManager_getNativeBufferTexture(app_data->capture_manager, BUF_1);
		}
		GST_WARNING("release input buffer");

		if (ret == 1 && output_surface) {
			int enc_size = tbm_surface_internal_get_size(output_surface) ;
			int enc_quality = 80;
			tbm_surface_info_s info = {0};
#ifdef NEED_RGB888_CONVERT
			mm_util_jpeg_yuv_format enc_fmt = MM_UTIL_JPEG_FMT_RGB888;
			unsigned char *rgb888_data = NULL;
#else
			mm_util_jpeg_yuv_format enc_fmt = MM_UTIL_JPEG_FMT_BGRA8888;
#endif
			int argb888_size = tbm_surface_internal_get_size(output_surface) ;

			if(tbm_surface_map(output_surface,TBM_OPTION_READ,&info))
			{
				GST_WARNING("tbm_surface_unmap as map gave error");
				tbm_surface_unmap(output_surface);
				goto FREE_DATA;
			}

			GST_WARNING("encode JPEG - %dx%d",
			             tbm_surface_get_width(output_surface),
			             tbm_surface_get_height(output_surface));
#if 0
			{
				FILE *fp = fopen("/opt/capture_output.rgb", "w");
				GST_ERROR("/opt/capture_output.rgb fopen %p", fp);
				fwrite(bo_handle.ptr, argb888_size, 1, fp);
				GST_ERROR("fwrite done");
				fclose(fp);
				fp = NULL;
			}
#endif
#ifdef NEED_RGB888_CONVERT
			rgb888_data = (unsigned char *)malloc((argb888_size*3)>>2);
			if (rgb888_data) {
				int i = 0;
				int j = 0;
				unsigned char *argb8888_data = (unsigned char *)info.planes[0].ptr;
				GST_WARNING("start convert ARGB8888 -> RGB888");

				/* convert BGRA8888 -> RGB888 */
				for (i = 0 ; i < argb888_size ; i += 4, j += 3) {
					//memcpy(rgb888_data + j, argb8888_data + i, 3);
					rgb888_data[j] = argb8888_data[i+2];
					rgb888_data[j+1] = argb8888_data[i+1];
					rgb888_data[j+2] = argb8888_data[i];
				}

				GST_WARNING("done convert ARGB8888 -> RGB888");

				ret = mm_util_jpeg_encode_to_memory(&output_jpeg, &enc_size, rgb888_data,
				                                    tbm_surface_get_width(output_surface),  tbm_surface_get_height(output_surface),
				                                    enc_fmt, enc_quality);
#else
				ret = mm_util_jpeg_encode_to_memory(&output_jpeg, &enc_size, info.planes[0].ptr,
													 tbm_surface_get_width(output_surface),  tbm_surface_get_height(output_surface),
													enc_fmt, enc_quality);
#endif
				GST_WARNING("encode JPEG done. ret 0x%x size %d", ret, enc_size);

				/* set buffer result */
				main_buf.start = output_jpeg;
				main_buf.size = enc_size;
				main_buf.width = tbm_surface_get_width(output_surface);
				main_buf.height = tbm_surface_get_height(output_surface);
				thumb_buf.start = NULL;
#ifdef NEED_RGB888_CONVERT
				scrnl_buf.start = rgb888_data;
#else
				scrnl_buf.start = (unsigned char *)info.planes[0].ptr;
#endif
				scrnl_buf.width = main_buf.width;
				scrnl_buf.height = main_buf.height;
#ifdef NEED_RGB888_CONVERT
				scrnl_buf.size = (argb888_size*3)>>2;
#else
				scrnl_buf.size = argb888_size;
#endif
				if (ret == 0) {
					GST_WARNING("JPEG encode DONE");
				} else {
					GST_WARNING("Failed to JPEG encode : 0x%x", ret);
				}

				GST_WARNING("CALL capture signal");

				/* send capture signal */
				emit_capture_signal(pip, &main_buf, &thumb_buf, &scrnl_buf);

				GST_WARNING("RETURN capture signal");

				/* release rgb888 and JPEG data */
				IF_FREE(output_jpeg);
#ifdef NEED_RGB888_CONVERT
				IF_FREE(rgb888_data);
			} else {
				GST_WARNING("non rgb888 data");
			}
#endif
			/* unmap output bo */
			tbm_surface_unmap(output_surface);
			TizenGLManager_setNativeBufferSurface(app_data->capture_manager, output_surface);

		} else {
			GST_ERROR("RENDER for CAPTURE failed. output buffer %p", output_surface);
		}
FREE_DATA:
		destroy_CaptureJpegData(jpegData);
		destroy_CaptureRawData(rawData);
		destroy_CaptureRawData(rawData2);

		if (decoded_data) {
			GST_WARNING("Free decoded_data");
			if (decoded_data->data) {
				free(decoded_data->data);
				decoded_data->data = NULL;
			}
			free(decoded_data);
			decoded_data = NULL;
		}

		GST_WARNING("bottom of capture loop");

		IF_G_MUTEX_LOCK(app_data->thread_mutex[MU_CAPTURE_QUEUE]);
	}

	if (HAS_CREATED_CAPTURE_GLM(pip)) {
		GST_WARNING("Destroy capture manager");
		/* destroy capture glm */
		destroy_capture_glm(app_data);
	}

	/*release remained buffer*/
	GST_WARNING("release remained buffer");
	g_queue_free_full(app_data->capture_buffer_list[0], destroy_CaptureJpegData);
	app_data->capture_buffer_list[0] = g_queue_new();
	g_queue_free_full(app_data->capture_buffer_list[1], destroy_CaptureRawData);
	app_data->capture_buffer_list[1] = g_queue_new();

	IF_G_MUTEX_UNLOCK(app_data->thread_mutex[MU_CAPTURE_QUEUE]);

	endfunc

	return NULL;
}

static gboolean
_sink_query_function (GstPad * pad, GstObject * parent, GstQuery * query)
{
  gboolean res;
  startfunc

  switch (GST_QUERY_TYPE (query)) {
    case GST_QUERY_CAPS:{
       GST_WARNING("in gst sink query caps");
       GstCaps *caps;
       
       caps = gst_static_pad_template_get_caps (&sink_factory);
       gst_query_set_caps_result (query, caps);
       gst_caps_unref (caps);
      res = TRUE;
      break;
    }
    default:
      res = gst_pad_query_default (pad, parent, query);
      break;
  }
  	endfunc
  return res;
}

/* initialize the new element
 * instantiate pads and add them to element
 * set functions
 * initialize structure
 */
static void gst_pip_init (GstPip * pip, GstPipClass * gclass)
{
	startfunc
	pip->srcpad = gst_pad_new_from_static_template (&src_factory, "src");

	gst_pad_use_fixed_caps (pip->srcpad);
	gst_element_add_pad (GST_ELEMENT (pip), pip->srcpad);

	pip->collect = gst_collect_pads_new ();

	gst_collect_pads_set_function (pip->collect,
	                               (GstCollectPadsFunction) (GST_DEBUG_FUNCPTR (gst_pip_do_buffer)),
	                               pip);

	/* set to clean state */
	gst_pip_reset (pip);

	padnum = 0;

	pip->appdata = init_app_data();
	pip->push_possible = TRUE;

	gst_pip_reset_property(pip);

	endfunc
}

static void gst_pip_set_property (GObject *object, guint prop_id,
                                  const GValue * value, GParamSpec * pspec)
{
	startfunc

	GstPip *pip = GST_PIP (object);
	if (!pip->appdata) {
		GST_LOG("appdata is null");
		return;
	}

	GST_INFO("Set: change prop_id is %u", prop_id);

	switch (prop_id) {
	case ARG_INSET_WINDOW_X:
		SET_INT_PROPERTY(pip->appdata->inset_window_x, value, PROC_IW);
		break;
	case ARG_INSET_WINDOW_Y:
		SET_INT_PROPERTY(pip->appdata->inset_window_y, value, PROC_IW);
		break;
	case ARG_INSET_WINDOW_W:
		SET_INT_PROPERTY(pip->appdata->inset_window_w, value, PROC_IW);
		break;
	case ARG_INSET_WINDOW_H:
		SET_INT_PROPERTY(pip->appdata->inset_window_h, value, PROC_IW);
		break;
	case ARG_INSET_WINDOW_PREVIEW_CROP_X:
		SET_INT_PROPERTY(pip->appdata->inset_window_preview_crop_x, value, PROC_IWPC);
		break;
	case ARG_INSET_WINDOW_PREVIEW_CROP_Y:
		SET_INT_PROPERTY(pip->appdata->inset_window_preview_crop_y, value, PROC_IWPC);
		break;
	case ARG_INSET_WINDOW_PREVIEW_CROP_W:
		SET_INT_PROPERTY(pip->appdata->inset_window_preview_crop_w, value, PROC_IWPC);
		break;
	case ARG_INSET_WINDOW_PREVIEW_CROP_H:
		SET_INT_PROPERTY(pip->appdata->inset_window_preview_crop_h, value, PROC_IWPC);
		break;
	case ARG_STYLE_TYPE:
		SET_ENUM_PROPERTY(pip->appdata->style_type, value, PROC_STYLE);
		break;
	case ARG_ORIENTATION_TYPE:
		SET_ENUM_PROPERTY(pip->appdata->orientation, value, PROC_ORIEN);
		break;
	case ARG_INSET_WINDOW_DEVICE:
		SET_ENUM_PROPERTY(pip->appdata->inset_window_device, value, PROC_IWD);
		break;
	case ARG_CAMERA_MODE:
		SET_ENUM_PROPERTY(pip->appdata->camera_mode, value, PROC_CM);
		break;
	case ARG_EFFECT_TYPE:
		SET_ENUM_PROPERTY(pip->appdata->single_camera_effect, value, PROC_EFFECT);
		break;
	case ARG_DOWANLOAD_EFFECT_TYPE:
		SET_STRING_PROPERTY(pip->appdata->single_camera_download_effect, value, PROC_DOWNLOAD_EFFECT);
		break;
	case ARG_PIP_SIGNAL_STILLCAPTURE:
		pip->appdata->signal_still_capture = g_value_get_boolean(value);
		break;
	case ARG_HIDE_INSET_WINDOW:
		SET_BOOLEAN_PROPERTY(pip->appdata->hide_inset_window, value, PROC_HIW);
		break;
	case ARG_ENABLE_FADING:
		//pip->appdata->enable_fading = g_value_get_boolean(value);
		SET_BOOLEAN_PROPERTY(pip->appdata->enable_fading, value, PROC_FADING);
		break;
	case ARG_CAPTURE_WIDTH:
		SET_INT_PROPERTY(pip->appdata->capture_width, value, PROC_CAPTURE_SIZE);
		break;
	case ARG_CAPTURE_HEIGHT:
		SET_INT_PROPERTY(pip->appdata->capture_height, value, PROC_CAPTURE_SIZE);
		break;
	case ARG_READY_TO_STOP:
	{
		gint i = 0;
		GTimeVal abstimeout;

		pip->appdata->ready_to_stop = g_value_get_boolean(value);

		/* wait for output buffer */
		IF_G_MUTEX_LOCK(pip->appdata->thread_mutex[MU_PIXMAP]);
		for (i = 0 ; i < SURFACE_COUNT ; i++) {
			if (pip->appdata->surface_using[i]) {
				GTimeVal abstimeout;
				g_get_current_time(&abstimeout);
				g_time_val_add(&abstimeout, BUFFER_WAIT_TIMEOUT);

				GST_WARNING("wait for output buffer [pixmap id %d]", i);

				if (!g_cond_timed_wait(pip->appdata->thread_cond[MU_PIXMAP], pip->appdata->thread_mutex[MU_PIXMAP], &abstimeout)) {
					GST_ERROR("buffer wait timeout, keep going...");
					break;
				} else {
					GST_WARNING("signal received, check again...");
					i = -1;
				}
			}
		}
		IF_G_MUTEX_UNLOCK(pip->appdata->thread_mutex[MU_PIXMAP]);
		break;
	}
	case ARG_OUTPUT_WIDTH:
		pip->appdata->width[0] = g_value_get_int(value);
		break;
	case ARG_OUTPUT_HEIGHT:
		pip->appdata->height[0] = g_value_get_int(value);
		break;
	case ARG_SET_ROI_INFO:
	{
		memcpy(&pip->appdata->roi_info, (g_value_get_pointer(value)), sizeof(PipGlueFilterArea));
		IF_G_MUTEX_LOCK (pip->appdata->thread_mutex[MU_PRO_CHANGE]);
		pip->appdata->property_change[PROC_ROI_INFO] = TRUE;
		IF_G_MUTEX_UNLOCK (pip->appdata->thread_mutex[MU_PRO_CHANGE]);
	}
	break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}

	endfunc

	return;
}

static void
gst_pip_get_property (GObject * object, guint prop_id,
                      GValue * value, GParamSpec * pspec)
{
	startfunc

	GstPip *pip = GST_PIP (object);
	if(!pip->appdata) {
		GST_LOG("appdata is null");
		return;
	}

	GST_INFO("Get: change prop_id is %u", prop_id);

	switch (prop_id) {
	case ARG_INSET_WINDOW_X:
		g_value_set_int (value, pip->appdata->inset_window_x);
		break;
	case ARG_INSET_WINDOW_Y:
		g_value_set_int (value, pip->appdata->inset_window_y);
		break;
	case ARG_INSET_WINDOW_W:
		g_value_set_int (value, pip->appdata->inset_window_w);
		break;
	case ARG_INSET_WINDOW_H:
		g_value_set_int (value, pip->appdata->inset_window_h);
		break;
	case ARG_INSET_WINDOW_PREVIEW_CROP_X:
		g_value_set_int (value, pip->appdata->inset_window_preview_crop_x);
		break;
	case ARG_INSET_WINDOW_PREVIEW_CROP_Y:
		g_value_set_int (value, pip->appdata->inset_window_preview_crop_y);
		break;
	case ARG_INSET_WINDOW_PREVIEW_CROP_W:
		g_value_set_int (value, pip->appdata->inset_window_preview_crop_w);
		break;
	case ARG_INSET_WINDOW_PREVIEW_CROP_H:
		g_value_set_int (value, pip->appdata->inset_window_preview_crop_h);
		break;
	case ARG_STYLE_TYPE:
		g_value_set_enum (value, pip->appdata->style_type);
		break;
	case ARG_ORIENTATION_TYPE:
		g_value_set_enum (value, pip->appdata->orientation);
		break;
	case ARG_INSET_WINDOW_DEVICE:
		g_value_set_enum (value, pip->appdata->inset_window_device);
		break;
	case ARG_CAMERA_MODE:
		g_value_set_enum (value, pip->appdata->camera_mode);
		break;
	case ARG_EFFECT_TYPE:
		g_value_set_enum (value, pip->appdata->single_camera_effect);
		break;
	case ARG_DOWANLOAD_EFFECT_TYPE:
		g_value_set_string (value, pip->appdata->single_camera_download_effect);
		break;
	case ARG_PIP_SIGNAL_STILLCAPTURE:
		g_value_set_boolean(value, pip->appdata->signal_still_capture);
		break;
	case ARG_HIDE_INSET_WINDOW:
		g_value_set_boolean(value, pip->appdata->hide_inset_window);
		break;
	case ARG_ENABLE_FADING:
		g_value_set_boolean(value, pip->appdata->enable_fading);
		break;
	case ARG_CAPTURE_WIDTH:
		g_value_set_int(value, pip->appdata->capture_width);
		break;
	case ARG_CAPTURE_HEIGHT:
		g_value_set_int(value, pip->appdata->capture_height);
		break;
	case ARG_READY_TO_STOP:
		g_value_set_boolean(value, pip->appdata->ready_to_stop);
		break;
	case ARG_OUTPUT_WIDTH:
		g_value_set_int(value, pip->appdata->width[0]);
		break;
	case ARG_OUTPUT_HEIGHT:
		g_value_set_int(value, pip->appdata->height[0]);
		break;
	case ARG_SET_ROI_INFO:
		g_value_set_pointer(value, &pip->appdata->roi_info);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}

	endfunc

	return;
}

/* GstElement vmethod implementations */

/* this function handles the link with other elements */
static gboolean gst_pip_set_caps (GstPad *pad, GstCaps *caps)
{
	startfunc

	gchar *sink_caps = gst_caps_to_string(caps);
    GstPip *pip = GST_PIP (gst_pad_get_parent (pad));
    GstCaps *outcaps = NULL;

	GST_WARNING("sink_caps [%s]", sink_caps);
    
	outcaps = gst_static_pad_template_get_caps (&src_factory);
	gst_pad_push_event (pip->srcpad, gst_event_new_caps (outcaps));

    g_free(sink_caps);
	gst_caps_unref (outcaps);
	return gst_pad_set_caps (pad, caps);
}

static void gst_pip_release_pad (GstElement *element, GstPad *pad)
{
	startfunc

	GstPip *pip = GST_PIP (element);
	GSList *node;

	node = pip->sinkpads;
	while (node) {
		GstPipPad *pippad = (GstPipPad *) node->data;
		if (pippad->collect->pad == pad) {
			GST_LOG("Release requested pad[%s]", GST_PAD_NAME(pad));
			/* pad count should not be adjusted,
			* as it also represent number of streams present */
			pippad->collect = NULL;
			GST_LOG ("removed pad '%s'", GST_PAD_NAME (pad));
			gst_collect_pads_remove_pad (pip->collect, pad);
			gst_element_remove_pad (element, pad);
			/* if not started yet, we can remove any sign this pad ever existed */
			/* in this case _start will take care of the real pad count */
			pip->sinkpads = g_slist_remove (pip->sinkpads, pippad);
			gst_pip_pad_reset (pippad, TRUE);
			g_free (pippad);
			padnum--;
			return;
		}
		node = node->next;
	}

	GST_LOG ("Unknown pad %s", GST_PAD_NAME (pad));
	endfunc

	return;
}

static gboolean gst_pip_handle_sink_event (GstPad *pad,GstObject * parent, GstEvent *event)
{
	startfunc

	gboolean ret = TRUE;

	GstPip *pip = GST_PIP (parent);
	switch (GST_EVENT_TYPE (event)) {
	case GST_EVENT_EOS:
		GST_ERROR("pip recieved eos event");
		if (pip->srcpad) {
			GST_ERROR ("start push event start");
			gst_pad_push_event (pip->srcpad, gst_event_new_eos ());
			GST_ERROR ("start push event end");
		}
	default:
		break;
	}

	endfunc

	return ret;
}

static GstPad* gst_pip_request_new_pad (GstElement *element,
                                        GstPadTemplate *templ, const gchar *req_name,const GstCaps * caps)
{
	startfunc

	GstPip *pip = (GstPip *) element;
	GstElementClass *klass = GST_ELEMENT_GET_CLASS (element);
	GstPipPad *pippad = NULL;
	GstPad *newpad = NULL;
	gchar *name = NULL;
	const gchar *pad_name = NULL;
	gint pad_id;
	g_return_val_if_fail (templ != NULL, NULL);

	GST_WARNING("template not null in gst_pip_request_new_pad");

	g_return_val_if_fail (templ->direction == GST_PAD_SINK, NULL);

	GST_WARNING("template direction is sink in gst_pip_request_new_pad");
	GST_INFO("req_name = %s", req_name);

	/* create pad */
	if (templ == gst_element_class_get_pad_template (klass, "camera_%d")) {
		if (pip->camera_pads > 1) {
			GST_WARNING("Already have a camera pad");
			return NULL;
		}

		/* don't mix named and unnamed pads, if the pad already exists we fail when
		 * trying to add it */
		if (req_name != NULL && sscanf (req_name, "camera_%02d", &pad_id) == 1) {
			pad_name = req_name;
		} else {
			name = g_strdup_printf ("camera_%02d", pip->camera_pads++);
			pad_name = name;
		}
		/* init pad specific data */
		pippad = g_malloc0 (sizeof (GstPipPad));
		pippad->padnum = padnum++;
		pip->sinkpads = g_slist_append (pip->sinkpads, pippad);
	} else {
		GST_WARNING("Invalid template");
		return NULL;
	}

	GST_WARNING("pad name in gst_pip_request_new_pad is %s", pad_name);

	newpad = gst_pad_new_from_template (templ, pad_name);
	pippad->capsfunc = GST_DEBUG_FUNCPTR (gst_pip_set_caps);
	pippad->collect = gst_collect_pads_add_pad (pip->collect, newpad, sizeof (GstPipCollectData),(GstCollectDataDestroyNotify) gst_pip_pad_free,FALSE);

	((GstPipCollectData *) (pippad->collect))->pippad = pippad;

	GST_INFO("Number of pads is %d in gst_pip_request_new_pad", padnum);

	pip->collect_event = (GstPadEventFunction) GST_PAD_EVENTFUNC (newpad);
	gst_pad_set_event_function (newpad,GST_DEBUG_FUNCPTR (gst_pip_handle_sink_event));

	gst_element_add_pad (element, newpad);

	gst_pad_set_query_function (newpad, _sink_query_function);

	/* we love debug output (c) (tm) (r) */
	GST_INFO ("Created %s pad",pad_name);

	g_free (name);
	name = NULL;

	endfunc
	return newpad;
}

static GstCaps *gst_set_srcpad_caps(gint width, gint height, gint rate, gint scale)
{
	startfunc
	GstCaps *caps = gst_caps_new_simple("video/x-raw", NULL);

	if (!caps) {
		GST_ERROR("failed to alloc caps");
		return NULL;
	}
	gst_caps_set_simple(caps, "format", G_TYPE_STRING, "SR32", NULL);
	gst_caps_set_simple(caps, "bpp", G_TYPE_INT, 32, NULL);
	gst_caps_set_simple(caps, "depth", G_TYPE_INT, 24, NULL);
	gst_caps_set_simple(caps, "endianness", G_TYPE_INT, 4321, NULL);
	gst_caps_set_simple(caps, "red_mask", G_TYPE_INT, 65280, NULL);
	gst_caps_set_simple(caps, "green_mask", G_TYPE_INT, 16711680, NULL);
	gst_caps_set_simple(caps, "blue_mask", G_TYPE_INT, -16777216, NULL);
	gst_caps_set_simple(caps, "width", G_TYPE_INT, width, NULL);
	gst_caps_set_simple(caps, "height", G_TYPE_INT, height, NULL);
	gst_caps_set_simple(caps, "framerate", GST_TYPE_FRACTION, rate, scale, NULL);

	gchar *type = gst_caps_to_string(caps);

	GST_WARNING("Set srcpad caps: %s",type);

	g_free(type);

	return caps;
}

static MMVideoBuffer *gst_pip_make_tbm_buffer(OutputData *output)
{
	if (!output) {
		GST_LOG("output is null");
		return NULL;
	}
	MMVideoBuffer *psimgb = NULL;
	psimgb = (MMVideoBuffer *)malloc(sizeof(MMVideoBuffer));

	if (!psimgb) {
		GST_LOG("Failed to alloc SCMN_IMGB");
		return NULL;
	}

	memset(psimgb, 0x00, sizeof(MMVideoBuffer));
	tbm_surface_h surface = output->surface;
	if (!surface) {
		GST_LOG("surface or imgb is null");
		free(psimgb);
		psimgb = NULL;
		return NULL;
	}
	tbm_surface_info_s info;

	tbm_surface_get_info(surface,&info);

	int num_bos = tbm_surface_internal_get_num_bos (surface);

	/*output buffer is SR32 format, which has only one plane*/

	psimgb->type = output->buf_share_method;

	if (psimgb->type == MM_VIDEO_BUFFER_TYPE_DMABUF_FD) {
		psimgb->handle.bo[0] = tbm_surface_internal_get_bo (surface, 0);
		psimgb->handle.dmabuf_fd[0] = tbm_bo_export_fd(psimgb->handle.bo[0]);
	} else {
		psimgb->handle.bo[0] = tbm_surface_internal_get_bo (surface, 0);
	}

	/*psimgb->a[0] point to vitual address of output buffer*/
	psimgb->data[0] = info.planes[0].ptr;
	psimgb->width[0] = tbm_surface_get_width(surface);
	psimgb->height[0] = tbm_surface_get_height(surface);
	psimgb->stride_width[0] = psimgb->width[0];
	psimgb->stride_height[0] = psimgb->height[0];

	return psimgb;
}

/* transfer data */
static GstFlowReturn gst_pip_do_buffer (GstCollectPads *pads, gpointer user_data)
{
	startfunc
	g_return_val_if_fail (user_data != NULL, GST_FLOW_ERROR);
	GstPip *pip = NULL;
	GstFlowReturn ret = GST_FLOW_OK;
	GstCaps *caps = NULL;
	gchar *type = NULL;
	GstStructure *structure = NULL;
	gint width[2] = {0, };
	gint height[2] = {0, };
	gint rate = 0;
	gint scale = 0;
	guint32 fourcc = 0;
	const gchar *format_gst = NULL;
	int i = 0;

	GstBuffer *buf[2] = {NULL, NULL};

	pip = (GstPip *) user_data;
        g_return_val_if_fail (pip->appdata != NULL, GST_FLOW_ERROR);
	GSList *node;
	node = pip->sinkpads;
	GstPipPad *pip_pad = NULL;
	int index = BUF_INVALID;

	for (; node; node = node->next) {
		if(index == BUF_INVALID) {
			index = BUF_0;
		} else if(index == BUF_0) {
			index = BUF_1;
		} else {
			goto ERROR;
		}

		pip_pad = (GstPipPad *) node->data;
		buf[index] = gst_collect_pads_pop (pip->collect, pip_pad->collect);
		if (HAS_SETTED_INPUT_BUF_PROP(pip)) {
			GST_LOG("Input buffer property has been setted");
		} else {
			if(buf[index]) {

				MMVideoBuffer *imgb = NULL;
				const GValue *famerate = NULL;
				caps =  gst_pad_get_current_caps( pip_pad->collect->pad);
				structure = gst_caps_get_structure (caps, 0);
				if (!gst_structure_get_int (structure, "width", &width[index]) ||
				    !gst_structure_get_int (structure, "height", &height[index])) {
					GST_ERROR("Get width and height failed");
					goto ERROR;
				}
				famerate = gst_structure_get_value(structure, "framerate");
				if (!famerate) {
					GST_ERROR("Get famerate failed");
					goto ERROR;
				}
				rate = gst_value_get_fraction_numerator(famerate);
				scale = gst_value_get_fraction_denominator(famerate);
				type = gst_caps_to_string(caps);
				if (type) {
					GST_DEBUG("GstCaps received in gst_pip_do_buffer is %s", type);
					g_free(type);
				}

				if (format_gst = gst_structure_get_string (structure, "format")) {
					if (!strcmp(format_gst,"S420") || !strcmp(format_gst,"SN12")) {
						/*get buffer share method*/
						GstMapInfo mapInfo;
						GstMemory *img_buf_memory = NULL;
						if (buf[index] && gst_buffer_n_memory(buf[index]) > 1){
							GST_WARNING_OBJECT(pip, "Its zero copy");
							img_buf_memory = gst_buffer_peek_memory(buf[index],1);
							gst_memory_map(img_buf_memory,&mapInfo,GST_MAP_READ);
							imgb = (MMVideoBuffer *)mapInfo.data;
							gst_memory_unmap(img_buf_memory,&mapInfo);
						}
						if(imgb){
							pip->appdata->buf_share_method = imgb->type;
						}
						GST_WARNING("buffer share method : %d", pip->appdata->buf_share_method);
					}
				} else {
					GST_ERROR("Get fourcc failed");
					goto ERROR;
				}
				gst_caps_unref(caps);
			} else {
				GST_ERROR("Get buffer failed");
				goto ERROR;
			}
		}
	}

	if (!HAS_SETTED_INPUT_BUF_PROP(pip)) {
		init_input_buf_property(pip->appdata, width[0], height[0], width[1], height[1], rate, scale, convert_string_to_fourcc_value(format_gst));
	}

	if (pip->appdata->buffer_flush_done) {
		GST_WARNING_OBJECT(pip, "skip this buffers to stop");
		for (i = 0 ; i < BUFFER_MAX ; i++) {
			if (buf[i]) {
				gst_buffer_unref(buf[i]);
				buf[i] = NULL;
			}
		}
	} else {
		/*Send input buffers to input queue for handling*/
		handle_camerasrc_buffer(pip->appdata, buf);
	}

	endfunc

	return ret;

ERROR:
	GST_LOG("ERROR");
	if (pip->srcpad) {
        GST_WARNING_OBJECT(pip, "got error, so sending  eos event");
		gst_pad_push_event (pip->srcpad, gst_event_new_eos ());
	}
	return GST_FLOW_ERROR;
}

static GstStateChangeReturn gst_pip_change_state (GstElement * element, GstStateChange transition)
{
	GstFlowReturn ret;
	GstPip *pip = (GstPip *) (element);
	switch (transition) {
	case GST_STATE_CHANGE_NULL_TO_READY:
		GST_WARNING("GST PIP: NULL -> READY");
		break;
	case GST_STATE_CHANGE_READY_TO_PAUSED:
		GST_WARNING("GST PIP: READY -> PAUSED");

		/* init ready_to_stop */
		pip->appdata->ready_to_stop = FALSE;
		pip->appdata->buffer_flush_done = FALSE;

		/*init working threads*/
		init_working_threads(pip->appdata);
		pip->appdata->threads[TH_CAPTURE] = g_thread_create ( (GThreadFunc) gst_pip_capture_thread_cb, pip, TRUE, NULL);
		pip->appdata->threads[TH_HANDLE_OUT_BUF] = g_thread_create ( (GThreadFunc) gst_pip_handle_outbuf_thread_cb, pip, TRUE, NULL);

		gst_collect_pads_start (pip->collect);
		break;
	case GST_STATE_CHANGE_PAUSED_TO_PLAYING:
		GST_WARNING("GST PIP: PAUSED -> PLAYING");
		pip->push_possible = TRUE;
		break;
	case GST_STATE_CHANGE_PAUSED_TO_READY:
		GST_WARNING("GST PIP: PAUSED -> READY");
		gst_collect_pads_stop (pip->collect);
		break;
	default:
		break;
	}

	ret = GST_ELEMENT_CLASS (parent_class)->change_state (element, transition);
	switch (transition) {
	case GST_STATE_CHANGE_PLAYING_TO_PAUSED:
		GST_WARNING("GST PIP: PLAYING -> PAUSED");
		pip->push_possible = FALSE;
		break;
	case GST_STATE_CHANGE_PAUSED_TO_READY:
		GST_WARNING("GST PIP: PAUSED -> READY");
		gst_pip_reset (pip);
		AppData *app_data = pip->appdata;
		if (!app_data) {
			GST_WARNING("AppData is null");
			return ret;
		}

		/*destroy working threads*/
		destroy_working_threads(pip->appdata);

		/*init srcpad*/
		break;
	case GST_STATE_CHANGE_READY_TO_NULL:
		GST_WARNING("GST PIP: READY -> NULL");
		break;
	default:
		break;
	}

	return ret;
}



/* entry point to initialize the plug-in
 * initialize the plug-in itself
 * register the element factories and pad templates
 * register the features
 *
 * exchange the string 'plugin' with your elemnt name
 */
static gboolean
plugin_init (GstPlugin * plugin)
{
	/* exchange the strings 'plugin' and 'Template plugin' with your
	* plugin name and description */
	GST_DEBUG_CATEGORY_INIT (gst_pip_debug, "pip", 0, "pip");

	return gst_element_register (plugin, "pip", GST_RANK_NONE, GST_TYPE_PIP);
}

/* this is the structure that gstreamer looks for to register plugins
 *
 * exchange the strings 'plugin' and 'Template plugin' with you plugin name and
 * description
 */
GST_PLUGIN_DEFINE (GST_VERSION_MAJOR,
                   GST_VERSION_MINOR,
                    pip,
                   "Picture In Picture",
                   plugin_init,
                   VERSION,
                   "Proprietary",
                   "Samsung Electronics Co",
                   "http://www.samsung.com")
