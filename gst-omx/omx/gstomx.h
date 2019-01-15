/*
 * Copyright (C) 2011, Hewlett-Packard Development Company, L.P.
 *   Author: Sebastian Dröge <sebastian.droege@collabora.co.uk>, Collabora Ltd.
 * Copyright (C) 2013, Collabora Ltd.
 *   Author: Sebastian Dröge <sebastian.droege@collabora.co.uk>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation
 * version 2.1 of the License.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA
 *
 */

#ifndef __GST_OMX_H__
#define __GST_OMX_H__

#include <gmodule.h>
#include <gst/gst.h>
#include <string.h>
#include <mm_types.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef GST_OMX_STRUCT_PACKING
# if GST_OMX_STRUCT_PACKING == 1
#  pragma pack(1)
# elif GST_OMX_STRUCT_PACKING == 2
#  pragma pack(2)
# elif GST_OMX_STRUCT_PACKING == 4
#  pragma pack(4)
# elif GST_OMX_STRUCT_PACKING == 8
#  pragma pack(8)
# else
#  error "Unsupported struct packing value"
# endif
#endif

#include <OMX_Core.h>
#include <OMX_Component.h>

#ifdef USE_OMX_TARGET_QCOM
#include "OMX_QCOMExtns.h"
#endif

#include <tbm_type.h>
#include <tbm_surface.h>
#include <tbm_bufmgr.h>
#ifdef GST_OMX_STRUCT_PACKING
#pragma pack()
#endif

G_BEGIN_DECLS

#define GST_OMX_INIT_STRUCT(st) G_STMT_START { \
  memset ((st), 0, sizeof (*(st))); \
  (st)->nSize = sizeof (*(st)); \
  (st)->nVersion.s.nVersionMajor = OMX_VERSION_MAJOR; \
  (st)->nVersion.s.nVersionMinor = OMX_VERSION_MINOR; \
  (st)->nVersion.s.nRevision = OMX_VERSION_REVISION; \
  (st)->nVersion.s.nStep = OMX_VERSION_STEP; \
} G_STMT_END

/* Different hacks that are required to work around
 * bugs in different OpenMAX implementations
 */
/* In the EventSettingsChanged callback use nData2 instead of nData1 for
 * the port index. Happens with Bellagio.
 */
#define GST_OMX_HACK_EVENT_PORT_SETTINGS_CHANGED_NDATA_PARAMETER_SWAP G_GUINT64_CONSTANT (0x0000000000000001)
/* In the EventSettingsChanged callback assume that port index 0 really
 * means port index 1. Happens with the Bellagio ffmpegdist video decoder.
 */
#define GST_OMX_HACK_EVENT_PORT_SETTINGS_CHANGED_PORT_0_TO_1          G_GUINT64_CONSTANT (0x0000000000000002)
/* If the video framerate is not specified as fraction (Q.16) but as
 * integer number. Happens with the Bellagio ffmpegdist video encoder.
 */
#define GST_OMX_HACK_VIDEO_FRAMERATE_INTEGER                          G_GUINT64_CONSTANT (0x0000000000000004)
/* If the SYNCFRAME flag on encoder output buffers is not used and we
 * have to assume that all frames are sync frames.
 * Happens with the Bellagio ffmpegdist video encoder.
 */
#define GST_OMX_HACK_SYNCFRAME_FLAG_NOT_USED                          G_GUINT64_CONSTANT (0x0000000000000008)
/* If the component needs to be re-created if the caps change.
 * Happens with Qualcomm's OpenMAX implementation.
 */
#define GST_OMX_HACK_NO_COMPONENT_RECONFIGURE                         G_GUINT64_CONSTANT (0x0000000000000010)

/* If the component does not accept empty EOS buffers.
 * Happens with Qualcomm's OpenMAX implementation.
 */
#define GST_OMX_HACK_NO_EMPTY_EOS_BUFFER                              G_GUINT64_CONSTANT (0x0000000000000020)

/* If the component might not acknowledge a drain.
 * Happens with TI's Ducati OpenMAX implementation.
 */
