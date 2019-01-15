 /*
 * gstcamerasrccontrol.c
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

#include <gst/gst.h>
#include "gstcamerasrccontrol.h"

// temp debug message
#ifndef GST_CAMERASRC_INTERFACE_PRINT_DEBUG
//#define GST_CAMERASRC_INTERFACE_PRINT_DEBUG
#endif
#ifdef GST_CAMERASRC_INTERFACE_PRINT_DEBUG
#define gst_camerasrc_debug( fmt, args... ) \
    do { \
        g_print( "[GST_CAMERASRC_INTERFACE_DEBUG]:[%05d][%s]: " fmt "\n", __LINE__, __func__, ##args ); \
    } while( 0 )
#else
#define gst_camerasrc_debug( fmt, args... )     GST_DEBUG(fmt, ##args);
#endif

#define CAMERA_CONTROL_AF_STOP_TOTALTIME        2000000
#define CAMERA_CONTROL_AF_STOP_INTERVAL         20000


G_DEFINE_TYPE (GstCamerasrcControlChannel,
    gst_camerasrc_control_channel,
    GST_TYPE_CAMERA_CONTROL_CHANNEL);

static void
gst_camerasrc_control_channel_base_init( gpointer g_class )
{
    gst_camerasrc_debug( "" );
}

static void
gst_camerasrc_control_channel_class_init( GstCamerasrcControlChannelClass* klass )
{
    gst_camerasrc_debug( "" );
}

//TODO: check whether second parameter is needed
static void
gst_camerasrc_control_channel_init( GstCamerasrcControlChannel* control_channel)//, GstCamerasrcControlChannelClass* klass )
{
    gst_camerasrc_debug( "" );

    control_channel->id = (guint32) - 1;
}

static G_GNUC_UNUSED gboolean
gst_camerasrc_control_contains_channel( GstCameraSrc* camerasrc, GstCamerasrcControlChannel* camerasrc_control_channel )
{
    gst_camerasrc_debug( "" );

    const GList* item;

    for( item = camerasrc->camera_controls ; item != NULL ; item = item->next ){
        if( item->data == camerasrc_control_channel ){
            return TRUE;
        }
    }

    return FALSE;
}

const GList*
gst_camerasrc_control_list_channels( GstCameraSrc* camerasrc )
{
    gst_camerasrc_debug( "" );

    return camerasrc->camera_controls;
}

gboolean
gst_camerasrc_control_set_value( GstCameraSrc* camerasrc, GstCameraControlChannel *control_channel, gint value )
{
    gst_camerasrc_debug( "" );

    int error = CAMERASRC_ERROR;

    GstCamerasrcControlChannel *camerasrc_control_channel = GST_CAMERASRC_CONTROL_CHANNEL( control_channel );

    g_return_val_if_fail( camerasrc, FALSE );
    g_return_val_if_fail( gst_camerasrc_control_contains_channel( camerasrc, camerasrc_control_channel ), FALSE );

    //TODO:Figure out later
    //error = camerasrc_set_control( camerasrc->qcam_handle, camerasrc_control_channel->id, value );

    if( error != CAMERASRC_SUCCESS ){
        gst_camerasrc_debug( "Failed to set value. Ctrl-id [%d], value [%d], err code [%d]", camerasrc_control_channel->id, value, error );
        return FALSE;
    }

    return TRUE;
}

gboolean
gst_camerasrc_control_get_value( GstCameraSrc* camerasrc, GstCameraControlChannel *control_channel, gint *value )
{
    gst_camerasrc_debug( "" );

    int error = CAMERASRC_ERROR;

    GstCamerasrcControlChannel *camerasrc_control_channel = GST_CAMERASRC_CONTROL_CHANNEL( control_channel );

    g_return_val_if_fail( camerasrc, FALSE );
    g_return_val_if_fail( gst_camerasrc_control_contains_channel( camerasrc, camerasrc_control_channel ), FALSE );

    //TODO:Figure out later
    //error = camerasrc_get_control( camerasrc->qcam_handle, camerasrc_control_channel->id, value );

    if( error != CAMERASRC_SUCCESS ){
        gst_camerasrc_debug( "Failed to get control value. Ctrl-id [%d], err code[%x]", camerasrc_control_channel->id, error );
        return FALSE;
    }

    return TRUE;
}

gboolean
gst_camerasrc_control_set_exposure( GstCameraSrc* camerasrc, gint type, gint value1, gint value2 )
{
    gst_camerasrc_debug( "" );

    int error = -1;

    g_return_val_if_fail( camerasrc, FALSE );

    // TODO : F number
    switch( type )
    {
        case GST_CAMERA_CONTROL_F_NUMBER:
        break;
        case GST_CAMERA_CONTROL_SHUTTER_SPEED:
        break;
        case GST_CAMERA_CONTROL_ISO:
            error = gst_qcamerasrc_set_camera_control(camerasrc,QCAMERASRC_PARAM_CONTROL_ISO,value1,value2,NULL);
        break;
        case GST_CAMERA_CONTROL_PROGRAM_MODE:
            error = gst_qcamerasrc_set_camera_control(camerasrc,QCAMERASRC_PARAM_CONTROL_SCENE_MODE,value1,value2,NULL);
        break;
        case GST_CAMERA_CONTROL_EXPOSURE_MODE:
            error = gst_qcamerasrc_set_camera_control(camerasrc,QCAMERASRC_PARAM_CONTROL_METERING,value1,value2,NULL);
        break;
        case GST_CAMERA_CONTROL_EXPOSURE_VALUE:
        break;
        default:
            gst_camerasrc_debug( "Not supported type." );
            return FALSE;
    }

    if( error == -1 ){
        gst_camerasrc_debug( "Failed to set exposure. Type[%d],value1[%d],value2[%d],err code[%x]", type, value1, value2, error );
        return FALSE;
    }

    return TRUE;
}

gboolean
gst_camerasrc_control_get_exposure( GstCameraSrc* camerasrc, gint type, gint* value1, gint* value2 )
{
    gst_camerasrc_debug( "" );

    int error = -1;

    g_return_val_if_fail( camerasrc, FALSE );

    switch( type )
    {
        case GST_CAMERA_CONTROL_F_NUMBER:
        break;
        case GST_CAMERA_CONTROL_SHUTTER_SPEED:
        break;
        case GST_CAMERA_CONTROL_ISO:
            error= get_camera_control( camerasrc->qcam_handle, QCAMERASRC_PARAM_CONTROL_ISO,value1,value2,NULL);
        break;
        case GST_CAMERA_CONTROL_PROGRAM_MODE:
            error = get_camera_control( camerasrc->qcam_handle, QCAMERASRC_PARAM_CONTROL_SCENE_MODE,value1,value2,NULL);
        break;
        case GST_CAMERA_CONTROL_EXPOSURE_MODE:
            error = get_camera_control( camerasrc->qcam_handle, QCAMERASRC_PARAM_CONTROL_METERING,value1,value2,NULL);
        break;
        case GST_CAMERA_CONTROL_EXPOSURE_VALUE:
        break;
        default:
            gst_camerasrc_debug( "Not supported type." );
            return FALSE;
    }

    if( error != CAMERASRC_SUCCESS ){
        gst_camerasrc_debug( "Failed to get exposure. Type [%d]", type );
        return FALSE;
    }

    return TRUE;
}

gboolean
gst_camerasrc_control_set_capture_mode( GstCameraSrc* camerasrc, gint type, gint value )
{
    // TODO : single/multishot select(capture mode), output mode, frame count, JPEG quality

    gst_camerasrc_debug( "" );

    int error = CAMERASRC_ERROR;

    g_return_val_if_fail( camerasrc, FALSE );

    switch( type )
    {
        case GST_CAMERA_CONTROL_CAPTURE_MODE:
        break;
        case GST_CAMERA_CONTROL_OUTPUT_MODE:
        break;
        case GST_CAMERA_CONTROL_FRAME_COUNT:
        break;
        case GST_CAMERA_CONTROL_JPEG_QUALITY:
        break;
        default:
            gst_camerasrc_debug( "Not supported type." );
            return FALSE;
    }

    if( error != CAMERASRC_SUCCESS ){
        gst_camerasrc_debug( "Failed to set capture mode. Type[%d],value[%d],err code[%x]", type, value, error );
        return FALSE;
    }

    return TRUE;
}

gboolean
gst_camerasrc_control_get_capture_mode( GstCameraSrc* camerasrc, gint type, gint *value )
{
    // TODO : single/multishot select(capture mode), output mode, frame count, JPEG quality

    gst_camerasrc_debug( "" );

    int error = CAMERASRC_ERROR;

    g_return_val_if_fail( camerasrc, FALSE );

    switch( type )
    {
        case GST_CAMERA_CONTROL_CAPTURE_MODE:
        break;
        case GST_CAMERA_CONTROL_OUTPUT_MODE:
        break;
        case GST_CAMERA_CONTROL_FRAME_COUNT:
        break;
        case GST_CAMERA_CONTROL_JPEG_QUALITY:
        break;
        default:
            gst_camerasrc_debug( "Not supported type." );
            return FALSE;
    }

    if( error != CAMERASRC_SUCCESS ){
        gst_camerasrc_debug( "Failed to set capture mode. Type[%d],value[%d],err code[%x]", type, value, error );
        return FALSE;
    }

    return TRUE;
}

gboolean
gst_camerasrc_control_set_strobe( GstCameraSrc* camerasrc, gint type, gint value )
{
    gst_camerasrc_debug( "" );

    int error = CAMERASRC_ERROR;

    g_return_val_if_fail( camerasrc, FALSE );

    switch( type )
    {
        case GST_CAMERA_CONTROL_STROBE_MODE:
            error = gst_qcamerasrc_set_camera_control(camerasrc,QCAMERASRC_PARAM_CONTROL_FLASH_MODE,value,0,NULL);
            if(camerasrc->snapshot_stream)
                camerasrc->snapshot_stream->flash_mode=value;
        break;
        case GST_CAMERA_CONTROL_STROBE_CONTROL:
        case GST_CAMERA_CONTROL_STROBE_CAPABILITIES:
        case GST_CAMERA_CONTROL_STROBE_STATUS:
        case GST_CAMERA_CONTROL_STROBE_EV:
        default:
            gst_camerasrc_debug( "Not supported type[%d], return CAMERASRC_ERR_DEVICE_NOT_SUPPORT.", type );
            error = CAMERASRC_ERR_DEVICE_NOT_SUPPORT;
        break;
    }

    if( error != CAMERASRC_SUCCESS ){
        gst_camerasrc_debug( "Failed to set strobe. Type[%d],value[%d],err code[%x]", type, value, error );
        return FALSE;
    }

    gst_camerasrc_debug( "Succeed to set strobe. Type[%d],value[%d]", type, value );

    return TRUE;
}

gboolean
gst_camerasrc_control_get_strobe( GstCameraSrc* camerasrc, gint type, gint* value )
{
    gst_camerasrc_debug( "" );

    int error = CAMERASRC_ERROR;

    g_return_val_if_fail( camerasrc, FALSE );

    switch( type )
    {
        case GST_CAMERA_CONTROL_STROBE_MODE:
            error = get_camera_control(camerasrc->qcam_handle,QCAMERASRC_PARAM_CONTROL_FLASH_MODE,value,0,NULL);
        break;
        case GST_CAMERA_CONTROL_STROBE_CONTROL:
        case GST_CAMERA_CONTROL_STROBE_CAPABILITIES:
        case GST_CAMERA_CONTROL_STROBE_STATUS:
        case GST_CAMERA_CONTROL_STROBE_EV:
        default:
            gst_camerasrc_debug( "Not supported type[%d].", type );
            error = CAMERASRC_ERR_DEVICE_NOT_SUPPORT;
        break;
    }

    if( error != CAMERASRC_SUCCESS ){
        gst_camerasrc_debug( "Failed to get strobe. Type[%d],err code[%x]", type, error );
        return FALSE;
    }

    return TRUE;
}

gboolean
gst_camerasrc_control_set_detect( GstCameraSrc* camerasrc, gint type, gint value )
{
    gst_camerasrc_debug( "" );

    int error = CAMERASRC_ERROR;

    g_return_val_if_fail( camerasrc, FALSE );

    // TODO : detection number, focus select, select number, detect status
    switch( type )
    {
        case GST_CAMERA_CONTROL_FACE_DETECT_MODE:   // Face detect mode ( ON/OFF )
            camerasrc->face_detect = value;
            error = gst_qcamerasrc_set_camera_control(camerasrc,QCAMERASRC_PARAM_CONTROL_FACE_DETECT,value,0,NULL);
            set_face_detection(camerasrc->qcam_handle,value);
        break;
        case GST_CAMERA_CONTROL_FACE_DETECT_NUMBER:     // Face detection number
        break;
        case GST_CAMERA_CONTROL_FACE_FOCUS_SELECT:      // Face focus select
        break;
        case GST_CAMERA_CONTROL_FACE_SELECT_NUMBER:     // Face select number
        break;
        case GST_CAMERA_CONTROL_FACE_DETECT_STATUS:     // Face detect status
        break;
        default:
            gst_camerasrc_debug( "Not supported type." );
            return FALSE;
    }

    if( error != CAMERASRC_SUCCESS ){
        gst_camerasrc_debug( "Failed to set detect. Type[%d],value[%d],err code[%x]", type, value, error );
        return FALSE;
    }

    return TRUE;
}

gboolean
gst_camerasrc_control_get_detect( GstCameraSrc* camerasrc, gint type, gint *value )
{
    gst_camerasrc_debug( "" );

    int error = CAMERASRC_ERROR;

    g_return_val_if_fail( camerasrc, FALSE );

    // TODO : detection number, focus select, select number, detect status
    switch( type )
    {
        case GST_CAMERA_CONTROL_FACE_DETECT_MODE:   // Face detect mode ( ON/OFF )
            error = get_camera_control(camerasrc->qcam_handle,QCAMERASRC_PARAM_CONTROL_FACE_DETECT,value,0,NULL);
        break;
        case GST_CAMERA_CONTROL_FACE_DETECT_NUMBER:     // Face detection number
        break;
        case GST_CAMERA_CONTROL_FACE_FOCUS_SELECT:  // Face focus select
        break;
        case GST_CAMERA_CONTROL_FACE_SELECT_NUMBER:     // Face select number
        break;
        case GST_CAMERA_CONTROL_FACE_DETECT_STATUS:     // Face detect status
        break;
        default:
            gst_camerasrc_debug( "Not supported type." );
            return FALSE;
    }

    if( error != CAMERASRC_SUCCESS ){
        gst_camerasrc_debug( "Failed to get detect. Type[%d],err code[%x]", type, error );
        return FALSE;
    }

    return TRUE;
}

gboolean
gst_camerasrc_control_set_zoom( GstCameraSrc* camerasrc, gint type, gint value )
{
    gst_camerasrc_debug( "" );

    int error = CAMERASRC_ERROR;

    g_return_val_if_fail( camerasrc, FALSE );

    switch( type )
    {
        case GST_CAMERA_CONTROL_DIGITAL_ZOOM:
            error = gst_qcamerasrc_set_camera_control(camerasrc,QCAMERASRC_PARAM_CONTROL_ZOOM,value,0,NULL);
        break;
        case GST_CAMERA_CONTROL_OPTICAL_ZOOM:
        break;
        default:
            gst_camerasrc_debug( "Not supported type." );
            return FALSE;
    }

    if( error != CAMERASRC_SUCCESS ){
        gst_camerasrc_debug( "Failed to set zoom. Type[%d],value[%d],err code[%x]", type, value, error );
        return FALSE;
    }

    return TRUE;
}

gboolean
gst_camerasrc_control_get_zoom( GstCameraSrc* camerasrc, gint type, gint *value )
{
    gst_camerasrc_debug( "" );

    int error = CAMERASRC_ERROR;

    g_return_val_if_fail( camerasrc, FALSE );

    switch( type )
    {
        case GST_CAMERA_CONTROL_DIGITAL_ZOOM:
            error = get_camera_control(camerasrc->qcam_handle,QCAMERASRC_PARAM_CONTROL_ZOOM,value,0,NULL);
        break;
        case GST_CAMERA_CONTROL_OPTICAL_ZOOM:
        break;
        default:
            gst_camerasrc_debug( "Not supported type." );
            return FALSE;
    }

    if( error != CAMERASRC_SUCCESS ){
        gst_camerasrc_debug( "Failed to get zoom. Type[%d],err code[%x]", type, error );
        return FALSE;
    }

    return TRUE;
}

gboolean
gst_camerasrc_control_set_focus( GstCameraSrc* camerasrc, gint focus_mode, gint focus_range )
{
    gst_camerasrc_debug("enter");

    int error = CAMERASRC_ERROR;
    int set_mode = CAM_FOCUS_MODE_AUTO;

    g_return_val_if_fail(camerasrc, FALSE);

    switch (focus_mode) {
    case GST_QCAMERASRC_FOCUS_MODE_AUTO:
    case GST_QCAMERASRC_FOCUS_MODE_TOUCH_AUTO:
        if (focus_range == GST_QCAMERASRC_FOCUS_RANGE_MACRO) {
            GST_DEBUG_OBJECT(camerasrc, "CAM_FOCUS_MODE_MACRO");
            set_mode = CAM_FOCUS_MODE_MACRO;
        } else {
            GST_DEBUG_OBJECT(camerasrc, "CAM_FOCUS_MODE_AUTO");
            set_mode = CAM_FOCUS_MODE_AUTO;
        }
        break;
    case GST_QCAMERASRC_FOCUS_MODE_MANUAL:
        GST_DEBUG_OBJECT(camerasrc, "NOT supported mode[MANUAL]. set AUTO");
        set_mode = CAM_FOCUS_MODE_AUTO;
        break;
    case GST_QCAMERASRC_FOCUS_MODE_PAN:
        GST_DEBUG_OBJECT(camerasrc, "NOT supported mode[PAN]. set AUTO");
        set_mode = CAM_FOCUS_MODE_INFINITY;
        break;
    case GST_QCAMERASRC_FOCUS_MODE_CONTINUOUS:
    {
        /* set AF area 0,0,0,0 to reset AF */
        GstCameraControlRectType clear_rect = {0, 0, 0, 0};
        gst_camerasrc_control_set_auto_focus_area(camerasrc, clear_rect);

        GST_DEBUG_OBJECT(camerasrc, "CAM_FOCUS_MODE_CONTINOUS_PICTURE");
        set_mode = CAM_FOCUS_MODE_CONTINOUS_PICTURE;
        camerasrc->is_focusing = TRUE;
        GST_WARNING("CONTINUOUS Auto Focus is started.");
        break;
    }
    default:
        GST_DEBUG_OBJECT(camerasrc, "UNKNOWN mode. set AUTO");
        break;
    }

    error = gst_qcamerasrc_set_camera_control(camerasrc,QCAMERASRC_PARAM_CONTROL_AF, set_mode, 0,NULL);
    if (error != CAMERASRC_SUCCESS) {
        GST_ERROR_OBJECT(camerasrc, "Failed to set AF mode." );
        return FALSE;
    }

    camerasrc->focus_mode = focus_mode;
    camerasrc->focus_range = focus_range;
    GST_WARNING_OBJECT(camerasrc, "gst_camerasrc_control_set_focus. focus_mode(%d) focus_range(%d)", focus_mode, focus_range);
    return TRUE;
}

