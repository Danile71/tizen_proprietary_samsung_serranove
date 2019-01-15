 /*
 * gstcamerasrccontrol.h
 *
 * Copyright (c) 2009 - 2015 Samsung Electronics Co., Ltd.
 *
 * Contact: Jeongmo Yang<jm80.yang@samsung.com>
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

#ifndef __GST_CAMERASRC_CONTROL_H__
#define __GST_CAMERASRC_CONTROL_H__

#include <gst/gst.h>
#include <gst/video/cameracontrol.h>
#include "gstqcamerasrc.h"

G_BEGIN_DECLS

#define GST_TYPE_CAMERASRC_CONTROL_CHANNEL (gst_camerasrc_control_channel_get_type ())
#define GST_CAMERASRC_CONTROL_CHANNEL(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), GST_TYPE_CAMERASRC_CONTROL_CHANNEL, GstCamerasrcControlChannel))
#define GST_CAMERASRC_CONTROL_CHANNEL_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), GST_TYPE_CAMERASRC_CONTROL_CHANNEL, GstCamerasrcControlChannelClass))
#define GST_IS_CAMERASRC_CONTROL_CHANNEL(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GST_TYPE_CAMERASRC_CONTROL_CHANNEL))
#define GST_IS_CAMERASRC_CONTROL_CHANNEL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GST_TYPE_CAMERASRC_CONTROL_CHANNEL))

typedef struct _GstCamerasrcControlChannel {
    GstCameraControlChannel parent;
    guint32 id;
} GstCamerasrcControlChannel;

typedef struct _GstCamerasrcControlChannelClass {
    GstCameraControlChannelClass parent;
} GstCamerasrcControlChannelClass;

GType gst_camerasrc_control_channel_get_type( void );

const GList*    gst_camerasrc_control_list_channels( GstCameraSrc* camera_src );

gboolean    gst_camerasrc_control_set_value( GstCameraSrc* camera_src, GstCameraControlChannel* control_channel, gint value );
gboolean    gst_camerasrc_control_get_value( GstCameraSrc* camera_src, GstCameraControlChannel* control_channel, gint* value );
gboolean    gst_camerasrc_control_set_exposure( GstCameraSrc* camera_src, gint type, gint value1, gint value2 );
gboolean    gst_camerasrc_control_get_exposure( GstCameraSrc* camera_src, gint type, gint* value1, gint* value2 );
gboolean    gst_camerasrc_control_set_capture_mode( GstCameraSrc* camera_src, gint type, gint value );
gboolean    gst_camerasrc_control_get_capture_mode( GstCameraSrc* camera_src, gint type, gint* value );
gboolean    gst_camerasrc_control_set_strobe( GstCameraSrc* camera_src, gint type, gint value );
gboolean    gst_camerasrc_control_get_strobe( GstCameraSrc* camera_src, gint type, gint *value );
gboolean    gst_camerasrc_control_set_detect( GstCameraSrc* camera_src, gint type, gint value );
gboolean    gst_camerasrc_control_get_detect( GstCameraSrc* camera_src, gint type, gint* value );
gboolean    gst_camerasrc_control_set_zoom( GstCameraSrc* camera_src, gint type, gint value );
gboolean    gst_camerasrc_control_get_zoom( GstCameraSrc* camera_src, gint type, gint* value );
gboolean    gst_camerasrc_control_set_focus( GstCameraSrc* camera_src, gint mode, gint range );
gboolean    gst_camerasrc_control_get_focus( GstCameraSrc* camera_src, gint* mode, gint* range );
gboolean    gst_camerasrc_control_start_auto_focus( GstCameraSrc* camera_src );
gboolean    gst_camerasrc_control_stop_auto_focus( GstCameraSrc* camera_src );
gboolean    gst_camerasrc_control_set_focus_level( GstCameraSrc* camera_src, gint manual_level );
gboolean    gst_camerasrc_control_get_focus_level( GstCameraSrc* camera_src, gint* manual_level );
gboolean    gst_camerasrc_control_set_auto_focus_area( GstCameraSrc* camera_src, GstCameraControlRectType rect );
gboolean    gst_camerasrc_control_get_auto_focus_area( GstCameraSrc* camera_src, GstCameraControlRectType* rect );
gboolean    gst_camerasrc_control_set_wdr( GstCameraSrc* camera_src, gint value );
gboolean    gst_camerasrc_control_get_wdr( GstCameraSrc* camera_src, gint* value );
gboolean    gst_camerasrc_control_set_ahs( GstCameraSrc* camera_src, gint value );
gboolean    gst_camerasrc_control_get_ahs( GstCameraSrc* camera_src, gint* value );
gboolean    gst_camerasrc_control_set_part_color( GstCameraSrc* camera_src, gint type, gint value );
gboolean    gst_camerasrc_control_get_part_color( GstCameraSrc* camera_src, gint type, gint* value );
gboolean    gst_camerasrc_control_get_exif_info( GstCameraSrc* camera_src, GstCameraControlExifInfo* info );
gboolean    gst_camerasrc_control_get_basic_dev_info( GstCameraSrc* camera_src, gint dev_id, GstCameraControlCapsInfoType* info );
gboolean    gst_camerasrc_control_get_misc_dev_info( GstCameraSrc* camera_src, gint dev_id, GstCameraControlCtrlListInfoType* info );
gboolean    gst_camerasrc_control_get_extra_dev_info( GstCameraSrc* camera_src, gint dev_id, GstCameraControlExtraInfoType* info );
gboolean    gst_camerasrc_control_set_ae_lock( GstCameraSrc* camera_src, gboolean lock );
gboolean    gst_camerasrc_control_get_ae_lock( GstCameraSrc* camera_src, gboolean *lock );
gboolean    gst_camerasrc_control_set_awb_lock( GstCameraSrc* camera_src, gboolean lock );
gboolean    gst_camerasrc_control_get_awb_lock( GstCameraSrc* camera_src, gboolean *lock );
void    gst_camerasrc_control_set_capture_command( GstCameraSrc* camera_src, GstCameraControlCaptureCommand cmd );
void    gst_camerasrc_control_set_record_command( GstCameraSrc* camerasrc, GstCameraControlRecordCommand cmd );

#define GST_IMPLEMENT_CAMERASRC_CONTROL_METHODS(Type, interface_as_function) \
 \
static const GList* \
interface_as_function ## _control_list_channels( GstCameraControl* control ) \
{ \
    Type* this = (Type*) control; \
    return gst_camerasrc_control_list_channels( this ); \
} \
 \
static gboolean \
interface_as_function ## _control_set_value( GstCameraControl* control, \
                             GstCameraControlChannel* control_channel, gint value ) \
{ \
    Type* this = (Type*)control; \
    return gst_camerasrc_control_set_value( this, control_channel, value ); \
} \
 \
static gboolean \
interface_as_function ## _control_get_value( GstCameraControl* control, \
                             GstCameraControlChannel* control_channel, gint* value ) \
{ \
    Type* this = (Type*)control; \
    return gst_camerasrc_control_get_value( this, control_channel, value ); \
} \
 \
static gboolean \
interface_as_function ## _control_set_exposure( GstCameraControl* control, \
                                                       gint type, gint value1, gint value2 ) \
{ \
    Type* this = (Type*)control; \
    return gst_camerasrc_control_set_exposure( this, type, value1, value2 ); \
} \
 \
static gboolean \
interface_as_function ## _control_get_exposure( GstCameraControl* control, \
                                                        gint type, gint* value1, gint* value2 ) \
{ \
    Type* this = (Type*)control; \
    return gst_camerasrc_control_get_exposure( this, type, value1, value2 ); \
} \
 \
static gboolean \
interface_as_function ## _control_set_capture_mode( GstCameraControl* control, \
                                                           gint type, gint value ) \
{ \
    Type* this = (Type*)control; \
    return gst_camerasrc_control_set_capture_mode( this, type, value ); \
} \
 \
static gboolean \
interface_as_function ## _control_get_capture_mode( GstCameraControl* control, \
                                                           gint type, gint* value ) \
{ \
    Type* this = (Type*)control; \
    return gst_camerasrc_control_get_capture_mode( this, type, value ); \
} \
 \
static gboolean \
interface_as_function ## _control_set_strobe( GstCameraControl* control, \
                                                     gint type, gint value ) \
{ \
    Type* this = (Type*)control; \
    return gst_camerasrc_control_set_strobe( this, type, value ); \
} \
 \
static gboolean \
interface_as_function ## _control_get_strobe( GstCameraControl* control, \
                                                     gint type, gint* value ) \
{ \
    Type* this = (Type*)control; \
    return gst_camerasrc_control_get_strobe( this, type, value ); \
} \
 \
static gboolean \
interface_as_function ## _control_set_detect( GstCameraControl* control, \
                                                     gint type, gint value ) \
{ \
    Type* this = (Type*)control; \
    return gst_camerasrc_control_set_detect( this, type, value ); \
} \
 \
static gboolean \
interface_as_function ## _control_get_detect( GstCameraControl* control, \
                                                     gint type, gint* value ) \
{ \
    Type* this = (Type*)control; \
    return gst_camerasrc_control_get_detect( this, type, value ); \
} \
 \
static gboolean \
interface_as_function ## _control_set_zoom( GstCameraControl* control, \
                                                   gint type, gint value ) \
{ \
Type* this = (Type*)control; \
return gst_camerasrc_control_set_zoom( this, type, value ); \
} \
 \
static gboolean \
interface_as_function ## _control_get_zoom( GstCameraControl* control, \
                                                   gint type, gint* value ) \
{ \
    Type* this = (Type*)control; \
    return gst_camerasrc_control_get_zoom( this, type, value ); \
} \
 \
static gboolean \
interface_as_function ## _control_set_focus( GstCameraControl* control, \
                                                    gint focus_mode, gint focus_range ) \
{ \
    Type* this = (Type*)control; \
    return gst_camerasrc_control_set_focus( this, focus_mode, focus_range ); \
} \
 \
static gboolean \
interface_as_function ## _control_get_focus( GstCameraControl* control, \
                                                    gint* focus_mode, gint* focus_range ) \
{ \
    Type* this = (Type*)control; \
    return gst_camerasrc_control_get_focus( this, focus_mode, focus_range ); \
} \
 \
static gboolean \
interface_as_function ## _control_start_auto_focus( GstCameraControl* control) \
{ \
    Type* this = (Type*)control; \
    return gst_camerasrc_control_start_auto_focus( this ); \
} \
 \
static gboolean \
interface_as_function ## _control_stop_auto_focus( GstCameraControl* control) \
{ \
    Type* this = (Type*)control; \
    return gst_camerasrc_control_stop_auto_focus( this ); \
} \
 \
static gboolean \
interface_as_function ## _control_set_focus_level( GstCameraControl* control, \
                                                          gint focus_level ) \
{ \
    Type* this = (Type*)control; \
    return gst_camerasrc_control_set_focus_level( this, focus_level ); \
} \
 \
static gboolean \
interface_as_function ## _control_get_focus_level( GstCameraControl* control, \
                                                          gint* focus_level ) \
{ \
    Type* this = (Type*)control; \
    return gst_camerasrc_control_get_focus_level( this, focus_level ); \
} \
 \
static gboolean \
interface_as_function ## _control_set_auto_focus_area( GstCameraControl* control, \
                                                              GstCameraControlRectType rect ) \
{ \
    Type* this = (Type*)control; \
    return gst_camerasrc_control_set_auto_focus_area( this, rect ); \
} \
 \
static gboolean \
interface_as_function ## _control_get_auto_focus_area( GstCameraControl* control, \
                                                              GstCameraControlRectType* rect ) \
{ \
    Type* this = (Type*)control; \
    return gst_camerasrc_control_get_auto_focus_area( this, rect ); \
} \
 \
static gboolean \
interface_as_function ## _control_set_wdr( GstCameraControl* control, \
                                                  gint value ) \
{ \
    Type* this = (Type*)control; \
    return gst_camerasrc_control_set_wdr( this, value ); \
} \
 \
static gboolean \
interface_as_function ## _control_get_wdr( GstCameraControl* control, \
                                                  gint* value ) \
{ \
    Type* this = (Type*)control; \
    return gst_camerasrc_control_get_wdr( this, value ); \
} \
 \
static gboolean \
interface_as_function ## _control_set_ahs( GstCameraControl* control, \
                                                  gint value ) \
{ \
    Type* this = (Type*)control; \
    return gst_camerasrc_control_set_ahs( this, value ); \
} \
 \
static gboolean \
interface_as_function ## _control_get_ahs( GstCameraControl* control, \
                                                  gint* value ) \
{ \
    Type* this = (Type*)control; \
    return gst_camerasrc_control_get_ahs( this, value ); \
} \
 \
static gboolean \
interface_as_function ## _control_set_part_color( GstCameraControl* control, \
                                                         gint type, gint value ) \
{ \
    Type* this = (Type*)control; \
    return gst_camerasrc_control_set_part_color( this, type, value ); \
} \
 \
static gboolean \
interface_as_function ## _control_get_part_color( GstCameraControl* control, \
                                                         gint type, gint* value ) \
{ \
    Type* this = (Type*)control; \
    return gst_camerasrc_control_get_part_color( this, type, value ); \
} \
 \
static gboolean \
interface_as_function ## _control_get_exif_info( GstCameraControl* control, \
                                                        GstCameraControlExifInfo* info ) \
{ \
    Type* this = (Type*)control; \
    return gst_camerasrc_control_get_exif_info( this, info); \
} \
 \
static gboolean \
interface_as_function ## _control_get_basic_dev_info( GstCameraControl* control, \
                                                             gint dev_id, \
                                                             GstCameraControlCapsInfoType* info ) \
{ \
    Type* this = (Type*)control; \
    return gst_camerasrc_control_get_basic_dev_info( this, dev_id, info); \
} \
 \
static gboolean \
interface_as_function ## _control_get_misc_dev_info( GstCameraControl* control, \
                                                            gint dev_id, \
                                                            GstCameraControlCtrlListInfoType* info ) \
{ \
    Type* this = (Type*)control; \
    return gst_camerasrc_control_get_misc_dev_info( this, dev_id, info); \
} \
 \
static gboolean \
interface_as_function ## _control_get_extra_dev_info( GstCameraControl* control, \
                                                             gint dev_id, \
                                                             GstCameraControlExtraInfoType* info ) \
{ \
    Type* this = (Type*)control; \
    return gst_camerasrc_control_get_extra_dev_info( this, dev_id, info); \
} \
 \
static void \
interface_as_function ## _control_set_capture_command( GstCameraControl* control, \
                                                              GstCameraControlCaptureCommand cmd ) \
{ \
    Type* this = (Type*)control; \
    gst_camerasrc_control_set_capture_command( this, cmd ); \
    return; \
} \
 \
static void \
interface_as_function ## _control_set_record_command( GstCameraControl* control, \
                                                              GstCameraControlRecordCommand cmd ) \
{ \
    Type* this = (Type*)control; \
    gst_camerasrc_control_set_record_command( this, cmd ); \
    return; \
} \
 \
static void \
interface_as_function ## _control_set_ae_lock( GstCameraControl* control, \
                                               gboolean lock ) \
{ \
    Type* this = (Type*)control; \
    gst_camerasrc_control_set_ae_lock( this, lock ); \
    return; \
} \
 \
static void \
interface_as_function ## _control_get_ae_lock( GstCameraControl* control, \
                                               gboolean *lock ) \
{ \
    Type* this = (Type*)control; \
    gst_camerasrc_control_get_ae_lock( this, lock ); \
    return; \
} \
 \
static void \
interface_as_function ## _control_set_awb_lock( GstCameraControl* control, \
                                                gboolean lock ) \
{ \
    Type* this = (Type*)control; \
    gst_camerasrc_control_set_awb_lock( this, lock ); \
    return; \
} \
 \
static void \
interface_as_function ## _control_get_awb_lock( GstCameraControl* control, \
                                                gboolean *lock ) \
{ \
    Type* this = (Type*)control; \
    gst_camerasrc_control_get_awb_lock( this, lock ); \
    return; \
} \
 \
void \
interface_as_function ## _control_interface_init( GstCameraControlClass *klass ) \
{ \
    GST_CAMERA_CONTROL_TYPE( klass ) = GST_CAMERA_CONTROL_HARDWARE; \
 \
    /* default virtual functions */ \
    klass->list_channels    = interface_as_function ## _control_list_channels; \
    klass->set_value    = interface_as_function ## _control_set_value; \
    klass->get_value    = interface_as_function ## _control_get_value; \
    klass->set_exposure = interface_as_function ## _control_set_exposure; \
    klass->get_exposure = interface_as_function ## _control_get_exposure; \
    klass->set_capture_mode = interface_as_function ## _control_set_capture_mode; \
    klass->get_capture_mode = interface_as_function ## _control_get_capture_mode; \
    klass->set_strobe   = interface_as_function ## _control_set_strobe; \
    klass->get_strobe   = interface_as_function ## _control_get_strobe; \
    klass->set_detect   = interface_as_function ## _control_set_detect; \
    klass->get_detect   = interface_as_function ## _control_get_detect; \
    klass->set_zoom    = interface_as_function ## _control_set_zoom; \
    klass->get_zoom    = interface_as_function ## _control_get_zoom; \
    klass->set_focus    = interface_as_function ## _control_set_focus; \
    klass->get_focus    = interface_as_function ## _control_get_focus; \
    klass->start_auto_focus    = interface_as_function ## _control_start_auto_focus; \
    klass->stop_auto_focus    = interface_as_function ## _control_stop_auto_focus; \
    klass->set_focus_level    = interface_as_function ## _control_set_focus_level; \
    klass->get_focus_level    = interface_as_function ## _control_get_focus_level; \
    klass->set_auto_focus_area    = interface_as_function ## _control_set_auto_focus_area; \
    klass->get_auto_focus_area    = interface_as_function ## _control_get_auto_focus_area; \
    klass->set_wdr    = interface_as_function ## _control_set_wdr; \
    klass->get_wdr    = interface_as_function ## _control_get_wdr; \
    klass->set_ahs    = interface_as_function ## _control_set_ahs; \
    klass->get_ahs    = interface_as_function ## _control_get_ahs; \
    klass->set_part_color   = interface_as_function ## _control_set_part_color; \
    klass->get_part_color   = interface_as_function ## _control_get_part_color; \
    klass->get_exif_info    = interface_as_function ## _control_get_exif_info; \
    klass->get_basic_dev_info    = interface_as_function ## _control_get_basic_dev_info; \
    klass->get_misc_dev_info    = interface_as_function ## _control_get_misc_dev_info; \
    klass->get_extra_dev_info   = interface_as_function ## _control_get_extra_dev_info; \
    klass->set_capture_command    = interface_as_function ## _control_set_capture_command; \
    klass->set_record_command    = interface_as_function ## _control_set_record_command; \
    klass->set_ae_lock    = interface_as_function ## _control_set_ae_lock; \
    klass->set_awb_lock    = interface_as_function ## _control_set_awb_lock; \
 \
}

#endif /* __GST_CAMERASRC_CONTROL_H__ */