#define GST_OMX_HACK_DRAIN_MAY_NOT_RETURN                             G_GUINT64_CONSTANT (0x0000000000000040)

/* If the component doesn't allow any component role to be set.
 * Happens with Broadcom's OpenMAX implementation.
 */
#define GST_OMX_HACK_NO_COMPONENT_ROLE                                G_GUINT64_CONSTANT (0x0000000000000080)

typedef struct _GstOMXCore GstOMXCore;
typedef struct _GstOMXPort GstOMXPort;
typedef enum _GstOMXPortDirection GstOMXPortDirection;
typedef struct _GstOMXComponent GstOMXComponent;
typedef struct _GstOMXBuffer GstOMXBuffer;
typedef struct _GstOMXClassData GstOMXClassData;
typedef struct _GstOMXMessage GstOMXMessage;

/* MODIFICATION */
#define ALIGN(x, a) (((x) + (a) - 1) & ~((a) - 1))
#define MSM_ALIGN(w, h) ALIGN(ALIGN(w, 128) * ALIGN(h, 32) + ALIGN(w, 128) * ALIGN((h+1)/2, 16) + (4*1024)+(8*1024), (4*1024)) // 4K buffer alignment for QC component
#define SPRD_ALIGN(w, h) (ALIGN(w, 16) * ALIGN(h, 16) * 3 / 2)
#define MAX_DEVICE_WIDTH 1280
#define MAX_DEVICE_HEIGHT 720

#define MAX_GEM_BUFFER_NUM 32 /* FIX ME: need to check max num */

/* image buffer definition
    +------------------------------------------+ ---
    |                                          |  ^
    |     a[], p[]                             |  |
    |     +---------------------------+ ---    |  |
    |     |                           |  ^     |  |
    |     |<---------- w[] ---------->|  |     |  |
    |     |                           |  |     |  |
    |     |                           |        |
    |     |                           |  h[]   |  e[]
    |     |                           |        |
    |     |                           |  |     |  |
    |     |                           |  |     |  |
    |     |                           |  v     |  |
    |     +---------------------------+ ---    |  |
    |                                          |  v
    +------------------------------------------+ ---

    |<----------------- s[] ------------------>|
*/

typedef enum {
  kMetadataBufferTypeCameraSource  = 0,
  kMetadataBufferTypeGrallocSource = 1,
} MetadataBufferType;

#ifdef USE_OMX_TARGET_QCOM
typedef struct native_handle {
  int version;        /* sizeof(native_handle_t) */
  int numFds;         /* number of file-descriptors at &data[0] */
  int numInts;        /* number of ints at &data[numFds] */
  int data[3];        /* numFds + numInts ints */
} native_handle;

typedef struct encoder_media_buffer_type {
  MetadataBufferType buffer_type;
  native_handle *meta_handle;
} encoder_media_buffer_type;
#endif

#ifdef USE_OMX_TARGET_SPRD
typedef struct sprd_encoder_meta_data {
  MetadataBufferType buffer_type;
  int shared_buf_fd;
  int* phy_addr;
  int* virt_addr;
} sprd_encoder_meta_data;
#endif

/* Extended color formats */
enum {
  OMX_EXT_COLOR_FormatNV12TPhysicalAddress = 0x7F000001, /**< Reserved region for introducing Vendor Extensions */
  OMX_EXT_COLOR_FormatNV12LPhysicalAddress = 0x7F000002,
  OMX_EXT_COLOR_FormatNV12Tiled = 0x7FC00002,
  OMX_EXT_COLOR_FormatNV12TFdValue = 0x7F000012,
  OMX_EXT_COLOR_FormatNV12LFdValue = 0x7F000013
};

/* modification: Add_component_vendor */
typedef enum
{
  GST_OMX_VENDOR_DEFAULT,
  GST_OMX_VENDOR_SLSI_SEC,
  GST_OMX_VENDOR_SLSI_EXYNOS,
  GST_OMX_VENDOR_QCT,
  GST_OMX_VENDOR_SPRD
} GstOMXVendor;