gboolean
gst_camerasrc_control_get_focus( GstCameraSrc* camerasrc, gint *focus_mode, gint *focus_range )
{
    gst_camerasrc_debug( "" );

    g_return_val_if_fail(camerasrc, FALSE);

	if (!focus_mode || !focus_range) {
		GST_ERROR_OBJECT(camerasrc, "NULL pointer %p %p", focus_mode, focus_range);
		return FALSE;
	}

	*focus_mode = camerasrc->focus_mode;
	*focus_range = camerasrc->focus_range;

	GST_DEBUG_OBJECT(camerasrc, "Focus mode %d, range %d", *focus_mode, *focus_range);

    return TRUE;
}

gboolean
gst_camerasrc_control_start_auto_focus( GstCameraSrc* camerasrc )
{
    gst_camerasrc_debug( "" );
    GST_INFO("gst_camerasrc_control_start_auto_focus");

    int error = CAMERASRC_ERROR;

    g_return_val_if_fail( camerasrc, FALSE );

    camerasrc->is_focusing = TRUE;

    /* CAF start If just set CAMERA_FOCUS_MODE_CAF to Focus mode - depends on Android HAL */
    if (camerasrc->focus_mode == GST_QCAMERASRC_FOCUS_MODE_CONTINUOUS) {
        error = gst_camerasrc_control_set_focus(camerasrc, GST_QCAMERASRC_FOCUS_MODE_CONTINUOUS, GST_QCAMERASRC_FOCUS_RANGE_NORMAL);
        if(!error) {
           GST_ERROR( "Failed to start CAF." );
           return FALSE;
        }
    } else {
        error = start_camera_auto_focus(camerasrc->qcam_handle);
        if( error != CAMERASRC_SUCCESS ){
           GST_ERROR( "Failed to start AF." );
           return FALSE;
        }
        GST_WARNING("Auto Focus is started.");
        gst_qcamerasrc_post_message_int(camerasrc, "camerasrc-AF", "focus-state", CAMERASRC_AUTO_FOCUS_RESULT_FUCUSING);
    }
    return TRUE;
}

