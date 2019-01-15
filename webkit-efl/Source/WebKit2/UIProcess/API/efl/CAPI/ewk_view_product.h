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
 * @file    ewk_view_product.h
 * @brief   WebKit main smart object.
 *
 * This object provides view related APIs of WebKit2 to EFL object.
 */

#ifndef ewk_view_product_h
#define ewk_view_product_h

#include "ewk_context_product.h"
#include <tizen.h>
#include <Eina.h>
#include <Evas.h>
#include "ewk_frame_product.h"
#include "ewk_history_product.h"
#include "ewk_hit_test_product.h"
#include "ewk_intercept_request_product.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup WEBVIEW
 * @{
 */

/**
 * @brief Enum values containing text directionality values.
 * @since_tizen 2.3
 */
typedef enum {
    EWK_TEXT_DIRECTION_RIGHT_TO_LEFT,   /**< Text direction : right to left */
    EWK_TEXT_DIRECTION_LEFT_TO_RIGHT    /**< Text direction : left to right */
} Ewk_Text_Direction;

/**
 * @brief Enumeration for screen orientation.
 * @since_tizen 2.3
 */
enum _Ewk_Screen_Orientation {
    EWK_SCREEN_ORIENTATION_PORTRAIT_PRIMARY = 1,    /**< 0 degrees */
    EWK_SCREEN_ORIENTATION_LANDSCAPE_PRIMARY = 1 << 1,  /**< 90 degrees */
    EWK_SCREEN_ORIENTATION_PORTRAIT_SECONDARY = 1 << 2, /**< 180 degrees */
    EWK_SCREEN_ORIENTATION_LANDSCAPE_SECONDARY = 1 << 3 /**< -90 degrees */
};
/**
 * @brief Creates a type name for Ewk_Screen_Orientation.
 * @since_tizen 2.3
 */
typedef enum _Ewk_Screen_Orientation Ewk_Screen_Orientation;

/**
 * @brief  Represents types of touch event.
 * @since_tizen 2.3
 */
typedef enum {
    EWK_TOUCH_START,    /**< Touch start */
    EWK_TOUCH_MOVE, /**< Touch move */
    EWK_TOUCH_END,  /**< Touch end */
    EWK_TOUCH_CANCEL    /**< Touch cancel */
} Ewk_Touch_Event_Type;

/**
 * @brief Creates a type name for Ewk_Touch_Point.
 * @since_tizen 2.3
 */
typedef struct Ewk_Touch_Point Ewk_Touch_Point;

/**
 * @brief Represents a touch point.
 * @since_tizen 2.3
 */
struct Ewk_Touch_Point {
    int id; // identifier of the touch event
    int x; // the horizontal position of the touch event
    int y; // the vertical position of the touch event
    Evas_Touch_Point_State state; // state of the touch event
};

/*
 * @brief Creates a type name for Ewk_View_Smart_Data.
 * @since_tizen 2.3
 */
typedef struct Ewk_View_Smart_Data Ewk_View_Smart_Data;
/*
 * @brief Creates a type name for Ewk_View_Smart_Class.
 * @since_tizen 2.3
 */
typedef struct Ewk_View_Smart_Class Ewk_View_Smart_Class;

/*
 * @brief Represents types of gesture.
 * @since_tizen 2.3
 */
enum _Ewk_Gesture_Type {
    EWK_GESTURE_TAP,    // Gesture type for tap
    EWK_GESTURE_LONG_PRESS, // Gesture type for long press
    EWK_GESTURE_PAN,    // Gesture type for pan
    EWK_GESTURE_FLICK,  // Gesture type for flick
    EWK_GESTURE_PINCH   // Gesture type for pinch
};
/*
 * @brief Creates a type name for @a _Ewk_Gesture_Type.
 * @since_tizen 2.3
 */
typedef enum _Ewk_Gesture_Type Ewk_Gesture_Type;

/*
 * @brief Creates a type name for _Ewk_Event_Gesture.
 * @since_tizen 2.3
 */
typedef struct Ewk_Event_Gesture Ewk_Event_Gesture;

/*
 * @brief Represents a gesture event
 * @since_tizen 2.3
 */
struct Ewk_Event_Gesture {
    Ewk_Gesture_Type type; // type of the gesture event
    Evas_Coord_Point position; // position of the gesture event
    Evas_Point velocity; // velocity of the gesture event. The unit is pixel per second.
    double scale; // scale of the gesture event
    int count; // count of the gesture
    unsigned int timestamp; // timestamp of the gesture
};

/**
 * @brief Enumeration for unfocus direction.
 * @since_tizen 2.3
 */
enum Ewk_Unfocus_Direction {
    EWK_UNFOCUS_DIRECTION_NONE = 0, /**< none */
    EWK_UNFOCUS_DIRECTION_FORWARD,  /**< forward */
    EWK_UNFOCUS_DIRECTION_BACKWARD, /**< backward */
    EWK_UNFOCUS_DIRECTION_UP,   /**< up */
    EWK_UNFOCUS_DIRECTION_DOWN, /**< down */
    EWK_UNFOCUS_DIRECTION_LEFT, /**< left */
    EWK_UNFOCUS_DIRECTION_RIGHT,    /**< right */
};
/**
 * @brief Creates a type name for Ewk_Unfocus_Direction
 * @since_tizen 2.3
 */
typedef enum Ewk_Unfocus_Direction Ewk_Unfocus_Direction;

/**
 * \enum    Ewk_Input_Type
 * @brief   Provides type of focused input element
 * @since_tizen 2.3
 */
enum Ewk_Input_Type {
    EWK_INPUT_TYPE_TEXT,    /**< Input type for text */
    EWK_INPUT_TYPE_TELEPHONE,   /**< Input type for telephone */
    EWK_INPUT_TYPE_NUMBER,  /**< Input type for number */
    EWK_INPUT_TYPE_EMAIL,   /**< Input type for email */
    EWK_INPUT_TYPE_URL, /**< Input type for url */
    EWK_INPUT_TYPE_PASSWORD,    /**< Input type for password */
    EWK_INPUT_TYPE_COLOR,   /**< Input type for color */
    EWK_INPUT_TYPE_DATE,    /**< Input type for date */
    EWK_INPUT_TYPE_DATETIME,    /**< Input type for datetime */
    EWK_INPUT_TYPE_DATETIMELOCAL,   /**< Input type for datetimelocal */
    EWK_INPUT_TYPE_MONTH,   /**< Input type for month */
    EWK_INPUT_TYPE_TIME,    /**< Input type for time */
    EWK_INPUT_TYPE_WEEK /**< Input type for week */
};
/**
 * @brief Creates a type name for Ewk_Input_Type
 * @since_tizen 2.3
 */
typedef enum Ewk_Input_Type Ewk_Input_Type;

/**
 * \enum    Ewk_Selection_Handle_Type
 * @brief   Provides type of selection handle
 * @since_tizen 2.3
 */
enum Ewk_Selection_Handle_Type {
    EWK_SELECTION_HANDLE_TYPE_LEFT, /**< Left selection handle */
    EWK_SELECTION_HANDLE_TYPE_RIGHT,    /**< Right selection handle */
    EWK_SELECTION_HANDLE_TYPE_LARGE  /**< Large selection handle */
};
/**
 * @brief Creates a type name for Ewk_Selection_Handle_Type
 * @since_tizen 2.3
 */
typedef enum Ewk_Selection_Handle_Type Ewk_Selection_Handle_Type;

/*
 * @brief Ewk view's class, to be overridden by sub-classes.
 * @since_tizen 2.3
 */
struct Ewk_View_Smart_Class {
    Evas_Smart_Class sc; // all but 'data' is free to be changed.
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
    const char* (*run_javascript_prompt)(Ewk_View_Smart_Data* sd, const char* message, const char* default_value); // return string should be stringshared.

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
 * @brief Callback for ewk_view_web_app_capable_get
 *
 * @since_tizen 2.3
 *
 * @param[in] capable web application capable
 * @param[in] user_data user_data will be passsed when ewk_view_web_app_capable_get is called
 */
typedef void (*Ewk_Web_App_Capable_Get_Callback)(Eina_Bool capable, void* user_data);

/**
 * @brief Callback for ewk_view_web_app_icon_get
 *
 * @since_tizen 2.3
 *
 * @param[in] icon_url web application icon
 * @param[in] user_data user_data will be passsed when ewk_view_web_app_icon_get is called
 */
typedef void (*Ewk_Web_App_Icon_URL_Get_Callback)(const char* icon_url, void* user_data);

/**
 * @brief Callback for ewk_view_web_app_icon_urls_get.
 *
 * @since_tizen 2.3
 *
 * @param[in] icon_urls list of Ewk_Web_App_Icon_Data for web app
 * @param[in] user_data user_data will be passsed when ewk_view_web_app_icon_urls_get is called
 */
typedef void (*Ewk_Web_App_Icon_URLs_Get_Callback)(Eina_List* icon_urls, void* user_data);

/*
 * @brief The version you have to put into the version field
 * in the @a Ewk_View_Smart_Class structure.
 * @since_tizen 2.3
 */
 #define EWK_VIEW_SMART_CLASS_VERSION 6UL

/*
 * @brief Initializer for whole Ewk_View_Smart_Class structure.
 *
 * @since_tizen 2.3
 *
 * @param[in] smart_class_init initializer to use for the "base" field
 * (Evas_Smart_Class).
 *
 * @see EWK_VIEW_SMART_CLASS_INIT_NULL
 * @see EWK_VIEW_SMART_CLASS_INIT_VERSION
 * @see EWK_VIEW_SMART_CLASS_INIT_NAME_VERSION
 */
#define EWK_VIEW_SMART_CLASS_INIT(smart_class_init) {smart_class_init, EWK_VIEW_SMART_CLASS_VERSION, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}

/*
 * @brief Initializer to zero a whole Ewk_View_Smart_Class structure.
 *
 * @since_tizen 2.3
 *
 * @see EWK_VIEW_SMART_CLASS_INIT_VERSION
 * @see EWK_VIEW_SMART_CLASS_INIT_NAME_VERSION
 * @see EWK_VIEW_SMART_CLASS_INIT
 */
#define EWK_VIEW_SMART_CLASS_INIT_NULL EWK_VIEW_SMART_CLASS_INIT(EVAS_SMART_CLASS_INIT_NULL)

/*
 * @brief Initializer to zero a whole Ewk_View_Smart_Class structure and set name and version.
 *
 * @details Similar to EWK_VIEW_SMART_CLASS_INIT_NULL, but will set version field of\n
 * Evas_Smart_Class (base field) to latest EVAS_SMART_CLASS_VERSION and name\n
 * to the specific value.\n
 *
 * It will keep a reference to name field as a "const char *", that is,\n
 * name must be available while the structure is used (hint: static or global!)\n
 * and will not be modified.
 *
 * @since_tizen 2.3
 *
 * @see EWK_VIEW_SMART_CLASS_INIT_NULL
 * @see EWK_VIEW_SMART_CLASS_INIT_VERSION
 * @see EWK_VIEW_SMART_CLASS_INIT
 */
#define EWK_VIEW_SMART_CLASS_INIT_NAME_VERSION(name) EWK_VIEW_SMART_CLASS_INIT(EVAS_SMART_CLASS_INIT_NAME_VERSION(name))

/*
 * @brief Creates a type name for EwkViewImpl
 * @since_tizen 2.3
 */
typedef struct EwkViewImpl EwkViewImpl;
/*
 * @brief Contains an internal View data.
 *
 * @details It is to be considered private by users, but may be extended or\n
 * changed by sub-classes (that's why it's in public header file).
 *
 * @since_tizen 2.3
 */
struct Ewk_View_Smart_Data {
    Evas_Object_Smart_Clipped_Data base;
    const Ewk_View_Smart_Class* api; // reference to casted class instance
    Evas_Object* self; // reference to owner object
    Evas_Object* image; // reference to evas_object_image for drawing web contents
    EwkViewImpl* priv;  // should never be accessed, c++ stuff
    struct {
        Evas_Coord x, y, w, h; // last used viewport
    } view;
    struct { // what changed since last smart_calculate
        Eina_Bool any:1;
        Eina_Bool size:1;
        Eina_Bool position:1;
    } changed;
};

#ifndef ewk_find_options_type
#define ewk_find_options_type
/**
 * Enum values used to specify search options.
 * @brief   Provides option to find text
 * @since_tizen 2.3
 */
enum Ewk_Find_Options {
    EWK_FIND_OPTIONS_NONE, /**< no search flags, this means a case sensitive, no wrap, forward only search. */
    EWK_FIND_OPTIONS_CASE_INSENSITIVE = 1 << 0, /**< case insensitive search. */
    EWK_FIND_OPTIONS_AT_WORD_STARTS = 1 << 1, /**< search text only at the beginning of the words. */
    EWK_FIND_OPTIONS_TREAT_MEDIAL_CAPITAL_AS_WORD_START = 1 << 2, /**< treat capital letters in the middle of words as word start. */
    EWK_FIND_OPTIONS_BACKWARDS = 1 << 3, /**< search backwards. */
    EWK_FIND_OPTIONS_WRAP_AROUND = 1 << 4, /**< if not present search will stop at the end of the document. */
    EWK_FIND_OPTIONS_SHOW_OVERLAY = 1 << 5, /**< show overlay */
    EWK_FIND_OPTIONS_SHOW_FIND_INDICATOR = 1 << 6, /**< show indicator */
    EWK_FIND_OPTIONS_SHOW_HIGHLIGHT = 1 << 7 /**< show highlight */
};
/**
 * @brief Creates a type name for Ewk_Find_Options
 * @since_tizen 2.3
 */
typedef enum Ewk_Find_Options Ewk_Find_Options;
#endif

/**
 * @brief Creates a new EFL WebKit view object.
 *
 * @since_tizen 2.3
 *
 * @param[in] e canvas object where to create the view object
 * @param[in] data a pointer to data to restore session data
 * @param[in] length length of session data to restore session data
 *
 * @return view object on success or @c NULL on failure
 */
EXPORT_API Evas_Object* ewk_view_add_with_session_data(Evas* e, const char* data, unsigned length);

/**
 * @brief Enumeration for page visibility state.
 * @since_tizen 2.3
 */
enum Ewk_Page_Visibility_State {
    EWK_PAGE_VISIBILITY_STATE_VISIBLE,  /**< The visible state of the page is visible. */
    EWK_PAGE_VISIBILITY_STATE_HIDDEN,   /**< The visible state of the page is hidden. */
    EWK_PAGE_VISIBILITY_STATE_PRERENDER /**< The visible state of the page is prerender. */
};
/**
 * @brief Creates a type name for Ewk_Page_Visibility_State
 * @since_tizen 2.3
 */
typedef enum Ewk_Page_Visibility_State Ewk_Page_Visibility_State;

/**
 * @brief Callback for ewk_view_script_execute
 *
 * @since_tizen 2.3
 *
 * @param[in] o the view object
 * @param[in] result_value value returned by script
 * @param[in] user_data user_data will be passsed when ewk_view_script_execute is called
 */
typedef void (*Ewk_View_Script_Execute_Callback)(Evas_Object* o, const char* result_value, void* user_data);

/**
 * @brief Callback for ewk_view_plain_text_get
 *
 * @since_tizen 2.3
 *
 * @param[in] o the view object
 * @param[in] plain_text the contents of the given frame converted to plain text
 * @param[in] user_data user data
 */
typedef void (*Ewk_View_Plain_Text_Get_Callback)(Evas_Object* o, const char* plain_text, void* user_data);

/**
 * @brief Creates a type name for the callback function used to get the page contents.
 *
 * @since_tizen 2.3
 *
 * @param[in] o view object
 * @param[in] data mhtml data of the page contents
 * @param[in] user_data user data will be passed when ewk_view_mhtml_data_get is called
 */
typedef void (*Ewk_View_MHTML_Data_Get_Callback)(Evas_Object* o, const char* data, void* user_data);

/**
 * @brief Gets the reference object for frame that represents the main frame.
 *
 * @since_tizen 2.3
 *
 * @param[in] o view object to get main frame
 *
 * @return frame reference of frame object on success, or NULL on failure
 */
EXPORT_API Ewk_Frame_Ref ewk_view_main_frame_get(Evas_Object* o);

/**
 * @brief Gets whether horizontal panning is holding.
 *
 * @since_tizen 2.3
 *
 * @param[in] o view object to get whether horizontal panning is holding
 *
 * @return @c EINA_TRUE if horizontal panning is holding
 *         @c EINA_FALSE if not or on failure
 */
EXPORT_API Eina_Bool ewk_view_horizontal_panning_hold_get(Evas_Object* o);

/**
 * @brief Sets to hold horizontal panning.
 *
 * @since_tizen 2.3
 *
 * @param[in] o view object to set to hold horizontal panning
 * @param[in] hold @c EINA_TRUE to hold horizontal panning
 *        @c EINA_FALSE not to hold
 */
EXPORT_API void ewk_view_horizontal_panning_hold_set(Evas_Object* o, Eina_Bool hold);

/**
 * @brief Gets whether vertical panning is holding.
 *
 * @since_tizen 2.3
 *
 * @param[in] o view object to get whether vertical panning is holding
 *
 * @return @c EINA_TRUE if vertical panning is holding
 *         @c EINA_FALSE if not or on failure
 */
EXPORT_API Eina_Bool ewk_view_vertical_panning_hold_get(Evas_Object* o);

/**
 * @brief Sets to hold vertical panning.
 *
 * @since_tizen 2.3
 *
 * @param[in] o view object to set to hold vertical panning
 * @param[in] hold @c EINA_TRUE to hold vertical panning
 *        @c EINA_FALSE not to hold
 */
EXPORT_API void ewk_view_vertical_panning_hold_set(Evas_Object* o, Eina_Bool hold);

/**
 * @brief Gets the minimum and maximum value of the scale range or -1 on failure
 *
 * @since_tizen 2.3
 *
 * @remarks Use @c NULL pointers on the scale components you're not\n
 * interested in: they'll be ignored by the function.
 *
 * @param[in] o view object to get the minimum and maximum value of the scale range
 * @param[in] min_scale Pointer to an double in which to store the minimum scale factor of the object
 * @param[in] max_scale Pointer to an double in which to store the maximum scale factor of the object
 */
EXPORT_API void ewk_view_scale_range_get(Evas_Object* o, double* min_scale, double* max_scale);

/**
 * @brief Gets the current text zoom level.
 *
 * @since_tizen 2.3
 *
 * @param[in] o view object to get the zoom level
 *
 * @return current zoom level in use on success or @c -1.0 on failure
 */
EXPORT_API double ewk_view_text_zoom_get(const Evas_Object* o);

/**
 * @brief Sets the current text zoom level.
 *
 * @since_tizen 2.3
 *
 * @param[in] o view object to set the zoom level
 * @param[in] text_zoom_factor a new level to set
 *
 * @return @c EINA_TRUE on success or @c EINA_FALSE otherwise
 */
EXPORT_API Eina_Bool ewk_view_text_zoom_set(Evas_Object* o, double text_zoom_factor);

/**
 * @brief Callback for javascript alert popup
 *
 * @since_tizen 2.3
 *
 * @param[in] o view object
 * @param[in] alert_text The contents of javascript alert popup
 * @param[in] user_data user data
 */
typedef Eina_Bool (*Ewk_View_JavaScript_Alert_Callback)(Evas_Object* o, const char* alert_text, void* user_data);

/**
 * @brief Sets callback of javascript alert popup
 *
 * @since_tizen 2.3
 *
 * @param[in] o view object to set the callback
 * @param[in] callback callback function for javascript alert popoup
 * @param[in] user_data user data
 */
EXPORT_API void ewk_view_javascript_alert_callback_set(Evas_Object* o, Ewk_View_JavaScript_Alert_Callback callback, void* user_data);

/**
 * @brief Reply of javascript alert popup
 *
 * @since_tizen 2.3
 *
 * @param[in] o view object
 */
EXPORT_API void ewk_view_javascript_alert_reply(Evas_Object* o);

/**
 * @brief Callback for javascript confirm popup
 *
 * @since_tizen 2.3
 *
 * @param[in] o view object
 * @param[in] message the contents of javascript confirm popup
 * @param[in] user_data user data
 */
typedef Eina_Bool (*Ewk_View_JavaScript_Confirm_Callback)(Evas_Object* o, const char* message, void* user_data);
/**
 * @brief Sets callback of javascript confirm popup
 *
 * @since_tizen 2.3
 *
 * @param[in] o view object to set the callback
 * @param[in] callback callback function for javascript confirm popoup
 * @param[in] user_data user data
 */
EXPORT_API void ewk_view_javascript_confirm_callback_set(Evas_Object* o, Ewk_View_JavaScript_Confirm_Callback callback, void* user_data);
/**
 * @brief Reply of javascript confirm popup
 *
 * @since_tizen 2.3
 *
 * @param[in] o view object
 * @param[in] result result of javascript confirm popup
 */
EXPORT_API void ewk_view_javascript_confirm_reply(Evas_Object* o, Eina_Bool result);

/**
 * @brief Callback for javascript prompt popup
 *
 * @since_tizen 2.3
 *
 * @param[in] o view object
 * @param[in] message the contents of javascript prompt popup
 * @param[in] default_value the contents in the input field of javascript prompt popup
 * @param[in] user_data user data
 */
typedef Eina_Bool (*Ewk_View_JavaScript_Prompt_Callback)(Evas_Object* o, const char* message, const char* default_value, void* user_data);
/**
 * @brief Sets callback of javascript prompt popup
 *
 * @since_tizen 2.3
 *
 * @param[in] o view object to set the callback
 * @param[in] callback callback function for javascript prompt popoup
 * @param[in] user_data user data
 */
EXPORT_API void ewk_view_javascript_prompt_callback_set(Evas_Object* o, Ewk_View_JavaScript_Prompt_Callback callback, void* user_data);
/**
 * @brief Reply of javascript prompt popup
 *
 * @since_tizen 2.3
 *
 * @param[in] o view object
 * @param[in] result entered characters of javascript prompt popup
 */
EXPORT_API void ewk_view_javascript_prompt_reply(Evas_Object* o, const char* result);

/**
 * @brief Callback for before unload popup
 *
 * @since_tizen 2.3
 *
 * @param[in] o view object
 * @param[in] message the contents of before unload popup
 * @param[in] user_data user data
 */
typedef Eina_Bool (*Ewk_View_Before_Unload_Confirm_Panel_Callback)(Evas_Object* o, const char* message, void* user_data);
/**
 * @brief Sets callback of before unload popup
 *
 * @since_tizen 2.3
 *
 * @param[in] o view object to set the callback
 * @param[in] callback callback function for before unload popoup
 * @param[in] user_data user data
 */
EXPORT_API void ewk_view_before_unload_confirm_panel_callback_set(Evas_Object* o, Ewk_View_Before_Unload_Confirm_Panel_Callback callback, void* user_data);
/**
 * @brief Reply of before unload popup
 *
 * @since_tizen 2.3
 *
 * @param[in] o view object
 * @param[in] result result of before unload popup
 */
EXPORT_API void ewk_view_before_unload_confirm_panel_reply(Evas_Object* o, Eina_Bool result);

/**
 * @brief Callback for ewk_view_application_cache_permission_callback_set
 *
 * @since_tizen 2.3
 *
 * @param[in] o view object to set application cache permission
 * @param[in] origin origin for getting permission of application cache
 * @param[in] user_data user_data will be passed when ewk_view_application_cache_permission_callback_set is called
 */
typedef Eina_Bool (*Ewk_View_Applicacion_Cache_Permission_Callback)(Evas_Object* o, Ewk_Security_Origin* origin,  void* user_data);
/**
 * @brief Sets callback of getting application cache permission.
 *
 * @since_tizen 2.3
 *
 * @param[in] o view object to set the callback of application cache permission
 * @param[in] callback function to be called when application cache need to get permission
 * @param[in] user_data user data
 */
EXPORT_API void ewk_view_application_cache_permission_callback_set(Evas_Object* o, Ewk_View_Applicacion_Cache_Permission_Callback callback, void* user_data);
/**
 * @brief Application cache permission confirm popup reply
 *
 * @since_tizen 2.3
 *
 * @param[in] o view object to reply permission confirm popup
 * @param[in] allow of response
 */
EXPORT_API void ewk_view_application_cache_permission_reply(Evas_Object* o, Eina_Bool allow);

/**
 * @brief Callback for ewk_view_exceeded_indexed_database_quota_callback_set
 *
 * @since_tizen 2.3
 *
 * @param[in] o view object to set indexed database quota
 * @param[in] origin origin for getting permission of indexed database
 * @param[in] current_quota indexed database quota
 * @param[in] user_data user_data will be passed when ewk_view_application_cache_permission_callback_set is called
 */
typedef Eina_Bool (*Ewk_View_Exceeded_Indexed_Database_Quota_Callback)(Evas_Object* o, Ewk_Security_Origin* origin,  long long current_quota, void* user_data);
/**
 * @brief Sets callback of exceeded indexed database quota
 *
 * @since_tizen 2.3
 *
 * @param[in] o view object to set the callback of exceeded indexed database quota
 * @param[in] callback function to be called when application want to use indexed database over current quota
 * @param[in] user_data user data
 */
EXPORT_API void ewk_view_exceeded_indexed_database_quota_callback_set(Evas_Object* o, Ewk_View_Exceeded_Indexed_Database_Quota_Callback callback, void* user_data);
/**
 * @brief Confirm popup reply for exceeded indexed database quota
 *
 * @since_tizen 2.3
 *
 * @param[in] o view object to reply confirm popup
 * @param[in] allow of response
 */
EXPORT_API void ewk_view_exceeded_indexed_database_quota_reply(Evas_Object* o, Eina_Bool allow);

/**
 * @brief Callback for ewk_view_exceeded_database_quota_callback_set
 *
 * @since_tizen 2.3
 *
 * @param[in] o view object to set exceeded database quota callback
 * @param[in] origin origin for database quota
 * @param[in] database_name database name of web database
 * @param[in] expected_quota expected quota
 * @param[in] user_data user_data will be passed when ewk_view_exceeded_database_quota_callback_set is called
 */
typedef Eina_Bool (*Ewk_View_Exceeded_Database_Quota_Callback)(Evas_Object* o, Ewk_Security_Origin* origin, const char* database_name, unsigned long long expected_quota, void* user_data);
/**
 * @brief Sets callback of exceeded web database quota
 *
 * @since_tizen 2.3
 *
 * @param[in] o view object to set the callback of exceeded web database quota
 * @param[in] callback function to be called when application want to use web database over current quota
 * @param[in] user_data user data
 */
EXPORT_API void ewk_view_exceeded_database_quota_callback_set(Evas_Object* o, Ewk_View_Exceeded_Database_Quota_Callback callback, void* user_data);
/**
 * @brief Confirm popup reply for exceeded web database quota
 *
 * @since_tizen 2.3
 *
 * @param[in] o view object to reply confirm popup
 * @param[in] allow of response
 */
EXPORT_API void ewk_view_exceeded_database_quota_reply(Evas_Object* o, Eina_Bool allow);

/**
 * @brief Callback for ewk_view_exceeded_local_file_system_quota_callback_set
 *
 * @since_tizen 2.3
 *
 * @param[in] o view object to set exceeded local file system quota callback
 * @param[in] origin origin for local file system
 * @param[in] current_quota local file system quota
 * @param[in] user_data user_data will be passed when ewk_view_exceeded_local_file_system_quota_callback_set is called
 */
typedef Eina_Bool (*Ewk_View_Exceeded_Local_File_System_Quota_Callback)(Evas_Object* o, Ewk_Security_Origin* origin,  long long current_quota, void* user_data);
/**
 * @brief Sets callback of exceeded local file system
 *
 * @since_tizen 2.3
 *
 * @param[in] o view object to set the callback of exceeded local file system quota
 * @param[in] callback function to be called when application want to use local file system over current quota
 * @param[in] user_data user data
 */
EXPORT_API void ewk_view_exceeded_local_file_system_quota_callback_set(Evas_Object* o, Ewk_View_Exceeded_Local_File_System_Quota_Callback callback, void* user_data);
/**
 * @brief Confirm popup reply for local file system quota
 *
 * @since_tizen 2.3
 *
 * @param[in] o view object to reply confirm popup
 * @param[in] allow of response
 */
EXPORT_API void ewk_view_exceeded_local_file_system_quota_reply(Evas_Object* o, Eina_Bool allow);

/**
 * @brief To give a chance to intercept request data before sending it.
 *
 * @since_tizen 2.3
 *
 * @param[in] o view object to intercept request
 * @param[in] intercept_request Defined structure to notify requesting infomation
 * @param[in] user_data user data
 *
 * @see ewk_view_intercept_request_callback_set()
 */
typedef void (*Ewk_View_Intercept_Request_Callback)(Evas_Object* o, Ewk_Intercept_Request* intercept_request, void* user_data);

/**
 * @brief  To set Ewk_View_Intercept_Request_Callback to give a chance to intercept request data before sending it.
 *
 * @since_tizen 2.3
 *
 * @param[in] o view object to intercept request
 * @param[in] callback Defined callback
 * @param[in] user_data user data
 *
 * @see Ewk_View_Intercept_Request_Callback
 */
EXPORT_API void ewk_view_intercept_request_callback_set (Evas_Object* o, Ewk_View_Intercept_Request_Callback callback, void* user_data);

/**
 * @brief Callback for ewk_view_unfocus_allow_callback_set
 *
 * @details If return value is EINA_TRUE, the focus is out from webview.\n
 *
 * @since_tizen 2.3
 *
 * @param[in] o view object
 * @param[in] direction
 * @param[in] user_data user data
 *
 * @return @c EINA_TRUE on successful or @c EINA_FALSE on failure
 */
typedef Eina_Bool (*Ewk_View_Unfocus_Allow_Callback)(Evas_Object* o, Ewk_Unfocus_Direction direction, void* user_data);

/**
 * @brief Set to callback to controll unfocus operation from the arrow of h/w keyboard.
 *
 * @since_tizen 2.3
 *
 * @param[in] o view object
 * @param[in] callback callback to controll unfocus operation from the arrow of h/w keyboard
 * @param[in] user_data user data
 */
EXPORT_API void ewk_view_unfocus_allow_callback_set (Evas_Object* o, Ewk_View_Unfocus_Allow_Callback callback, void* user_data);

/**
 * @brief Requests loading the given contents.
 *
 * @since_tizen 2.3
 *
 * @param[in] o view object to load document
 * @param[in] html what to load
 * @param[in] base_uri base uri to use for relative resources, may be @c 0,\n
 *        if provided @b must be an absolute uri
 *
 * @return @c EINA_TRUE on successful request, @c EINA_FALSE on errors
 */
EXPORT_API Eina_Bool ewk_view_html_contents_set(Evas_Object* o, const char* html, const char* base_uri);

/**
 * @brief Requests for setting page visibility state.
 *
 * @since_tizen 2.3
 *
 * @param[in] o view object to set the page visibility
 * @param[in] page_visibility_state visible state of the page to set
 * @param[in] initial_state @c EINA_TRUE if this function is called at page initialization time,\n
 *                     @c EINA_FALSE otherwise
 *
 * @return @c EINA_TRUE on successful request or @c EINA_FALSE on failure
 */
EXPORT_API Eina_Bool ewk_view_page_visibility_state_set(Evas_Object* o, Ewk_Page_Visibility_State page_visibility_state, Eina_Bool initial_state);

/**
 * @brief Request to set the user agent with application name.
 *
 * @since_tizen 2.3
 *
 * @param[in] o view object to set the user agent with application name
 * @param[in] application_name string to set the user agent
 *
 * @return @c EINA_TRUE on success or @c EINA_FALSE on failure
 */
EXPORT_API Eina_Bool ewk_view_application_name_for_user_agent_set(Evas_Object* o, const char* application_name);

/**
 * @brief Returns application name string.
 *
 * @since_tizen 2.3
 *
 * @param[in] o view object to get the application name
 *
 * @return @c application name
 */
EXPORT_API const char* ewk_view_application_name_for_user_agent_get(const Evas_Object* o);

/**
 * @brief add custom header
 *
 * @since_tizen 2.3
 *
 * @param[in] o view object to add custom header
 * @param[in] name custom header name to add the custom header
 * @param[in] value custom header value to add the custom header
 *
 * @return @c EINA_TRUE on success or @c EINA_FALSE on failure
 */
EXPORT_API Eina_Bool ewk_view_custom_header_add(const Evas_Object* o, const char* name, const char* value);
/**
 * @brief remove custom header
 *
 * @since_tizen 2.3
 *
 * @param[in] o view object to remove custom header
 * @param[in] name custom header name to remove the custom header
 *
 * @return @c EINA_TRUE on success or @c EINA_FALSE on failure
 */
EXPORT_API Eina_Bool ewk_view_custom_header_remove(const Evas_Object* o, const char* name);

/**
 * @brief Request to set the current page's visibility.
 *
 * @since_tizen 2.3
 *
 * @param[in] o view object to set the visibility
 * @param[in] enable EINA_TRUE to set on the visibility of the page, EINA_FALSE otherwise
 *
 * @return @c EINA_TRUE on successful request, @c EINA_FALSE on errors
 */
EXPORT_API Eina_Bool ewk_view_visibility_set(Evas_Object* o, Eina_Bool enable);

/**
 * @brief Returns the evas image object of the specified viewArea of page
 *
 * @details The returned evas image object @b should be freed after use.
 *
 * @since_tizen 2.3
 *
 * @param[in] o view object to get specified rectangle of cairo surface
 * @param[in] view_area rectangle of cairo surface
 * @param[in] scale_factor scale factor of cairo surface
 * @param[in] canvas canvas for creating evas image
 *
 * @return newly allocated evas image object on sucess or @c 0 on failure
 */
EXPORT_API Evas_Object* ewk_view_screenshot_contents_get(const Evas_Object* o, Eina_Rectangle view_area, float scale_factor, Evas* canvas);

/**
 * @brief Returns the evas image object for the cache image specified in url.
 *
 * @details The returned evas image object @b should be freed after use.
 *
 * @since_tizen 2.3
 *
 * @param[in] o view object to get specified rectangle of cairo surface
 * @param[in] image_url url of the image in the cache
 * @param[in] canvas canvas for creating evas image
 *
 * @return newly allocated evas image object on sucess or @c 0 on failure
 */
EXPORT_API Evas_Object* ewk_view_cache_image_get(const Evas_Object* o, const char* image_url, Evas* canvas);

/**
 * @brief Gets the possible scroll size of the given view.
 *
 * @details Possible scroll size is contents size minus the viewport size.
 *
 * @since_tizen 2.3
 *
 * @param[in] o view object to get scroll size
 * @param[in] w the pointer to store the horizontal size that is possible to scroll,\n
 *        may be @c 0
 * @param[in] h the pointer to store the vertical size that is possible to scroll,\n
 *        may be @c 0
 *
 * @return @c EINA_TRUE on success, @c EINA_FALSE otherwise and\n
 *         values are zeroed
 */
EXPORT_API Eina_Bool ewk_view_scroll_size_get(const Evas_Object* o, int* w, int* h);

/**
 * @brief Requests for getting web application capable.
 *
 * @since_tizen 2.3
 *
 * @param[in] o view object
 * @param[in] callback result callback to get web database quota
 * @param[in] user_data user_data will be passed when result_callback is called\n
 *    -I.e., user data will be kept until callback is called
 *
 * @return @c EINA_TRUE on successful request or @c EINA_FALSE on failure
 */
EXPORT_API Eina_Bool ewk_view_web_application_capable_get(Evas_Object* o, Ewk_Web_App_Capable_Get_Callback callback, void* user_data);

/**
 * @brief Requests for getting web application icon string.
 *
 * @since_tizen 2.3
 *
 * @param[in] o view object
 * @param[in] callback result callback to get web database quota
 * @param[in] user_data user_data will be passed when result_callback is called\n
 *    -I.e., user data will be kept until callback is called
 *
 * @return @c EINA_TRUE on successful request or @c EINA_FALSE on failure
 */
EXPORT_API Eina_Bool ewk_view_web_application_icon_url_get(Evas_Object* o, Ewk_Web_App_Icon_URL_Get_Callback callback, void* user_data);

/**
 * @brief Requests for getting web application icon list of Ewk_Web_App_Icon_Data.
 *
 * @since_tizen 2.3
 *
 * @param[in] o view object
 * @param[in] callback result callback to get web application icon urls
 * @param[in] user_data user_data will be passed when result_callback is called\n
 *    -I.e., user data will be kept until callback is called
 *
 * @return @c EINA_TRUE on successful request or @c EINA_FALSE on failure
 */
EXPORT_API Eina_Bool ewk_view_web_application_icon_urls_get(Evas_Object* o, Ewk_Web_App_Icon_URLs_Get_Callback callback, void* user_data);

/**
 * @brief Executes editor command.
 *
 * @since_tizen 2.3
 *
 * @param[in] o view object to execute command
 * @param[in] command editor command to execute
 * @param[in] value the value to be passed into command
 *
 * @return @c EINA_TRUE on success or @c EINA_FALSE on failure
 */
EXPORT_API Eina_Bool ewk_view_command_execute(Evas_Object* o, const char* command, const char* value);

/**
 * @brief Create PDF file of page contents
 *
 * @since_tizen 2.3
 *
 * @param[in] o view object to get page contents.
 * @param[in] width the suface width of PDF file.
 * @param[in] height the suface height of PDF file.
 * @param[in] file_name the file name for creating PDF file.
 *
 * @return @c EINA_TRUE on success or @c EINA_FALSE on failure
 */
EXPORT_API Eina_Bool ewk_view_contents_pdf_get(Evas_Object* o, int width, int height, const char* file_name);

/**
 * @brief Retrieve the contents in plain text.
 *
 * @since_tizen 2.3
 *
 * @param[in] o view object whose contents to retrieve.
 * @param[in] callback result callback
 * @param[in] user_data user data
 *
 * @return @c EINA_TRUE on success or @c EINA_FALSE otherwise
 */
EXPORT_API Eina_Bool ewk_view_plain_text_get(Evas_Object* o, Ewk_View_Plain_Text_Get_Callback callback, void* user_data);

/**
 * @brief Get page contents as MHTML data
 *
 * @since_tizen 2.3
 *
 * @param[in] o view object to get the page contents
 * @param[in] callback callback function to be called when the operation is finished
 * @param[in] user_data user data to be passed to the callback function
 *
 * @return @c EINA_TRUE on success or @c EINA_FALSE otherwise
 */
EXPORT_API Eina_Bool ewk_view_mhtml_data_get(Evas_Object* o, Ewk_View_MHTML_Data_Get_Callback callback, void* user_data);

/**
 * @brief Creates a new hit test for the given veiw object and point.
 *
 * @since_tizen 2.3
 *
 * @remarks The returned object should be freed by ewk_hit_test_free().
 *
 * @param[in] o view object to do hit test on
 * @param[in] x the horizontal position to query
 * @param[in] y the vertical position to query
 * @param[in] hit_test_mode the #Ewk_Hit_Test_Mode enum value to query
 *
 * @return a newly allocated hit test on success, @c 0 otherwise
 */
EXPORT_API Ewk_Hit_Test* ewk_view_hit_test_new(Evas_Object* o, int x, int y, int hit_test_mode);

/**
 * @brief Get the whole history(whole back & forward list) associated with this view.
 *
 * @since_tizen 2.3
 *
 * @param[in] o view object to get the history(whole back & forward list)
 *
 * @return a newly allocated history of @b newly allocated item\n
 *         instance. This memory of each item must be released with\n
 *         ewk_history_free() after use
 *
 * @see ewk_history_free()
 */
EXPORT_API Ewk_History* ewk_view_history_get(Evas_Object* o);

/**
 * @brief Notify that notification is closed.
 *
 * @since_tizen 2.3
 *
 * @param[in] o view object
 * @param[in] notification_list list of Ewk_Notification pointer
 *
 * @return @c EINA_TRUE on successful request or @c EINA_FALSE on failure
 */
EXPORT_API Eina_Bool ewk_view_notification_closed(Evas_Object* o, Eina_List* notification_list);

/**
 * @brief Sends the orientation of the device.
 *
 * @details If orientation value is changed, orientationchanged event will occur.\n
 *
 * Orientation will be 0 degrees when the device is oriented to natural position,\n
 *                     -90 degrees when it's left side is at the top,\n
 *                     90 degrees when it's right side is at the top,\n
 *                     180 degrees when it is upside down.
 *
 * @since_tizen 2.3
 *
 * @param[in] o view object to receive orientation event.
 * @param[in] orientation the new orientation of the device. (degree)
 */
EXPORT_API void ewk_view_orientation_send(Evas_Object* o, int orientation);

/**
 * @brief Gets the selection ranges
 *
 * @since_tizen 2.3
 *
 * @param[in] o view object to get theselection ranges
 * @param[out] left_rect the start lect(left rect) of the selection ranges
 * @param[out] right_rect the end lect(right rect) of the selection ranges
 *
 * @return @c EINA_TRUE on success, @c EINA_FALSE on failure
 */
EXPORT_API Eina_Bool ewk_view_text_selection_range_get(Evas_Object* o, Eina_Rectangle* left_rect, Eina_Rectangle* right_rect);

/**
 * @brief Gets the selected text
 *
 * @since_tizen 2.3
 *
 * @param[in] o view object to get the selected text
 *
 * @return the selected text on success or NULL on failure
 */
EXPORT_API const char* ewk_view_text_selection_text_get(Evas_Object* o);

/**
 * @brief Clear the selection range
 *
 * @since_tizen 2.3
 *
 * @param[in] o view object to clear the selection range
 *
 * @return @c EINA_TRUE on success, @c EINA_FALSE on failure
 */
EXPORT_API Eina_Bool ewk_view_text_selection_clear(Evas_Object* o);

/**
 * @brief Sets the focused input element value
 *
 * @since_tizen 2.3
 *
 * @param[in] o view object to send the value
 * @param[in] value the string value to be set
 */
EXPORT_API void ewk_view_focused_input_element_value_set(Evas_Object* o, const char* value);

/**
 * @brief Gets the focused input element's value
 *
 * @since_tizen 2.3
 *
 * @param[in] o view object to get the value
 *
 * @return focused input element's value on success or NULL on failure
 */
EXPORT_API const char* ewk_view_focused_input_element_value_get(Evas_Object* o);

/**
 * @brief Selects index of current popup menu.
 *
 * @since_tizen 2.3
 *
 * @param[in] o view object contains popup menu
 * @param[in] index index of item to select
 *
 * @return @c EINA_TRUE on success, @c EINA_FALSE on failure (probably\n
 *         popup menu is not selected or index is out of range)
 */
EXPORT_API Eina_Bool ewk_view_popup_menu_select(Evas_Object* o, unsigned int index);

/**
 * @brief Selects Multiple indexes  of current popup menu.
 *
 * @since_tizen 2.3
 *
 * @param[in] o view object contains popup menu.
 * @param[in] changed_list  list of item selected and deselected
 *
 * @return @c EINA_TRUE on success, @c EINA_FALSE on failure (probably\n
 *         popup menu is not selected or index is out of range)
 */
EXPORT_API Eina_Bool ewk_view_popup_menu_multiple_select(Evas_Object* o, Eina_Inarray* changed_list);

/**
 * @brief Forces web page to relayout
 *
 * @since_tizen 2.3
 *
 * @param [in] o view
 */
EXPORT_API void ewk_view_force_layout(const Evas_Object* o);

/**
 * @brief Clears the highlight of searched text.
 *
 * @since_tizen 2.3
 *
 * @param[in] o view object to find text
 *
 * @return @c EINA_TRUE on success, @c EINA_FALSE on errors
 */
EXPORT_API Eina_Bool ewk_view_text_find_highlight_clear(Evas_Object* o);

/**
 * @brief Counts the given string in the document.
 *
 * @details This does not highlight the matched string and just count the matched string.\n
 *
 * As the search is carried out through the whole document,\n
 * only the following #Ewk_Find_Options are valid.\n
 *  - EWK_FIND_OPTIONS_NONE\n
 *  - EWK_FIND_OPTIONS_CASE_INSENSITIVE\n
 *  - EWK_FIND_OPTIONS_AT_WORD_START\n
 *  - EWK_FIND_OPTIONS_TREAT_MEDIAL_CAPITAL_AS_WORD_START\n
 *
 * The "text,found" callback will be called with the number of matched string.
 *
 * @since_tizen 2.3
 *
 * @param[in] o view object to find text
 * @param[in] text text to find
 * @param[in] options options to find
 * @param[in] max_match_count maximum match count to find, unlimited if 0
 *
 * @return @c EINA_TRUE on success, @c EINA_FALSE on errors
 */
EXPORT_API Eina_Bool ewk_view_text_matches_count(Evas_Object* o, const char* text, Ewk_Find_Options options, unsigned max_match_count);

/*
 * @brief Sets the user chosen color. To be used when implementing a color picker.
 *
 * @details The function should only be called when a color has been requested by the document.\n
 * If called when this is not the case or when the input picker has been dismissed, this\n
 * function will fail and return EINA_FALSE.
 *
 * @since_tizen 2.3
 *
 * @param[in] o view object contains color picker
 * @param[in] r red channel value to be set
 * @param[in] g green channel value to be set
 * @param[in] b blue channel value to be set
 * @param[in] a alpha channel value to be set
 *
 * @return @c EINA_TRUE on success @c EINA_FALSE otherwise
 */
EXPORT_API Eina_Bool ewk_view_color_picker_color_set(Evas_Object* o, int r, int g, int b, int a);

/**
 * @brief Feeds the touch event to the view.
 *
 * @since_tizen 2.3
 *
 * @param[in] o view object to feed touch event
 * @param[in] type the type of touch event
 * @param[in] points a list of points (Ewk_Touch_Point) to process
 * @param[in] modifiers an Evas_Modifier handle to the list of modifier keys\n
 *        registered in the Evas. Users can get the Evas_Modifier from the Evas\n
 *        using evas_key_modifier_get() and can set each modifier key using\n
 *        evas_key_modifier_on() and evas_key_modifier_off()
 *
 * @return @c EINA_TRUE on success or @c EINA_FALSE on failure
 */
EXPORT_API Eina_Bool ewk_view_feed_touch_event(Evas_Object* o, Ewk_Touch_Event_Type type, const Eina_List* points, const Evas_Modifier* modifiers);

/**
 * @brief Sets the visibility of main frame scrollbar.
 *
 * @since_tizen 2.3
 *
 * @param[in] o view object
 * @param[in] visible visibility of main frame scrollbar
 *
 * @return @c EINA_TRUE on success or @c EINA_FALSE on failure
 */
EXPORT_API Eina_Bool ewk_view_main_frame_scrollbar_visible_set(Evas_Object* o, Eina_Bool visible);

/**
 * @brief Gets the visibility of main frame scrollbar.
 *
 * @since_tizen 2.3
 *
 * @param[in] o view object
 *
 * @return @c EINA_TRUE if scrollbar is visible or @c EINA_FALSE on failure
 */
EXPORT_API Eina_Bool ewk_view_main_frame_scrollbar_visible_get(const Evas_Object* o);

/**
 * @brief When font-family is "Tizen", use system's Settings font as default font-family
 *
 * @since_tizen 2.3
 *
 * @param[in] o view object
 */
EXPORT_API void ewk_view_use_settings_font(Evas_Object* o);

/**
 * @brief Get cookies associated with an URL.
 *
 * @since_tizen 2.3
 *
 * @remarks The return character array has to be owned by the application and freed when not required.
 *
 * @param[in] o view object in which URL is opened.
 * @param[in] url the url for which cookies needs to be obtained.
 *
 * @return @c character array containing cookies, @c NULL if no cookies are found
 */
EXPORT_API char* ewk_view_get_cookies_for_url(Evas_Object* o, const char* url);

/*
 * Get cookies associated with an URL.
 *
 * @param o view object in which URL is opened.
 * @param url the url for which cookies needs to be obtained.
 *
 * @return @c character array containing cookies, @c NULL if no cookies are found.
 *
 * The return character array has to be owned by the application and freed when not required.
 */
EXPORT_API char* ewk_view_cookies_get(Evas_Object* o, const char* url);

/**
 * @brief Sets whether to draw transparent background or not.
 *
 * @since_tizen 2.3
 *
 * @param[in] o view object to enable/disable transparent background
 * @param[in] enabled a state to set
 *
 * @return @c EINA_TRUE on success or @c EINA_FALSE on failure
 */
EXPORT_API Eina_Bool ewk_view_draws_transparent_background_set(Evas_Object* o, Eina_Bool enabled);

/**
 * @brief Queries if transparent background is enabled.
 *
 * @since_tizen 2.3
 *
 * @param[in] o view object to get whether transparent background is enabled or not
 *
 * @return @c EINA_TRUE on enable or @c EINA_FALSE on disable
 */
EXPORT_API Eina_Bool ewk_view_draws_transparent_background_get(Evas_Object* o);

/**
 * @brief Set a font for browser application
 *
 * @since_tizen 2.3
 *
 * @param[in] o view object
 */
EXPORT_API void ewk_view_browser_font_set(Evas_Object* o);

/**
 * @brief Gets the session data to be saved in a persistent store on browser exit
 *
 * @since_tizen 2.3
 *
 * @param[in] o view object whose session needs to be stored.
 * @param[in] data out parameter session data
 * @param[in] length out parameter length of session data
 */
EXPORT_API void ewk_view_session_data_get(Evas_Object* o, const char** data, unsigned* length);

/**
 * @brief Gets the staus of split scrolling supporting for overflow scroll.
 *
 * @since_tizen 2.3
 *
 * @param[in] o view object to get the status of split scrolling supporting
 *
 * @return the status of split scrolling supporting
 */
EXPORT_API Eina_Bool ewk_view_split_scroll_overflow_enabled_get(const Evas_Object* o);

/**
 * @brief Enable or disable supporting of the split scrolling for overflow scroll.
 *
 * @since_tizen 2.3
 *
 * @param[in] o view object to set the support of the split scrolling for overflow scroll
 * @param[in] enabled @c EINA_TRUE to support split scrolling, @c EINA_FALSE not to support
 *
 * @return @c EINA_TRUE on success or @c EINA_FALSE on failure
 */
EXPORT_API Eina_Bool ewk_view_split_scroll_overflow_enabled_set(Evas_Object* o, const Eina_Bool enabled);

/**
 * @brief Reloads the current page's document without cache.
 *
 * @since_tizen 2.3
 *
 * @param[in] o view object to reload current document
 *
 * @return @c EINA_TRUE on success or @c EINA_FALSE otherwise
 */
EXPORT_API Eina_Bool ewk_view_reload_bypass_cache(Evas_Object* o);

/**
 * @brief Gets the current custom character encoding name.
 *
 * @since_tizen 2.3
 *
 * @param[in] o view object to get the current encoding
 *
 * @return @c eina_strinshare containing the current encoding, or\n
 *         @c NULL if it's not set
 */
EXPORT_API const char* ewk_view_custom_encoding_get(const Evas_Object* o);

/**
 * @brief Sets the custom character encoding and reloads the page.
 *
 * @since_tizen 2.3
 *
 * @param[in] o view to set the encoding
 * @param[in] encoding the new encoding to set or @c NULL to restore the default one
 *
 * @return @c EINA_TRUE on success @c EINA_FALSE otherwise
 */
EXPORT_API Eina_Bool ewk_view_custom_encoding_set(Evas_Object* o, const char* encoding);

/**
 * @brief Callback for ewk_view_geolocation_permission_callback_set
 *
 * @since_tizen 2.3
 *
 * @param[in] o view object to request the geolocation permission
 * @param[in] request Ewk_Geolocation_Permission_Request object to get the information about geolocation permission request
 * @param[in] user_data user data
 */
typedef Eina_Bool (*Ewk_View_Geolocation_Permission_Callback)(Evas_Object* o, Ewk_Geolocation_Permission_Request* request, void* user_data);

/**
 * @brief Sets the geolocation permission callback.
 *
 * @since_tizen 2.3
 *
 * @param[in] o view object to request the geolocation permission
 * @param[in] callback Ewk_View_Geolocation_Permission_Callback function to geolocation permission
 * @param[in] user_data user data
 */
EXPORT_API void ewk_view_geolocation_permission_callback_set(Evas_Object* o, Ewk_View_Geolocation_Permission_Callback callback, void* user_data);

/**
 * @brief Callback for Ewk_Notification_Show_Callback_set
 *
 * @param[in] o view object to show the notification
 * @param[in] notification Ewk_Notification object for showing
 * @param[in] user_data user data
 */
typedef Eina_Bool (*Ewk_Notification_Show_Callback)(Evas_Object* o, Ewk_Notification* notification, void* user_data);

/**
 * @brief Callback for Ewk_Notification_Cancel_Callback_set
 *
 * @param[in] o view object to cancel the notification
 * @param[in] notificationID Ewk_Notification ID for canceling
 * @param[in] user_data user data
 */
typedef Eina_Bool (*Ewk_Notification_Cancel_Callback)(Evas_Object* o, uint64_t notificationID, void* user_data);

/**
 * @brief Sets the notification show / cancel callback.
 *
 * @param[in] o view object to show / cancel the notification
 * @param[in] showCallback Ewk_Notification_Show_Callback function to show notification
 * @param[in] cancelCallback Ewk_Notification_Cancel_Callback function to cancel notification
 * @param[in] user_data user data
 */
EXPORT_API void ewk_view_notification_callback_set(Evas_Object* o, Ewk_Notification_Show_Callback showCallback, Ewk_Notification_Cancel_Callback cancelCallback, void* user_data);

/**
 * @brief Callback for ewk_view_notification_permission_callback_set
 *
 * @since_tizen 2.3
 *
 * @param[in] o view object to request the notification permission
 * @param[in] request Ewk_Notification_Permission_Request object to get the information about notification permission request
 * @param[in] user_data user data
 */
typedef Eina_Bool (*Ewk_View_Notification_Permission_Callback)(Evas_Object* o, Ewk_Notification_Permission_Request* request, void* user_data);

/**
 * @brief Sets the notification permission callback.
 *
 * @since_tizen 2.3
 *
 * @param[in] o view object to request the notification permission
 * @param[in] callback Ewk_View_Notification_Permission_Callback function to notification permission
 * @param[in] user_data user data
 */
EXPORT_API void ewk_view_notification_permission_callback_set(Evas_Object* o, Ewk_View_Notification_Permission_Callback callback, void* user_data);

/**
 * @brief Callback for ewk_view_user_media_permission_callback_set
 *
 * @since_tizen 2.3
 *
 * @param[in] o view object to request the getUserMedia permission
 * @param[in] request Ewk_View_User_Media_Permission_Callback object to get the infomation about getUserMedia permission request
 * @param[in] user_data user data
 */
typedef Eina_Bool (*Ewk_View_User_Media_Permission_Callback)(Evas_Object* o, Ewk_User_Media_Permission_Request* request, void* user_data);

/**
 * @brief Sets the getUserMedia permission callback.
 *
 * @since_tizen 2.3
 *
 * @param[in] o view object to request the getUserMedia permission
 * @param[in] callback Ewk_View_User_Media_Permission_Callback function to getUserMedia permission
 * @param[in] user_data user data
 */
EXPORT_API void ewk_view_user_media_permission_callback_set(Evas_Object* o, Ewk_View_User_Media_Permission_Callback callback, void* user_data);
/**
 * @internal
 * @brief Callback for ewk_view_notification_show_callback_set
 *
 * @since_tizen 2.3
 *
 * @param[in] o view object to request the notification show
 * @param[in] notification Ewk_Notification object to get the information about notification show request
 * @param[in] user_data user data
 */
typedef Eina_Bool (*Ewk_View_Notification_Show_Callback)(Evas_Object *o, Ewk_Notification *notification, void *user_data);

/**
 * @internal
 * @brief Sets the notification show callback.
 *
 * @since_tizen 2.3
 *
 * @param[in] o view object to request the notification show
 * @param[in] show_callback Ewk_View_Notification_Show_Callback function to notification show
 * @param[in] user_data user data
 */
EXPORT_API void ewk_view_notification_show_callback_set(Evas_Object *o, Ewk_View_Notification_Show_Callback show_callback, void *user_data);

/**
 * @internal
 * @brief Callback for ewk_view_notification_cancel_callback_set
 *
 * @since_tizen 2.3
 *
 * @param[in] o view object to request the notification cancel
 * @param[in] notification_id Ewk_Notification object to get the information about notification cancel request
 * @param[in] user_data user data
 */
typedef Eina_Bool (*Ewk_View_Notification_Cancel_Callback)(Evas_Object *o, uint64_t notification_id, void *user_data);

/**
 * @internal
 * @brief Sets the notification cancel callback.
 *
 * @since_tizen 2.3
 *
 * @param[in] o view object to request the notification show
 * @param[in] cancel_callback Ewk_View_Notification_Cancel_Callback function to notification cancel
 * @param[in] user_data user data
 */
EXPORT_API void ewk_view_notification_cancel_callback_set(Evas_Object *o, Ewk_View_Notification_Cancel_Callback cancel_callback, void *user_data);

/**
* @}
*/

#ifdef __cplusplus
}
#endif
#endif // ewk_view_product_h