/* modification: buffer type */
typedef enum
{
  GST_OMX_BUFFER_DEFAULT,
  GST_OMX_BUFFER_GST,
  GST_OMX_BUFFER_TBM_VDEC_OUTPUT,
  GST_OMX_BUFFER_SECURE_VDEC_INPUT,
  GST_OMX_BUFFER_SECURE_VDEC_OUTPUT,
  GST_OMX_BUFFER_SECURE_VENC_OUTPUT
} GstOMXPortBufferType;

typedef enum GstOmxCodecType
{
  GST_OMX_CODEC_TYPE_DEFAULT,
  GST_OMX_CODEC_TYPE_VIDEO_DEC,
  GST_OMX_CODEC_TYPE_VIDEO_ENC,
  GST_OMX_CODEC_TYPE_AUDIO_DEC
} GstOmxCodecType;

typedef enum {
  /* Everything good and the buffer is valid */
  GST_OMX_ACQUIRE_BUFFER_OK = 0,
  /* The port is flushing, exit ASAP */
  GST_OMX_ACQUIRE_BUFFER_FLUSHING,
  /* The port must be reconfigured */
  GST_OMX_ACQUIRE_BUFFER_RECONFIGURE,
  /* The port is EOS */
  GST_OMX_ACQUIRE_BUFFER_EOS,
  /* A fatal error happened */
  GST_OMX_ACQUIRE_BUFFER_ERROR
} GstOMXAcquireBufferReturn;

typedef struct {
  tbm_bo bo_y;
  tbm_bo bo_uv;
} GstOmxDecOutputBufferObject;

struct _GstOMXCore {
  /* Handle to the OpenMAX IL core shared library */
  GModule *module;

  /* Current number of users, transitions from/to 0
   * call init/deinit */
  GMutex lock;
  gint user_count; /* LOCK */

  /* MODIFICATION */
  GstOMXVendor component_vendor; /* to check omx vender */
  gboolean secure; /* trust zone */

  OMX_VIDEO_CODINGTYPE compression_format; /* this is for Exynos align func */

  /* OpenMAX core library functions, protected with LOCK */
  OMX_ERRORTYPE (*init) (void);
  OMX_ERRORTYPE (*deinit) (void);
  OMX_ERRORTYPE (*get_handle) (OMX_HANDLETYPE * handle, OMX_STRING name, OMX_PTR data, OMX_CALLBACKTYPE * callbacks);
  OMX_ERRORTYPE (*free_handle) (OMX_HANDLETYPE handle);
  OMX_ERRORTYPE (*setup_tunnel) (OMX_HANDLETYPE output, OMX_U32 outport, OMX_HANDLETYPE input, OMX_U32 inport);
};

typedef enum {
  GST_OMX_MESSAGE_STATE_SET,
  GST_OMX_MESSAGE_FLUSH,
  GST_OMX_MESSAGE_ERROR,
  GST_OMX_MESSAGE_PORT_ENABLE,
  GST_OMX_MESSAGE_PORT_SETTINGS_CHANGED,
  GST_OMX_MESSAGE_BUFFER_FLAG,
  GST_OMX_MESSAGE_BUFFER_DONE,
} GstOMXMessageType;

struct _GstOMXMessage {
  GstOMXMessageType type;

  union {
    struct {
      OMX_STATETYPE state;
    } state_set;
    struct {
      OMX_U32 port;
    } flush;
    struct {
      OMX_ERRORTYPE error;
    } error;
    struct {
      OMX_U32 port;
      OMX_BOOL enable;
    } port_enable;
    struct {
      OMX_U32 port;
    } port_settings_changed;
    struct {
      OMX_U32 port;
      OMX_U32 flags;
    } buffer_flag;
    struct {
      OMX_HANDLETYPE component;
      OMX_PTR app_data;
      OMX_BUFFERHEADERTYPE *buffer;
      OMX_BOOL empty;
    } buffer_done;
  } content;
};

struct _GstOMXPort {
  GstOMXComponent *comp;
  guint32 index;

  gboolean tunneled;