gboolean
gst_camerasrc_control_stop_auto_focus( GstCameraSrc* camerasrc )
{
    gst_camerasrc_debug( "" );
    GST_INFO("gst_camerasrc_control_stop_auto_focus");
    int error = CAMERASRC_ERROR;

    g_return_val_if_fail( camerasrc, FALSE );

    camerasrc->is_focusing = FALSE;

    /* in case of CAF, To stop continuous focus, applications should change the focus mode to other modes. - depends on Android HAL */
    if (camerasrc->focus_mode == GST_QCAMERASRC_FOCUS_MODE_CONTINUOUS) {
        error = gst_qcamerasrc_set_camera_control(camerasrc,QCAMERASRC_PARAM_CONTROL_AF, CAM_FOCUS_MODE_AUTO, 0,NULL);
        GST_WARNING("CONTINUOUS Auto Focus is stopped.");
    } else {
        error = cancel_camera_auto_focus(camerasrc->qcam_handle);
        GST_WARNING("Auto Focus is canceled.");
    }
    if( error != CAMERASRC_SUCCESS ){
        GST_ERROR( "Failed to stop autofocus." );
        return FALSE;
    }

    return TRUE;
}

gboolean
gst_camerasrc_control_set_focus_level( GstCameraSrc* camerasrc, gint focus_level )
{
    // TODO :

    gst_camerasrc_debug( "Not support" );
    return FALSE;
}

