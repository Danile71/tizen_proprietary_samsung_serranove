/*
   Copyright (C) 2011 Samsung Electronics
   Copyright (C) 2012 Intel Corporation. All rights reserved.

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

/**
 * @file    ewk_view_internal.h
 * @brief   WebKit main smart object.
 *
 * This object provides view related APIs of WebKit2 to EFL object.
 */

#ifndef ewk_view_internal_h
#define ewk_view_internal_h

#include "ewk_context_internal.h"
#include <tizen.h>
#include <Eina.h>
#include <Evas.h>

#ifdef __cplusplus
extern "C" {
#endif

/// Enum values containing text directionality values.
typedef enum {
    EWK_TEXT_DIRECTION_RIGHT_TO_LEFT,
    EWK_TEXT_DIRECTION_LEFT_TO_RIGHT
} Ewk_Text_Direction;

/// Represents types of gesture.
enum _Ewk_Gesture_Type {
    EWK_GESTURE_TAP,
    EWK_GESTURE_LONG_PRESS,
    EWK_GESTURE_PAN,
    EWK_GESTURE_FLICK,
    EWK_GESTURE_PINCH
};
/// Creates a type name for @a _Ewk_Gesture_Type.
typedef enum _Ewk_Gesture_Type Ewk_Gesture_Type;

enum _Ewk_Screen_Orientation {
    EWK_SCREEN_ORIENTATION_PORTRAIT_PRIMARY = 1,
    EWK_SCREEN_ORIENTATION_LANDSCAPE_PRIMARY = 1 << 1,
    EWK_SCREEN_ORIENTATION_PORTRAIT_SECONDARY = 1 << 2,
    EWK_SCREEN_ORIENTATION_LANDSCAPE_SECONDARY = 1 << 3
};
typedef enum _Ewk_Screen_Orientation Ewk_Screen_Orientation;

/// Represents types of touch event.
typedef enum {
    EWK_TOUCH_START,
    EWK_TOUCH_MOVE,
    EWK_TOUCH_END,
    EWK_TOUCH_CANCEL
} Ewk_Touch_Event_Type;

typedef struct Ewk_View_Smart_Data Ewk_View_Smart_Data;
typedef struct Ewk_View_Smart_Class Ewk_View_Smart_Class;

/// Creates a type name for _Ewk_Event_Gesture.
typedef struct Ewk_Event_Gesture Ewk_Event_Gesture;

/// Represents a gesture event.
struct Ewk_Event_Gesture {
    Ewk_Gesture_Type type; /**< type of the gesture event */
    Evas_Coord_Point position; /**< position of the gesture event */
    Evas_Point velocity; /**< velocity of the gesture event. The unit is pixel per second. */
    double scale; /**< scale of the gesture event */
    int count; /**< count of the gesture */
    unsigned int timestamp; /**< timestamp of the gesture */
};

/**
 * \enum    Ewk_Input_Type
 * @brief   Provides type of focused input element
 */
enum Ewk_Input_Type {
    EWK_INPUT_TYPE_TEXT,
    EWK_INPUT_TYPE_TELEPHONE,
    EWK_INPUT_TYPE_NUMBER,
    EWK_INPUT_TYPE_EMAIL,
    EWK_INPUT_TYPE_URL,
    EWK_INPUT_TYPE_PASSWORD,
    EWK_INPUT_TYPE_COLOR,
    EWK_INPUT_TYPE_DATE,
    EWK_INPUT_TYPE_DATETIME,
    EWK_INPUT_TYPE_DATETIMELOCAL,
    EWK_INPUT_TYPE_MONTH,
    EWK_INPUT_TYPE_TIME,
    EWK_INPUT_TYPE_WEEK
};
typedef enum Ewk_Input_Type Ewk_Input_Type;

/**
 * \enum    Ewk_Selection_Handle_Type
 * @brief   Provides type of selection handle
 */
enum Ewk_Selection_Handle_Type {
    EWK_SELECTION_HANDLE_TYPE_LEFT,
    EWK_SELECTION_HANDLE_TYPE_RIGHT,
    EWK_SELECTION_HANDLE_TYPE_LARGE
};
typedef enum Ewk_Selection_Handle_Type Ewk_Selection_Handle_Type;

/// Ewk view's class, to be overridden by sub-classes.
struct Ewk_View_Smart_Class {
    Evas_Smart_Class sc; /**< all but 'data' is free to be changed. */
    unsigned long version;

    Eina_Bool (*popup_menu_show)(Ewk_View_Smart_Data* sd, Eina_Rectangle rect, Ewk_Text_Direction text_direction, double page_scale_factor, Eina_List* items, int selected_index);
    Eina_Bool (*multiple_popup_menu_show)(Ewk_View_Smart_Data* sd, Eina_Rectangle rect,Ewk_Text_Direction text_direction, double page_scale_factor, Eina_List* items);
    Eina_Bool (*popup_menu_hide)(Ewk_View_Smart_Data* sd);
    Eina_Bool (*popup_menu_update)(Ewk_View_Smart_Data* sd, Eina_Rectangle rect, Ewk_Text_Direction text_direction, Eina_List* items, int selected_index);

    Eina_Bool (*text_selection_down)(Ewk_View_Smart_Data* sd, int x, int y);
    Eina_Bool (*text_selection_move)(Ewk_View_Smart_Data* sd, int x, int y);
    Eina_Bool (*text_selection_up)(Ewk_View_Smart_Data* sd, int x, int y);

    Eina_Bool (*input_picker_show)(Ewk_View_Smart_Data* sd, Ewk_Input_Type input_type, const char* input_value);

    Eina_Bool (*orientation_lock)(Ewk_View_Smart_Data* sd, int orientations);
    void (*orientation_unlock)(Ewk_View_Smart_Data* sd);

    // event handling:
    //  - returns true if handled
    //  - if overridden, have to call parent method if desired
    Eina_Bool (*focus_in)(Ewk_View_Smart_Data* sd);
    Eina_Bool (*focus_out)(Ewk_View_Smart_Data* sd);
    Eina_Bool (*fullscreen_enter)(Ewk_View_Smart_Data* sd);
    Eina_Bool (*fullscreen_exit)(Ewk_View_Smart_Data* sd);
    Eina_Bool (*mouse_wheel)(Ewk_View_Smart_Data* sd, const Evas_Event_Mouse_Wheel* ev);
    Eina_Bool (*mouse_down)(Ewk_View_Smart_Data* sd, const Evas_Event_Mouse_Down* ev);
    Eina_Bool (*mouse_up)(Ewk_View_Smart_Data* sd, const Evas_Event_Mouse_Up* ev);
    Eina_Bool (*mouse_move)(Ewk_View_Smart_Data* sd, const Evas_Event_Mouse_Move* ev);
    Eina_Bool (*key_down)(Ewk_View_Smart_Data* sd, const Evas_Event_Key_Down* ev);
    Eina_Bool (*key_up)(Ewk_View_Smart_Data* sd, const Evas_Event_Key_Up* ev);

    // javascript popup:
    //   - All strings should be guaranteed to be stringshared.
    void (*run_javascript_alert)(Ewk_View_Smart_Data* sd, const char* message);
    Eina_Bool (*run_javascript_confirm)(Ewk_View_Smart_Data* sd, const char* message);
    const char* (*run_javascript_prompt)(Ewk_View_Smart_Data* sd, const char* message, const char* default_value); /**< return string should be stringshared. */

    // color picker:
    //   - Shows and hides color picker.
    Eina_Bool (*input_picker_color_request)(Ewk_View_Smart_Data* sd, int r, int g, int b, int a);
    Eina_Bool (*input_picker_color_dismiss)(Ewk_View_Smart_Data* sd);

    // storage:
    //   - Web database.
    unsigned long long (*exceeded_database_quota)(Ewk_View_Smart_Data* sd, const char* database_name, const char* display_name, unsigned long long current_quota, unsigned long long current_origin_usage, unsigned long long current_database_usage, unsigned long long expected_usage);

    Eina_Bool (*formdata_candidate_show)(Ewk_View_Smart_Data* sd, int x, int y, int w, int h);
    Eina_Bool (*formdata_candidate_hide)(Ewk_View_Smart_Data* sd);
    Eina_Bool (*formdata_candidate_update_data)(Ewk_View_Smart_Data* sd, Eina_List* data_list);
    Eina_Bool (*formdata_candidate_is_showing)(Ewk_View_Smart_Data* sd);

    Eina_Bool (*gesture_start)(Ewk_View_Smart_Data* sd, const Ewk_Event_Gesture* ev);
    Eina_Bool (*gesture_end)(Ewk_View_Smart_Data* sd, const Ewk_Event_Gesture* ev);
    Eina_Bool (*gesture_move)(Ewk_View_Smart_Data* sd, const Ewk_Event_Gesture* ev);

    Eina_Bool (*screen_reader_action_execute)(Ewk_View_Smart_Data* sd, void* elm_access_action_info);

    void (*selection_handle_down)(Ewk_View_Smart_Data* sd, Ewk_Selection_Handle_Type handle_type, int x, int y);
    void (*selection_handle_move)(Ewk_View_Smart_Data* sd, Ewk_Selection_Handle_Type handle_type, int x, int y);
    void (*selection_handle_up)(Ewk_View_Smart_Data* sd, Ewk_Selection_Handle_Type handle_type, int x, int y);
};

/**
 * The version you have to put into the version field
 * in the @a Ewk_View_Smart_Class structure.
 */
#define EWK_VIEW_SMART_CLASS_VERSION 6UL

/**
 * Initializer for whole Ewk_View_Smart_Class structure.
 *
 * @param smart_class_init initializer to use for the "base" field
 * (Evas_Smart_Class).
 *
 * @see EWK_VIEW_SMART_CLASS_INIT_NULL
 * @see EWK_VIEW_SMART_CLASS_INIT_VERSION
 * @see EWK_VIEW_SMART_CLASS_INIT_NAME_VERSION
 */
#define EWK_VIEW_SMART_CLASS_INIT(smart_class_init) {smart_class_init, EWK_VIEW_SMART_CLASS_VERSION, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}

/**
 * Initializer to zero a whole Ewk_View_Smart_Class structure.
 *
 * @see EWK_VIEW_SMART_CLASS_INIT_VERSION
 * @see EWK_VIEW_SMART_CLASS_INIT_NAME_VERSION
 * @see EWK_VIEW_SMART_CLASS_INIT
 */
#define EWK_VIEW_SMART_CLASS_INIT_NULL EWK_VIEW_SMART_CLASS_INIT(EVAS_SMART_CLASS_INIT_NULL)

/**
 * Initializer to zero a whole Ewk_View_Smart_Class structure and set
 * name and version.
 *
 * Similar to EWK_VIEW_SMART_CLASS_INIT_NULL, but will set version field of
 * Evas_Smart_Class (base field) to latest EVAS_SMART_CLASS_VERSION and name
 * to the specific value.
 *
 * It will keep a reference to name field as a "const char *", that is,
 * name must be available while the structure is used (hint: static or global!)
 * and will not be modified.
 *
 * @see EWK_VIEW_SMART_CLASS_INIT_NULL
 * @see EWK_VIEW_SMART_CLASS_INIT_VERSION
 * @see EWK_VIEW_SMART_CLASS_INIT
 */
#define EWK_VIEW_SMART_CLASS_INIT_NAME_VERSION(name) EWK_VIEW_SMART_CLASS_INIT(EVAS_SMART_CLASS_INIT_NAME_VERSION(name))

typedef struct EwkViewImpl EwkViewImpl;
/**
 * @brief Contains an internal View data.
 *
 * It is to be considered private by users, but may be extended or
 * changed by sub-classes (that's why it's in public header file).
 */
struct Ewk_View_Smart_Data {
    Evas_Object_Smart_Clipped_Data base;
    const Ewk_View_Smart_Class* api; /**< reference to casted class instance */
    Evas_Object* self; /**< reference to owner object */
    Evas_Object* image; /**< reference to evas_object_image for drawing web contents */
    EwkViewImpl* priv; /**< should never be accessed, c++ stuff */
    struct {
        Evas_Coord x, y, w, h; /**< last used viewport */
    } view;
    struct { /**< what changed since last smart_calculate */
        Eina_Bool any:1;
        Eina_Bool size:1;
        Eina_Bool position:1;
    } changed;
};

enum Ewk_Page_Visibility_State {
    EWK_PAGE_VISIBILITY_STATE_VISIBLE,
    EWK_PAGE_VISIBILITY_STATE_HIDDEN,
    EWK_PAGE_VISIBILITY_STATE_PRERENDER
};
typedef enum Ewk_Page_Visibility_State Ewk_Page_Visibility_State;

/**
 * Callback for ewk_view_script_execute
 *
 * @param o the view object
 * @param result_value value returned by script
 * @param user_data user data
 */
typedef void (*Ewk_View_Script_Execute_Callback)(Evas_Object* o, const char* result_value, void* user_data);

/**
 * Creates a new EFL WebKit view object based on specific Ewk_Context.
 *
 * @param e canvas object where to create the view object
 * @param context Ewk_Context object to declare process model
 *
 * @return view object on success or @c NULL on failure
 */
EXPORT_API Evas_Object* ewk_view_add_with_context(Evas* e, Ewk_Context* context);

/**
 * Gets the current text zoom level.
 *
 * @param o view object to get the zoom level
 *
 * @return current zoom level in use on success or @c -1.0 on failure
 */
EXPORT_API double ewk_view_text_zoom_get(const Evas_Object* o);

/**
 * Sets the current text zoom level.
 *
 * @param o view object to set the zoom level
 * @param textZoomFactor a new level to set
 *
 * @return @c EINA_TRUE on success or @c EINA_FALSE otherwise
 */
EXPORT_API Eina_Bool ewk_view_text_zoom_set(Evas_Object* o, double text_zoom_factor);

typedef Eina_Bool (*Ewk_View_Exceeded_Indexed_Database_Quota_Callback)(Evas_Object* o, Ewk_Security_Origin* origin,  long long current_quota, void* user_data);
EXPORT_API void ewk_view_exceeded_indexed_database_quota_callback_set(Evas_Object* o, Ewk_View_Exceeded_Indexed_Database_Quota_Callback callback, void* user_data);
EXPORT_API void ewk_view_exceeded_indexed_database_quota_reply(Evas_Object* o, Eina_Bool allow);

typedef Eina_Bool (*Ewk_View_Exceeded_Database_Quota_Callback)(Evas_Object* o, Ewk_Security_Origin* origin, const char* database_name, unsigned long long expected_quota, void* user_data);
EXPORT_API void ewk_view_exceeded_database_quota_callback_set(Evas_Object* o, Ewk_View_Exceeded_Database_Quota_Callback callback, void* user_data);
EXPORT_API void ewk_view_exceeded_database_quota_reply(Evas_Object* o, Eina_Bool allow);

typedef Eina_Bool (*Ewk_View_Exceeded_Local_File_System_Quota_Callback)(Evas_Object* o, Ewk_Security_Origin* origin,  long long current_quota, void* user_data);
EXPORT_API void ewk_view_exceeded_local_file_system_quota_callback_set(Evas_Object* o, Ewk_View_Exceeded_Local_File_System_Quota_Callback callback, void* user_data);
EXPORT_API void ewk_view_exceeded_local_file_system_quota_reply(Evas_Object* o, Eina_Bool allow);

/**
 * Requests for setting page visibility state.
 *
 * @param o view object to set the page visibility
 * @param page_visibility_state visible state of the page to set
 * @param initial_state @c EINA_TRUE if this function is called at page initialization time,
 *                     @c EINA_FALSE otherwise
 *
 * @return @c EINA_TRUE on successful request or @c EINA_FALSE on failure
 */
EXPORT_API Eina_Bool ewk_view_page_visibility_state_set(Evas_Object* o, Ewk_Page_Visibility_State page_visibility_state, Eina_Bool initial_state);

/**
* Request to set the user agent with application name.
*
* @param o view object to set the user agent with application name
*
* @param application_name string to set the user agent
*
* @return @c EINA_TRUE on success or @c EINA_FALSE on failure
*/
EXPORT_API Eina_Bool ewk_view_application_name_for_user_agent_set(Evas_Object* o, const char* application_name);

/**
* Returns application name string.
*
* @param o view object to get the application name
*
* @return @c application name
*/
EXPORT_API const char* ewk_view_application_name_for_user_agent_get(const Evas_Object* o);

/**
* add custom header
*
* @param o view object to add custom header
*
* @param name custom header name to add the custom header
*
* @param value custom header value to add the custom header
*
* @return @c EINA_TRUE on success or @c EINA_FALSE on failure
*/
EXPORT_API Eina_Bool ewk_view_custom_header_add(const Evas_Object* o, const char* name, const char* value);

/**
 * Request to set the current page's visibility.
 *
 * @param o view object to set the visibility.
 * @param enable EINA_TRUE to set on the visibility of the page, EINA_FALSE otherwise.
 *
 * @return @c EINA_TRUE on successful request, @c EINA_FALSE on errors
 */
EXPORT_API Eina_Bool ewk_view_visibility_set(Evas_Object* o, Eina_Bool enable);

/**
 * Notify the foreground/background status of app.
 *
 * @param o view object that is in given status.
 * @param enable EINA_TRUE to notify that page is foreground, EINA_FALSE otherwise.
 *
 * @return @c EINA_TRUE on successful request, @c EINA_FALSE on errors
 */
EXPORT_API Eina_Bool ewk_view_foreground_set(Evas_Object* o, Eina_Bool enable);

/*
 * Notify that notification is closed.
 *
 * @param notification_list list of Ewk_Notification pointer
 *
 * @return @c EINA_TRUE on successful request or @c EINA_FALSE on failure
 */
EXPORT_API Eina_Bool ewk_view_notification_closed(Evas_Object* o, Eina_List* notification_list);

/*
 * Sends the orientation of the device.
 *
 * If orientation value is changed, orientationchanged event will occur.
 *
 * @param o view object to receive orientation event.
 * @param orientation the new orientation of the device. (degree)
 *
 * orientation will be 0 degrees when the device is oriented to natural position,
 *                     -90 degrees when it's left side is at the top,
 *                     90 degrees when it's right side is at the top,
 *                     180 degrees when it is upside down.
 */
EXPORT_API void ewk_view_orientation_send(Evas_Object* o, int orientation);

EXPORT_API Eina_Bool ewk_view_text_selection_clear(Evas_Object* o);

typedef Eina_Bool (*Ewk_Orientation_Lock_Cb)(Evas_Object* o, Eina_Bool need_lock, int orientation, void* user_data);
 /**
 * Deprecated
 * Sets callback of orientation lock function
 *
 * func will be called when screen lock is called or unlock is called.
 * When screen.lockOrientation is called, need_lock will be true and orientation
 * will be the flags which should be locked.
 * For example, when contents called 'screen.lockOrientation("portrait"), orientation
 * will be EWK_SCREEN_ORIENTATION_PORTRAIT_PRIMARY | EWK_SCREEN_ORIENTATION_PORTRAIT_SECONDARY
 * When screen.unlockOrientation is called, need_lock will be false.
 *
 * @param o view object to set the callback of orientation
 * @param func callback function to be called when screen orientation is locked or unlocked.
 * @param use_data user_data will be passsed when ewk_view_web_app_icon_get is called
 *
 * @return current URI on success or @c 0 on failure
 */
EXPORT_API void ewk_view_orientation_lock_callback_set(Evas_Object* o, Ewk_Orientation_Lock_Cb func, void* user_data);

/// Enum values containing Content Security Policy header types.
enum _Ewk_CSP_Header_Type {
    EWK_REPORT_ONLY,
    EWK_ENFORCE_POLICY
};
typedef enum _Ewk_CSP_Header_Type Ewk_CSP_Header_Type;

/*
 * Set received Content Security Policy data from web app
 *
 * @param o view object
 * @param policy Content Security Policy data
 * @param type Content Security Policy header type
 *
 */
EXPORT_API void ewk_view_content_security_policy_set(Evas_Object* o, const char* policy, Ewk_CSP_Header_Type type);

/**
 * When font-family is "Tizen", use system's Settings font as default font-family
 *
 * @param o view object
 *
 */
EXPORT_API void ewk_view_use_settings_font(Evas_Object* o);

/**
 * Sets whether to draw transparent background or not.
 *
 * @param o view object to enable/disable transparent background
 * @param enabled a state to set
 *
 * @return @c EINA_TRUE on success or @c EINA_FALSE on failure
 */
EXPORT_API Eina_Bool ewk_view_draws_transparent_background_set(Evas_Object* o, Eina_Bool enabled);

/**
 * Queries if transparent background is enabled.
 *
 * @param o view object to get whether transparent background is enabled or not.
 *
 * @return @c EINA_TRUE on enable or @c EINA_FALSE on disable
 */
EXPORT_API Eina_Bool ewk_view_draws_transparent_background_get(Evas_Object* o);

/**
 * set a font for browser application
 *
 * @param o view object
 *
 */
EXPORT_API void ewk_view_browser_font_set(Evas_Object* o);

/**
 * Callback for ewk_view_geolocation_permission_callback_set
 *
 * @param o view object to request the geolocation permission
 * @param request Ewk_Geolocation_Permission_Request object to get the information about geolocation permission request.
 * @param user_data user data
 */
typedef Eina_Bool (*Ewk_View_Geolocation_Permission_Callback)(Evas_Object* o, Ewk_Geolocation_Permission_Request* request, void* user_data);

/**
 * Sets the geolocation permission callback.
 *
 * @param o view object to request the geolocation permission
 * @param callback Ewk_View_Geolocation_Permission_Callback function to geolocation permission
 * @param user_data user data
 */
EXPORT_API void ewk_view_geolocation_permission_callback_set(Evas_Object* o, Ewk_View_Geolocation_Permission_Callback callback, void* user_data);

/**
 * Callback for Ewk_Notification_Show_Callback_set
 *
 * @param o view object to show the notification
 * @param notification Ewk_Notification object for showing
 * @param user_data user data
 */
typedef Eina_Bool (*Ewk_Notification_Show_Callback)(Evas_Object* o, Ewk_Notification* notification, void* user_data);

/**
 * Callback for Ewk_Notification_Cancel_Callback_set
 *
 * @param o view object to cancel the notification
 * @param notificationID Ewk_Notification ID for canceling
 * @param user_data user data
 */
typedef Eina_Bool (*Ewk_Notification_Cancel_Callback)(Evas_Object* o, uint64_t notificationID, void* user_data);

/**
 * Sets the notification show / cancel callback.
 *
 * @param o view object to show / cancel the notification
 * @param showCallback Ewk_Notification_Show_Callback function to show notification
  * @param cancelCallback Ewk_Notification_Cancel_Callback function to cancel notification
 * @param user_data user data
 */
EXPORT_API void ewk_view_notification_callback_set(Evas_Object* o, Ewk_Notification_Show_Callback showCallback, Ewk_Notification_Cancel_Callback cancelCallback, void* user_data);

/**
 * Callback for ewk_view_notification_permission_callback_set
 *
 * @param o view object to request the notification permission
 * @param request Ewk_Notification_Permission_Request object to get the information about notification permission request
 * @param user_data user data
 */
typedef Eina_Bool (*Ewk_View_Notification_Permission_Callback)(Evas_Object* o, Ewk_Notification_Permission_Request* request, void* user_data);

/**
 * Sets the notification permission callback.
 *
 * @param o view object to request the notification permission
 * @param callback Ewk_View_Notification_Permission_Callback function to notification permission
 * @param user_data user data
 */
EXPORT_API void ewk_view_notification_permission_callback_set(Evas_Object* o, Ewk_View_Notification_Permission_Callback callback, void* user_data);

/**
 * Callback for ewk_view_user_media_permission_callback_set
 *
 * @param o view object to request the getUserMedia permission
 * @param request Ewk_View_User_Media_Permission_Callback object to get the infomation about getUserMedia permission request
 * @param user_data user data
 */
typedef Eina_Bool (*Ewk_View_User_Media_Permission_Callback)(Evas_Object* o, Ewk_User_Media_Permission_Request* request, void* user_data);

/**
 * Sets the getUserMedia permission callback.
 *
 * @param o view object to request the getUserMedia permission
 * @param callback Ewk_View_User_Media_Permission_Callback function to getUserMedia permission
 * @param user_data user data
 */
EXPORT_API void ewk_view_user_media_permission_callback_set(Evas_Object* o, Ewk_View_User_Media_Permission_Callback callback, void* user_data);

/**
 * Callback for ewk_view_notification_show_callback_set
 *
 * @param o view object to request the notification show
 * @param notification Ewk_Notification object to get the information about notification show request
 * @param user_data user data
 */
typedef Eina_Bool (*Ewk_View_Notification_Show_Callback)(Evas_Object *o, Ewk_Notification *notification, void *user_data);

/**
 * Sets the notification show callback.
 *
 * @param o view object to request the notification show
 * @param show_callback Ewk_View_Notification_Show_Callback function to notification show
 * @param user_data user data
 */
EXPORT_API void ewk_view_notification_show_callback_set(Evas_Object *o, Ewk_View_Notification_Show_Callback show_callback, void *user_data);

/**
 * Callback for ewk_view_notification_cancel_callback_set
 *
 * @param o view object to request the notification cancel
 * @param notification_id Ewk_Notification object to get the information about notification cancel request
 * @param user_data user data
 */
typedef Eina_Bool (*Ewk_View_Notification_Cancel_Callback)(Evas_Object *o, uint64_t notification_id, void *user_data);

/**
 * Sets the notification cancel callback.
 *
 * @param o view object to request the notification show
 * @param cancel_callback Ewk_View_Notification_Cancel_Callback function to notification cancel
 * @param user_data user data
 */
EXPORT_API void ewk_view_notification_cancel_callback_set(Evas_Object *o, Ewk_View_Notification_Cancel_Callback cancel_callback, void *user_data);
#ifdef __cplusplus
}
#endif
#endif // ewk_view_internal_h