  OMX_PARAM_PORTDEFINITIONTYPE port_def;
  GPtrArray *buffers; /* Contains GstOMXBuffer* */
  GQueue pending_buffers; /* Contains GstOMXBuffer* */
  gboolean flushing;
  gboolean flushed; /* TRUE after OMX_CommandFlush was done */
  gboolean enabled_pending;  /* TRUE after OMX_Command{En,Dis}able */
  gboolean disabled_pending; /* was done until it took effect */
  gboolean eos; /* TRUE after a buffer with EOS flag was received */

  /* Increased whenever the settings of these port change.
   * If settings_cookie != configured_settings_cookie
   * the port has to be reconfigured.
   */
  gint settings_cookie;
  gint configured_settings_cookie;

  tbm_bufmgr bufmgr;
  gint drm_fd;
  MMVideoBuffer mm_video_buf[MAX_GEM_BUFFER_NUM];
  GstOmxDecOutputBufferObject bo[MAX_GEM_BUFFER_NUM];

  /* TRUE if HLS streaming */
  gboolean is_adaptive_stream;

  GstOMXPortBufferType buffer_type;
  GstOmxCodecType codec_type;
  gboolean omx_allocate; /* TRUE if memory allocated by component */

#ifdef USE_OMX_TARGET_QCOM
  guint32 cropped_width;
  guint32 cropped_heigth;
#endif
};

struct _GstOMXComponent {
  GstObject *parent;

  gchar *name; /* for debugging mostly */

  OMX_HANDLETYPE handle;
  GstOMXCore *core;

  guint64 hacks; /* Flags, GST_OMX_HACK_* */

  /* Added once, never changed. No locks necessary */
  GPtrArray *ports; /* Contains GstOMXPort* */
  gint n_in_ports, n_out_ports;

  /* Locking order: lock -> messages_lock
   *
   * Never hold lock while waiting for messages_cond
   * Always check that messages is empty before waiting */
  GMutex lock;

  GQueue messages; /* Queue of GstOMXMessages */
  GMutex messages_lock;
  GCond messages_cond;

  OMX_STATETYPE state;
  /* OMX_StateInvalid if no pending state */
  OMX_STATETYPE pending_state;
  /* OMX_ErrorNone usually, if different nothing will work */
  OMX_ERRORTYPE last_error;

  GList *pending_reconfigure_outports;
  /* TRUE if OMX_IndexConfigCommonOutputCrop received*/
  gboolean crop_changed;
};

struct _GstOMXBuffer {
#ifdef USE_OMX_TARGET_QCOM
  /** Number of entries */
  OMX_U32 nEntries;

  /** Pointer to array of platform specific entries *
   * Contiguous block of OMX_QCOM_PLATFORM_PRIVATE_ENTRY element
  */
  OMX_QCOM_PLATFORM_PRIVATE_ENTRY* entryList;
#endif
#ifdef USE_OMX_TARGET_SPRD
  int shared_buf_fd;
  int phy_addr;
  int phy_size;
#endif

  GstOMXPort *port;
  OMX_BUFFERHEADERTYPE *omx_buf;

  /* TRUE if the buffer is used by the port, i.e.
   * between {Empty,Fill}ThisBuffer and the callback
   */
  gboolean used;

  /* Cookie of the settings when this buffer was allocated */
  gint settings_cookie;

  /* TRUE if this is an EGLImage */
  gboolean eglimage;
};

struct _GstOMXClassData {
  const gchar *core_name;
  const gchar *component_name;
  const gchar *component_role;

  const gchar *default_src_template_caps;
  const gchar *default_sink_template_caps;

  guint32 in_port_index, out_port_index;

  guint64 hacks;
};

#ifdef USE_OMX_TARGET_QCOM
gboolean          create_platform_private_object(GstOMXPort *port, GstOMXBuffer *buf, guint buffer_index);
#endif

GKeyFile *        gst_omx_get_configuration (void);

const gchar *     gst_omx_error_to_string (OMX_ERRORTYPE err);
const gchar *     gst_omx_state_to_string (OMX_STATETYPE state);
const gchar *     gst_omx_command_to_string (OMX_COMMANDTYPE cmd);