gboolean
gst_camerasrc_control_get_focus_level( GstCameraSrc* camerasrc, gint *focus_level )
{
    // TODO :

    gst_camerasrc_debug( "Not support" );
    return FALSE;
}

gboolean
gst_camerasrc_control_set_auto_focus_area( GstCameraSrc* camerasrc, GstCameraControlRectType rect )
{
    gst_camerasrc_debug( "" );

    int error = CAMERASRC_ERROR;
    char str[80] = "";
    int left_top_x = 0;
    int left_top_y = 0;
    int right_bottom_x = 0;
    int right_bottom_y = 0;

    g_return_val_if_fail( camerasrc, FALSE );

    if (camerasrc->camera_id == CAMERASRC_DEV_ID_SECONDARY) {
        GST_INFO_OBJECT(camerasrc, "It's secondary camera. Skip setting...");
        return TRUE;
    }

    GST_WARNING_OBJECT(camerasrc, "Set AF area %d,%d,%dx%d, preview %dx%d",
                                  rect.x, rect.y, rect.width, rect.height, camerasrc->width, camerasrc->height);

    if (rect.x != 0 || rect.y != 0 ||
        rect.width != 0 || rect.height != 0) {
        left_top_x = ((rect.x * 2000)/camerasrc->width) - 1000;
        left_top_y = ((rect.y * 2000)/camerasrc->height) - 1000;
        right_bottom_x = (((rect.x+5) * 2000)/camerasrc->width) - 1000;
        right_bottom_y = (((rect.y+5) * 2000)/camerasrc->height) - 1000;
    } else {
        left_top_x = -1000;
        left_top_y = -1000;
        /* bottom x,y could not be set as -1000. because failed to check validation in AndroidHAL */
        right_bottom_x = -999;
        right_bottom_y = -999;
    }

    GST_WARNING_OBJECT(camerasrc, "Set AF area after transformation - left top[%d,%d] right bottom[%d,%d]",
                                  left_top_x, left_top_y, right_bottom_x, right_bottom_y);

    if (left_top_x < -1000 || left_top_x > 1000 ||
        left_top_y < -1000 || left_top_y > 1000 ||
        right_bottom_x < -1000 || right_bottom_x > 1000 ||
        right_bottom_y < -1000 || right_bottom_y > 1000) {
        GST_ERROR_OBJECT(camerasrc, "invalid area : left top[%d,%d] right bottom[%d,%d]",
                                    left_top_x, left_top_y, right_bottom_x, right_bottom_y);
        return FALSE;
    }

    snprintf(str, 80, "(%d,%d,%d,%d,1)", left_top_x, left_top_y, right_bottom_x, right_bottom_y);
    error = set_camera_control(camerasrc->qcam_handle,QCAMERASRC_PARAM_CONTROL_AF_AREAS, 0, 0, str);
    if (error != CAMERASRC_SUCCESS) {
        GST_ERROR_OBJECT(camerasrc, "Failed to set auto focus area.");
        return FALSE;
    }

    return TRUE;
}

