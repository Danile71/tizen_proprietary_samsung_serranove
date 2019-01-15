 /*
 * gstcamerasrccolorbalance.c
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
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
// 
#include <gst/gst.h>
#include "gstcamerasrccolorbalance.h"

G_DEFINE_TYPE( GstCameraSrcColorBalanceChannel,
                 gst_camerasrc_color_balance_channel,
                 GST_TYPE_COLOR_BALANCE_CHANNEL );

// temp debug message
#ifndef GST_CAMERA_SRC_INTERFACE_DEBUG
//#define GST_CAMERA_SRC_INTERFACE_DEBUG
#endif
#ifdef GST_CAMERA_SRC_INTERFACE_DEBUG
#define gst_camerasrc_debug( fmt, args... ) \
    do { \
        g_print( "[GST_CAMERA_SRC_INTERFACE_DEBUG]:[%05d][%s]: " fmt "\n", __LINE__, __func__, ##args ); \
    } while( 0 )
#else
#define gst_camerasrc_debug(...)
#endif

static void
gst_camerasrc_color_balance_channel_base_init( gpointer g_class )
{
}

static void
gst_camerasrc_color_balance_channel_class_init( GstCameraSrcColorBalanceChannelClass* klass )
{
}

static void
gst_camerasrc_color_balance_channel_init( GstCameraSrcColorBalanceChannel* camerasrc_color_channel) //, GstCameraSrcColorBalanceChannelClass* klass )
{
    camerasrc_color_channel->id = (guint32) - 1;
}

static G_GNUC_UNUSED gboolean
gst_camerasrc_color_balance_contains_channel( GstCameraSrc* camerasrc, GstCameraSrcColorBalanceChannel* camerasrc_color_channel )
{
    const GList *item;

    for( item = camerasrc->colors ; item != NULL ; item = item->next ){
        if (item->data == camerasrc_color_channel)
        return TRUE;
    }

    return FALSE;
}

const GList *
gst_camerasrc_color_balance_list_channels( GstCameraSrc* camerasrc )
{
    return camerasrc->colors;
}

void
gst_camerasrc_color_balance_set_value( GstCameraSrc* camerasrc, GstColorBalanceChannel* color_channel, gint value )
{
    int error = CAMERASRC_ERR_UNKNOWN;

    GstCameraSrcColorBalanceChannel *camerasrc_color_channel = GST_CAMERASRC_COLOR_BALANCE_CHANNEL( color_channel );

    /* assert that we're opened and that we're using a known item */
    g_return_if_fail( camerasrc );
    g_return_if_fail( gst_camerasrc_color_balance_contains_channel( camerasrc, camerasrc_color_channel ) );

    GST_INFO("set [%s] -> value [%d]", color_channel->label, value);

    if(strcmp(color_channel->label,"white balance")==0)
        error = gst_qcamerasrc_set_camera_control(camerasrc,QCAMERASRC_PARAM_CONTROL_WB,value,0,NULL);
    else if(strcmp(color_channel->label,"brightness")==0)
        error = gst_qcamerasrc_set_camera_control(camerasrc,QCAMERASRC_PARAM_CONTROL_BRIGHTNESS,value,0,NULL);
    else if(strcmp(color_channel->label,"contrast")==0)
        error = gst_qcamerasrc_set_camera_control(camerasrc,QCAMERASRC_PARAM_CONTROL_CONTRAST,value,0,NULL);
    else if(strcmp(color_channel->label,"color tone")==0)
        error = gst_qcamerasrc_set_camera_control(camerasrc,QCAMERASRC_PARAM_CONTROL_COLOR_TONE,value,0,NULL);
    else if(strcmp(color_channel->label,"saturation")==0)
        error = gst_qcamerasrc_set_camera_control(camerasrc,QCAMERASRC_PARAM_CONTROL_SATURATION,value,0,NULL);
    else if(strcmp(color_channel->label,"sharpness")==0)
        error = gst_qcamerasrc_set_camera_control(camerasrc,QCAMERASRC_PARAM_CONTROL_SHARPNESS,value,0,NULL);

    if( error != CAMERASRC_SUCCESS )
    {
        GST_WARNING("Failed to Set ColorBalance[%s],value[%d]", camerasrc_color_channel->parent.label, value);
    }
}

gint
gst_camerasrc_color_balance_get_value( GstCameraSrc* camerasrc, GstColorBalanceChannel* color_channel )
{
    int error = CAMERASRC_ERR_UNKNOWN, value;
    GstCameraSrcColorBalanceChannel *camerasrc_color_channel = GST_CAMERASRC_COLOR_BALANCE_CHANNEL( color_channel );

    /* assert that we're opened and that we're using a known item */
    g_return_val_if_fail( camerasrc, FALSE );
    g_return_val_if_fail( gst_camerasrc_color_balance_contains_channel( camerasrc, camerasrc_color_channel ), FALSE );

    if(strcmp(color_channel->label,"white balance")==0)
        error = get_camera_control(camerasrc->qcam_handle,QCAMERASRC_PARAM_CONTROL_WB,&value,0,NULL);
    else if(strcmp(color_channel->label,"brightness")==0)
        error = get_camera_control(camerasrc->qcam_handle,QCAMERASRC_PARAM_CONTROL_BRIGHTNESS,&value,0,NULL);
    else if(strcmp(color_channel->label,"contrast")==0)
        error = get_camera_control(camerasrc->qcam_handle,QCAMERASRC_PARAM_CONTROL_CONTRAST,&value,0,NULL);
    else if(strcmp(color_channel->label,"color tone")==0)
        error = get_camera_control(camerasrc->qcam_handle,QCAMERASRC_PARAM_CONTROL_COLOR_TONE,&value,0,NULL);
    else if(strcmp(color_channel->label,"saturation")==0)
        error = get_camera_control(camerasrc->qcam_handle,QCAMERASRC_PARAM_CONTROL_SATURATION,&value,0,NULL);
    else if(strcmp(color_channel->label,"sharpness")==0)
        error = get_camera_control(camerasrc->qcam_handle,QCAMERASRC_PARAM_CONTROL_SHARPNESS,&value,0,NULL);
    if( error != CAMERASRC_SUCCESS )
    {
        GST_WARNING("Failed to Get ColorBalance[%s],value[%d]", camerasrc_color_channel->parent.label, value);
    }

    return value;
}