guint64           gst_omx_parse_hacks (gchar ** hacks);

GstOMXCore *      gst_omx_core_acquire (const gchar * filename);
void              gst_omx_core_release (GstOMXCore * core);


GstOMXComponent * gst_omx_component_new (GstObject * parent, const gchar *core_name, const gchar *component_name, const gchar * component_role, guint64 hacks);
void              gst_omx_component_free (GstOMXComponent * comp);

OMX_ERRORTYPE     gst_omx_component_set_state (GstOMXComponent * comp, OMX_STATETYPE state);
OMX_STATETYPE     gst_omx_component_get_state (GstOMXComponent * comp, GstClockTime timeout);

OMX_ERRORTYPE     gst_omx_component_get_last_error (GstOMXComponent * comp);
const gchar *     gst_omx_component_get_last_error_string (GstOMXComponent * comp);

GstOMXPort *      gst_omx_component_add_port (GstOMXComponent * comp, guint32 index);
GstOMXPort *      gst_omx_component_get_port (GstOMXComponent * comp, guint32 index);

OMX_ERRORTYPE     gst_omx_component_get_parameter (GstOMXComponent * comp, OMX_INDEXTYPE index, gpointer param);
OMX_ERRORTYPE     gst_omx_component_set_parameter (GstOMXComponent * comp, OMX_INDEXTYPE index, gpointer param);

OMX_ERRORTYPE     gst_omx_component_get_config (GstOMXComponent * comp, OMX_INDEXTYPE index, gpointer config);
OMX_ERRORTYPE     gst_omx_component_set_config (GstOMXComponent * comp, OMX_INDEXTYPE index, gpointer config);
OMX_ERRORTYPE     gst_omx_component_setup_tunnel (GstOMXComponent * comp1, GstOMXPort * port1, GstOMXComponent * comp2, GstOMXPort * port2);
OMX_ERRORTYPE     gst_omx_component_close_tunnel (GstOMXComponent * comp1, GstOMXPort * port1, GstOMXComponent * comp2, GstOMXPort * port2);


OMX_ERRORTYPE     gst_omx_port_get_port_definition (GstOMXPort * port, OMX_PARAM_PORTDEFINITIONTYPE * port_def);
OMX_ERRORTYPE     gst_omx_port_update_port_definition (GstOMXPort *port, OMX_PARAM_PORTDEFINITIONTYPE *port_definition);

GstOMXAcquireBufferReturn gst_omx_port_acquire_buffer (GstOMXPort *port, GstOMXBuffer **buf);
OMX_ERRORTYPE     gst_omx_port_release_buffer (GstOMXPort *port, GstOMXBuffer *buf);

OMX_ERRORTYPE     gst_omx_port_set_flushing (GstOMXPort *port, GstClockTime timeout, gboolean flush);
gboolean          gst_omx_port_is_flushing (GstOMXPort *port);

OMX_ERRORTYPE     gst_omx_port_allocate_buffers (GstOMXPort *port);
OMX_ERRORTYPE     gst_omx_port_use_buffers (GstOMXPort *port, const GList *buffers);
OMX_ERRORTYPE     gst_omx_port_use_eglimages (GstOMXPort *port, const GList *images);
OMX_ERRORTYPE     gst_omx_port_deallocate_buffers (GstOMXPort *port);
OMX_ERRORTYPE     gst_omx_port_populate (GstOMXPort *port);
OMX_ERRORTYPE     gst_omx_port_wait_buffers_released (GstOMXPort * port, GstClockTime timeout);

OMX_ERRORTYPE     gst_omx_port_mark_reconfigured (GstOMXPort * port);

OMX_ERRORTYPE     gst_omx_port_set_enabled (GstOMXPort * port, gboolean enabled);
OMX_ERRORTYPE     gst_omx_port_wait_enabled (GstOMXPort * port, GstClockTime timeout);
gboolean          gst_omx_port_is_enabled (GstOMXPort * port);


void              gst_omx_set_default_role (GstOMXClassData *class_data, const gchar *default_role);


G_END_DECLS

#endif /* __GST_OMX_H__ */
