/*-*- Mode: C; c-basic-offset: 2 -*-*/

/*
 *  GStreamer pulseaudio plugin
 *
 *  Copyright (c) 2004-2008 Lennart Poettering
 *
 *  gst-pulse is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as
 *  published by the Free Software Foundation; either version 2.1 of the
 *  License, or (at your option) any later version.
 *
 *  gst-pulse is distributed in the hope that it will be useful, but
 *  WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with gst-pulse; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301
 *  USA.
 */

#ifndef __GST_PULSESINK_H__
#define __GST_PULSESINK_H__

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef PCM_DUMP_ENABLE
#include <stdio.h>
#endif

#include <gst/gst.h>
#include <gst/audio/audio.h>
#include <gst/audio/gstaudiosink.h>

#include <pulse/pulseaudio.h>
#include <pulse/thread-mainloop.h>

#include "pulseutil.h"

G_BEGIN_DECLS

#define GST_TYPE_PULSESINK \
  (gst_pulsesink_get_type())
#define GST_PULSESINK(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_PULSESINK,GstPulseSink))
#define GST_PULSESINK_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_PULSESINK,GstPulseSinkClass))
#define GST_IS_PULSESINK(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_PULSESINK))
#define GST_IS_PULSESINK_CLASS(obj) \
  (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_PULSESINK))
#define GST_PULSESINK_CAST(obj) \
  ((GstPulseSink *)(obj))

#ifdef __TIZEN__
enum {
  PULSESINK_LOCAL_CONFIGURATION_LOW,
  PULSESINK_LOCAL_CONFIGURATION_MID,
  PULSESINK_LOCAL_CONFIGURATION_HIGH,
  PULSESINK_LOCAL_CONFIGURATION_VERY_HIGH,
  PULSESINK_LOCAL_CONFIGURATION_MAX,
};
#ifdef USE_PA_AUDIO_FILTER
#define CUSTOM_EQ_BAND_MAX				7
#define DHA_GAIN_ARRAY_SIZE				12
enum {
  CUSTOM_EXT_3D_LEVEL,
  CUSTOM_EXT_BASS_LEVEL,
  CUSTOM_EXT_CLARITY_LEVEL,
  CUSTOM_EXT_PARAM_MAX
};
#define CUSTOM_EXT_PULSESINK_MAX				5
#endif /* USE_PA_AUDIO_FILTER */
#endif /* __TIZEN__ */

typedef struct _GstPulseSink GstPulseSink;
typedef struct _GstPulseSinkClass GstPulseSinkClass;

typedef struct _GstPulseDeviceInfo {
  gchar *description;
  GList *formats;
} GstPulseDeviceInfo;

struct _GstPulseSink
{
  GstAudioBaseSink sink;

  gchar *server, *device, *stream_name, *client_name;
  GstPulseDeviceInfo device_info;
#ifdef __TIZEN__
  gint volume_type;
  gint latency;
  gint user_route;
  pa_buffer_attr voip_attr;
#endif

  gdouble volume;
  gboolean volume_set:1;
#ifdef __TIZEN__
  gint mute;
#else
  gboolean mute:1;
#endif
  gboolean mute_set:1;
  guint32 current_sink_idx;
  gchar *current_sink_name;

  guint defer_pending;

  gint notify; /* atomic */

#ifdef __TIZEN__
  gboolean once:1;
  gint mute_requested;
  gint fade;
  gint fade_duration;
  gint support_type;
  gboolean cork_on_prepare;
  gint buffer_time[PULSESINK_LOCAL_CONFIGURATION_MAX];
  gint latency_time[PULSESINK_LOCAL_CONFIGURATION_MAX];
#ifdef PCM_DUMP_ENABLE
  gint need_dump_input;
  FILE *dump_fd_input;
#endif
#ifdef USE_PA_AUDIO_FILTER
  guint		filter_action;
  guint		square_row;
  guint		square_col;
  guint		filter_output_mode;
  guint		preset_mode;
  gint		custom_eq[CUSTOM_EQ_BAND_MAX];
  gint		custom_ext[CUSTOM_EXT_PULSESINK_MAX];
  float		playback_rate;
  gboolean	dha_onoff;
  gint		dha_gain[DHA_GAIN_ARRAY_SIZE];
#endif /* USE_PA_AUDIO_FILTER */
#endif

  const gchar *pa_version;

  GstStructure *properties;
  pa_proplist *proplist;

  GMutex *sink_formats_lock;
  GList *sink_formats;
  volatile gint format_lost;
  GstClockTime format_lost_time;
};

struct _GstPulseSinkClass
{
  GstAudioBaseSinkClass parent_class;
};

GType gst_pulsesink_get_type (void);

#define PULSE_SINK_TEMPLATE_CAPS \
  _PULSE_CAPS_PCM \
  _PULSE_CAPS_AC3 \
  _PULSE_CAPS_EAC3 \
  _PULSE_CAPS_DTS \
  _PULSE_CAPS_MP3 \
  _PULSE_CAPS_AAC

G_END_DECLS

#endif /* __GST_PULSESINK_H__ */