/*===========================================================================
 * FUNCTION   : parseNDimVector
 *
 * DESCRIPTION: helper function to parse a string like "(1, 2, 3, 4, ..., N)"
 *              into N-dimension vector
 *
 * PARAMETERS :
 *   @str     : string to be parsed
 *   @num     : output array of size N to store vector element values
 *   @N       : number of dimension
 *   @delim   : delimeter to seperete string
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t parseNDimVector(const char *str, int *num, int N)
{
    char *start, *end;
    char delim = ',';
    if (num == NULL) {
        GST_ERROR("%s: Invalid output array (num == NULL)", __func__);
        return -1;
    }

    //check if string starts and ends with parantheses
    if (str[0] != '(' || str[strlen(str)-1] != ')') {
        GST_ERROR("%s: Invalid format of string %s, valid format is (n1, n2, n3, n4 ...)",
              __func__, str);
        return -1;
    }
    start = (char*) str;
    start++;
    int i;
    for (i=0; i<N; i++) {
        *(num+i) = (int) strtol(start, &end, 10);
        if (*end != delim && i < N-1) {
            GST_ERROR("%s: Cannot find delimeter '%c' in string \"%s\". end = %c",
                  __func__, delim, str, *end);
            return -1;
        }
        start = end+1;
    }
    return 0;
}

/*===========================================================================
 * FUNCTION   : parseCameraAreaString
 *
 * DESCRIPTION: helper function to parse a string of camera areas like
 *              "(1, 2, 3, 4, 5),(1, 2, 3, 4, 5),..."
 *
 * PARAMETERS :
 *   @str             : string to be parsed
 *   @max_num_areas   : max number of areas
 *   @pAreas          : ptr to struct to store areas
 *   @num_areas_found : number of areas found
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t parseCameraAreaString(const char *str,
                                                 int max_num_areas,
                                                 camerasrc_rect_t *pAreas,
                                                 int* num_areas_found)
{
    char area_str[32];
    const char *start, *end, *p;
    start = str; end = NULL;
    int values[5], index=0;
    num_areas_found = 0;

    memset(values, 0, sizeof(values));
    while (start != NULL) {
       if (*start != '(') {
            GST_ERROR("%s: error: Ill formatted area string: %s", __func__, str);
            return -1;
       }
       end = strchr(start, ')');
       if (end == NULL) {
            GST_ERROR("%s: error: Ill formatted area string: %s", __func__, str);
            return -1;
       }
       int i;
       for (i=0,p=start; p<=end; p++, i++) {
           area_str[i] = *p;
       }
       area_str[i] = '\0';
       if (parseNDimVector(area_str, values, 5) < 0) {
            GST_ERROR("%s: error: Failed to parse the area string: %s", __func__, area_str);
            return -1;
       }
       // no more areas than max_num_areas are accepted.
       if (index >= max_num_areas) {
            GST_ERROR("%s: error: too many areas specified %s", __func__, str);
            return -1;
       }
       pAreas->x = values[0];
       pAreas->y = values[1];
       pAreas->width = values[2] - values[0];
       pAreas->height = values[3] - values[1];

       index++;
       start = strchr(end, '('); // serach for next '('
    }
    num_areas_found = (int*)index;
    return 0;
}

gboolean
gst_camerasrc_control_get_auto_focus_area( GstCameraSrc* camerasrc, GstCameraControlRectType* rect )
{
    gst_camerasrc_debug( "" );

    int error = CAMERASRC_ERROR;
    camerasrc_rect_t camerasrc_rect = { 0, 0, 0, 0 };
    char* str = (char*)malloc(80);
    memset(str,0,80);
     //has to be in format "(0,0,0,0,0)"
    g_return_val_if_fail( camerasrc, FALSE );
    g_return_val_if_fail( rect, FALSE );

    error = get_camera_control(camerasrc->qcam_handle,QCAMERASRC_PARAM_CONTROL_AF_AREAS,0,0,str);

    if( error != CAMERASRC_SUCCESS ){
        gst_camerasrc_debug( "Failed to get auto focus area." );

        rect->x = rect->y = -1;
        rect->width = rect->height = -1;
        if(str != NULL){
            free(str);
        }
        return FALSE;
    }
    int num;
    GST_DEBUG("%s: error:parseCameraAreaString: %s", __func__, str);
    if(str != NULL)
    {
        GST_DEBUG("%s: string is not null:parseCameraAreaString: %s", __func__, str);
        parseCameraAreaString(str,1,&camerasrc_rect,&num);

        //transform the coords from (-1000, 1000) to (0, previewWidth or previewHeight)
        camerasrc_rect.x = (int32_t)((camerasrc_rect.x + 1000.0f) * (camerasrc->width/ 2000.0f));
        camerasrc_rect.y = (int32_t)((camerasrc_rect.y + 1000.0f) * (camerasrc->height/ 2000.0f));
        camerasrc_rect.width = (int32_t)(camerasrc_rect.width * camerasrc->width/ 2000.0f);
        camerasrc_rect.height = (int32_t)(camerasrc_rect.height *camerasrc->height / 2000.0f);
        GST_DEBUG("%s: string is not null:parseCameraAreaString: %s", __func__, str);
        GST_DEBUG("Get AF area after transformation %d,%d,%dx%d",
            camerasrc_rect.x, camerasrc_rect.y, camerasrc_rect.width, camerasrc_rect.height);
        free(str);
    }
    rect->x = camerasrc_rect.x;
    rect->y = camerasrc_rect.y;
    rect->width = camerasrc_rect.width-camerasrc_rect.x;
    rect->height = camerasrc_rect.height-camerasrc_rect.y;

    return TRUE;
}

gboolean
gst_camerasrc_control_set_wdr( GstCameraSrc* camerasrc, gint value )
{
    gst_camerasrc_debug( "" );

    int error = CAMERASRC_ERROR;

    g_return_val_if_fail( camerasrc, FALSE );

    if( error != CAMERASRC_SUCCESS ){
        gst_camerasrc_debug( "Failed to set wdr. value[%d],err code[%x]", value, error );
        return FALSE;
    }

    return TRUE;
}

gboolean
gst_camerasrc_control_get_wdr( GstCameraSrc* camerasrc, gint* value )
{
    gst_camerasrc_debug( "" );

    int error = CAMERASRC_ERROR;

    g_return_val_if_fail( camerasrc, FALSE );

    if( error != CAMERASRC_SUCCESS ){
        gst_camerasrc_debug( "Failed to get wdr. err code[%x]", error );
        return FALSE;
    }

    return TRUE;
}

gboolean
gst_camerasrc_control_set_ahs( GstCameraSrc* camerasrc, gint value )
{
    gst_camerasrc_debug( "" );

    int error = CAMERASRC_ERROR;

    g_return_val_if_fail( camerasrc, FALSE );

    error = gst_qcamerasrc_set_camera_control(camerasrc,
                                                QCAMERASRC_PARAM_CONTROL_ANTI_SHAKE,
                                                value,
                                                0,
                                                NULL);
    if( error != CAMERASRC_SUCCESS ){
        gst_camerasrc_debug( "Failed to set ahs. will be set later value[%d],err code[%x]", value, error );
        return FALSE;
    }

    return TRUE;
}

gboolean
gst_camerasrc_control_get_ahs( GstCameraSrc* camerasrc, gint* value )
{
    gst_camerasrc_debug( "" );

    int error = CAMERASRC_ERROR;

    g_return_val_if_fail( camerasrc, FALSE );

    error = get_camera_control(camerasrc->qcam_handle,
                                            QCAMERASRC_PARAM_CONTROL_ANTI_SHAKE,
                                            value,
                                            0,
                                            NULL);

    if( error != CAMERASRC_SUCCESS ){
        gst_camerasrc_debug( "Failed to get ahs. err code[%x]", error );
        return FALSE;
    }

    return TRUE;
}

gboolean
gst_camerasrc_control_set_part_color( GstCameraSrc* camerasrc, gint type, gint value )
{
    gst_camerasrc_debug( "" );

    int error = CAMERASRC_ERROR;

    g_return_val_if_fail( camerasrc, FALSE );

    switch( type )
    {
        case GST_CAMERA_CONTROL_PART_COLOR_SRC:
        break;
        case GST_CAMERA_CONTROL_PART_COLOR_DST:
        break;
        case GST_CAMERA_CONTROL_PART_COLOR_MODE:
        break;
        default:
            gst_camerasrc_debug( "Not supported type." );
            return FALSE;
    }

    if( error != CAMERASRC_SUCCESS ){
        gst_camerasrc_debug( "Failed to set part color. Type[%d],value[%d],err code[%x]", type, value, error );
        return FALSE;
    }

    return TRUE;
}

gboolean
gst_camerasrc_control_get_part_color( GstCameraSrc* camerasrc, gint type, gint* value )
{
    gst_camerasrc_debug( "" );

    int error = CAMERASRC_ERROR;

    g_return_val_if_fail( camerasrc, FALSE );

    switch( type )
    {
        case GST_CAMERA_CONTROL_PART_COLOR_SRC:
        break;
        case GST_CAMERA_CONTROL_PART_COLOR_DST:
        break;
        case GST_CAMERA_CONTROL_PART_COLOR_MODE:
        break;
        default:
            gst_camerasrc_debug( "Not supported type." );
            return FALSE;
    }

    if( error != CAMERASRC_SUCCESS ){
        gst_camerasrc_debug( "Failed to get part color. Type[%d],err code[%x]", type, error );
        return FALSE;
    }

    return TRUE;
}

gboolean
gst_camerasrc_control_get_exif_info( GstCameraSrc* camerasrc, GstCameraControlExifInfo* info )
{
    return TRUE;
}

gboolean gst_camerasrc_control_get_basic_dev_info ( GstCameraSrc* camerasrc, gint dev_id, GstCameraControlCapsInfoType* info )
{
    gst_camerasrc_debug( "" );

    g_return_val_if_fail( camerasrc, FALSE );

    return TRUE;
}

gboolean gst_camerasrc_control_get_misc_dev_info( GstCameraSrc* camerasrc, gint dev_id, GstCameraControlCtrlListInfoType * info)
{
    gst_camerasrc_debug( "" );

    g_return_val_if_fail( camerasrc, FALSE );

    return TRUE;
}

gboolean gst_camerasrc_control_get_extra_dev_info( GstCameraSrc* camerasrc, gint dev_id, GstCameraControlExtraInfoType * info)
{
    gst_camerasrc_debug( "" );

    g_return_val_if_fail( camerasrc, FALSE );

    return TRUE;
}

void gst_camerasrc_control_set_capture_command( GstCameraSrc* camerasrc, GstCameraControlCaptureCommand cmd )
{
    gst_camerasrc_debug( "" );

    if (camerasrc == NULL) {
        gst_camerasrc_debug("camerasrc is NULL");
        return;
    }

    gst_qcamerasrc_set_capture_command(camerasrc, cmd);

    return ;
}

void gst_camerasrc_control_set_record_command( GstCameraSrc* camerasrc, GstCameraControlRecordCommand cmd )
{
    gst_camerasrc_debug( "" );

    if (camerasrc == NULL) {
        gst_camerasrc_debug("camerasrc is NULL");
        return;
    }

    gst_qcamerasrc_set_record_command(camerasrc, cmd);

    return ;
}

gboolean gst_camerasrc_control_set_ae_lock( GstCameraSrc* camerasrc, gboolean lock )
{
    int error = CAMERASRC_SUCCESS;

    if (camerasrc == NULL) {
        GST_ERROR("handle is NULL");
        return FALSE;
    }

    error = gst_qcamerasrc_set_camera_control(camerasrc, QCAMERASRC_PARAM_CONTROL_AE_LOCK, lock, 0, NULL);
    if (error != CAMERASRC_SUCCESS) {
        GST_ERROR_OBJECT(camerasrc, "Failed to set AE lock");
        return FALSE;
    }

    return TRUE;
}

gboolean gst_camerasrc_control_get_ae_lock( GstCameraSrc* camerasrc, gboolean *lock )
{
    int error = CAMERASRC_SUCCESS;

    if (camerasrc == NULL) {
        GST_ERROR("handle is NULL");
        return FALSE;
    }

    error = get_camera_control(camerasrc->qcam_handle, QCAMERASRC_PARAM_CONTROL_AE_LOCK, lock, 0, NULL);
    if (error != CAMERASRC_SUCCESS) {
        GST_ERROR_OBJECT(camerasrc, "Failed to get AE lock");
        return FALSE;
    }

    return TRUE;
}

gboolean gst_camerasrc_control_set_awb_lock( GstCameraSrc* camerasrc, gboolean lock )
{
    int error = CAMERASRC_SUCCESS;

    if (camerasrc == NULL) {
        GST_ERROR("handle is NULL");
        return FALSE;
    }

    error = gst_qcamerasrc_set_camera_control(camerasrc, QCAMERASRC_PARAM_CONTROL_AWB_LOCK, lock, 0, NULL);
    if (error != CAMERASRC_SUCCESS) {
        GST_ERROR_OBJECT(camerasrc, "Failed to set AWB lock");
        return FALSE;
    }

    return TRUE;
}

gboolean gst_camerasrc_control_get_awb_lock( GstCameraSrc* camerasrc, gboolean *lock )
{
    int error = CAMERASRC_SUCCESS;

    if (camerasrc == NULL) {
        GST_ERROR("handle is NULL");
        return FALSE;
    }

    error = get_camera_control(camerasrc->qcam_handle, QCAMERASRC_PARAM_CONTROL_AWB_LOCK, lock, 0, NULL);
    if (error != CAMERASRC_SUCCESS) {
        GST_ERROR_OBJECT(camerasrc, "Failed to get AWB lock");
        return FALSE;
    }

    return TRUE;
}
