/*
 * ISF(Input Service Framework)
 *
 * ISF is based on SCIM 1.4.7 and extended for supporting more mobile fitable.
 * Copyright (c) 2012-2014 Samsung Electronics Co., Ltd.
 *
 * Contact: Haifeng Deng <haifeng.deng@samsung.com>, Jihoon Kim <jihoon48.kim@samsung.com>
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

#define Uses_SCIM_CONFIG_PATH
#define Uses_SCIM_HELPER_MODULE
#define Uses_SCIM_PANEL_AGENT
#define Uses_SCIM_COMPOSE_KEY
#define Uses_SCIM_IMENGINE_MODULE
#define WAIT_WM

#include <sys/wait.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/times.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <list>
#include <glib.h>
#include <Evas.h>
#include <Ecore.h>
#include <Ecore_X.h>
#include <Ecore_File.h>
#include <Elementary.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <malloc.h>
#include "scim_private.h"
#include "scim.h"
#include "scim_stl_map.h"
#if HAVE_VCONF
#include <vconf.h>
#include <vconf-keys.h>
#endif
#include <privilege-control.h>
#include <dlog.h>
#if HAVE_NOTIFICATION
#include <notification.h>
#include <notification_internal.h>
#endif
#if HAVE_TTS
#include <tts.h>
#endif
#include <E_DBus.h>
#if HAVE_FEEDBACK
#include <feedback.h>
#endif
#if HAVE_BLUETOOTH
#include <bluetooth.h>
#endif
#if HAVE_PKGMGR_INFO
#include <package_manager.h>
#include <pkgmgr-info.h>
#endif
#if defined(HAVE_SYSTEMD)
#include <Ecore_Ipc.h>
#endif
#include <sys/smack.h>
#include "isf_panel_efl.h"
#include "isf_panel_utility.h"
#include "isf_query_utility.h"
#include <app_control.h>
#include <security-server.h>
#include "isf_pkg.h"

using namespace scim;


/////////////////////////////////////////////////////////////////////////////
// Declaration of macro.
/////////////////////////////////////////////////////////////////////////////
#define EFL_CANDIDATE_THEME1                            (SCIM_DATADIR "/isf_candidate_theme1.edj")

#define ISF_CANDIDATE_TABLE                             0

#define ISF_EFL_AUX                                     1
#define ISF_EFL_CANDIDATE_0                             2
#define ISF_EFL_CANDIDATE_ITEMS                         3

#define ISE_DEFAULT_HEIGHT_PORTRAIT                     444
#define ISE_DEFAULT_HEIGHT_LANDSCAPE                    316

#define ISF_SYSTEM_WM_READY_FILE                        "/tmp/.wm_ready"
#define ISF_SYSTEM_WM_WAIT_COUNT                        200
#define ISF_SYSTEM_WAIT_DELAY                           100 * 1000
#define ISF_CANDIDATE_DESTROY_DELAY                     3
#define ISF_ISE_HIDE_DELAY                              0.15

#define ISF_PREEDIT_BORDER                              16
#define ISE_LAUNCH_TIMEOUT                              2.0

#define ISF_POP_PLAY_ICON_FILE                          "/usr/share/scim/icons/pop_play.png"
#define ISF_KEYBOARD_ICON_FILE                          "/usr/share/scim/icons/noti_icon_hwkbd_module.png"
#define ISF_ISE_SELECTOR_ICON_FILE                      "/usr/share/scim/icons/noti_icon_ise_selector.png"

#define HOST_BUS_NAME        "org.tizen.usb.host"
#define HOST_OBJECT_PATH     "/Org/Tizen/Usb/Host"
#define HOST_INTERFACE_NAME  "org.tizen.usb.host"
#define HOST_KEYBOARD_SIGNAL "usbkeyboard"
#define HOST_ADDED           "added"
#define HOST_REMOVED         "removed"

#define E_PROP_DEVICEMGR_INPUTWIN                       "DeviceMgr Input Window"


/////////////////////////////////////////////////////////////////////////////
// Declaration of external variables.
/////////////////////////////////////////////////////////////////////////////
extern MapStringVectorSizeT         _groups;
extern std::vector<ImeInfoDB>       _ime_info;

extern EXAPI CommonLookupTable       g_isf_candidate_table;


/////////////////////////////////////////////////////////////////////////////
// Declaration of internal data types.
/////////////////////////////////////////////////////////////////////////////
typedef enum _WINDOW_STATE {
    WINDOW_STATE_HIDE = 0,
    WINDOW_STATE_WILL_HIDE,
    WINDOW_STATE_WILL_SHOW,
    WINDOW_STATE_SHOW,
    WINDOW_STATE_ON,
} WINDOW_STATE;

typedef struct NotiData
{
    const char* title;
    const char* content;
    const char* icon;
    String launch_app;
    int noti_id;
} NotificationData;

typedef std::vector < std::pair <String, uint32> >    VectorPairStringUint32;

/////////////////////////////////////////////////////////////////////////////
// Declaration of internal functions.
/////////////////////////////////////////////////////////////////////////////
static Evas_Object *efl_create_window                  (const char *strWinName, const char *strEffect);
static void       efl_set_transient_for_app_window     (Ecore_X_Window window);
static int        efl_get_app_window_angle             (void);
static int        efl_get_ise_window_angle             (void);
static int        efl_get_quickpanel_window_angle      (void);

static int        ui_candidate_get_valid_height        (void);
static void       ui_candidate_hide                    (bool bForce, bool bSetVirtualKbd = true, bool will_hide = false);
static void       ui_destroy_candidate_window          (void);
static void       ui_settle_candidate_window           (void);
static void       ui_candidate_show                    (bool bSetVirtualKbd = true);
static void       ui_create_candidate_window           (void);
static void       update_table                         (int table_type, const LookupTable &table);
static void       ui_candidate_window_close_button_cb  (void *data, Evas *e, Evas_Object *button, void *event_info);
static void       set_soft_candidate_geometry          (int x, int y, int width, int height);
static void       set_highlight_color                  (Evas_Object *item, uint32 nForeGround, uint32 nBackGround, bool bSetBack);
static void       ui_tts_focus_rect_hide               (void);

static Evas_Object *get_candidate                      (const String& str, Evas_Object *parent, int *total_width, uint32 ForeGround, uint32 BackGround, bool HighLight, bool SetBack, int item_num, int item);
static bool       tokenize_tag                         (const String& str, struct image *image_data);

static bool       check_wm_ready                       (void);
static void       launch_default_soft_keyboard         (keynode_t *key = NULL, void* data = NULL);

/* PanelAgent related functions */
static bool       initialize_panel_agent               (const String &config, const String &display, bool resident);

static void       slot_reload_config                   (void);
static void       slot_focus_in                        (void);
static void       slot_focus_out                       (void);
static void       slot_expand_candidate                (void);
static void       slot_contract_candidate              (void);
static void       slot_set_candidate_style             (int portrait_line, int mode);
static void       slot_update_input_context            (int type, int value);
static void       slot_update_ise_geometry             (int x, int y, int width, int height);
static void       slot_update_spot_location            (int x, int y, int top_y);
static void       slot_update_factory_info             (const PanelFactoryInfo &info);
static void       slot_show_preedit_string             (void);
static void       slot_show_aux_string                 (void);
static void       slot_show_candidate_table            (void);
static void       slot_hide_preedit_string             (void);
static void       slot_hide_aux_string                 (void);
static void       slot_hide_candidate_table            (void);
static void       slot_update_preedit_string           (const String &str, const AttributeList &attrs, int caret);
static void       slot_update_preedit_caret            (int caret);
static void       slot_update_aux_string               (const String &str, const AttributeList &attrs);
static void       slot_update_candidate_table          (const LookupTable &table);
static void       slot_select_candidate                (int index);
static void       slot_set_active_ise                  (const String &uuid, bool changeDefault);
static bool       slot_get_ise_list                    (std::vector<String> &list);
static bool       slot_get_all_helper_ise_info         (HELPER_ISE_INFO &info);
static void       slot_set_has_option_helper_ise_info  (const String &appid, bool has_option);
static void       slot_set_enable_helper_ise_info      (const String &appid, bool is_enabled);
static void       slot_show_helper_ise_list            (void);
static void       slot_show_helper_ise_selector        (void);
static bool       slot_is_helper_ise_enabled           (String appid, int &enabled);
static bool       slot_get_ise_information             (String uuid, String &name, String &language, int &type, int &option, String &module_name);
static bool       slot_get_keyboard_ise_list           (std::vector<String> &name_list);
static void       slot_get_language_list               (std::vector<String> &name);
static void       slot_get_all_language                (std::vector<String> &lang);
static void       slot_get_ise_language                (char *name, std::vector<String> &list);
static bool       slot_get_ise_info                    (const String &uuid, ISE_INFO &info);
static void       slot_get_candidate_geometry          (struct rectinfo &info);
static void       slot_get_input_panel_geometry        (struct rectinfo &info);
static void       slot_get_recent_ise_geometry         (struct rectinfo &info);
static bool       slot_check_privilege_by_sockfd       (int client_id, int cmd, String mode);
static void       slot_set_keyboard_ise                (const String &uuid);
static void       slot_get_keyboard_ise                (String &ise_name, String &ise_uuid);
static void       slot_accept_connection               (int fd);
static void       slot_close_connection                (int fd);
static void       slot_exit                            (void);

static void       slot_register_helper_properties      (int id, const PropertyList &props);
static void       slot_show_ise                        (void);
static void       slot_hide_ise                        (void);

static void       slot_will_hide_ack                   (void);
static void       slot_candidate_will_hide_ack         (void);

static void       slot_set_keyboard_mode               (int mode);
static void       slot_get_ise_state                   (int &state);
static void       slot_start_default_ise               (void);
static void       slot_stop_default_ise                (void);

static Eina_Bool  panel_agent_handler                  (void *data, Ecore_Fd_Handler *fd_handler);

static Eina_Bool  efl_create_control_window            (void);
static Ecore_X_Window efl_get_app_window               (void);
static Ecore_X_Window efl_get_quickpanel_window        (void);
static void       change_keyboard_mode                 (TOOLBAR_MODE_T mode);
static unsigned int get_ise_index                      (const String uuid);
static bool       set_active_ise                       (const String &uuid, bool launch_ise);
static bool       update_ise_list                      (std::vector<String> &list);
static void       update_ise_locale                    ();
#ifdef HAVE_NOTIFICATION
static void       delete_notification                  (NotificationData *noti_data);
static void       create_notification                  (NotificationData *noti_data);
#endif
static void       set_language_and_locale              (void);

/////////////////////////////////////////////////////////////////////////////
// Declaration of internal variables.
/////////////////////////////////////////////////////////////////////////////
static Evas_Object       *_candidate_window                 = 0;
static Evas_Object       *_candidate_area_1                 = 0;
static Evas_Object       *_candidate_area_2                 = 0;
static Evas_Object       *_candidate_bg                     = 0;
static Evas_Object       *_candidate_0_scroll               = 0;
static Evas_Object       *_candidate_scroll                 = 0;
static Evas_Object       *_scroller_bg                      = 0;
static Evas_Object       *_candidate_0_table                = 0;
static Evas_Object       *_candidate_table                  = 0;
static Evas_Object       *_candidate_0 [SCIM_LOOKUP_TABLE_MAX_PAGESIZE];
static Evas_Object       *_candidate_text [SCIM_LOOKUP_TABLE_MAX_PAGESIZE];
static Evas_Object       *_candidate_image [SCIM_LOOKUP_TABLE_MAX_PAGESIZE];
static Evas_Object       *_candidate_pop_image [SCIM_LOOKUP_TABLE_MAX_PAGESIZE];
static Evas_Object       *_seperate_0 [SCIM_LOOKUP_TABLE_MAX_PAGESIZE];
static Evas_Object       *_seperate_items [SCIM_LOOKUP_TABLE_MAX_PAGESIZE];
static Evas_Object       *_line_0 [SCIM_LOOKUP_TABLE_MAX_PAGESIZE];
static Evas_Object       *_line_items [SCIM_LOOKUP_TABLE_MAX_PAGESIZE];
static Evas_Object       *_more_btn                         = 0;
static Evas_Object       *_close_btn                        = 0;
static bool               _candidate_show_requested         = false;
static bool               _updated_hide_state_geometry      = false;

static int                _candidate_x                      = 0;
static int                _candidate_y                      = 0;
static int                _candidate_width                  = 0;
static int                _candidate_height                 = 0;
static int                _soft_candidate_width             = 0;
static int                _soft_candidate_height            = 0;
static int                _candidate_valid_height           = 0;

static bool               _candidate_area_1_visible         = false;
static bool               _candidate_area_2_visible         = false;
static bool               _aux_area_visible                 = false;

static ISF_CANDIDATE_MODE_T          _candidate_mode        = SOFT_CANDIDATE_WINDOW;
static ISF_CANDIDATE_PORTRAIT_LINE_T _candidate_port_line   = ONE_LINE_CANDIDATE;

static int                _candidate_port_width             = 480;
static int                _candidate_port_height_min        = 76;
static int                _candidate_port_height_min_2      = 150;
static int                _candidate_port_height_max        = 286;
static int                _candidate_port_height_max_2      = 350;
static int                _candidate_land_width             = 784;
static int                _candidate_land_height_min        = 84;
static int                _candidate_land_height_min_2      = 168;
static int                _candidate_land_height_max        = 150;
static int                _candidate_land_height_max_2      = 214;
static int                _candidate_area_1_pos [2]         = {0, 2};
static int                _more_btn_pos [4]                 = {369, 11, 689, 11};
static int                _close_btn_pos [4]                = {362, 211, 682, 75};
static int                _more_btn_width                   = 80;
static int                _more_btn_height                  = 64;

static int                _h_padding                        = 4;
static int                _v_padding                        = 2;
static int                _item_min_width                   = 99;
static int                _item_min_height                  = 82;

static int                _candidate_scroll_0_width_min     = 350;
static int                _candidate_scroll_0_width_max     = 670;

static int                _candidate_scroll_width           = 453;
static int                _candidate_scroll_width_min       = 453;
static int                _candidate_scroll_width_max       = 663;
static int                _candidate_scroll_height_min      = 124;
static int                _candidate_scroll_height_max      = 190;

const  int                MORE_BUTTON_INDEX                 = -1;
const  int                CLOSE_BUTTON_INDEX                = -2;
const  int                INVALID_TTS_FOCUS_INDEX           = -100;
static int                _candidate_tts_focus_index        = INVALID_TTS_FOCUS_INDEX;
static uint32             _candidate_display_number         = 0;
static std::vector<uint32> _candidate_row_items;
static Evas_Object       *_tts_focus_rect                   = 0;
static bool               _wait_stop_event                  = false;

static Evas_Object       *_preedit_window                   = 0;
static Evas_Object       *_preedit_text                     = 0;
static int                _preedit_width                    = 100;
static int                _preedit_height                   = 54;

static Evas_Object       *_aux_area                         = 0;
static Evas_Object       *_aux_line                         = 0;
static Evas_Object       *_aux_table                        = 0;
static int                _aux_height                       = 0;
static int                _aux_port_width                   = 444;
static int                _aux_land_width                   = 764;
static std::vector<Evas_Object *> _aux_items;
static std::vector<Evas_Object *> _aux_seperates;

static Evas_Object       *_tmp_preedit_text                 = 0;
static Evas_Object       *_tmp_aux_text                     = 0;
static Evas_Object       *_tmp_candidate_text               = 0;

static int                _spot_location_x                  = -1;
static int                _spot_location_y                  = -1;
static int                _spot_location_top_y              = -1;
static int                _candidate_angle                  = 0;
static int                _ise_angle                        = -1;

static int                _ise_width                        = 0;
static int                _ise_height                       = 0;
static WINDOW_STATE       _ise_state                        = WINDOW_STATE_HIDE;
static WINDOW_STATE       _candidate_state                  = WINDOW_STATE_HIDE;

static int                _indicator_height                 = 0;//24;
static int                _screen_width                     = 720;
static int                _screen_height                    = 1280;
static float              _width_rate                       = 1.0;
static float              _height_rate                      = 1.0;
static int                _blank_width                      = 30;

static String             _candidate_name                   = String ("candidate");
static String             _candidate_edje_file              = String (EFL_CANDIDATE_THEME1);

static String             _candidate_font_name              = String ("Tizen");
static int                _candidate_font_size              = 38;
static int                _aux_font_size                    = 38;
static int                _click_object                     = 0;
static int                _click_down_pos [2]               = {0, 0};
static int                _click_up_pos [2]                 = {0, 0};
static bool               _is_click                         = true;
static String             _initial_ise_uuid                 = String ("");
static String             _locale_string                    = String ("");
static ConfigPointer      _config;
static PanelAgent        *_panel_agent                      = 0;
static std::vector<Ecore_Fd_Handler *> _read_handler_list;

static clock_t            _clock_start;

static Ecore_Timer       *_check_size_timer                 = NULL;
static Ecore_Timer       *_longpress_timer                  = NULL;
static Ecore_Timer       *_destroy_timer                    = NULL;
static Ecore_Timer       *_off_prepare_done_timer           = NULL;
static Ecore_Timer       *_candidate_hide_timer             = NULL;
static Ecore_Timer       *_ise_hide_timer                   = NULL;

static Ecore_X_Window     _ise_window                       = 0;
static Ecore_X_Window     _app_window                       = 0;
static Ecore_X_Window     _control_window                   = 0;
static Ecore_X_Window     _input_win                        = 0;

#if HAVE_PKGMGR_INFO
static package_manager_h    pkgmgr                          = NULL;
static VectorPairStringUint32   g_pkgids_to_be_uninstalled;
static Ecore_Timer    *g_release_uninstalled_ime_info_timer = NULL;
static String               g_stopped_helper_pkgid          = "";
static Ecore_Timer          *g_start_default_helper_timer   = NULL;
static VectorPairStringUint32   g_pkgids_to_be_updated_and_installed;
static String               g_updated_helper_pkgid          = "";
#endif

static bool               _launch_ise_on_request            = false;
static bool               _soft_keyboard_launched           = false;
static bool               _focus_in                         = false;

static bool               candidate_expanded                = false;
static int                _candidate_image_count            = 0;
static int                _candidate_text_count             = 0;
static int                _candidate_pop_image_count        = 0;
static int                candidate_image_height            = 38;
static int                candidate_play_image_width_height = 19;

static const int          CANDIDATE_TEXT_OFFSET             = 2;

static double             _app_scale                        = 1.0;
static double             _system_scale                     = 1.0;

#ifdef HAVE_NOTIFICATION
static NotificationData hwkbd_module_noti                   = {"Input detected from hardware keyboard", "Tap to use virtual keyboard", ISF_KEYBOARD_ICON_FILE, "", 0};
static NotificationData ise_selector_module_noti            = {"Select input method", NULL, ISF_ISE_SELECTOR_ICON_FILE, "", 0};
#endif

#if HAVE_TTS
static tts_h              _tts                              = NULL;
#endif

#if HAVE_FEEDBACK
static bool               feedback_initialized              = false;
#endif

static E_DBus_Connection     *edbus_conn;
static E_DBus_Signal_Handler *edbus_handler;

static Ecore_Event_Handler *_candidate_show_handler         = NULL;

static String ime_selector_app = "";
static String ime_list_app = "";

enum {
    EMOJI_IMAGE_WIDTH = 0,
    EMOJI_IMAGE_HEIGHT,
    EMOJI_IMAGE_KIND,
    EMOJI_IMAGE_TAG_FLAG,
    EMOJI_IMAGE_POP_FLAG,
    EMOJI_IMAGE_END
};

struct image
{
    String path;
    int emoji_option[EMOJI_IMAGE_END];
};

/* This structure stores the geometry information reported by ISE */
struct GeometryCache
{
    bool valid;                /* Whether this information is currently valid */
    int angle;                 /* For which angle this information is useful */
    struct rectinfo geometry;  /* Geometry information */
};

static struct GeometryCache _ise_reported_geometry          = {0, 0, {0, 0, 0, 0}};
static struct GeometryCache _portrait_recent_ise_geometry   = {0, 0, {0, 0, 0, 0}};
static struct GeometryCache _landscape_recent_ise_geometry  = {0, 0, {0, 0, 0, 0}};

static void show_soft_keyboard (void)
{
    /* If the current toolbar mode is not HELPER_MODE, do not proceed */
    if (_panel_agent->get_current_toolbar_mode () != TOOLBAR_HELPER_MODE) {
        LOGD ("Current toolbar mode should be TOOBAR_HELPER_MODE but is %d, returning", _panel_agent->get_current_toolbar_mode ());
        return;
    }
    if (_ise_state == WINDOW_STATE_HIDE) {
        ecore_x_test_fake_key_press ("XF86MenuKB");
    }
}

static void get_input_window (void)
{
    int win_ret = -1;
    Ecore_X_Atom atom = 0;

    if (_input_win == 0) {
        atom = ecore_x_atom_get (E_PROP_DEVICEMGR_INPUTWIN);
        win_ret = ecore_x_window_prop_window_get (ecore_x_window_root_first_get (), atom, &_input_win, 1);
        if (_input_win == 0 || win_ret < 1) {
            LOGW ("Input window is NULL!\n");
        } else {
            ecore_x_event_mask_set (_input_win, ECORE_X_EVENT_MASK_WINDOW_PROPERTY);
        }
    }
}

static void usb_keyboard_signal_cb (void *data, DBusMessage *msg)
{
    DBusError err;
    char *str = NULL;

    if (!msg) {
        LOGW ("No Message");
        return;
    }

    if (dbus_message_is_signal (msg, HOST_INTERFACE_NAME, HOST_KEYBOARD_SIGNAL) == 0) {
        LOGW ("HOST_KEYBOARD_SIGNAL");
        return;
    }

    dbus_error_init (&err);

    if (dbus_message_get_args (msg, &err, DBUS_TYPE_STRING, &str, DBUS_TYPE_INVALID) == 0) {
        LOGW ("DBUS_TYPE_INVALID");
        return;
    }

    if (!str) return;

    if (!strncmp (str, HOST_ADDED, strlen (HOST_ADDED))) {
        LOGD ("HOST_ADDED");
        return;
    }

    if (!strncmp (str, HOST_REMOVED, strlen (HOST_REMOVED))) {
        LOGD ("HOST_REMOVED");
        if (_panel_agent->get_current_toolbar_mode () == TOOLBAR_KEYBOARD_MODE) {
            change_keyboard_mode (TOOLBAR_HELPER_MODE);
        }
        return;
    }

    LOGW ("ERROR: msg (%s) is improper", str);
}

static void unregister_edbus_signal_handler (void)
{
    if (edbus_conn) {
        LOGD ("unregister signal handler for keyboard");
        if (edbus_handler) {
            e_dbus_signal_handler_del (edbus_conn, edbus_handler);
            edbus_handler = NULL;
        }
        e_dbus_connection_close (edbus_conn);
        edbus_conn = NULL;
    }
    e_dbus_shutdown ();
}

static int register_edbus_signal_handler (void)
{
    int retry;
    LOGD ("start register_edbus_signal_handler");

    retry = 0;
    while (e_dbus_init () == 0) {
        retry++;
        if (retry >= 10) {
            LOGW ("retry fail");
            return -1;
        }
    }

    edbus_conn = e_dbus_bus_get (DBUS_BUS_SYSTEM);
    if (!edbus_conn) {
        LOGW ("connection fail");
        return -1;
    }

    edbus_handler = e_dbus_signal_handler_add (edbus_conn, NULL, HOST_OBJECT_PATH, HOST_INTERFACE_NAME, HOST_KEYBOARD_SIGNAL, usb_keyboard_signal_cb, NULL);
    if (!edbus_handler) {
        LOGW ("cannot register signal");
        return -1;
    }

    LOGD ("Success register");

    return 0;
}

#ifdef HAVE_NOTIFICATION
static void delete_notification (NotificationData *noti_data)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "...\n";

    if (noti_data->noti_id != 0) {
        notification_delete_by_priv_id ("isf-panel-efl", NOTIFICATION_TYPE_ONGOING, noti_data->noti_id);
        LOGD ("deleted notification : %s\n", noti_data->launch_app.c_str ());
        noti_data->noti_id = 0;
    }
}

static void create_notification (NotificationData *noti_data)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "...\n";

    notification_h notification = NULL;

    if (noti_data->noti_id != 0) {
        notification_delete_by_priv_id ("isf-panel-efl", NOTIFICATION_TYPE_ONGOING, noti_data->noti_id);
        noti_data->noti_id = 0;
    }

    notification = notification_create (NOTIFICATION_TYPE_ONGOING);

    notification_set_pkgname (notification, "isf-panel-efl");
    notification_set_layout (notification, NOTIFICATION_LY_NOTI_EVENT_SINGLE);
    notification_set_image (notification, NOTIFICATION_IMAGE_TYPE_ICON, noti_data->icon);
    notification_set_text (notification, NOTIFICATION_TEXT_TYPE_TITLE, _(noti_data->title), NULL, NOTIFICATION_VARIABLE_TYPE_NONE);
    notification_set_text (notification, NOTIFICATION_TEXT_TYPE_CONTENT, _(noti_data->content), NULL, NOTIFICATION_VARIABLE_TYPE_NONE);
    notification_set_display_applist (notification, NOTIFICATION_DISPLAY_APP_NOTIFICATION_TRAY);

    app_control_h service = NULL;
    app_control_create (&service);
    app_control_set_operation (service, APP_CONTROL_OPERATION_DEFAULT);
    app_control_set_app_id (service, noti_data->launch_app.c_str ());

    notification_set_launch_option (notification, NOTIFICATION_LAUNCH_OPTION_APP_CONTROL, (void *)service);
    notification_insert (notification, &noti_data->noti_id);
    app_control_destroy (service);
    notification_free (notification);
}
#endif

static bool tokenize_tag (const String& str, struct image *image_token)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << " str=" << str << ", length=" << str.length () << "\n";
    if (str.length () <= 0) {
        LOGW ("str is empty!!!\n");
        return false;
    }

    char **tag_str = NULL;
    int i = 0;
    tag_str = eina_str_split (str.c_str (), "\u3013", 0);

    if (!tag_str)
        return false;

    for (i = 0; tag_str [i]; i++) {
        if (i == 0) {
            if (str.length () == strlen (tag_str[i])) {
                if (tag_str) {
                    if (tag_str[0])
                        free (tag_str[0]);

                    free (tag_str);
                }
                return false;
            }

            image_token->path = String (tag_str[i]);
        } else {
            if (i - 1 < EMOJI_IMAGE_END)
                image_token->emoji_option [i - 1] = atoi (tag_str[i]);
            else
                LOGW ("emoji option is more than EMOJI_IMAGE_END!!!\n");
        }
    }

    if (tag_str[0])
        free (tag_str[0]);

    free (tag_str);

    return true;
}

static Evas_Object* get_candidate (const String& str, Evas_Object *parent, int *total_width, uint32 ForeGround, uint32 BackGround, bool HighLight, bool SetBack, int item_num, int item_index)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << " str=" << str << ", length=" << str.length () << "\n";

    struct image image_data;
    int object_width = 0, text_width = 0, image_width = 0, image_height = 0, max_width = 0, button_width = 0;
    int i = 0, j = 0;
    int image_get_width = 0, image_get_height = 0;
    char image_key [10] = {0, };
    char **splited_string = NULL;
    char **sub_splited_string = NULL;
    double image_rate = 0.0;
    bool tokenize_result = false;
    bool candidate_is_long = false;

    Evas_Object *candidate_object_table = NULL;
    Evas_Object *candidate_object_table_bg_rect = NULL;
    Evas_Object *candidate_left_padding = NULL;

    candidate_object_table = elm_table_add (parent);

    candidate_left_padding = evas_object_rectangle_add (evas_object_evas_get (parent));
    evas_object_size_hint_weight_set (candidate_left_padding, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
    evas_object_size_hint_align_set (candidate_left_padding, EVAS_HINT_FILL, EVAS_HINT_FILL);
    evas_object_size_hint_min_set (candidate_left_padding, _blank_width, 1);
    evas_object_color_set (candidate_left_padding, 0, 0, 0, 0);
    elm_table_pack (candidate_object_table, candidate_left_padding, 0, 0, _blank_width, 1);
    evas_object_show (candidate_left_padding);

    object_width += _blank_width;
    if (item_num > 1 && item_index == 0)
        button_width = 92 * _width_rate;
    else
        button_width = 0;

    splited_string = eina_str_split (str.c_str (), "\uE000", 0);
    if (splited_string) {
        for (i = 0; splited_string [i]; i++) {
            if (candidate_is_long)
                break;
            sub_splited_string = eina_str_split (splited_string [i], "\uE001", 0);
            if (sub_splited_string) {
                for (j = 0; sub_splited_string [j]; j++) {
                    if (candidate_is_long)
                        break;
                    tokenize_result = tokenize_tag (sub_splited_string [j], &image_data);
                    if (tokenize_result && _candidate_image_count < SCIM_LOOKUP_TABLE_MAX_PAGESIZE) {
                        _candidate_image [_candidate_image_count] = elm_image_add (parent);
                        snprintf (image_key, sizeof (image_key), "%d",_candidate_image_count);
                        elm_image_file_set (_candidate_image [_candidate_image_count], image_data.path.c_str (), image_key);
                        elm_image_animated_set (_candidate_image [_candidate_image_count], EINA_TRUE);
                        elm_image_animated_play_set (_candidate_image [_candidate_image_count], EINA_TRUE);
                        elm_image_object_size_get (_candidate_image [_candidate_image_count], &image_get_width, &image_get_height);
                        LOGD ("image_path=%s, key=%s\n", image_data.path.c_str (), image_key);

                        if (image_get_height > image_get_width)
                            image_rate = ((double)candidate_image_height / (double)image_get_width);
                        else
                            image_rate = ((double)candidate_image_height / (double)image_get_height);

                        image_width = (int)((double)image_get_width * image_rate);
                        image_height = candidate_image_height;

                        if (_candidate_angle == 90 || _candidate_angle == 270)
                            max_width = _candidate_land_width - (_blank_width + object_width + button_width + (2 * CANDIDATE_TEXT_OFFSET));
                        else
                            max_width = _candidate_port_width - (_blank_width + object_width + button_width + (2 * CANDIDATE_TEXT_OFFSET));

                        if (image_width > max_width) {
                            Evas_Object *candidate_end = edje_object_add (evas_object_evas_get (parent));
                            edje_object_file_set (candidate_end, _candidate_edje_file.c_str (), _candidate_name.c_str ());
                            evas_object_show (candidate_end);
                            edje_object_part_text_set (candidate_end, "candidate", "...");
                            edje_object_scale_set (_candidate_text [_candidate_text_count], _height_rate);

                            text_width = max_width;
                            evas_object_size_hint_min_set (candidate_end, text_width + (2 * CANDIDATE_TEXT_OFFSET), _item_min_height);
                            if (HighLight || SetBack) {
                                set_highlight_color (candidate_end, ForeGround, BackGround, SetBack);
                            }
                            elm_table_pack (candidate_object_table, candidate_end, object_width, 0, text_width + (2 * CANDIDATE_TEXT_OFFSET), _candidate_font_size);
                            object_width += (text_width + (2 * CANDIDATE_TEXT_OFFSET));

                            if (_candidate_image [_candidate_image_count]) {
                                evas_object_del (_candidate_image [_candidate_image_count]);
                                _candidate_image [_candidate_image_count] = NULL;
                            }
                            candidate_is_long = true;
                            break;
                        }

                        evas_object_resize (_candidate_image [_candidate_image_count], image_width, image_height);
                        evas_object_show (_candidate_image [_candidate_image_count]);
                        evas_object_size_hint_min_set (_candidate_image [_candidate_image_count], image_width, image_height);

                        elm_table_pack (candidate_object_table, _candidate_image [_candidate_image_count], object_width, 1, image_width, image_height);
                        object_width += image_width;
                        _candidate_image_count++;

                        if (image_data.emoji_option [EMOJI_IMAGE_POP_FLAG] == 1 && image_width > 0 &&  _candidate_pop_image_count < SCIM_LOOKUP_TABLE_MAX_PAGESIZE) {
                            _candidate_pop_image [_candidate_pop_image_count] = elm_image_add (parent);
                            elm_image_file_set (_candidate_pop_image [_candidate_pop_image_count], ISF_POP_PLAY_ICON_FILE, image_key);
                            evas_object_resize (_candidate_pop_image [_candidate_pop_image_count], candidate_play_image_width_height, candidate_play_image_width_height);
                            evas_object_show (_candidate_pop_image [_candidate_pop_image_count]);
                            evas_object_size_hint_min_set (_candidate_pop_image [_candidate_pop_image_count], candidate_play_image_width_height, candidate_play_image_width_height);

                            elm_table_pack (candidate_object_table, _candidate_pop_image [_candidate_pop_image_count],
                                    object_width - candidate_play_image_width_height, image_height - candidate_play_image_width_height - 2,
                                    candidate_play_image_width_height, candidate_play_image_width_height);

                            _candidate_pop_image_count++;
                        }

                    } else if (strlen (sub_splited_string [j]) > 0 && _candidate_text_count < SCIM_LOOKUP_TABLE_MAX_PAGESIZE) {
                        _candidate_text [_candidate_text_count] = edje_object_add (evas_object_evas_get (parent));
                        edje_object_file_set (_candidate_text [_candidate_text_count], _candidate_edje_file.c_str (), _candidate_name.c_str ());
                        evas_object_show (_candidate_text [_candidate_text_count]);
                        edje_object_part_text_set (_candidate_text [_candidate_text_count], "candidate", sub_splited_string [j]);
                        edje_object_text_class_set (_candidate_text [_candidate_text_count], "tizen",
                                _candidate_font_name.c_str (), _candidate_font_size);
                        evas_object_text_text_set (_tmp_candidate_text, sub_splited_string [j]);
                        evas_object_geometry_get (_tmp_candidate_text, NULL, NULL, &text_width, NULL);

                        if (_candidate_angle == 90 || _candidate_angle == 270)
                            max_width = _candidate_land_width - (_blank_width + object_width + button_width + (2 * CANDIDATE_TEXT_OFFSET));
                        else
                            max_width = _candidate_port_width - (_blank_width + object_width + button_width + (2 * CANDIDATE_TEXT_OFFSET));

                        if (text_width > max_width) {
                            candidate_is_long = true;
                            /* In order to avoid overlap issue, calculate show_string */
                            String show_string = String (sub_splited_string [j]);
                            int    show_length = text_width;
                            while (show_length > max_width && show_string.length () > 1) {
                                show_string = show_string.substr (0, show_string.length () - 1);
                                evas_object_text_text_set (_tmp_candidate_text, (show_string + String ("...")).c_str ());
                                evas_object_geometry_get (_tmp_candidate_text, NULL, NULL, &show_length, NULL);
                            }
                            edje_object_part_text_set (_candidate_text [_candidate_text_count], "candidate", (show_string + String ("...")).c_str ());
                            text_width = max_width;
                        }

                        evas_object_size_hint_min_set (_candidate_text [_candidate_text_count], text_width + (2 * CANDIDATE_TEXT_OFFSET), _item_min_height);
                        if (HighLight || SetBack) {
                            set_highlight_color (_candidate_text [_candidate_text_count], ForeGround, BackGround, SetBack);
                        }
                        elm_table_pack (candidate_object_table, _candidate_text [_candidate_text_count], object_width, 0, text_width + (2 * CANDIDATE_TEXT_OFFSET), _candidate_font_size);
                        object_width += (text_width + (2 * CANDIDATE_TEXT_OFFSET));
                        _candidate_text_count++;
                    }
                }
            }
        }

        if (splited_string [0])
            free (splited_string [0]);

        free (splited_string);
    }

    if (sub_splited_string) {
        if (sub_splited_string [0])
            free (sub_splited_string [0]);

        free (sub_splited_string);
    }

    *total_width = object_width + _blank_width;

    candidate_object_table_bg_rect = edje_object_add (evas_object_evas_get (parent));
    edje_object_file_set (candidate_object_table_bg_rect, _candidate_edje_file.c_str (), "candidate_object_table");
    evas_object_size_hint_weight_set (candidate_object_table_bg_rect, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
    evas_object_size_hint_align_set (candidate_object_table_bg_rect, EVAS_HINT_FILL, EVAS_HINT_FILL);
    evas_object_size_hint_min_set (candidate_object_table_bg_rect, *total_width, _item_min_height);
    elm_table_pack (candidate_object_table, candidate_object_table_bg_rect, 0, 0, *total_width ,_item_min_height);
    evas_object_show (candidate_object_table_bg_rect);

    evas_object_size_hint_align_set (candidate_object_table, EVAS_HINT_FILL, EVAS_HINT_FILL);
    evas_object_size_hint_weight_set (candidate_object_table, EVAS_HINT_EXPAND, 0.0);

    return candidate_object_table;
}

/////////////////////////////////////////////////////////////////////////////
// Implementation of internal functions.
/////////////////////////////////////////////////////////////////////////////
/**
 * @brief Print system time point for panel performance.
 *
 * @param strInfo The output information.
 */
static void check_time (const char *strInfo)
{
    gettime (_clock_start, strInfo);
    ISF_LOG ("%s ppid=%d pid=%d\n", strInfo, getppid (), getpid ());
}

/**
 * @brief Check the specific file.
 *
 * @param strFile The file path.
 *
 * @return true if the file is existed, otherwise false.
 */
static bool check_file (const char* strFile)
{
    struct stat st;

    /* Workaround so that "/" returns a true, otherwise we can't monitor "/" in ecore_file_monitor */
    if (stat (strFile, &st) < 0 && strcmp (strFile, "/"))
        return false;
    else
        return true;
}

/**
 * @brief Flush memory for elm.
 *
 * @return void
 */
static void flush_memory (void)
{
    elm_cache_all_flush ();
    malloc_trim (0);
}

/**
 * @brief Get ISE geometry information.
 *        Returns the "expected" ISE geometry when kbd_state is ON, otherwise w/h set to 0
 *
 * @param info The data is used to store ISE position and size.
 * @param kbd_state The keyboard state.
 */
static struct rectinfo get_ise_geometry ()
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "...\n";

    struct rectinfo info = {0, 0, 0, 0};

    int win_w = _screen_width, win_h = _screen_height;
    int angle = (_ise_angle == -1) ? efl_get_app_window_angle () : _ise_angle;

    if (angle == 90 || angle == 270) {
        win_w = _screen_height;
        win_h = _screen_width;
    }

    /* If we have geometry reported by ISE, use the geometry information */
    if (_ise_reported_geometry.valid && _ise_reported_geometry.angle == angle) {
        info = _ise_reported_geometry.geometry;
        /* But still, if the current ISE is not in SHOW state, set w/h to 0 */
        if (_ise_state != WINDOW_STATE_SHOW) {
            info.pos_y = (win_h > win_w) ? win_h : win_w;
            info.width = 0;
            info.height = 0;
        }
    } else {
        /* READ ISE's SIZE HINT HERE */
        int pos_x, pos_y, width, height;
        if (ecore_x_e_window_rotation_geometry_get (_ise_window, angle,
                &pos_x, &pos_y, &width, &height)) {
            info.pos_x = pos_x;
            info.pos_y = pos_y;

            if (angle == 90 || angle == 270) {
                info.width = height;
                info.height = width;
            } else {
                info.width = width;
                info.height = height;
            }

            info.pos_x = (int)info.width > win_w ? 0 : (win_w - info.width) / 2;
            if (_panel_agent->get_current_toolbar_mode () == TOOLBAR_KEYBOARD_MODE) {
                info.pos_x = 0;
                info.pos_y = (win_h > win_w) ? win_h : win_w;
                info.width = 0;
                info.height = 0;
            } else {
                if (_ise_state == WINDOW_STATE_SHOW) {
                    info.pos_y = win_h - info.height;
                } else {
                    info.pos_y = (win_h > win_w) ? win_h : win_w;
                    info.width = 0;
                    info.height = 0;
                }
            }

            LOGD ("angle : %d, w_angle : %d, mode : %d, Geometry : %d %d %d %d\n",
                    angle, _ise_angle,
                    _panel_agent->get_current_toolbar_mode (),
                    info.pos_x, info.pos_y, info.width, info.height);
        } else {
            pos_x = 0;
            pos_y = 0;
            width = 0;
            height = 0;
        }
    }

    _ise_width  = info.width;
    _ise_height = info.height;

    return info;
}

/**
 * @brief Set keyboard geometry for autoscroll.
 *        This includes the ISE geometry together with candidate window
 *
 * @param kbd_state The keyboard state.
 */
static void set_keyboard_geometry_atom_info (Ecore_X_Window window, struct rectinfo ise_rect)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "...\n";

    if (_panel_agent->get_current_toolbar_mode () == TOOLBAR_KEYBOARD_MODE) {
        ise_rect.pos_x = 0;

        if (_candidate_mode == FIXED_CANDIDATE_WINDOW) {
            if (_candidate_window && _candidate_state == WINDOW_STATE_SHOW) {
                ise_rect.width  = _candidate_width;
                ise_rect.height = _candidate_height;
            }
        } else if (_candidate_mode == SOFT_CANDIDATE_WINDOW) {
                ise_rect.width  = _soft_candidate_width;
                ise_rect.height = _soft_candidate_height;
        }
        int angle = efl_get_app_window_angle ();
        if (angle == 90 || angle == 270)
            ise_rect.pos_y = _screen_width - ise_rect.height;
        else
            ise_rect.pos_y = _screen_height - ise_rect.height;
    } else {
        if (_candidate_mode == FIXED_CANDIDATE_WINDOW) {
            if (_candidate_window && _candidate_state == WINDOW_STATE_SHOW) {
                _candidate_valid_height = ui_candidate_get_valid_height ();
                if ((_candidate_height - _candidate_valid_height) > _ise_height) {
                    _candidate_valid_height = _candidate_height;
                    ise_rect.pos_y  = ise_rect.pos_y + ise_rect.height - _candidate_height;
                    ise_rect.height = _candidate_height;
                } else {
                    ise_rect.pos_y  -= _candidate_valid_height;
                    ise_rect.height += _candidate_valid_height;
                }
            }
        }
    }

    ecore_x_e_illume_keyboard_geometry_set (window, ise_rect.pos_x, ise_rect.pos_y, ise_rect.width, ise_rect.height);
    LOGD ("KEYBOARD_GEOMETRY_SET : %d %d %d %d\n", ise_rect.pos_x, ise_rect.pos_y, ise_rect.width, ise_rect.height);
    SCIM_DEBUG_MAIN (3) << "    KEYBOARD_GEOMETRY x=" << ise_rect.pos_x << " y=" << ise_rect.pos_y
        << " width=" << ise_rect.width << " height=" << ise_rect.height << "\n";

    /* even the kbd_state is OFF, consider the keyboard is still ON if we have candidate opened */
    if (ise_rect.width == 0 && ise_rect.height == 0) {
        ecore_x_e_virtual_keyboard_state_set (window, ECORE_X_VIRTUAL_KEYBOARD_STATE_OFF);
    } else {
        ecore_x_e_virtual_keyboard_state_set (window, ECORE_X_VIRTUAL_KEYBOARD_STATE_ON);

        int angle = efl_get_ise_window_angle ();

        if (angle == 0 || angle == 180) {
            _portrait_recent_ise_geometry.valid = true;
            _portrait_recent_ise_geometry.geometry = ise_rect;
        }
        else {
            _landscape_recent_ise_geometry.valid = true;
            _landscape_recent_ise_geometry.geometry = ise_rect;
        }
    }
}

/**
 * @brief Get ISE index according to uuid.
 *
 * @param uuid The ISE uuid.
 *
 * @return The ISE index
 */
static unsigned int get_ise_index (const String uuid)
{
    unsigned int index = 0;
    if (uuid.length () > 0) {
        for (unsigned int i = 0; i < _ime_info.size (); i++) {
            if (uuid == _ime_info[i].appid) {
                index = i;
                break;
            }
        }
    }

    return index;
}

static void set_keyboard_engine (String active_uuid)
{
    String IMENGINE_KEY  = String (SCIM_CONFIG_DEFAULT_IMENGINE_FACTORY) + String ("/") + String ("~other");
    String keyboard_uuid = _config->read (IMENGINE_KEY, String (""));
    if (active_uuid != keyboard_uuid) {
        _panel_agent->change_factory (active_uuid);
        _config->write (IMENGINE_KEY, active_uuid);
        _config->flush ();
    }
}

static void _update_ime_info(void)
{
    std::vector<String> ise_langs;

    _ime_info.clear();
    isf_pkg_select_all_ime_info_db(_ime_info);

    /* Update _groups */
    _groups.clear();
    for (size_t i = 0; i < _ime_info.size (); ++i) {
        scim_split_string_list(ise_langs, _ime_info[i].languages);
        for (size_t j = 0; j < ise_langs.size (); j++) {
            if (std::find (_groups[ise_langs[j]].begin (), _groups[ise_langs[j]].end (), i) == _groups[ise_langs[j]].end ())
                _groups[ise_langs[j]].push_back (i);
        }
        ise_langs.clear ();
    }
}

static void _initialize_ime_info (void)
{
    std::vector<ImeInfoDB>::iterator iter;
    VectorPairStringUint32   ime_on_off;
    // Store is_enabled values of each keyboard
    for (iter = _ime_info.begin (); iter != _ime_info.end (); iter++) {
        if (iter->mode == TOOLBAR_HELPER_MODE) {
            ime_on_off.push_back (std::make_pair (iter->appid, iter->is_enabled));
        }
    }
    // Delete the whole ime_info DB and reload
    isf_db_delete_ime_info ();
    _update_ime_info ();
    // Restore is_enabled value to valid keyboards
    for (iter = _ime_info.begin (); iter != _ime_info.end (); iter++) {
        if (iter->mode == TOOLBAR_HELPER_MODE) {
            for (VectorPairStringUint32::iterator it = ime_on_off.begin (); it != ime_on_off.end (); it++) {
                if (it->first.compare (iter->appid) == 0) {
                    if (it->second != iter->is_enabled) {
                        iter->is_enabled = it->second;
                        isf_db_update_is_enabled_by_appid (iter->appid.c_str (), static_cast<bool>(iter->is_enabled));
                    }
                    ime_on_off.erase (it);
                    break;
                }
            }
        }
    }
}

#if HAVE_PKGMGR_INFO
/**
 * @brief Insert or update ime_info data with pkgid.
 *
 * @param pkgid pkgid to insert/update ime_info table.
 *
 * @return 1 on successfull insert, 2 on successful update, -1 if pkgid is not IME package, otherwise return 0.
 */
static int _isf_insert_ime_info_by_pkgid(const char *pkgid)
{
    int ret = -1;
    pkgmgrinfo_pkginfo_h handle = NULL;
    int result = 0;  // 0: not IME, 1: Inserted, 2: Updated (because of the same appid)

    if (!pkgid) {
        LOGW("pkgid is null.");
        return 0;
    }

    ret = pkgmgrinfo_pkginfo_get_pkginfo(pkgid, &handle);
    if (ret != PMINFO_R_OK) {
        LOGW("pkgmgrinfo_pkginfo_get_pkginfo(\"%s\",~) returned %d", pkgid, ret);
        return 0;
    }

    ret = pkgmgrinfo_appinfo_get_list(handle, PMINFO_UI_APP, isf_pkg_ime_app_list_cb, (void *)&result);
    if (ret != PMINFO_R_OK) {
        LOGW("pkgmgrinfo_appinfo_get_list failed(%d)", ret);
        ret = 0;
    }
    else if (result)
        ret = result;

    pkgmgrinfo_pkginfo_destroy_pkginfo(handle);

    return ret;
}

/**
 * @brief Timer to start initial Helper ISE if the active (selected) 3rd party keyboard is uninstalled.
 *
 * @param data User data
 *
 * @return If it returns ECORE_CALLBACK_RENEW, it will be called again at the next tick, or if it returns
 * ECORE_CALLBACK_CANCEL it will be deleted automatically making any references/handles for it invalid.
 */
static Eina_Bool _start_default_helper_timer(void *data)
{
    std::vector<String> total_appids;
    std::vector<ImeInfoDB>::iterator it;
    VectorPairStringUint32::iterator iter;

    /* Let panel know that ise is deleted... */
    for (it = _ime_info.begin(); it != _ime_info.end(); it++) {
        total_appids.push_back(it->appid);
    }
    if (total_appids.size() > 0)
        _panel_agent->update_ise_list (total_appids);

    LOGD("Try to start the initial helper");
    set_active_ise(_initial_ise_uuid, true);

    for (iter = g_pkgids_to_be_uninstalled.begin (); iter != g_pkgids_to_be_uninstalled.end (); iter++) {
        if (iter->first.compare(g_stopped_helper_pkgid) == 0) {
            g_pkgids_to_be_uninstalled.erase (iter);
            break;
        }
    }
    g_stopped_helper_pkgid = "";

    g_start_default_helper_timer = NULL;
    return ECORE_CALLBACK_CANCEL;
}

/**
 * @brief Timer to release uninstalled IME related info; g_pkgids_to_be_uninstalled has appid and is_enabled.
 *
 * @param data User data
 *
 * @return If it returns ECORE_CALLBACK_RENEW, it will be called again at the next tick, or if it returns
 * ECORE_CALLBACK_CANCEL it will be deleted automatically making any references/handles for it invalid.
 */
static Eina_Bool _release_uninstalled_pkginfo_timer(void *data)
{
    g_pkgids_to_be_uninstalled.clear ();
    g_release_uninstalled_ime_info_timer = NULL;
    return ECORE_CALLBACK_CANCEL;
}

/**
 * @brief Called when the package is installed, uninstalled or updated, and the progress of the request to the package manager changes.
 * @since_tizen 2.3
 * @param[in] type The type of the package to be installed, uninstalled or updated
 * @param[in] package The name of the package to be installed, uninstalled or updated
 * @param[in] event_type The type of the request to the package manager
 * @param[in] event_state The current state of the request to the package manager
 * @param[in] progress The progress for the request that is being processed by the package manager \n
 *            The range of progress is from 0 to 100
 * @param[in] error The error code when the package manager failed to process the request
 * @param[in] user_data The user data passed from package_manager_set_event_cb()
 * @see package_manager_set_event_cb()
 * @see package_manager_unset_event_cb()

INFO: Package install/update/uninstall scenario
Install and Uninstall are obviously simple.
   Install: just INSTALL
   Uninstall: just UNINSTALL
Update package (change the source codes in IME project and Run As again), there are four scenarios:
1. UPDATE
   Source code change
2. UNINSTALL -> INSTALL
   This happens when Tizen IDE Property > Tizen SDK > Rapid Development Support > Check "Enable Project specific settings"
   and change Application ID in tizen-manifest.xml file and Run As.
3. UPDATE -> INSTALL
   This happens when Tizen IDE Property > Tizen SDK > Rapid Development Support > Uncheck "Enable Project specific settings"
   and change Application ID in tizen-manifest.xml file and Run As.
   At UPDATE event, pkgid (package parameter) is invalid...
4. UPDATE
   Exceptionally, only UPDATE can be called when Application ID in tizen-manifest.xml file is changed.
   At UPDATE event, pkgid (package parameter) is valid, and only appid is changed; the previous appid is invalid.

If multiple packages (including non-IME pkgs) are uninstalled and installed; Z300H UPS (ultra power saving) mode scenario.
For example, A and B packages are uninstalled and installed, the package manager works in this order: A UNINSTALL -> B UNINSTALL -> A INSTALL -> B INSTALL

Assuming IMEngine won't be changed through this. IMEngine might have multiple appids for one pkgid.
Assuming preinstalled IME won't be changed through this.
 */
static void _package_manager_event_cb (const char *type, const char *package, package_manager_event_type_e event_type, package_manager_event_state_e event_state, int progress, package_manager_error_e error, void *user_data)
{
    String current_ime_appid; // now appid is uuid.
    std::vector<String> appids;
    std::vector<String> total_appids;
    std::vector<ImeInfoDB>::iterator it;
    std::vector<String>::iterator it2;
    VectorPairStringUint32::iterator it3;
    int ret = 0;

    if (!package || !type)
        return;

    if (event_type == PACKAGE_MANAGER_EVENT_TYPE_UPDATE) {
        if (event_state == PACKAGE_MANAGER_EVENT_STATE_COMPLETED) {
            LOGD ("type=%s package=%s event_type=UPDATE event_state=COMPLETED progress=%d error=%d", type, package, progress, error);

            ret = _isf_insert_ime_info_by_pkgid (package);   // If package is not IME, -1 would be returned.
            if (ret == 1) { // In case the package is updated with the changed appid. In this case, there will be two IMEs
                ret = isf_db_select_appids_by_pkgid (package, appids);
                if (ret > 1) {
                    if (_ime_info.size () > 0 && _ime_info [get_ise_index (appids.front ())].is_enabled)
                        isf_db_update_is_enabled_by_appid(appids.back ().c_str (), true);
                    isf_db_delete_ime_info_by_appid (appids.front ().c_str ());
                }

                _update_ime_info ();

                /* Let panel know that ise list is changed... */
                for (it = _ime_info.begin(); it != _ime_info.end(); it++) {
                    total_appids.push_back(it->appid);
                }
                if (total_appids.size() > 0)
                    _panel_agent->update_ise_list (total_appids);

                if (ret > 1 && _soft_keyboard_launched) { // If the previous appid of pkgid is the current IME, restart it with new appid.
                    current_ime_appid = scim_global_config_read(String(SCIM_GLOBAL_CONFIG_DEFAULT_ISE_UUID), _initial_ise_uuid);
                    if (current_ime_appid.compare (appids.front ()) == 0) {
                        LOGD("Stop IME(%s)", current_ime_appid.c_str ());
                        _panel_agent->hide_helper (current_ime_appid);
                        _panel_agent->stop_helper (current_ime_appid);
                        LOGD("Start IME(%s)", appids.back ().c_str ());
                        scim_global_config_write (String (SCIM_GLOBAL_CONFIG_DEFAULT_ISE_UUID), appids.back ());
                        set_keyboard_engine (String (SCIM_COMPOSE_KEY_FACTORY_UUID));
                        _panel_agent->start_helper (appids.back ());
                    }
                }
            }
            else if (ret == 2) {  // In case IME package is just updated...
                _update_ime_info ();

                if (_soft_keyboard_launched) { // If package is the current IME, restart it.
                    current_ime_appid = scim_global_config_read(String(SCIM_GLOBAL_CONFIG_DEFAULT_ISE_UUID), _initial_ise_uuid);
                    if (isf_db_select_appids_by_pkgid(package, appids)) {
                        if (std::find(appids.begin(), appids.end(), current_ime_appid) != appids.end()) { // If the current ISE package is updated, restart it.
                            for (it = _ime_info.begin (); it != _ime_info.end (); it++) {
                                if (it->mode == TOOLBAR_HELPER_MODE && it->appid.compare(current_ime_appid) == 0) { // Make sure it's Helper ISE...
                                    LOGD("Restart IME(%s)", current_ime_appid.c_str ());
                                    _panel_agent->hide_helper (current_ime_appid);
                                    _panel_agent->stop_helper (current_ime_appid);
                                    _panel_agent->start_helper (current_ime_appid);
                                    break;
                                }
                            }
                        }
                    }
                }
            }
            else if (ret == 0) {    // For example, this happens if appid is changed in IME project and Run As again. Assuming only Helper (3rd party) might be updated and there is one appid per each pkgid...
                if (isf_db_select_appids_by_pkgid(package, appids) == 1) {
                    for (it = _ime_info.begin (); it != _ime_info.end (); it++) {
                        if (it->pkgid.compare(package) == 0) {
                            g_pkgids_to_be_updated_and_installed.push_back (std::make_pair (it->pkgid, it->is_enabled));
                            break;
                        }
                    }
                    if (it == _ime_info.end ()) // Probably not going to happen.
                        g_pkgids_to_be_updated_and_installed.push_back(std::make_pair (String(package), 0));

                    current_ime_appid = scim_global_config_read(String(SCIM_GLOBAL_CONFIG_DEFAULT_ISE_UUID), _initial_ise_uuid);
                    if (_soft_keyboard_launched && std::find(appids.begin(), appids.end(), current_ime_appid) != appids.end()) { // If the updated IME is the current ISE...
                        for (it = _ime_info.begin (); it != _ime_info.end (); it++) {
                            if (it->appid.compare(current_ime_appid) == 0 && it->mode == TOOLBAR_HELPER_MODE) { // Make sure it's Helper ISE...
                                LOGD("Stop IME(%s)", current_ime_appid.c_str ());
                                _panel_agent->hide_helper (current_ime_appid);
                                _panel_agent->stop_helper (current_ime_appid);
                                _soft_keyboard_launched = false;
                                g_updated_helper_pkgid = package;
                                break;
                            }
                        }
                    }

                    if (appids.size () > 0) // Probably appids size is 1.
                        LOGD("Delete IME(%s)", appids[0].c_str ());
                    if (isf_db_delete_ime_info_by_pkgid(package)) { // Delete package from ime_info db.
                        _update_ime_info();

                       /* Let panel know that ise is deleted... */
                       for (it = _ime_info.begin(); it != _ime_info.end(); it++) {
                           total_appids.push_back(it->appid);
                       }
                       if (total_appids.size() > 0)
                           _panel_agent->update_ise_list (total_appids);
                    }
                }
                else {
                    LOGW("isf_db_select_appids_by_pkgid returned %d.", ret);
                }
            }
        }
    }
    else if (event_type == PACKAGE_MANAGER_EVENT_TYPE_INSTALL) {
        if (event_state == PACKAGE_MANAGER_EVENT_STATE_COMPLETED) {
            LOGD ("type=%s package=%s event_type=INSTALL event_state=COMPLETED progress=%d error=%d", type, package, progress, error);

            ///////////////// UNINSTALL -> INSTALL and if the uninstalled IME is reinstalled /////////////////
            if (g_stopped_helper_pkgid.compare(package) == 0 && g_start_default_helper_timer) {
                LOGD("Cancel timer to start the default IME");
                ecore_timer_del(g_start_default_helper_timer);
                g_start_default_helper_timer = NULL;
                g_stopped_helper_pkgid = "";

                ret = _isf_insert_ime_info_by_pkgid(package);
                if (ret > 0) {
                    /* Find appid by pkgid. There might be multiple appid, but assume Helper always has one appid.
                       And appid can be changed, but pkgid won't be changed. */
                    ret = isf_db_select_appids_by_pkgid(package, appids);
                    if (ret == 1 && appids.size () == 1) {
                        for (it3 = g_pkgids_to_be_uninstalled.begin (); it3 != g_pkgids_to_be_uninstalled.end (); it3++) {
                            if (it3->first.compare(package) == 0) {
                                if (it3->second)
                                    isf_db_update_is_enabled_by_appid(appids[0].c_str (), (bool)it3->second);

                                _update_ime_info();

                                /* Let panel know that ise is added... */
                                for (it = _ime_info.begin(); it != _ime_info.end(); it++) {
                                    total_appids.push_back(it->appid);
                                }
                                if (total_appids.size() > 0)
                                    _panel_agent->update_ise_list (total_appids);

                                LOGD("Restart IME(%s)", appids[0].c_str ());
                                scim_global_config_write (String (SCIM_GLOBAL_CONFIG_DEFAULT_ISE_UUID), appids[0]);
                                set_keyboard_engine (String (SCIM_COMPOSE_KEY_FACTORY_UUID));
                                _panel_agent->start_helper (appids[0]);
                                _soft_keyboard_launched = true;

                                g_pkgids_to_be_uninstalled.erase (it3);
                                break;
                            }
                        }
                    }
                    else {
                        LOGW("isf_db_select_appids_by_pkgid returned %d.", ret);

                        _update_ime_info();

                        /* Let panel know that ise is added... */
                        for (it = _ime_info.begin(); it != _ime_info.end(); it++) {
                            total_appids.push_back(it->appid);
                        }
                        if (total_appids.size() > 0)
                            _panel_agent->update_ise_list (total_appids);
                    }
                }
                else {
                    LOGW("_isf_insert_ime_info_by_pkgid returned %d.", ret);
                }
            }
            else {  // If new package is installed...
                ret = _isf_insert_ime_info_by_pkgid(package);   // If package is not IME, -1 would be returned.
                if (ret > 0) { // In case package is IME...
                    ///////////////// INSTALL /////////////////
                    _update_ime_info();

                    /* Let panel know that ise is added... */
                    for (it = _ime_info.begin(); it != _ime_info.end(); it++) {
                        total_appids.push_back(it->appid);
                    }
                    if (total_appids.size() > 0)
                       _panel_agent->update_ise_list (total_appids);
                    ///////////////// END /////////////////

                    /* For example, the following happens if appid is changed in IME project and Run As again. The appid would be changed this time.
                       Assuming only Helper (3rd party) might be installed after update or uninstall and there is one appid per each pkgid...*/

                    ///////////////// UPDATE -> INSTALL /////////////////
                    for (it3 = g_pkgids_to_be_updated_and_installed.begin (); it3 != g_pkgids_to_be_updated_and_installed.end (); it3++) {
                        if (it3->first.compare(package) == 0) {
                            if (it3->second) {
                                for (it = _ime_info.begin(); it != _ime_info.end(); it++) {
                                    if (it->pkgid.compare(package) == 0) {
                                        it->is_enabled = it3->second;
                                        isf_db_update_is_enabled_by_appid(it->appid.c_str (), (bool)it->is_enabled);
                                        break;
                                    }
                                }
                            }
                            g_pkgids_to_be_updated_and_installed.erase (it3);
                            break;
                        }
                    }
                    if (g_updated_helper_pkgid.compare(package) == 0) {
                        for (it = _ime_info.begin(); it != _ime_info.end(); it++) {
                            if (it->mode == TOOLBAR_HELPER_MODE && it->pkgid.compare(package) == 0) {
                                LOGD("Start IME(%s)", it->appid.c_str ());
                                scim_global_config_write (String (SCIM_GLOBAL_CONFIG_DEFAULT_ISE_UUID), it->appid);
                                set_keyboard_engine (String (SCIM_COMPOSE_KEY_FACTORY_UUID));
                                _panel_agent->start_helper (it->appid);
                                _soft_keyboard_launched = true;
                                break;
                            }
                        }
                        g_updated_helper_pkgid = "";
                        return;
                    }
                    ///////////////// END /////////////////

                    ///////////////// UNINSTALL -> INSTALL /////////////////
                    for (it3 = g_pkgids_to_be_uninstalled.begin (); it3 != g_pkgids_to_be_uninstalled.end (); it3++) {
                        if (it3->first.compare(package) == 0) {
                            if (it3->second) {
                                for (it = _ime_info.begin(); it != _ime_info.end(); it++) {
                                    if (it->pkgid.compare(package) == 0) {
                                        it->is_enabled = it3->second;
                                        isf_db_update_is_enabled_by_appid(it->appid.c_str (), (bool)it->is_enabled);
                                        break;
                                    }
                                }
                            }
                            g_pkgids_to_be_uninstalled.erase (it3);
                            break;
                        }
                    }
                    ///////////////// END /////////////////
                }
                else if (ret == 0) {
                    LOGW("_isf_insert_ime_info_by_pkgid returned %d.", ret);
                }
            }
        }
    }
    else if (event_type == PACKAGE_MANAGER_EVENT_TYPE_UNINSTALL) {
        switch (event_state) {
            case PACKAGE_MANAGER_EVENT_STATE_STARTED:
                LOGD ("type=%s package=%s event_type=UNINSTALL event_state=STARTED progress=%d error=%d", type, package, progress, error);
                {
                    // Need to check if there is "http://tizen.org/category/ime" category; it can be done by comparing pkgid with ime_info db.
                    int imeCnt = 0;
                    if (_ime_info.size() == 0)
                        _update_ime_info();

                    for (it = _ime_info.begin (); it != _ime_info.end (); it++) {
                        if (it->pkgid.compare(package) == 0 && it->is_preinstalled == 0) {   // Ignore if it's preinstalled IME and IMEngine.
                            imeCnt++;
                            break;
                        }
                    }

                    if (imeCnt > 0) {
                        // There might be more than one appid for one pkgid, but let's assume Helper always has one appid per a pkgid. Stop Helper ISE, but not delete it from ime_info db.
                        LOGD("%s for pkgid(\"%s\") is about to be deleted", it->appid.c_str (), package);
                        g_pkgids_to_be_uninstalled.push_back(std::make_pair (String(package), it->is_enabled));

                        if (_soft_keyboard_launched && isf_db_select_appids_by_pkgid(package, appids)) {
                            current_ime_appid = scim_global_config_read(String(SCIM_GLOBAL_CONFIG_DEFAULT_ISE_UUID), _initial_ise_uuid);
                            if (std::find(appids.begin(), appids.end(), current_ime_appid) != appids.end()) { // If the uninstalled IME is the current ISE... appids size is probably 1.
                                for (it = _ime_info.begin (); it != _ime_info.end (); it++) {
                                    if (it->appid.compare(current_ime_appid) == 0 && it->mode == TOOLBAR_HELPER_MODE) { // Make sure it's Helper ISE...
                                        LOGD("Stop IME(%s)", current_ime_appid.c_str ());
                                        _panel_agent->hide_helper (current_ime_appid);
                                        _panel_agent->stop_helper (current_ime_appid);
                                        _soft_keyboard_launched = false;
                                        g_stopped_helper_pkgid = package;
                                        break;
                                    }
                                }
                            }
                        }
                    }
                }
                break;

            case PACKAGE_MANAGER_EVENT_STATE_COMPLETED:
                LOGD ("type=%s package=%s event_type=UNINSTALL event_state=COMPLETED progress=%d error=%d", type, package, progress, error);

                for (it3 = g_pkgids_to_be_uninstalled.begin (); it3 != g_pkgids_to_be_uninstalled.end (); it3++) {
                    if (it3->first.compare(package) == 0) {
                        if (isf_db_delete_ime_info_by_pkgid(package)) { // Delete package from ime_info db.
                            _update_ime_info();
                        }

                        if (g_stopped_helper_pkgid.compare(package) == 0) { // If the uninstalled ISE is the current ISE, start the initial helper ISE by timer.
                            if (g_start_default_helper_timer)
                                ecore_timer_del(g_start_default_helper_timer);
                            LOGD("Add timer to start the default IME");
                            g_start_default_helper_timer = ecore_timer_add(3.0, _start_default_helper_timer, NULL);
                        }
                        else {  // Need to clean up g_pkgids_to_be_uninstalled info unless the same package is installed again; e.g., UNINSTALL -> INSTALL case.
                            if (g_release_uninstalled_ime_info_timer)
                                ecore_timer_del(g_release_uninstalled_ime_info_timer);
                            LOGD("Add timer to release uninstalled IME pkg info");
                            g_release_uninstalled_ime_info_timer = ecore_timer_add(7.0, _release_uninstalled_pkginfo_timer, NULL);
                        }

                        /* Let panel know that ise is deleted... */
                        for (it = _ime_info.begin(); it != _ime_info.end(); it++) {
                            total_appids.push_back(it->appid);
                        }
                        if (total_appids.size() > 0)
                            _panel_agent->update_ise_list (total_appids);

                        break;
                    }
                }
                break;

            case PACKAGE_MANAGER_EVENT_STATE_FAILED:
                LOGD ("type=%s package=%s event_type=UNINSTALL event_state=FAILED progress=%d error=%d", type, package, progress, error);

                for (it3 = g_pkgids_to_be_uninstalled.begin (); it3 != g_pkgids_to_be_uninstalled.end (); it3++) {
                    if (it3->first.compare(package) == 0) {
                        // Update _ime_info for sure...
                        _update_ime_info();

                        if (g_stopped_helper_pkgid.compare(package) == 0) {
                            ret = isf_db_select_appids_by_pkgid(package, appids);
                            if (ret == 1 && appids.size () == 1) {
                                for (it = _ime_info.begin (); it != _ime_info.end (); it++) {
                                    if (it->appid.compare(appids[0]) == 0 && it->mode == TOOLBAR_HELPER_MODE) { // Make sure it's Helper ISE...
                                        LOGD("Restart IME(%s)", appids[0].c_str ());
                                        set_keyboard_engine (String (SCIM_COMPOSE_KEY_FACTORY_UUID));
                                        _panel_agent->start_helper (appids[0]);
                                        _soft_keyboard_launched = true;
                                        break;
                                    }
                                }
                            }
                            else {
                                LOGW("isf_db_select_appids_by_pkgid returned %d.", ret);
                            }
                            g_stopped_helper_pkgid = "";
                        }

                        g_pkgids_to_be_uninstalled.erase (it3);
                        break;
                    }
                }
                break;

            default:
                break;
        }
    }
}
#endif

/**
 * @brief Set keyboard ISE.
 *
 * @param uuid The keyboard ISE's uuid.
 *
 * @return false if keyboard ISE change is failed, otherwise return true.
 */
static bool set_keyboard_ise (const String &uuid)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "...\n";

    TOOLBAR_MODE_T mode = _panel_agent->get_current_toolbar_mode ();

    if (TOOLBAR_HELPER_MODE == mode) {
        String pre_uuid = _panel_agent->get_current_helper_uuid ();
        _panel_agent->hide_helper (pre_uuid);
        _panel_agent->stop_helper (pre_uuid);
        _soft_keyboard_launched = false;
    } else if (TOOLBAR_KEYBOARD_MODE == mode) {
        uint32 kbd_option = 0;
        String kbd_uuid, kbd_name;
        isf_get_keyboard_ise (_config, kbd_uuid, kbd_name, kbd_option);
        if (kbd_uuid == uuid)
            return false;
    }

    _panel_agent->change_factory (uuid);

    String language = String ("~other");/*scim_get_locale_language (scim_get_current_locale ());*/
    _config->write (String (SCIM_CONFIG_DEFAULT_IMENGINE_FACTORY) + String ("/") + language, uuid);

    return true;
}

/**
 * @brief Set helper ISE.
 *
 * @param uuid The helper ISE's uuid.
 * @param launch_ise The flag for launching helper ISE.
 *
 * @return false if helper ISE change is failed, otherwise return true.
 */
static bool set_helper_ise (const String &uuid, bool launch_ise)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "...\n";

    TOOLBAR_MODE_T mode = _panel_agent->get_current_toolbar_mode ();
    String pre_uuid = _panel_agent->get_current_helper_uuid ();
    LOGD("pre_appid=%s, appid=%s, launch_ise=%d, %d", pre_uuid.c_str(), uuid.c_str(), launch_ise, _soft_keyboard_launched);
    if (pre_uuid == uuid && _soft_keyboard_launched)
        return false;

    if (TOOLBAR_HELPER_MODE == mode && pre_uuid.length () > 0 && _soft_keyboard_launched) {
        _panel_agent->hide_helper (pre_uuid);
        _panel_agent->stop_helper (pre_uuid);
        _soft_keyboard_launched = false;
        LOGD ("stop helper : %s", pre_uuid.c_str ());
    }

    if (launch_ise) {
        LOGD ("Start helper (%s)", uuid.c_str ());

        set_keyboard_engine (String (SCIM_COMPOSE_KEY_FACTORY_UUID));
        if (_panel_agent->start_helper (uuid))
            _soft_keyboard_launched = true;
    }
    _config->write (String (SCIM_CONFIG_DEFAULT_HELPER_ISE), uuid);

    return true;
}

/**
 * @brief Set active ISE.
 *
 * @param uuid The ISE's uuid.
 * @param launch_ise The flag for launching helper ISE.
 *
 * @return false if ISE change is failed, otherwise return true.
 */
static bool set_active_ise (const String &uuid, bool launch_ise)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "...\n";
    LOGD ("set ISE (%s) %d", uuid.c_str(), launch_ise);

    if (uuid.length () <= 0)
        return false;

    bool ise_changed = false, valid = false;

    for (unsigned int i = 0; i < _ime_info.size (); i++) {
        if (!uuid.compare (_ime_info[i].appid)) {
            if (TOOLBAR_KEYBOARD_MODE == _ime_info[i].mode)
                ise_changed = set_keyboard_ise (_ime_info[i].appid);
            else if (TOOLBAR_HELPER_MODE == _ime_info[i].mode) {
                if (_ime_info[i].is_enabled) {
                    /* If IME so is deleted somehow, main() in scim_helper_launcher.cpp will return -1.
                       Checking HelperModule validity seems necessary here. */
                    HelperModule helper_module (_ime_info[i].module_name);
                    if (helper_module.valid ())
                        valid = true;
                    helper_module.unload ();

                    if (valid && _ime_info[i].pkgtype.compare ("tpk") == 0) {
                        int ret = smack_have_access(_ime_info[i].pkgid.c_str (), "isf::ime", "rw");
                        if (ret != 1) {
                            LOGW("smack_have_access for \"isf::ime\" failed (%d)", ret);
                            valid = false;
                        }
                        else {
                            ret = smack_have_access(_ime_info[i].pkgid.c_str (), "isf", "rwx");
                            if (ret != 1) {
                                LOGW("smack_have_access for \"isf\" failed (%d)", ret);
                                valid = false;
                            }
                        }
                    }

                    if (valid)
                        ise_changed = set_helper_ise (_ime_info[i].appid, launch_ise);
                    else
                        LOGW("Helper ISE(appid=\"%s\") is not valid.", _ime_info[i].appid.c_str ());
                }
                else {
                    LOGW("Helper ISE(appid=\"%s\") is not enabled.", _ime_info[i].appid.c_str ());
                }
            }
            _panel_agent->set_current_toolbar_mode (_ime_info[i].mode);
            if (ise_changed) {
                _panel_agent->set_current_helper_option (_ime_info[i].options);
                _panel_agent->set_current_ise_name (_ime_info[i].label);
                _ise_width  = 0;
                _ise_height = 0;
                _ise_state  = WINDOW_STATE_HIDE;
                _candidate_mode      = SOFT_CANDIDATE_WINDOW;
                _candidate_port_line = ONE_LINE_CANDIDATE;
                _soft_candidate_width = 0;
                _soft_candidate_height = 0;
                if (_candidate_window)
                    ui_create_candidate_window ();

                scim_global_config_write (String (SCIM_GLOBAL_CONFIG_DEFAULT_ISE_UUID), _ime_info[i].appid);
                scim_global_config_flush ();

                _config->flush ();
                _config->reload ();
                _panel_agent->reload_config ();

                vconf_set_str (VCONFKEY_ISF_ACTIVE_KEYBOARD_UUID, uuid.c_str ());
            }

            return ise_changed;
        }
    }

    return false;
}

/**
 * @brief Load ISF configuration and ISEs information.
 */
static void load_config (void)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "...\n";

    /* Read configurations. */
    if (!_config.null ()) {
        bool shared_ise = _config->read (String (SCIM_CONFIG_FRONTEND_SHARED_INPUT_METHOD), false);
        _panel_agent->set_should_shared_ise (shared_ise);
    }
    _launch_ise_on_request = scim_global_config_read (String (SCIM_GLOBAL_CONFIG_LAUNCH_ISE_ON_REQUEST), _launch_ise_on_request);

    isf_load_ise_information (ALL_ISE, _config);
}

/**
 * @brief Reload config callback function for ISF panel.
 *
 * @param config The config pointer.
 */
static void config_reload_cb (const ConfigPointer &config)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "...\n";

    /* load_config (); */
}

//////////////////////////////////////////////////////////////////////
// Start of Candidate Functions
//////////////////////////////////////////////////////////////////////
/**
 * @brief Get candidate window valid height for autoscroll.
 *
 * @return The valid height.
 */
static int ui_candidate_get_valid_height (void)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "\n";

    int height = 0;
    int angle = 0;

    if (_candidate_window) {
        if (_candidate_state == WINDOW_STATE_SHOW)
            angle = _candidate_angle;
        else
            angle = efl_get_app_window_angle ();

        if (_aux_area_visible && _candidate_area_1_visible) {
            if (angle == 90 || angle == 270)
                height = _candidate_land_height_min_2;
            else
                height = _candidate_port_height_min_2;
        } else {
            if (angle == 90 || angle == 270)
                height = _candidate_land_height_min;
            else
                height = _candidate_port_height_min;
        }
    }
    return height;
}

/**
 * @brief Resize candidate window size.
 *
 * @param new_width  New width for candidate window.
 * @param new_height New height for candidate window.
 */
static void ui_candidate_window_resize (int new_width, int new_height)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << " width:" << new_width << " height:" << new_height << "\n";

    if (!_candidate_window)
        return;

    int height;

    LOGD ("%s (w: %d, h: %d)\n", __func__, new_width, new_height);
    evas_object_resize (_aux_line, new_width, 2);
    _candidate_width  = new_width;
    _candidate_height = new_height;
    if (_candidate_state == WINDOW_STATE_SHOW)
        _panel_agent->update_candidate_panel_event ((uint32)ECORE_IMF_CANDIDATE_PANEL_GEOMETRY_EVENT, 0);

    if (_candidate_state == WINDOW_STATE_SHOW && _candidate_mode == FIXED_CANDIDATE_WINDOW) {
        height = ui_candidate_get_valid_height ();
        if ((_ise_width == 0 && _ise_height == 0) ||
            (_ise_height > 0 && _candidate_valid_height != height) ||
            (_ise_height > 0 && (_candidate_height - height) > _ise_height) ||
            ((_candidate_angle == 90 || _candidate_angle == 270) && (_ise_width < _screen_height)) ||
            ((_candidate_angle == 0  || _candidate_angle == 180) && (_ise_width > _screen_width ))) {

            set_keyboard_geometry_atom_info (_app_window, get_ise_geometry ());
            _panel_agent->update_input_panel_event (ECORE_IMF_INPUT_PANEL_GEOMETRY_EVENT, 0);
        }
    }

    /* Get height for portrait and landscape */
    int port_width  = _candidate_port_width;
    int port_height = _candidate_port_height_min;
    int land_width  = _candidate_land_width;
    int land_height = _candidate_land_height_min;
    if (_candidate_angle == 90 || _candidate_angle == 270) {
        land_height = new_height;
        if (land_height == _candidate_land_height_min_2) {
            port_height = _candidate_port_height_min_2;
        } else if (land_height == _candidate_land_height_max) {
            port_height = _candidate_port_height_max;
        } else if (land_height == _candidate_land_height_max_2) {
            port_height = _candidate_port_height_max_2;
        }
    } else {
        port_height = new_height;
        if (port_height == _candidate_port_height_min_2) {
            land_height = _candidate_land_height_min_2;
        } else if (port_height == _candidate_port_height_max) {
            land_height = _candidate_land_height_max;
        } else if (port_height == _candidate_port_height_max_2) {
            land_height = _candidate_land_height_max_2;
        }
    }

    LOGD ("ecore_x_e_window_rotation_geometry_set (_candidate_window), port (%d, %d), land (%d, %d)\n",
            port_width, port_height, land_width, land_height);

    ecore_x_e_window_rotation_geometry_set (elm_win_xwindow_get (_candidate_window),
            0, 0, 0, port_width, port_height);
    ecore_x_e_window_rotation_geometry_set (elm_win_xwindow_get (_candidate_window),
            90, 0, 0, land_height, land_width);
    ecore_x_e_window_rotation_geometry_set (elm_win_xwindow_get (_candidate_window),
            180, 0, 0, port_width, port_height);
    ecore_x_e_window_rotation_geometry_set (elm_win_xwindow_get (_candidate_window),
            270, 0, 0, land_height, land_width);
}

/**
 * @brief This function will show/hide widgets of candidate window,
 *        and resize candidate window size according to aux_area/candidate_area.
 */
static void ui_candidate_window_adjust (void)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "...\n";
    if (!_candidate_window)
        return;

    int x, y, width, height;

    /* Get candidate window size */
    if (_candidate_angle == 90 || _candidate_angle == 270) {
        ecore_x_e_window_rotation_geometry_get (elm_win_xwindow_get (_candidate_window), _candidate_angle,
                &x, &y, &height, &width);
    } else {
        ecore_x_e_window_rotation_geometry_get (elm_win_xwindow_get (_candidate_window), _candidate_angle,
                &x, &y, &width, &height);
    }

    if (_aux_area_visible && _candidate_area_2_visible) {
        evas_object_show (_aux_line);
        evas_object_move (_candidate_area_1, _candidate_area_1_pos[0], _candidate_area_1_pos[1] + _candidate_port_height_min_2 - _candidate_port_height_min);
        if (_candidate_angle == 90 || _candidate_angle == 270) {
            if (_candidate_mode == FIXED_CANDIDATE_WINDOW && _ise_state == WINDOW_STATE_SHOW &&
                _ise_height > _candidate_land_height_max_2 - _candidate_land_height_min_2)
                ui_candidate_window_resize (width, _candidate_land_height_min_2 + _ise_height);
            else
                ui_candidate_window_resize (width, _candidate_land_height_max_2);
            evas_object_move (_close_btn, _close_btn_pos[2], _close_btn_pos[3] + _candidate_port_height_min_2 - _candidate_port_height_min);
            evas_object_move (_candidate_area_2, 0, _candidate_land_height_min_2);
            evas_object_move (_scroller_bg, 0, _candidate_land_height_min_2);
            evas_object_resize (_candidate_bg, width, _candidate_land_height_min_2);
        } else {
            if (_candidate_mode == FIXED_CANDIDATE_WINDOW && _ise_state == WINDOW_STATE_SHOW &&
                _ise_height > _candidate_port_height_max_2 - _candidate_port_height_min_2)
                ui_candidate_window_resize (width, _candidate_port_height_min_2 + _ise_height);
            else
                ui_candidate_window_resize (width, _candidate_port_height_max_2);
            evas_object_move (_close_btn, _close_btn_pos[0], _close_btn_pos[1] + _candidate_port_height_min_2 - _candidate_port_height_min);
            evas_object_move (_candidate_area_2, 0, _candidate_port_height_min_2);
            evas_object_move (_scroller_bg, 0, _candidate_port_height_min_2);
            evas_object_resize (_candidate_bg, width, _candidate_port_height_min_2);
        }
    } else if (_aux_area_visible && _candidate_area_1_visible) {
        evas_object_show (_aux_line);
        evas_object_move (_candidate_area_1, _candidate_area_1_pos[0], _candidate_area_1_pos[1] + _candidate_port_height_min_2 - _candidate_port_height_min);
        if (_candidate_angle == 90 || _candidate_angle == 270) {
            ui_candidate_window_resize (width, _candidate_land_height_min_2);
            evas_object_move (_more_btn, _more_btn_pos[2], _more_btn_pos[3] + _candidate_port_height_min_2 - _candidate_port_height_min);
            evas_object_resize (_candidate_bg, width, _candidate_land_height_min_2);
        } else {
            ui_candidate_window_resize (width, _candidate_port_height_min_2);
            evas_object_move (_more_btn, _more_btn_pos[0], _more_btn_pos[1] + _candidate_port_height_min_2 - _candidate_port_height_min);
            evas_object_resize (_candidate_bg, width, _candidate_port_height_min_2);
        }
    } else if (_aux_area_visible) {
        evas_object_hide (_aux_line);
        ui_candidate_window_resize (width, _aux_height + 2);
        evas_object_resize (_candidate_bg, width, _aux_height + 2);
    } else if (_candidate_area_2_visible) {
        evas_object_hide (_aux_line);
        evas_object_move (_candidate_area_1, _candidate_area_1_pos[0], _candidate_area_1_pos[1]);
        if (_candidate_angle == 90 || _candidate_angle == 270) {
            if (_candidate_mode == FIXED_CANDIDATE_WINDOW && _ise_state == WINDOW_STATE_SHOW &&
                _ise_height > _candidate_land_height_max - _candidate_land_height_min)
                ui_candidate_window_resize (width, _candidate_land_height_min + _ise_height);
            else
                ui_candidate_window_resize (width, _candidate_land_height_max);
            evas_object_move (_close_btn, _close_btn_pos[2], _close_btn_pos[3]);
            evas_object_move (_candidate_area_2, 0, _candidate_land_height_min);
            evas_object_move (_scroller_bg, 0, _candidate_land_height_min);
            evas_object_resize (_candidate_bg, width, _candidate_land_height_min);
        } else {
            if (_candidate_mode == FIXED_CANDIDATE_WINDOW && _ise_state == WINDOW_STATE_SHOW &&
                _ise_height > _candidate_port_height_max - _candidate_port_height_min)
                ui_candidate_window_resize (width, _candidate_port_height_min + _ise_height);
            else
                ui_candidate_window_resize (width, _candidate_port_height_max);
            evas_object_move (_close_btn, _close_btn_pos[0], _close_btn_pos[1]);
            evas_object_move (_candidate_area_2, 0, _candidate_port_height_min);
            evas_object_move (_scroller_bg, 0, _candidate_port_height_min);
            evas_object_resize (_candidate_bg, width, _candidate_port_height_min);
        }
    } else {
        evas_object_hide (_aux_line);
        evas_object_move (_candidate_area_1, _candidate_area_1_pos[0], _candidate_area_1_pos[1]);
        if (_candidate_angle == 90 || _candidate_angle == 270) {
            ui_candidate_window_resize (width, _candidate_land_height_min);
            evas_object_move (_more_btn, _more_btn_pos[2], _more_btn_pos[3]);
            evas_object_resize (_candidate_bg, width, _candidate_land_height_min);
        } else {
            ui_candidate_window_resize (width, _candidate_port_height_min);
            evas_object_move (_more_btn, _more_btn_pos[0], _more_btn_pos[1]);
            evas_object_resize (_candidate_bg, width, _candidate_port_height_min);
        }
    }
}

/**
 * @brief Rotate candidate window.
 *
 * @param angle The angle of candidate window.
 */
static void ui_candidate_window_rotate (int angle)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "...\n";
    if (!_candidate_window)
        return;

    ui_candidate_window_close_button_cb (NULL, NULL, NULL, NULL);

    if (angle == 90 || angle == 270) {
        _candidate_scroll_width = _candidate_scroll_width_max;
        ui_candidate_window_resize (_candidate_land_width, _candidate_land_height_min);
        evas_object_resize (_aux_area, _aux_land_width, _aux_height);
        evas_object_resize (_candidate_area_1, _candidate_scroll_0_width_max, _item_min_height);
        evas_object_resize (_candidate_area_2, _candidate_scroll_width, _candidate_scroll_height_min);
        evas_object_resize (_scroller_bg, _candidate_scroll_width, _candidate_scroll_height_min + 6);
    } else {
        _candidate_scroll_width = _candidate_scroll_width_min;
        ui_candidate_window_resize (_candidate_port_width, _candidate_port_height_min);
        evas_object_resize (_aux_area, _aux_port_width, _aux_height);
        evas_object_resize (_candidate_area_1, _candidate_scroll_0_width_min, (_item_min_height+2)*_candidate_port_line-2);
        evas_object_resize (_candidate_area_2, _candidate_scroll_width, _candidate_scroll_height_max);
        evas_object_resize (_scroller_bg, _candidate_scroll_width, _candidate_scroll_height_max + 6);
    }

    evas_object_hide (_candidate_area_2);
    _candidate_area_2_visible = false;
    ui_candidate_window_adjust ();
    if (_candidate_area_1_visible) {
        update_table (ISF_CANDIDATE_TABLE, g_isf_candidate_table);
        _candidate_tts_focus_index = INVALID_TTS_FOCUS_INDEX;
        ui_tts_focus_rect_hide ();
    }
    flush_memory ();
}

/**
 * @brief This function is used to judge whether candidate window should be hidden.
 *
 * @return true if candidate window should be hidden, otherwise return false.
 */
static bool ui_candidate_can_be_hide (void)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "...\n";

    if (_aux_area_visible || _candidate_area_1_visible || _candidate_area_2_visible)
        return false;
    else
        return true;
}

/**
 * @brief Delete check candidate window size timer.
 *
 * @return void
 */
static void ui_candidate_delete_check_size_timer (void)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "...\n";

    if (_check_size_timer != NULL) {
        ecore_timer_del (_check_size_timer);
        _check_size_timer = NULL;
    }
}

/**
 * @brief Callback function for check candidate window size timer.
 *
 * @param data Data to pass when it is called.
 *
 * @return ECORE_CALLBACK_CANCEL
 */
static Eina_Bool ui_candidate_check_size_timeout (void *data)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "...\n";

    ui_candidate_delete_check_size_timer ();
    ui_candidate_window_resize (_candidate_width, _candidate_height);
    ui_settle_candidate_window ();
    return ECORE_CALLBACK_CANCEL;
}

/**
 * @brief Delete longpress timer.
 *
 * @return void
 */
static void ui_candidate_delete_longpress_timer (void)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "...\n";

    if (_longpress_timer != NULL) {
        ecore_timer_del (_longpress_timer);
        _longpress_timer = NULL;
    }
}

/**
 * @brief Callback function for candidate longpress timer.
 *
 * @param data Data to pass when it is called.
 *
 * @return ECORE_CALLBACK_CANCEL
 */
static Eina_Bool ui_candidate_longpress_timeout (void *data)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "...\n";

    int index = (int)GPOINTER_TO_INT (data);
    ui_candidate_delete_longpress_timer ();
    _is_click = false;
    _panel_agent->send_longpress_event (_click_object, index);
    return ECORE_CALLBACK_CANCEL;
}

/**
 * @brief Delete destroy timer.
 *
 * @return void
 */
static void ui_candidate_delete_destroy_timer (void)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "...\n";

    if (_destroy_timer != NULL) {
        ecore_timer_del (_destroy_timer);
        _destroy_timer = NULL;
    }
}

/**
 * @brief Callback function for destroy timer.
 *
 * @param data Data to pass when it is called.
 *
 * @return ECORE_CALLBACK_CANCEL
 */
static Eina_Bool ui_candidate_destroy_timeout (void *data)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "...\n";

    ui_candidate_delete_destroy_timer ();
    ui_destroy_candidate_window ();
    return ECORE_CALLBACK_CANCEL;
}

/**
 * @brief Callback function for off_prepare_done.
 *
 * @param data Data to pass when it is called.
 *
 * @return ECORE_CALLBACK_CANCEL
 */
static Eina_Bool off_prepare_done_timeout (void *data)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "...\n";

    /* WMSYNC, #8 Let the Window Manager to actually hide keyboard window */
    // WILL_HIDE_REQUEST_DONE Ack to WM
    Ecore_X_Window root_window = ecore_x_window_root_get (_control_window);
    ecore_x_e_virtual_keyboard_off_prepare_done_send (root_window, _control_window);
    LOGD ("_ecore_x_e_virtual_keyboard_off_prepare_done_send (%x, %x)\n",
            root_window, _control_window);

    _off_prepare_done_timer = NULL;

    return ECORE_CALLBACK_CANCEL;
}

/**
 * @brief Delete candidate hide timer.
 *
 * @return void
 */
static void delete_candidate_hide_timer (void)
{
    LOGD ("deleting candidate_hide_timer");
    if (_candidate_hide_timer) {
        ecore_timer_del (_candidate_hide_timer);
        _candidate_hide_timer = NULL;
    }
}

static void candidate_window_hide (void)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "\n";

    delete_candidate_hide_timer ();
    _candidate_state = WINDOW_STATE_HIDE;

    LOGD ("evas_object_hide (_candidate_window, %p)\n", elm_win_xwindow_get (_candidate_window));

    if (_candidate_window) {
        /* There are cases that when there are rapid ISE_HIDE and ISE_SHOW requests,
           candidate window should be displayed but STATE_OFF for the first ISE_HIDE
           calls this function, so when the candidate window is shown by the following
           STATE_ON message, a blank area is displayed in candidate window -
           so we let the _candidate_area_1 as the default area that would be displayed */
        //evas_object_hide (_candidate_area_1);
        //evas_object_hide (_more_btn);
        _candidate_area_1_visible = false;

        evas_object_hide (_candidate_window);
        SCIM_DEBUG_MAIN (3) << "    Hide candidate window\n";
    }
}

/**
 * @brief Callback function for candidate hide timer
 *
 * @param data Data to pass when it is called.
 *
 * @return ECORE_CALLBACK_CANCEL
 */
static Eina_Bool candidate_hide_timer (void *data)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "...\n";

    LOGD ("calling candidate_window_hide ()");
    candidate_window_hide ();

    return ECORE_CALLBACK_CANCEL;
}

/**
 * @brief Delete candidate show handler.
 *
 * @return void
 */
static void delete_candidate_show_handler (void)
{
    if (_candidate_show_handler) {
        ecore_event_handler_del (_candidate_show_handler);
        _candidate_show_handler = NULL;
    }
}

/**
 * @brief Callback function for window show completion event
 *
 * @param data Data to pass when it is called.
 *
 * @return ECORE_CALLBACK_CANCEL
 */

static Eina_Bool x_event_window_show_cb (void *data, int ev_type, void *event)
{
    delete_candidate_show_handler ();

    Ecore_X_Event_Window_Show *e = (Ecore_X_Event_Window_Show*)event;
    if (_candidate_state == WINDOW_STATE_WILL_SHOW) {
        if (e->win == elm_win_xwindow_get (_candidate_window)) {
            LOGD ("Candidate window show callback");

            /* If our candidate window is in WILL_SHOW state and this show callback was called,
               now we are finally displayed on to the screen */
            _candidate_state = WINDOW_STATE_SHOW;

            /* Update the geometry information for auto scrolling */
            set_keyboard_geometry_atom_info (_app_window, get_ise_geometry ());
            _panel_agent->update_input_panel_event (ECORE_IMF_INPUT_PANEL_GEOMETRY_EVENT, 0);
            _panel_agent->update_input_panel_event (ECORE_IMF_CANDIDATE_PANEL_GEOMETRY_EVENT, 0);

            /* And the state event */
            _panel_agent->update_candidate_panel_event ((uint32)ECORE_IMF_CANDIDATE_PANEL_STATE_EVENT, (uint32)ECORE_IMF_CANDIDATE_PANEL_SHOW);

            /* If we are in hardware keyboard mode, this candidate window is now considered to be a input panel */
            if (_candidate_mode == FIXED_CANDIDATE_WINDOW) {
                if (_panel_agent->get_current_toolbar_mode () == TOOLBAR_KEYBOARD_MODE) {
                    _panel_agent->update_input_panel_event ((uint32)ECORE_IMF_INPUT_PANEL_STATE_EVENT, (uint32)ECORE_IMF_INPUT_PANEL_STATE_SHOW);
                }
            }
        }
    } else {
        if (e->win == elm_win_xwindow_get (_candidate_window)) {
            LOGD ("Candidate window show callback, but _candidate_state is %d", _candidate_state);
        }
    }

    return ECORE_CALLBACK_CANCEL;
}

/**
 * @brief Show candidate window.
 *
 * @param bSetVirtualKbd The flag for set_keyboard_geometry_atom_info () calling.
 */
static void ui_candidate_show (bool bSetVirtualKbd)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "...\n";

    delete_candidate_hide_timer ();

    if (!_candidate_window) return;
    if (_candidate_mode == SOFT_CANDIDATE_WINDOW)
        return;

    /* FIXME : SHOULD UNIFY THE METHOD FOR CHECKING THE HW KEYBOARD EXISTENCE */
    /* If the ISE is not visible currently, wait for the ISE to be opened and then show our candidate window */
    _candidate_show_requested = true;
    if ((_panel_agent->get_current_toolbar_mode () == TOOLBAR_HELPER_MODE) && (_ise_state != WINDOW_STATE_SHOW)) {
        LOGD ("setting _show_candidate_requested to TRUE");
        return;
    }

    /* If the ISE angle is valid, respect the value to make sure
       the candidate window always have the same angle with ISE */
    if (_ise_angle != -1) {
        _candidate_angle = _ise_angle;
    }
    ui_candidate_window_rotate (_candidate_angle);

    /* If the candidate window was about to hide, turn it back to SHOW state now */
    if (_candidate_state == WINDOW_STATE_WILL_HIDE) {
        _candidate_state = WINDOW_STATE_SHOW;
    }

    /* Change to WILL_SHOW state only when we are not currently in SHOW state */
    if (_candidate_state != WINDOW_STATE_SHOW) {
        _candidate_state = WINDOW_STATE_WILL_SHOW;
    }

    if (_panel_agent->get_current_toolbar_mode () == TOOLBAR_KEYBOARD_MODE) {
        /* WMSYNC, #3 Clear the existing application's conformant area and set transient_for */
        // Unset conformant area
        Ecore_X_Window current_app_window = efl_get_app_window ();
        if (_app_window != current_app_window) {
            struct rectinfo info = {0, 0, 0, 0};
            info.pos_y = _screen_width > _screen_height ? _screen_width : _screen_height;
            set_keyboard_geometry_atom_info (_app_window, info);
            LOGD ("Conformant reset for window %x\n", _app_window);
            _app_window = current_app_window;
        }
    }
    if (_candidate_mode == FIXED_CANDIDATE_WINDOW) {
        if (bSetVirtualKbd) {
            set_keyboard_geometry_atom_info (_app_window, get_ise_geometry ());
        }
    }

    ui_candidate_delete_check_size_timer ();
    _check_size_timer = ecore_timer_add (0.02, ui_candidate_check_size_timeout, NULL);

    SCIM_DEBUG_MAIN (3) << "    Show candidate window\n";
    efl_set_transient_for_app_window (elm_win_xwindow_get (_candidate_window));
    if (_ise_state == WINDOW_STATE_SHOW) {
        edje_object_file_set (_more_btn, _candidate_edje_file.c_str (), "more_button");
        edje_object_file_set (_close_btn, _candidate_edje_file.c_str (), "close_button");
    } else {
        edje_object_file_set (_more_btn, _candidate_edje_file.c_str (), "close_button");
        edje_object_file_set (_close_btn, _candidate_edje_file.c_str (), "more_button");
    }

    /* If we are in hardware keyboard mode, this candidate window is now considered to be a input panel */
    if (_candidate_mode == FIXED_CANDIDATE_WINDOW) {
        if (_panel_agent->get_current_toolbar_mode () == TOOLBAR_KEYBOARD_MODE) {
            LOGD ("sending ECORE_IMF_INPUT_PANEL_STATE_WILL_SHOW");
            _panel_agent->update_input_panel_event ((uint32)ECORE_IMF_INPUT_PANEL_STATE_EVENT, (uint32)ECORE_IMF_INPUT_PANEL_STATE_WILL_SHOW);
        }
    }

    if (_candidate_state != WINDOW_STATE_SHOW) {
        if (_candidate_show_handler) {
            LOGD ("Was still waiting for CANDIDATE_WINDOW_SHOW....");
        } else {
            delete_candidate_show_handler ();
            LOGD ("Registering ECORE_X_EVENT_WINDOW_SHOW event, %d", _candidate_state);
            _candidate_show_handler = ecore_event_handler_add (ECORE_X_EVENT_WINDOW_SHOW, x_event_window_show_cb, NULL);
        }
    } else {
        LOGD ("The candidate window was already in SHOW state, update geometry information");
        _panel_agent->update_input_panel_event (ECORE_IMF_INPUT_PANEL_GEOMETRY_EVENT, 0);
        _panel_agent->update_input_panel_event (ECORE_IMF_CANDIDATE_PANEL_GEOMETRY_EVENT, 0);

        /* And the state event */
        _panel_agent->update_candidate_panel_event ((uint32)ECORE_IMF_CANDIDATE_PANEL_STATE_EVENT, (uint32)ECORE_IMF_CANDIDATE_PANEL_SHOW);

        /* If we are in hardware keyboard mode, this candidate window is now considered to be a input panel */
        if (_candidate_mode == FIXED_CANDIDATE_WINDOW) {
            if (_panel_agent->get_current_toolbar_mode () == TOOLBAR_KEYBOARD_MODE) {
                _panel_agent->update_input_panel_event ((uint32)ECORE_IMF_INPUT_PANEL_STATE_EVENT, (uint32)ECORE_IMF_INPUT_PANEL_STATE_SHOW);
            }
        }
    }

    evas_object_show (_candidate_window);
}

/**
 * @brief Hide candidate window.
 *
 * @param bForce The flag to hide candidate window by force.
 * @param bSetVirtualKbd The flag for set_keyboard_geometry_atom_info () calling.
 */
static void ui_candidate_hide (bool bForce, bool bSetVirtualKbd, bool will_hide)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << " bForce:" << bForce << " bSetVirtualKbd:" << bSetVirtualKbd << " will_hide:" << will_hide << "...\n";

    if (!_candidate_window)
        return;

    if (bForce) {
        if (_candidate_area_2 && _candidate_area_2_visible) {
            evas_object_hide (_candidate_area_2);
            _candidate_area_2_visible = false;
            evas_object_hide (_scroller_bg);
            evas_object_hide (_close_btn);
            _panel_agent->candidate_more_window_hide ();
            ui_candidate_window_adjust ();
        }
    }

    if (bForce || ui_candidate_can_be_hide ()) {
        if (will_hide) {
            LOGD ("candidate_state = WILL_HIDE");
            _candidate_state = WINDOW_STATE_WILL_HIDE;

            delete_candidate_hide_timer ();
            _candidate_hide_timer = ecore_timer_add (2.0, candidate_hide_timer, NULL);
        }

        if (_candidate_mode == FIXED_CANDIDATE_WINDOW) {
            _panel_agent->update_input_panel_event (ECORE_IMF_INPUT_PANEL_GEOMETRY_EVENT, 0);
            /* FIXME : should check if bSetVirtualKbd flag is really needed in this case */
            if (_ise_state == WINDOW_STATE_SHOW) {
                set_keyboard_geometry_atom_info (_app_window, get_ise_geometry ());
            } else {
                if (bSetVirtualKbd) {
                    set_keyboard_geometry_atom_info (_app_window, get_ise_geometry ());
                }
            }
            if (_panel_agent->get_current_toolbar_mode () == TOOLBAR_KEYBOARD_MODE) {
                _panel_agent->update_input_panel_event
                    ((uint32)ECORE_IMF_INPUT_PANEL_STATE_EVENT, (uint32)ECORE_IMF_INPUT_PANEL_STATE_HIDE);
            }
        }

        /* Update the new keyboard geometry first, and then send the candidate hide event */
        _panel_agent->update_candidate_panel_event ((uint32)ECORE_IMF_CANDIDATE_PANEL_STATE_EVENT, (uint32)ECORE_IMF_CANDIDATE_PANEL_HIDE);

        if (!will_hide) {
            /* If we are not in will_hide state, hide the candidate window immediately */
            candidate_window_hide ();
            evas_object_hide (_preedit_window);
        }
    }
}

/**
 * @brief Callback function for more button.
 *
 * @param data Data to pass when it is called.
 * @param e The evas for current event.
 * @param button The evas object for current event.
 * @param event_info The information for current event.
 */
static void ui_candidate_window_more_button_cb (void *data, Evas *e, Evas_Object *button, void *event_info)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "...\n";

    _panel_agent->candidate_more_window_show ();

    if (candidate_expanded == false) {
        candidate_expanded = true;
        int number = SCIM_LOOKUP_TABLE_MAX_PAGESIZE;
        for (int i = 0; i < SCIM_LOOKUP_TABLE_MAX_PAGESIZE; ++i) {
            if (_candidate_0 [i] == NULL) {
                number = i;
                break;
            }
        }
        if (g_isf_candidate_table.get_current_page_size () != number)
            update_table (ISF_CANDIDATE_TABLE, g_isf_candidate_table);
    }

    if (_candidate_angle == 180) {
        Ecore_Evas *ee = ecore_evas_ecore_evas_get (evas_object_evas_get (_candidate_window));
        ecore_evas_move_resize (ee, 0, 0, 0, 0);
        LOGD ("ecore_evas_move_resize (%p, %d, %d, %d, %d)", ee, 0, 0, 0, 0);
    } else if (_candidate_mode == FIXED_CANDIDATE_WINDOW && _candidate_angle == 270) {
        /*
         * when screen rotate 270 degrees, candidate have to move then resize for expanding more
         * candidates, but it will flash or locate in a wrong position, this code just a workaround
         * for avoiding this situation.
         */
        Ecore_Evas *ee = ecore_evas_ecore_evas_get (evas_object_evas_get (_candidate_window));
        ecore_evas_move_resize (ee, 0, 0, _screen_height, ui_candidate_get_valid_height () + _ise_height);
        LOGD ("ecore_evas_move_resize (%p, %d, %d, %d, %d)",
            ee, 0, 0, _screen_height, ui_candidate_get_valid_height () + _ise_height);
    }

    evas_object_show (_candidate_area_2);
    _candidate_area_2_visible = true;
    evas_object_show (_scroller_bg);
    evas_object_hide (_more_btn);
    evas_object_show (_close_btn);

    ui_candidate_window_adjust ();
    ui_settle_candidate_window ();
    flush_memory ();
}

/**
 * @brief Callback function for close button.
 *
 * @param data Data to pass when it is called.
 * @param e The evas for current event.
 * @param button The evas object for current event.
 * @param event_info The information for current event.
 */
static void ui_candidate_window_close_button_cb (void *data, Evas *e, Evas_Object *button, void *event_info)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "...\n";

    if (_candidate_area_2 == NULL || !_candidate_area_2_visible)
        return;

    _panel_agent->candidate_more_window_hide ();

    evas_object_hide (_candidate_area_2);
    _candidate_area_2_visible = false;
    evas_object_hide (_scroller_bg);
    evas_object_hide (_close_btn);

    candidate_expanded = false;
    evas_object_show (_candidate_area_1);
    _candidate_area_1_visible = true;
    evas_object_show (_more_btn);

    elm_scroller_region_show (_candidate_area_2, 0, 0, _candidate_scroll_width, 100);
    if (_candidate_angle == 180) {
        Ecore_Evas *ee= ecore_evas_ecore_evas_get (evas_object_evas_get (_candidate_window));
        ecore_evas_move_resize (ee, 0, 0, 0, 0);
        LOGD ("ecore_evas_move_resize (%p, %d, %d, %d, %d)", ee, 0, 0, 0, 0);
    } else if (_candidate_mode == FIXED_CANDIDATE_WINDOW && _candidate_angle == 270) {
        /*
         * when screen rotate 270 degrees, candidate have to move then resize for expanding more
         * candidates, but it will flash or locate in a wrong position, this code just a workaround
         * for avoiding this situation.
         */
        Ecore_Evas *ee = ecore_evas_ecore_evas_get (evas_object_evas_get (_candidate_window));
        ecore_evas_move_resize (ee, _ise_height, 0, _screen_height, ui_candidate_get_valid_height ());
        LOGD ("ecore_evas_move_resize (%p, %d, %d, %d, %d)",
            ee, _ise_height, 0, _screen_height, ui_candidate_get_valid_height ());
    }

    ui_candidate_window_adjust ();
    ui_settle_candidate_window ();
    flush_memory ();
}

/**
 * @brief Callback function for mouse button press.
 *
 * @param data Data to pass when it is called.
 * @param e The evas for current event.
 * @param button The evas object for current event.
 * @param event_info The information for current event.
 */
static void ui_mouse_button_pressed_cb (void *data, Evas *e, Evas_Object *button, void *event_info)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "...\n";

    _click_object = GPOINTER_TO_INT (data) & 0xFF;
    _is_click     = true;

    Evas_Event_Mouse_Down *ev = (Evas_Event_Mouse_Down *)event_info;
    if (!ev) return;

    _click_down_pos [0] = ev->canvas.x;
    _click_down_pos [1] = ev->canvas.y;

    if (_click_object == ISF_EFL_CANDIDATE_0 || _click_object == ISF_EFL_CANDIDATE_ITEMS) {
        int index = GPOINTER_TO_INT (data) >> 8;

#if HAVE_FEEDBACK
        if (feedback_initialized) {
            int feedback_result = 0;
            bool sound_feedback = _config->read (SCIM_GLOBAL_CONFIG_PANEL_SOUND_FEEDBACK, false);

            if (sound_feedback) {
                feedback_result = feedback_play_type (FEEDBACK_TYPE_SOUND, FEEDBACK_PATTERN_SIP);

                if (FEEDBACK_ERROR_NONE == feedback_result)
                    LOGD ("Sound play successful");
                else
                    LOGW ("Cannot play feedback sound : %d", feedback_result);
            }

            bool vibrate_feedback = _config->read (SCIM_GLOBAL_CONFIG_PANEL_VIBRATION_FEEDBACK, false);

            if (vibrate_feedback) {
                feedback_result = feedback_play_type (FEEDBACK_TYPE_VIBRATION, FEEDBACK_PATTERN_SIP);

                if (FEEDBACK_ERROR_NONE == feedback_result)
                    LOGD ("Vibration play successful");
                else
                    LOGW ("Cannot play feedback vibration : %d", feedback_result);
            }
        }
#endif

        ui_candidate_delete_longpress_timer ();
        _longpress_timer = ecore_timer_add (1.0, ui_candidate_longpress_timeout, (void *)index);
    }
}

/**
 * @brief Callback function for mouse button release.
 *
 * @param data Data to pass when it is called.
 * @param e The evas for current event.
 * @param button The evas object for current event.
 * @param event_info The information for current event.
 */
static void ui_mouse_button_released_cb (void *data, Evas *e, Evas_Object *button, void *event_info)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << " index:" << GPOINTER_TO_INT (data) << "...\n";

    ui_candidate_delete_longpress_timer ();

    int index = GPOINTER_TO_INT (data);
    if (_click_object == ISF_EFL_AUX && _is_click) {
/*      double ret = 0;
        const char *buf = edje_object_part_state_get (button, "aux", &ret);
        if (strcmp ("selected", buf)) {
            for (unsigned int i = 0; i < _aux_items.size (); i++) {
                buf = edje_object_part_state_get (_aux_items [i], "aux", &ret);
                if (!strcmp ("selected", buf))
                    edje_object_signal_emit (_aux_items [i], "aux,state,unselected", "aux");
            }
            edje_object_signal_emit (button, "aux,state,selected", "aux");
            _panel_agent->select_aux (index);
        }*/
        int r, g, b, a, r2, g2, b2, a2, r3, g3, b3, a3;
        edje_object_color_class_get (_aux_items [index], "text_color", &r, &g, &b, &a, &r2, &g2, &b2, &a2, &r3, &g3, &b3, &a3);
        // Normal item is clicked
        if (!(r == 62 && g == 207 && b == 255)) {
            for (unsigned int i = 0; i < _aux_items.size (); i++) {
                edje_object_color_class_set (_aux_items [i], "text_color", 249, 249, 249, 255, r2, g2, b2, a2, r3, g3, b3, a3);
            }
            edje_object_color_class_set (_aux_items [index], "text_color", 62, 207, 255, 255, r2, g2, b2, a2, r3, g3, b3, a3);
            _panel_agent->select_aux (index);
        }
    } else if (_click_object == ISF_EFL_CANDIDATE_0 && _is_click) {
        ui_candidate_window_close_button_cb (NULL, NULL, _close_btn, NULL);
        _panel_agent->select_candidate (index);
    } else if (_click_object == ISF_EFL_CANDIDATE_ITEMS && _is_click) {
        ui_candidate_window_close_button_cb (NULL, NULL, _close_btn, NULL);
        _panel_agent->select_candidate (index);
    }
}

/**
 * @brief Callback function for mouse move.
 *
 * @param data Data to pass when it is called.
 * @param e The evas for current event.
 * @param button The evas object for current event.
 * @param event_info The information for current event.
 */
static void ui_mouse_moved_cb (void *data, Evas *e, Evas_Object *button, void *event_info)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "...\n";

    Evas_Event_Mouse_Down *ev = (Evas_Event_Mouse_Down *)event_info;
    if (!ev) return;

    _click_up_pos [0] = ev->canvas.x;
    _click_up_pos [1] = ev->canvas.y;

    if (abs (_click_up_pos [0] - _click_down_pos [0]) >= (int)(15 * _height_rate) ||
        abs (_click_up_pos [1] - _click_down_pos [1]) >= (int)(15 * _height_rate)) {
        _is_click = false;
        ui_candidate_delete_longpress_timer ();
    }
}

/**
 * @brief Open TTS device.
 *
 * @return false if open is failed, otherwise return true.
 */
static bool ui_open_tts (void)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "...\n";

#if HAVE_TTS
    int r = tts_create (&_tts);
    if (TTS_ERROR_NONE != r) {
        LOGW ("tts_create FAILED : result (%d)\n", r);
        _tts = NULL;
        return false;
    }

    r = tts_set_mode (_tts, TTS_MODE_SCREEN_READER);
    if (TTS_ERROR_NONE != r) {
        LOGW ("tts_set_mode FAILED : result (%d)\n", r);
    }

    tts_state_e current_state;
    r = tts_get_state (_tts, &current_state);
    if (TTS_ERROR_NONE != r) {
        LOGW ("tts_get_state FAILED : result (%d)\n", r);
    }

    if (TTS_STATE_CREATED == current_state)  {
        r = tts_prepare (_tts);
        if (TTS_ERROR_NONE != r) {
            LOGW ("tts_prepare FAILED : ret (%d)\n", r);
        }
    }
    return true;
#else
    return false;
#endif
}

/**
 * @brief Close TTS device.
 */
static void ui_close_tts (void)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "...\n";

#if HAVE_TTS
    if (_tts) {
        int r = tts_unprepare (_tts);
        if (TTS_ERROR_NONE != r) {
            LOGW ("tts_unprepare FAILED : result (%d)\n", r);
        }

        r = tts_destroy (_tts);
        if (TTS_ERROR_NONE != r) {
            LOGW ("tts_destroy FAILED : result (%d)\n", r);
        }
    }
#endif
}

/**
 * @brief Play string by TTS.
 *
 * @param str The string for playing.
 */
static void ui_play_tts (const char* str)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << " str=" << str << "\n";

    if (!str) return;

#if HAVE_TTS
    if (_tts == NULL) {
        if (!ui_open_tts ())
            return;
    }

    int r;
    int utt_id = 0;
    tts_state_e current_state;

    r = tts_get_state (_tts, &current_state);
    if (TTS_ERROR_NONE != r) {
        LOGW ("Fail to get state from TTS : ret (%d)\n", r);
    }

    if (TTS_STATE_PLAYING == current_state)  {
        r = tts_stop (_tts);
        if (TTS_ERROR_NONE != r) {
            LOGW ("Fail to stop TTS : ret (%d)\n", r);
        }
    }

    /* Get ISE language */
    String default_uuid = scim_global_config_read (String (SCIM_GLOBAL_CONFIG_DEFAULT_ISE_UUID), String (""));
    String language = String ("en_US");
    if (default_uuid.length () > 0) {
        language = _ime_info[get_ise_index (default_uuid)].languages;
        if (language.length () > 0) {
            std::vector<String> ise_langs;
            scim_split_string_list (ise_langs, language);
            language = ise_langs[0];
        }
    }
    LOGD ("TTS language:%s, str:%s\n", language.c_str (), str);

    r = tts_add_text (_tts, str, language.c_str (), TTS_VOICE_TYPE_AUTO, TTS_SPEED_AUTO, &utt_id);
    if (TTS_ERROR_NONE == r) {
        r = tts_play (_tts);
        if (TTS_ERROR_NONE != r) {
            LOGW ("Fail to play TTS : ret (%d)\n", r);
        }
    }
#endif
}

/**
 * @brief Show rect for candidate focus object when screen reader is enabled.
 *
 * @param x Rect X position.
 * @param y Rect Y position.
 * @param w Rect width.
 * @param h Rect height.
 */
static void ui_tts_focus_rect_show (int x, int y, int w, int h)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "...\n";
    if (_candidate_window == NULL || _candidate_state != WINDOW_STATE_SHOW)
        return;

    if (_tts_focus_rect == NULL) {
        _tts_focus_rect = evas_object_rectangle_add (evas_object_evas_get ((Evas_Object*)_candidate_window));
        evas_object_color_set (_tts_focus_rect, 0, 0, 0, 0);
        elm_access_highlight_set (elm_access_object_register (_tts_focus_rect, (Evas_Object*)_candidate_window));
    }
    evas_object_move (_tts_focus_rect, x, y);
    evas_object_resize (_tts_focus_rect, w, h);
    evas_object_raise (_tts_focus_rect);
    evas_object_show (_tts_focus_rect);
}

/**
 * @brief Hide rect for candidate focus object when screen reader is enabled.
 */
static void ui_tts_focus_rect_hide (void)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "...\n";

    if (_tts_focus_rect) {
        //evas_object_hide (_tts_focus_rect);
        evas_object_move (_tts_focus_rect, -1000, -1000);
    }
}

/**
 * @brief Callback function for candidate scroller stop event.
 *
 * @param data Data to pass when it is called.
 * @param obj The evas object for current event.
 * @param event_info The information for current event.
 */
static void ui_candidate_scroller_stop_cb (void *data, Evas_Object *obj, void *event_info)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "...\n";
    if (!_wait_stop_event)
        return;

    if (_candidate_tts_focus_index >= 0 && _candidate_tts_focus_index < g_isf_candidate_table.get_current_page_size ()) {
        if (_candidate_0 [_candidate_tts_focus_index]) {
            int x, y, w, h;
            evas_object_geometry_get (_candidate_0 [_candidate_tts_focus_index], &x, &y, &w, &h);
            ui_tts_focus_rect_show (x, y, w, h);
        }
    }
    _wait_stop_event = false;
}

/**
 * @brief Mouse over (find focus object and play text by TTS) when screen reader is enabled.
 *
 * @param mouse_x Mouse X position.
 * @param mouse_y Mouse Y position.
 */
static void ui_mouse_over (int mouse_x, int mouse_y)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "...\n";
    if (_candidate_window == NULL || _candidate_state != WINDOW_STATE_SHOW)
        return;

    int x, y, width, height;
    for (int i = 0; i < SCIM_LOOKUP_TABLE_MAX_PAGESIZE; ++i) {
        if (_candidate_0 [i]) {
            evas_object_geometry_get (_candidate_0 [i], &x, &y, &width, &height);
            if (mouse_x >= x && mouse_x <= x + width && mouse_y >= y && mouse_y <= y + height) {
                /* FIXME: Should consider emoji case */
                String mbs = utf8_wcstombs (g_isf_candidate_table.get_candidate_in_current_page (i));
                SCIM_DEBUG_MAIN (3) << __FUNCTION__ << " play candidate string: " << mbs << "\n";
                ui_play_tts (mbs.c_str ());
                _candidate_tts_focus_index = i;
                ui_tts_focus_rect_show (x, y, width, height);
                return;
            }
        }
    }

    String strTts = String ("");
    if (_candidate_area_2_visible) {
        evas_object_geometry_get (_close_btn, &x, &y, &width, &height);
        if (mouse_x >= x && mouse_x <= x + width && mouse_y >= y && mouse_y <= y + height) {
            strTts = String (_("close button"));
            _candidate_tts_focus_index = CLOSE_BUTTON_INDEX;
            ui_tts_focus_rect_show (x, y, width, height);
        }
    } else {
        evas_object_geometry_get (_more_btn, &x, &y, &width, &height);
        if (mouse_x >= x && mouse_x <= x + width && mouse_y >= y && mouse_y <= y + height) {
            strTts = String (_("more button"));
            _candidate_tts_focus_index = MORE_BUTTON_INDEX;
            ui_tts_focus_rect_show (x, y, width, height);
        }
    }
    if (strTts.length () > 0)
        ui_play_tts (strTts.c_str ());
}

/**
 * @brief Mouse click (find focus object and do click event) when screen reader is enabled.
 *
 * @param focus_index focused candidate index.
 */
static void ui_mouse_click (int focus_index)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "...\n";
    if (_candidate_window == NULL || _candidate_state != WINDOW_STATE_SHOW || focus_index == INVALID_TTS_FOCUS_INDEX)
        return;

    if (focus_index >= 0 && focus_index < g_isf_candidate_table.get_current_page_size ()) {
        if (_candidate_0 [focus_index]) {
            int x, y, width, height;
            evas_object_geometry_get (_candidate_0 [focus_index], &x, &y, &width, &height);
            Evas_Event_Mouse_Down event_info;
            event_info.canvas.x = x + width / 2;
            event_info.canvas.y = y + height / 2;
            ui_mouse_button_pressed_cb (GINT_TO_POINTER ((focus_index << 8) + ISF_EFL_CANDIDATE_0), NULL, NULL, &event_info);
            ui_mouse_button_released_cb (GINT_TO_POINTER (focus_index), NULL, NULL, &event_info);
        }
        _candidate_tts_focus_index = INVALID_TTS_FOCUS_INDEX;
        ui_tts_focus_rect_hide ();
        return;
    }

    if (_candidate_area_2_visible) {
        if (focus_index == CLOSE_BUTTON_INDEX) {
            ui_candidate_window_close_button_cb (NULL, NULL, NULL, NULL);
            _candidate_tts_focus_index = MORE_BUTTON_INDEX;
        }
    } else {
        if (focus_index == MORE_BUTTON_INDEX) {
            ui_candidate_window_more_button_cb (NULL, NULL, NULL, NULL);
            _candidate_tts_focus_index = CLOSE_BUTTON_INDEX;
        }
    }
}

/**
 * @brief Create preedit window.
 */
static void ui_create_preedit_window (void)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "...\n";

    _preedit_width  = 100;
    _preedit_height = _preedit_height * _height_rate;
    if (_preedit_window == NULL) {
        _preedit_window = efl_create_window ("preedit", "Preedit Window");
        evas_object_resize (_preedit_window, _preedit_width, _preedit_height);
        int rots [4] = {0, 90, 180, 270};
        elm_win_wm_rotation_available_rotations_set (_preedit_window, rots, 4);
        int preedit_font_size = (int)(32 * _width_rate);

        _preedit_text = edje_object_add (evas_object_evas_get (_preedit_window));
        edje_object_file_set (_preedit_text, _candidate_edje_file.c_str (), "preedit_text");
        evas_object_size_hint_fill_set (_preedit_text, EVAS_HINT_FILL, EVAS_HINT_FILL);
        evas_object_size_hint_weight_set (_preedit_text, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
        elm_win_resize_object_add (_preedit_window, _preedit_text);
        evas_object_show (_preedit_text);

        _tmp_preedit_text = evas_object_text_add (evas_object_evas_get (_preedit_window));
        evas_object_text_font_set (_tmp_preedit_text, _candidate_font_name.c_str (), preedit_font_size);
    }
}

/**
 * @brief Create native style candidate window.
 */
static void ui_create_native_candidate_window (void)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "...\n";
    _more_btn_width = 80 * (_width_rate > 1 ? 1 : _width_rate);
    _more_btn_height = 64 * _height_rate;

    _candidate_port_width        = _screen_width;
    _candidate_port_height_min   = 84 * _height_rate * _candidate_port_line;
    _candidate_port_height_min_2 = 84 * _height_rate + _candidate_port_height_min;
    _candidate_port_height_max   = 426 * _height_rate + _candidate_port_height_min;
    _candidate_port_height_max_2 = 84 * _height_rate + _candidate_port_height_max;
    _candidate_land_width        = _screen_height;
    _candidate_land_height_min   = 84 * _width_rate;
    _candidate_land_height_min_2 = 168 * _width_rate;
    _candidate_land_height_max   = 342 * _width_rate;
    _candidate_land_height_max_2 = 426 * _width_rate;

    _candidate_scroll_0_width_min= _screen_width;
    _candidate_scroll_0_width_max= _screen_height;
    _candidate_scroll_width_min  = _screen_width;
    _candidate_scroll_width_max  = _screen_height;
    _candidate_scroll_height_min = 252 * _width_rate;
    _candidate_scroll_height_max = 420 * _height_rate;

    _candidate_area_1_pos [0]    = 0 * _width_rate;
    _candidate_area_1_pos [1]    = 2 * _height_rate;
    _more_btn_pos [0]            = _candidate_port_width - _more_btn_width - _h_padding;
    _more_btn_pos [1]            = 12 * _height_rate;
    _more_btn_pos [2]            = _candidate_land_width - _more_btn_width - _h_padding;
    _more_btn_pos [3]            = 12 * _width_rate;
    _close_btn_pos [0]           = _candidate_port_width - _more_btn_width - _h_padding;
    _close_btn_pos [1]           = 12 * _height_rate;
    _close_btn_pos [2]           = _candidate_land_width - _more_btn_width - _h_padding;
    _close_btn_pos [3]           = 12 * _width_rate;

    _aux_height                  = 84 * _height_rate - 2;
    _aux_port_width              = _screen_width;
    _aux_land_width              = _screen_height;

    _item_min_height             = 84 * _height_rate - 2;

    /* Create candidate window */
    if (_candidate_window == NULL) {
        _candidate_window = efl_create_window ("candidate", "Prediction Window");
        int rots [4] = {0, 90, 180, 270};
        elm_win_wm_rotation_available_rotations_set (_candidate_window, rots, 4);
        if (_candidate_angle == 90 || _candidate_angle == 270) {
            _candidate_width  = _candidate_land_width;
            _candidate_height = _candidate_land_height_min;
        } else {
            _candidate_width  = _candidate_port_width;
            _candidate_height = _candidate_port_height_min;
        }
        ecore_x_e_window_rotation_geometry_set (elm_win_xwindow_get (_candidate_window),
                0, 0, 0, _candidate_port_width, _candidate_port_height_min);
        ecore_x_e_window_rotation_geometry_set (elm_win_xwindow_get (_candidate_window),
                90, 0, 0, _candidate_land_height_min, _candidate_land_width);
        ecore_x_e_window_rotation_geometry_set (elm_win_xwindow_get (_candidate_window),
                180, 0, 0, _candidate_port_width, _candidate_port_height_min);
        ecore_x_e_window_rotation_geometry_set (elm_win_xwindow_get (_candidate_window),
                270, 0, 0, _candidate_land_height_min, _candidate_land_width);

        /* Add dim background */
        Evas_Object *dim_bg = elm_bg_add (_candidate_window);
        evas_object_color_set (dim_bg, 0, 0, 0, 153);
        elm_win_resize_object_add (_candidate_window, dim_bg);
        evas_object_show (dim_bg);

        /* Add candidate background */
        _candidate_bg = edje_object_add (evas_object_evas_get (_candidate_window));
        edje_object_file_set (_candidate_bg, _candidate_edje_file.c_str (), "candidate_bg");
        evas_object_size_hint_weight_set (_candidate_bg, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
        evas_object_resize (_candidate_bg, _candidate_port_width, _candidate_port_height_min);
        evas_object_move (_candidate_bg, 0, 0);
        evas_object_show (_candidate_bg);

        /* Create _candidate_0 scroller */
        _candidate_0_scroll = elm_scroller_add (_candidate_window);
        elm_scroller_bounce_set (_candidate_0_scroll, EINA_TRUE, EINA_FALSE);
        elm_scroller_policy_set (_candidate_0_scroll, ELM_SCROLLER_POLICY_AUTO, ELM_SCROLLER_POLICY_OFF);
        evas_object_resize (_candidate_0_scroll, _candidate_scroll_0_width_min, (_item_min_height+2)*_candidate_port_line-2);
        evas_object_move (_candidate_0_scroll, _candidate_area_1_pos[0], _candidate_area_1_pos[1]);
        _candidate_0_table = elm_table_add (_candidate_window);
        evas_object_size_hint_weight_set (_candidate_0_table, 0.0, 0.0);
        evas_object_size_hint_align_set (_candidate_0_table, 0.0, 0.0);
        elm_table_padding_set (_candidate_0_table, 0, 0);
        elm_object_content_set (_candidate_0_scroll, _candidate_0_table);
        evas_object_show (_candidate_0_table);
        _candidate_area_1 = _candidate_0_scroll;

        /* Create more button */
        _more_btn = edje_object_add (evas_object_evas_get (_candidate_window));
        if (_ise_width == 0 && _ise_height == 0)
            edje_object_file_set (_more_btn, _candidate_edje_file.c_str (), "close_button");
        else
            edje_object_file_set (_more_btn, _candidate_edje_file.c_str (), "more_button");
        evas_object_move (_more_btn, _more_btn_pos[0], _more_btn_pos[1]);
        evas_object_resize (_more_btn, _more_btn_width, _more_btn_height);
        evas_object_event_callback_add (_more_btn, EVAS_CALLBACK_MOUSE_UP, ui_candidate_window_more_button_cb, NULL);

        /* Add scroller background */
        _candidate_scroll_width = _candidate_scroll_width_min;
        _scroller_bg = edje_object_add (evas_object_evas_get (_candidate_window));
        edje_object_file_set (_scroller_bg, _candidate_edje_file.c_str (), "scroller_bg");
        evas_object_size_hint_weight_set (_scroller_bg, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
        evas_object_resize (_scroller_bg, _candidate_scroll_width, _candidate_scroll_height_max + 6);
        evas_object_move (_scroller_bg, 0, _candidate_port_height_min);

        /* Create vertical scroller */
        _candidate_scroll = elm_scroller_add (_candidate_window);
        elm_scroller_bounce_set (_candidate_scroll, 0, 1);
        elm_scroller_policy_set (_candidate_scroll, ELM_SCROLLER_POLICY_OFF, ELM_SCROLLER_POLICY_AUTO);
        evas_object_resize (_candidate_scroll, _candidate_scroll_width, _candidate_scroll_height_max);
        evas_object_resize (_scroller_bg, _candidate_scroll_width, _candidate_scroll_height_max + 6);
        elm_scroller_page_size_set (_candidate_scroll, 0, _item_min_height+_v_padding);
        evas_object_move (_candidate_scroll, 0, _candidate_port_height_min);
        _candidate_table = elm_table_add (_candidate_window);
        evas_object_size_hint_weight_set (_candidate_table, 0.0, 0.0);
        evas_object_size_hint_align_set (_candidate_table, 0.0, 0.0);
        elm_table_padding_set (_candidate_table, 0, 0);
        elm_object_content_set (_candidate_scroll, _candidate_table);
        evas_object_show (_candidate_table);
        _candidate_area_2 = _candidate_scroll;
        evas_object_smart_callback_add (_candidate_scroll, "scroll,anim,stop", ui_candidate_scroller_stop_cb, NULL);

        /* Create close button */
        _close_btn = edje_object_add (evas_object_evas_get (_candidate_window));
        if (_ise_width == 0 && _ise_height == 0)
            edje_object_file_set (_close_btn, _candidate_edje_file.c_str (), "more_button");
        else
            edje_object_file_set (_close_btn, _candidate_edje_file.c_str (), "close_button");
        evas_object_move (_close_btn, _close_btn_pos[0], _close_btn_pos[1]);
        evas_object_resize (_close_btn, _more_btn_width, _more_btn_height);
        evas_object_event_callback_add (_close_btn, EVAS_CALLBACK_MOUSE_UP, ui_candidate_window_close_button_cb, NULL);

        _tmp_candidate_text = evas_object_text_add (evas_object_evas_get (_candidate_window));
        evas_object_text_font_set (_tmp_candidate_text, _candidate_font_name.c_str (), _candidate_font_size);

        /* Create aux */
        _aux_area = elm_scroller_add (_candidate_window);
        elm_scroller_bounce_set (_aux_area, 1, 0);
        elm_scroller_policy_set (_aux_area, ELM_SCROLLER_POLICY_AUTO, ELM_SCROLLER_POLICY_OFF);
        evas_object_resize (_aux_area, _aux_port_width, _aux_height);
        evas_object_move (_aux_area, _candidate_area_1_pos[0], _candidate_area_1_pos[1]);

        _aux_table = elm_table_add (_candidate_window);
        elm_object_content_set (_aux_area, _aux_table);
        elm_table_padding_set (_aux_table, 0, 0);
        evas_object_size_hint_weight_set (_aux_table, 0.0, 0.0);
        evas_object_size_hint_align_set (_aux_table, 0.0, 0.0);
        evas_object_show (_aux_table);

        _aux_line = edje_object_add (evas_object_evas_get (_candidate_window));
        edje_object_file_set (_aux_line, _candidate_edje_file.c_str (), "popup_line");
        evas_object_resize (_aux_line, _candidate_port_width, 2);
        evas_object_move (_aux_line, 0, _aux_height + 2);

        _tmp_aux_text = evas_object_text_add (evas_object_evas_get (_candidate_window));
        evas_object_text_font_set (_tmp_aux_text, _candidate_font_name.c_str (), _aux_font_size);
    } else {
        evas_object_hide (_candidate_window);
    }

    flush_memory ();
}

/**
 * @brief Create candidate window.
 *
 * @return void
 */
static void ui_create_candidate_window (void)
{
    check_time ("\nEnter ui_create_candidate_window");
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "...\n";
    if (_candidate_mode == SOFT_CANDIDATE_WINDOW)
        return;

    _candidate_x     = 0;
    _candidate_y     = 0;
    _candidate_angle = 0;

    ui_create_native_candidate_window ();

    unsigned int set = 1;
    ecore_x_window_prop_card32_set (elm_win_xwindow_get (_candidate_window),
            ECORE_X_ATOM_E_WINDOW_ROTATION_SUPPORTED,
            &set, 1);

    int angle = efl_get_app_window_angle ();
    if (_candidate_angle != angle) {
        _candidate_angle = angle;
        ui_candidate_window_rotate (angle);
    } else {
        ui_settle_candidate_window ();
    }

    candidate_expanded = false;

    check_time ("Exit ui_create_candidate_window");
}

/**
 * @brief Destroy candidate window.
 *
 * @return void
 */
static void ui_destroy_candidate_window (void)
{
    check_time ("Enter ui_destroy_candidate_window");
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "...\n";

    /* Delete candidate items, popup lines and seperator items */
    for (int i = 0; i < SCIM_LOOKUP_TABLE_MAX_PAGESIZE; ++i) {
        if (_candidate_0 [i]) {
            evas_object_del (_candidate_0 [i]);
            _candidate_0 [i] = NULL;
        }
        if (_seperate_0 [i]) {
            evas_object_del (_seperate_0 [i]);
            _seperate_0 [i] = NULL;
        }
        if (_seperate_items [i]) {
            evas_object_del (_seperate_items [i]);
            _seperate_items [i] = NULL;
        }
        if (_line_0 [i]) {
            evas_object_del (_line_0 [i]);
            _line_0 [i] = NULL;
        }
        if (_line_items [i]) {
            evas_object_del (_line_items [i]);
            _line_items [i] = NULL;
        }
    }

    _aux_items.clear ();
    _aux_seperates.clear ();
    /* Delete candidate window */
    if (_candidate_window) {
        LOGD ("calling ui_candidate_hide (true)");
        ui_candidate_hide (true);

        evas_object_del (_candidate_window);
        _candidate_window = NULL;
        _aux_area         = NULL;
        _candidate_area_1 = NULL;
        _candidate_area_2 = NULL;
        _scroller_bg      = NULL;
        _tts_focus_rect   = NULL;
    }

    if (_preedit_text) {
        evas_object_del (_preedit_text);
        _preedit_text = NULL;
    }

    if (_preedit_window) {
        evas_object_hide (_preedit_window);
        evas_object_del (_preedit_window);
        _preedit_window = NULL;
    }

    flush_memory ();
    check_time ("Exit ui_destroy_candidate_window");
}

/**
 * @brief Settle candidate window position.
 */
static void ui_settle_candidate_window (void)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "...\n";

    if (!_candidate_window)
        return;

    /* If both ISE and candidate window are going to be hidden,
       let's just not move our candidate window */
    if (_ise_state == WINDOW_STATE_WILL_HIDE && _candidate_state == WINDOW_STATE_WILL_HIDE)
        return;

    int spot_x, spot_y;
    int x, y, width, height;
    int pos_x = 0, pos_y = 0, ise_width = 0, ise_height = 0;
    bool get_geometry_result = false;

    /* Get candidate window position */
    ecore_evas_geometry_get (ecore_evas_ecore_evas_get (evas_object_evas_get (_candidate_window)), &x, &y, &width, &height);

    if (_candidate_angle == 90 || _candidate_angle == 270)
        get_geometry_result = ecore_x_e_window_rotation_geometry_get (_ise_window, _candidate_angle, &pos_x, &pos_y, &ise_height, &ise_width);
    else
        get_geometry_result = ecore_x_e_window_rotation_geometry_get (_ise_window, _candidate_angle, &pos_x, &pos_y, &ise_width, &ise_height);

    if ((_ise_state != WINDOW_STATE_SHOW && _ise_state != WINDOW_STATE_WILL_HIDE) ||
            (get_geometry_result == false) || (_panel_agent->get_current_toolbar_mode () == TOOLBAR_KEYBOARD_MODE)) {
        ise_height = 0;
        ise_width = 0;
    }

    int height2 = ui_candidate_get_valid_height ();

    if (_candidate_mode == FIXED_CANDIDATE_WINDOW) {
        if (_candidate_angle == 90) {
            spot_x = _screen_width - ise_height - height2;
            spot_y = 0;
        } else if (_candidate_angle == 270) {
            spot_x = ise_height - (_candidate_height - height2);
            spot_y = 0;
        } else if (_candidate_angle == 180) {
            spot_x = 0;
            spot_y = ise_height - (_candidate_height - height2);
        } else {
            spot_x = 0;
            spot_y = _screen_height - ise_height - height2;
        }
    } else {
        spot_x = _spot_location_x;
        spot_y = _spot_location_y;

        rectinfo ise_rect = {0, 0, (uint32)ise_width, (uint32)ise_height};
        if (_candidate_angle == 90 || _candidate_angle == 270) {
            if (ise_rect.height <= (uint32)0 || ise_rect.height >= (uint32)_screen_width)
                ise_rect.height = ISE_DEFAULT_HEIGHT_LANDSCAPE * _width_rate;
        } else {
            if (ise_rect.height <= (uint32)0 || ise_rect.height >= (uint32) _screen_height)
                ise_rect.height = ISE_DEFAULT_HEIGHT_PORTRAIT * _height_rate;
        }

        int nOffset = _candidate_port_height_min / 3;
        if (_candidate_angle == 270) {
            if (ise_rect.height > 0 && spot_y + height2 > _screen_width - (int)ise_rect.height + nOffset) {
                spot_x = _screen_width - _spot_location_top_y - (_candidate_height - height2);
            } else {
                spot_x = _screen_width - _spot_location_y - _candidate_height;
            }
        } else if (_candidate_angle == 90) {
            if (ise_rect.height > 0 && spot_y + height2 > _screen_width - (int)ise_rect.height + nOffset) {
                spot_x = _spot_location_top_y - height2;
            } else {
                spot_x = spot_y;
            }
        } else if (_candidate_angle == 180) {
            if (ise_rect.height > 0 && spot_y + height2 > _screen_height - (int)ise_rect.height + nOffset) {
                spot_y = _screen_height - _spot_location_top_y - (_candidate_height - height2);
            } else {
                spot_y = _screen_height - _spot_location_y - _candidate_height;
            }
        } else {
            if (ise_rect.height > 0 && spot_y + height2 > _screen_height - (int)ise_rect.height + nOffset) {
                spot_y = _spot_location_top_y - height2;
            } else {
                spot_y = spot_y;
            }
        }
    }

    if (_candidate_angle == 90) {
        spot_y = (_screen_height - _candidate_width) / 2;
        spot_x = spot_x < _indicator_height ? _indicator_height : spot_x;
        if (spot_x > _screen_width - _candidate_height)
            spot_x = _screen_width - _candidate_height;
    } else if (_candidate_angle == 270) {
        spot_y = (_screen_height - _candidate_width) / 2;
        spot_x = spot_x < 0 ? 0 : spot_x;
        if (spot_x > _screen_width - (_indicator_height+_candidate_height))
            spot_x = _screen_width - (_indicator_height+_candidate_height);
    } else if (_candidate_angle == 180) {
        spot_x = (_screen_width - _candidate_width) / 2;
        spot_y = spot_y < 0 ? 0 : spot_y;
        if (spot_y > _screen_height - (_indicator_height+_candidate_height))
            spot_y = _screen_height - (_indicator_height+_candidate_height);
    } else {
        spot_x = (_screen_width - _candidate_width) / 2;
        spot_y = spot_y < _indicator_height ? _indicator_height : spot_y;
        if (spot_y > _screen_height - _candidate_height)
            spot_y = _screen_height - _candidate_height;
    }

    if (spot_x != x || spot_y != y) {
        _candidate_x = spot_x;
        _candidate_y = spot_y;
        evas_object_move (_candidate_window, spot_x, spot_y);
        LOGD ("Moving candidate window to : %d %d", spot_x, spot_y);
        if (_preedit_window) {
            if (_candidate_angle == 90) {
                spot_x -= _preedit_height;
                spot_y = _screen_height - _preedit_width;
            } else if (_candidate_angle == 270) {
                spot_x += height2;
            } else if (_candidate_angle == 180) {
                spot_x = _screen_width - _preedit_width;
                spot_y += height2;
            } else {
                spot_y -= _preedit_height;
            }
            evas_object_move (_preedit_window, spot_x, spot_y);
        }
        if (_candidate_state == WINDOW_STATE_SHOW) {
            _panel_agent->update_candidate_panel_event ((uint32)ECORE_IMF_CANDIDATE_PANEL_GEOMETRY_EVENT, 0);
        }
    }
}

/**
 * @brief Set soft candidate geometry.
 *
 * @param x      The x position in screen.
 * @param y      The y position in screen.
 * @param width  The candidate window width.
 * @param height The candidate window height.
 */
static void set_soft_candidate_geometry (int x, int y, int width, int height)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << " x:" << x << " y:" << y << " width:" << width << " height:" << height << "...\n";

    LOGD ("candidate geometry x: %d , y: %d , width: %d , height: %d, _ise_state: %d, candidate_mode: %d", x, y, width, height, _ise_state, _candidate_mode);

    if ((_candidate_mode != SOFT_CANDIDATE_WINDOW) || (_panel_agent->get_current_toolbar_mode () != TOOLBAR_KEYBOARD_MODE))
        return;

     _soft_candidate_width  = width;
     _soft_candidate_height = height;

     set_keyboard_geometry_atom_info (_app_window, get_ise_geometry());
    _panel_agent->update_input_panel_event (ECORE_IMF_INPUT_PANEL_GEOMETRY_EVENT, 0);

}

//////////////////////////////////////////////////////////////////////
// End of Candidate Functions
//////////////////////////////////////////////////////////////////////

/**
 * @brief Set transient for app window.
 *
 * @param window The Ecore_X_Window handler of app window.
 */
static void efl_set_transient_for_app_window (Ecore_X_Window window)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "...\n";

    /* Set a transient window for window stack */
    Ecore_X_Window   xAppWindow = efl_get_app_window ();
    ecore_x_icccm_transient_for_set (window, xAppWindow);

    LOGD ("win : %x, forwin : %x\n", window, xAppWindow);
}

static int efl_get_window_rotate_angle (Ecore_X_Window win)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "...\n";

    int ret;
    int count;
    int angle = 0;
    unsigned char *prop_data = NULL;

    ret = ecore_x_window_prop_property_get (win,
            ECORE_X_ATOM_E_ILLUME_ROTATE_WINDOW_ANGLE, ECORE_X_ATOM_CARDINAL, 32, &prop_data, &count);
    if (ret && prop_data) {
        memcpy (&angle, prop_data, sizeof (int));
        LOGD ("WINDOW angle of %p is %d", win, angle);
    } else {
        std::cerr << "ecore_x_window_prop_property_get () is failed!!!\n";
        LOGW ("WINDOW angle of %p FAILED!", win);
    }
    if (prop_data)
        XFree (prop_data);

    return angle;
}

/**
 * @brief Get angle for app window.
 *
 * @param win_obj The Evas_Object handler of application window.
 *
 * @return The angle of app window.
 */
static int efl_get_app_window_angle ()
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "...\n";

    return efl_get_window_rotate_angle (efl_get_app_window ());
}

/**
 * @brief Get angle for ise window.
 *
 * @param win_obj The Evas_Object handler of ise window.
 *
 * @return The angle of ise window.
 */
static int efl_get_ise_window_angle ()
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "...\n";

    return efl_get_window_rotate_angle (_ise_window);
}

/**
 * @brief Get angle of quickpanel window.
 *
 * @return The angle of quickpanel window.
 */
static int efl_get_quickpanel_window_angle ()
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "...\n";

    return efl_get_window_rotate_angle (efl_get_quickpanel_window ());
}

/**
 * @brief Set showing effect for application window.
 *
 * @param win The Evas_Object handler of application window.
 * @param strEffect The pointer of effect string.
 */
static void efl_set_showing_effect_for_app_window (Evas_Object *win, const char* strEffect)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "...\n";
    ecore_x_icccm_name_class_set (elm_win_xwindow_get (static_cast<Evas_Object*>(win)), strEffect, "ISF");
}

/**
 * @brief Create elementary window.
 *
 * @param strWinName The window name.
 * @param strEffect The window effect string.
 *
 * @return The window pointer
 */
static Evas_Object *efl_create_window (const char *strWinName, const char *strEffect)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "...\n";

    Evas_Object *win = elm_win_add (NULL, strWinName, ELM_WIN_UTILITY);
    elm_win_title_set (win, strWinName);

    /* set window properties */
    elm_win_autodel_set (win, EINA_TRUE);
    elm_object_focus_allow_set (win, EINA_FALSE);
    elm_win_borderless_set (win, EINA_TRUE);
    elm_win_alpha_set (win, EINA_TRUE);
    elm_win_prop_focus_skip_set (win, EINA_TRUE);
    efl_set_showing_effect_for_app_window (win, strEffect);

    return win;
}

/**
 * @brief Create elementary control window.
 *
 * @return EINA_TRUE if successful, otherwise return EINA_FALSE
 */
static Eina_Bool efl_create_control_window (void)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "...\n";

    /* WMSYNC, #1 Creating and registering control window */
    if (ecore_x_display_get () == NULL)
        return EINA_FALSE;

    Ecore_X_Window root = ecore_x_window_root_first_get ();
    _control_window = ecore_x_window_input_new (root, -100, -100, 1, 1);
    ecore_x_e_virtual_keyboard_control_window_set (root, _control_window, 0, EINA_TRUE);

    Ecore_X_Atom atom = ecore_x_atom_get ("_ISF_CONTROL_WINDOW");
    ecore_x_window_prop_xid_set (root, atom, ECORE_X_ATOM_WINDOW, &_control_window, 1);

    return EINA_TRUE;
}

/**
 * @brief Get app window's x window id.
 *
 * @param win_obj The Evas_Object handler of app window.
 */
static Ecore_X_Window efl_get_app_window (void)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "...\n";

    /* Gets the current XID of the active window from the root window property */
    int  ret = 0;
    Atom type_return;
    int  format_return;
    unsigned long    nitems_return;
    unsigned long    bytes_after_return;
    unsigned char   *data = NULL;
    Ecore_X_Window   xAppWindow = 0;

    ret = XGetWindowProperty ((Display *)ecore_x_display_get (),
                              ecore_x_window_root_get (_control_window),
                              ecore_x_atom_get ("_ISF_ACTIVE_WINDOW"),
                              0, G_MAXLONG, False, XA_WINDOW, &type_return,
                              &format_return, &nitems_return, &bytes_after_return,
                              &data);

    if (ret == Success) {
        if ((type_return == XA_WINDOW) && (format_return == 32) && (data)) {
            void *pvoid = data;
            xAppWindow = *(Window *)pvoid;
            if (data)
                XFree (data);
        }
    } else {
        std::cerr << "XGetWindowProperty () is failed!!!\n";
    }

    return xAppWindow;
}

/**
 * @brief Get clipboard window's x window id.
 *
 */
static Ecore_X_Window efl_get_clipboard_window (void)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "...\n";

    /* Gets the XID of the clipboard window from the root window property */
    int  ret = 0;
    Atom type_return;
    int  format_return;
    unsigned long    nitems_return;
    unsigned long    bytes_after_return;
    unsigned char   *data = NULL;
    Ecore_X_Window   clipboard_window = 0;

    ret = XGetWindowProperty ((Display *)ecore_x_display_get (),
                              ecore_x_window_root_get (_control_window),
                              ecore_x_atom_get ("CBHM_ELM_WIN"),
                              0, G_MAXLONG, False, XA_WINDOW, &type_return,
                              &format_return, &nitems_return, &bytes_after_return,
                              &data);

    if (ret == Success) {
        if ((type_return == XA_WINDOW) && (format_return == 32) && (data)) {
            clipboard_window = *(Window *)data;
            if (data)
                XFree (data);
        }
    } else {
        std::cerr << "XGetWindowProperty () is failed!!!\n";
    }

    return clipboard_window;
}

static Ecore_X_Window efl_get_quickpanel_window (void)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "...\n";

    Ecore_X_Window rootWin = ecore_x_window_root_first_get ();
    Ecore_X_Window qpwin;
    ecore_x_window_prop_xid_get (rootWin, ecore_x_atom_get ("_E_ILLUME_QUICKPANEL_WINDOW_LIST"), ECORE_X_ATOM_WINDOW, &qpwin, 1);

    return qpwin;
}

/**
 * @brief Get default zone geometry.
 *
 * @param x The zone x position.
 * @param y The zone y position.
 * @param w The zone width.
 * @param h The zone height.
 *
 * @return EINA_TRUE if successful, otherwise return EINA_FALSE
 */
static Eina_Bool efl_get_default_zone_geometry_info (Ecore_X_Window root, uint *x, uint *y, uint *w, uint *h)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "...\n";

    Ecore_X_Atom    zone_geometry_atom;
    Ecore_X_Window *zone_lists;
    int             num_zone_lists;
    int             num_ret;
    Eina_Bool       ret;

    zone_geometry_atom = ecore_x_atom_get ("_E_ILLUME_ZONE_GEOMETRY");
    if (!zone_geometry_atom) {
        /* Error... */
        return EINA_FALSE;
    }

    uint geom[4];
    num_zone_lists = ecore_x_window_prop_window_list_get (root, ECORE_X_ATOM_E_ILLUME_ZONE_LIST, &zone_lists);
    if (num_zone_lists > 0) {
        num_ret = ecore_x_window_prop_card32_get (zone_lists[0], zone_geometry_atom, geom, 4);
        if (num_ret == 4) {
            if (x) *x = geom[0];
            if (y) *y = geom[1];
            if (w) *w = geom[2];
            if (h) *h = geom[3];
            ret = EINA_TRUE;
        } else {
            ret = EINA_FALSE;
        }
    } else {
        /* if there is no zone available */
        ret = EINA_FALSE;
    }

    if (zone_lists) {
        /* We must free zone_lists */
        free (zone_lists);
    }

    return ret;
}

/**
 * @brief Get screen resolution.
 *
 * @param width The screen width.
 * @param height The screen height.
 */
static void efl_get_screen_resolution (int &width, int &height)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "...\n";

    static Evas_Coord scr_w = 0, scr_h = 0;

    if (scr_w == 0 || scr_h == 0) {
        uint w = 0, h = 0;
        if (efl_get_default_zone_geometry_info (ecore_x_window_root_first_get (), NULL, NULL, &w, &h)) {
            scr_w = w;
            scr_h = h;
        } else {
            ecore_x_window_size_get (ecore_x_window_root_first_get (), &scr_w, &scr_h);
        }
    }

    width  = scr_w;
    height = scr_h;
}

//////////////////////////////////////////////////////////////////////
// Start of PanelAgent Functions
//////////////////////////////////////////////////////////////////////

/**
 * @brief Initialize panel agent.
 *
 * @param config The config string for PanelAgent.
 * @param display The current display.
 * @param resident The variable indicates whether panel will be resident.
 *
 * @return true if initialize is successful, otherwise return false.
 */
static bool initialize_panel_agent (const String &config, const String &display, bool resident)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "...\n";

    LOGD ("initializing panel agent");

    _panel_agent = new PanelAgent ();

    if (!_panel_agent || !_panel_agent->initialize (config, display, resident)) {
        ISF_SAVE_LOG ("panel_agent initialize fail!\n");
        return false;
    }

    _panel_agent->signal_connect_reload_config              (slot (slot_reload_config));
    _panel_agent->signal_connect_focus_in                   (slot (slot_focus_in));
    _panel_agent->signal_connect_focus_out                  (slot (slot_focus_out));
    _panel_agent->signal_connect_expand_candidate           (slot (slot_expand_candidate));
    _panel_agent->signal_connect_contract_candidate         (slot (slot_contract_candidate));
    _panel_agent->signal_connect_set_candidate_ui           (slot (slot_set_candidate_style));
    _panel_agent->signal_connect_update_factory_info        (slot (slot_update_factory_info));
    _panel_agent->signal_connect_update_spot_location       (slot (slot_update_spot_location));
    _panel_agent->signal_connect_update_input_context       (slot (slot_update_input_context));
    _panel_agent->signal_connect_update_ise_geometry        (slot (slot_update_ise_geometry));
    _panel_agent->signal_connect_show_preedit_string        (slot (slot_show_preedit_string));
    _panel_agent->signal_connect_show_aux_string            (slot (slot_show_aux_string));
    _panel_agent->signal_connect_show_lookup_table          (slot (slot_show_candidate_table));
    _panel_agent->signal_connect_hide_preedit_string        (slot (slot_hide_preedit_string));
    _panel_agent->signal_connect_hide_aux_string            (slot (slot_hide_aux_string));
    _panel_agent->signal_connect_hide_lookup_table          (slot (slot_hide_candidate_table));
    _panel_agent->signal_connect_update_preedit_string      (slot (slot_update_preedit_string));
    _panel_agent->signal_connect_update_preedit_caret       (slot (slot_update_preedit_caret));
    _panel_agent->signal_connect_update_aux_string          (slot (slot_update_aux_string));
    _panel_agent->signal_connect_update_lookup_table        (slot (slot_update_candidate_table));
    _panel_agent->signal_connect_select_candidate           (slot (slot_select_candidate));
    _panel_agent->signal_connect_get_candidate_geometry     (slot (slot_get_candidate_geometry));
    _panel_agent->signal_connect_get_input_panel_geometry   (slot (slot_get_input_panel_geometry));
    _panel_agent->signal_connect_set_active_ise_by_uuid     (slot (slot_set_active_ise));
    _panel_agent->signal_connect_get_ise_list               (slot (slot_get_ise_list));
    _panel_agent->signal_connect_get_all_helper_ise_info    (slot (slot_get_all_helper_ise_info));
    _panel_agent->signal_connect_set_has_option_helper_ise_info(slot (slot_set_has_option_helper_ise_info));
    _panel_agent->signal_connect_set_enable_helper_ise_info (slot (slot_set_enable_helper_ise_info));
    _panel_agent->signal_connect_show_helper_ise_list       (slot (slot_show_helper_ise_list));
    _panel_agent->signal_connect_show_helper_ise_selector   (slot (slot_show_helper_ise_selector));
    _panel_agent->signal_connect_is_helper_ise_enabled      (slot (slot_is_helper_ise_enabled));
    _panel_agent->signal_connect_get_ise_information        (slot (slot_get_ise_information));
    _panel_agent->signal_connect_get_keyboard_ise_list      (slot (slot_get_keyboard_ise_list));
    _panel_agent->signal_connect_get_language_list          (slot (slot_get_language_list));
    _panel_agent->signal_connect_get_all_language           (slot (slot_get_all_language));
    _panel_agent->signal_connect_get_ise_language           (slot (slot_get_ise_language));
    _panel_agent->signal_connect_get_ise_info_by_uuid       (slot (slot_get_ise_info));
    _panel_agent->signal_connect_set_keyboard_ise           (slot (slot_set_keyboard_ise));
    _panel_agent->signal_connect_get_keyboard_ise           (slot (slot_get_keyboard_ise));
    _panel_agent->signal_connect_accept_connection          (slot (slot_accept_connection));
    _panel_agent->signal_connect_close_connection           (slot (slot_close_connection));
    _panel_agent->signal_connect_exit                       (slot (slot_exit));

    _panel_agent->signal_connect_register_helper_properties (slot (slot_register_helper_properties));
    _panel_agent->signal_connect_show_ise                   (slot (slot_show_ise));
    _panel_agent->signal_connect_hide_ise                   (slot (slot_hide_ise));

    _panel_agent->signal_connect_will_hide_ack              (slot (slot_will_hide_ack));

    _panel_agent->signal_connect_set_keyboard_mode          (slot (slot_set_keyboard_mode));

    _panel_agent->signal_connect_candidate_will_hide_ack    (slot (slot_candidate_will_hide_ack));
    _panel_agent->signal_connect_get_ise_state              (slot (slot_get_ise_state));
    _panel_agent->signal_connect_start_default_ise          (slot (slot_start_default_ise));
    _panel_agent->signal_connect_stop_default_ise           (slot (slot_stop_default_ise));
    _panel_agent->signal_connect_show_panel                 (slot (slot_show_helper_ise_selector));

    _panel_agent->signal_connect_get_recent_ise_geometry    (slot (slot_get_recent_ise_geometry));
    _panel_agent->signal_connect_check_privilege_by_sockfd  (slot (slot_check_privilege_by_sockfd));

    std::vector<String> load_ise_list;
    _panel_agent->get_active_ise_list (load_ise_list);

    LOGD ("initializing panel agent succeeded");

    return true;
}

static void delete_ise_hide_timer (void)
{
    LOGD ("deleting ise_hide_timer");
    if (_ise_hide_timer) {
        ecore_timer_del (_ise_hide_timer);
        _ise_hide_timer = NULL;
    }
}

static void hide_ise ()
{
    LOGD ("send request to hide helper\n");
    String uuid = _panel_agent->get_current_helper_uuid ();
    _panel_agent->hide_helper (uuid);

    /* Only if we are not already in HIDE state */
    if (_ise_state != WINDOW_STATE_HIDE) {
        /* From this point, slot_get_input_panel_geometry should return hidden state geometry */
        _ise_state = WINDOW_STATE_WILL_HIDE;

        _updated_hide_state_geometry = false;
    }
    _ise_angle = -1;

    ecore_x_event_mask_unset (_app_window, ECORE_X_EVENT_MASK_WINDOW_FOCUS_CHANGE);

    if (_candidate_window) {
        if (_panel_agent->get_current_toolbar_mode () == TOOLBAR_KEYBOARD_MODE)
            ui_candidate_hide (true, true, true);
    }
}

#if ENABLE_MULTIWINDOW_SUPPORT
static Eina_Bool ise_hide_timeout (void *data)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "...\n";

    delete_ise_hide_timer ();
    hide_ise ();

    return ECORE_CALLBACK_CANCEL;
}
#endif

/**
 * @brief Insert data to ime_info table.
 *
 * @param list The list to store uuid
 *
 * @return true if it is successful, otherwise return false.
 */
static bool update_ise_list (std::vector<String> &list)
{
    std::vector<String> uuids;
    std::vector<TOOLBAR_MODE_T>  modes;
    std::vector<ImeInfoDB>::iterator iter;
    bool result = true;

    if (_ime_info.size() == 0) {
        if (isf_pkg_select_all_ime_info_db(_ime_info) == 0)
            result = false;
    }

    /* Update _groups */
    _groups.clear();
    std::vector<String> ise_langs;
    for (size_t i = 0; i < _ime_info.size (); ++i) {
        scim_split_string_list(ise_langs, _ime_info[i].languages);
        for (size_t j = 0; j < ise_langs.size (); j++) {
            if (std::find (_groups[ise_langs[j]].begin (), _groups[ise_langs[j]].end (), i) == _groups[ise_langs[j]].end ())
            _groups[ise_langs[j]].push_back (i);
        }
        ise_langs.clear ();
    }

    for (iter = _ime_info.begin(); iter != _ime_info.end(); iter++) {
        uuids.push_back(iter->appid);
        modes.push_back(iter->mode);
    }

    if (uuids.size() > 0) {
        list.clear ();
        list = uuids;

        _panel_agent->update_ise_list (list);

        if (_initial_ise_uuid.length () > 0) {
            String active_uuid   = _initial_ise_uuid;
            String default_uuid  = scim_global_config_read (String (SCIM_GLOBAL_CONFIG_DEFAULT_ISE_UUID), String (""));
            if (std::find (uuids.begin (), uuids.end (), default_uuid) == uuids.end ()) {
                if ((_panel_agent->get_current_toolbar_mode () == TOOLBAR_KEYBOARD_MODE) && (modes[get_ise_index (_initial_ise_uuid)] != TOOLBAR_KEYBOARD_MODE)) {
                    active_uuid = String (SCIM_COMPOSE_KEY_FACTORY_UUID);
                }
                if (set_active_ise (active_uuid, _soft_keyboard_launched) == false) {
                    if (_initial_ise_uuid.compare (active_uuid))
                        set_active_ise (_initial_ise_uuid, _soft_keyboard_launched);
                }
            } else if (_panel_agent->get_current_toolbar_mode () == TOOLBAR_HELPER_MODE) {    // Check whether keyboard engine is installed
                String IMENGINE_KEY  = String (SCIM_CONFIG_DEFAULT_IMENGINE_FACTORY) + String ("/") + String ("~other");
                String keyboard_uuid = _config->read (IMENGINE_KEY, String (""));
                if (std::find (uuids.begin (), uuids.end (), keyboard_uuid) == uuids.end ()) {
                    active_uuid = String (SCIM_COMPOSE_KEY_FACTORY_UUID);
                    _panel_agent->change_factory (active_uuid);
                    _config->write (IMENGINE_KEY, active_uuid);
                    _config->flush ();
                }
            }
        }

        char *lang_str = vconf_get_str (VCONFKEY_LANGSET);
        if (lang_str) {
            if (_ime_info.size () > 0 && _ime_info[0].display_lang.compare(lang_str) == 0)
                _locale_string = String (lang_str);
            free (lang_str);
        }
    }


#if HAVE_PKGMGR_INFO
    if (!pkgmgr) {
        int ret = package_manager_create (&pkgmgr);
        if (ret == PACKAGE_MANAGER_ERROR_NONE) {
            package_manager_set_event_cb (pkgmgr, _package_manager_event_cb, NULL);
            if (ret == PACKAGE_MANAGER_ERROR_NONE) {
                LOGD("package_manager_set_event_cb succeeded.");
            }
            else {
                ISF_SAVE_LOG("package_manager_set_event_cb failed(%d)\n", ret);
                LOGW("package_manager_set_event_cb failed(%d)", ret);
            }
        }
        else {
            ISF_SAVE_LOG("package_manager_create failed(%d)\n", ret);
            LOGW("package_manager_create failed(%d)", ret);
        }
    }
#endif

    return result;
}

/**
 * @brief Reload config slot function for PanelAgent.
 */
static void slot_reload_config (void)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "...\n";

    if (!_config.null ())
        _config->reload ();
}

/**
 * @brief Focus in slot function for PanelAgent.
 */
static void slot_focus_in (void)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "...\n";

    _focus_in = true;
    if ((_panel_agent->get_current_toolbar_mode () == TOOLBAR_KEYBOARD_MODE)) {
        if (_launch_ise_on_request && !_soft_keyboard_launched) {
            String uuid = _config->read (SCIM_CONFIG_DEFAULT_HELPER_ISE, String (""));
            if (uuid.length () > 0 && (_ime_info[get_ise_index(uuid)].options & ISM_HELPER_PROCESS_KEYBOARD_KEYEVENT)) {
                LOGD ("Start helper (%s)", uuid.c_str ());

                set_keyboard_engine (String (SCIM_COMPOSE_KEY_FACTORY_UUID));
                if (_panel_agent->start_helper (uuid))
                    _soft_keyboard_launched = true;
            }
        }
    }

    ui_candidate_delete_destroy_timer ();
}

/**
 * @brief Focus out slot function for PanelAgent.
 */
static void slot_focus_out (void)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "...\n";

    _focus_in = false;

    ui_candidate_delete_destroy_timer ();
    _destroy_timer = ecore_timer_add (ISF_CANDIDATE_DESTROY_DELAY, ui_candidate_destroy_timeout, NULL);
}

/**
 * @brief Expand candidate slot function for PanelAgent.
 */
static void slot_expand_candidate (void)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "...\n";
    if (_candidate_mode == SOFT_CANDIDATE_WINDOW)
        return;

    if (_candidate_area_2 && !_candidate_area_2_visible)
        ui_candidate_window_more_button_cb (NULL, NULL, NULL, NULL);
}

/**
 * @brief Contract candidate slot function for PanelAgent.
 */
static void slot_contract_candidate (void)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "...\n";

    if (_candidate_mode == SOFT_CANDIDATE_WINDOW)
        return;

    ui_candidate_window_close_button_cb (NULL, NULL, NULL, NULL);
}

/**
 * @brief Set candidate style slot function for PanelAgent.
 *
 * @param portrait_line The displayed line number for portrait.
 * @param mode The candidate mode.
 */
static void slot_set_candidate_style (int portrait_line, int mode)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << " display_line:" << portrait_line << " mode:" << mode << "\n";
    if ((portrait_line != _candidate_port_line) || (mode != _candidate_mode)) {
        _candidate_mode      = (ISF_CANDIDATE_MODE_T)mode;
        _candidate_port_line = (ISF_CANDIDATE_PORTRAIT_LINE_T)portrait_line;
        _soft_candidate_width = 0;
        _soft_candidate_height = 0;

        if (_candidate_mode == SOFT_CANDIDATE_WINDOW) {
            if (_candidate_window)
                ui_destroy_candidate_window ();

            return;
        }

        if (_candidate_window)
            ui_create_candidate_window ();
    }
}

static unsigned int get_ise_count (TOOLBAR_MODE_T mode, bool valid_helper)
{
    unsigned int ise_count = 0;
    for (unsigned int i = 0; i < _ime_info.size (); i++) {
        if (mode == _ime_info[i].mode) {
            if (mode == TOOLBAR_KEYBOARD_MODE || !valid_helper)
                ise_count++;
            else if (_ime_info[i].is_enabled)
                ise_count++;
        }
    }

    return ise_count;
}

/**
 * @brief Update keyboard ISE information slot function for PanelAgent.
 *
 * @param info The information of current Keyboard ISE.
 */
static void slot_update_factory_info (const PanelFactoryInfo &info)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "...\n";

    String ise_name = info.name;
    String ise_icon = info.icon;

    String old_ise = _panel_agent->get_current_ise_name ();
    if (old_ise != ise_name) {
        if ((_panel_agent->get_current_toolbar_mode () == TOOLBAR_KEYBOARD_MODE) && _candidate_window) {
            ui_destroy_candidate_window ();
        }
    }

    TOOLBAR_MODE_T mode = _panel_agent->get_current_toolbar_mode ();

    if (TOOLBAR_HELPER_MODE == mode)
        ise_name = _ime_info[get_ise_index (_panel_agent->get_current_helper_uuid())].label;

    if (ise_name.length () > 0)
        _panel_agent->set_current_ise_name (ise_name);

#ifdef HAVE_NOTIFICATION
    if (old_ise != ise_name) {
        if (TOOLBAR_KEYBOARD_MODE == mode) {
            char noti_msg[256] = {0};
            unsigned int keyboard_ise_count = get_ise_count (TOOLBAR_KEYBOARD_MODE, false);
            if (keyboard_ise_count == 0) {
                LOGD ("the number of keyboard ise is %d\n", keyboard_ise_count);
                return;
            }
            else if (keyboard_ise_count >= 2) {
                snprintf (noti_msg, sizeof (noti_msg), _("%s selected"), ise_name.c_str ());
            }
            else if (keyboard_ise_count == 1) {
                snprintf (noti_msg, sizeof (noti_msg), _("Only %s available"), ise_name.c_str ());
            }

            notification_status_message_post (noti_msg);
            LOGD ("%s\n", noti_msg);
        }
    }
#endif
}

/**
 * @brief Update cursor position slot function for PanelAgent.
 *
 * @param x The x position of current cursor.
 * @param y The bottom y position of current cursor.
 * @param top_y The top y position of current cursor.
 */
static void slot_update_spot_location (int x, int y, int top_y)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "...\n";

    if (x >= 0 && x < _screen_height && y >= 0 && y < _screen_height) {
        _spot_location_x = x;
        _spot_location_y = y;
        _spot_location_top_y = top_y;

        ui_settle_candidate_window ();
    }
}

/**
 * @brief The input context of ISE is changed.
 *
 * @param type  The event type.
 * @param value The event value.
 */
static void slot_update_input_context (int type, int value)
{
}

/**
 * @brief Update ise geometry.
 *
 * @param x      The x position in screen.
 * @param y      The y position in screen.
 * @param width  The ISE window width.
 * @param height The ISE window height.
 */
static void slot_update_ise_geometry (int x, int y, int width, int height)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << " x:" << x << " y:" << y << " width:" << width << " height:" << height << "...\n";

    LOGD ("x : %d , y : %d , width : %d , height : %d, _ise_state : %d", x, y, width, height, _ise_state);

    if (_panel_agent->get_current_toolbar_mode () == TOOLBAR_KEYBOARD_MODE) {
        if (_candidate_mode == SOFT_CANDIDATE_WINDOW) {
            /*IF ISE sent the ise_geometry information when the current_keyboard_mode is H/W mode and candidate_mode is SOFT_CANDIDATE,
             It means that given geometry information is for the candidate window */
            set_soft_candidate_geometry (x, y, width, height);
        }
        return;
    }

    _ise_width  = width;
    _ise_height = height;

    if (_candidate_window && _candidate_state == WINDOW_STATE_SHOW) {
        ui_settle_candidate_window ();
    }

    if (_ise_state == WINDOW_STATE_SHOW) {
        _ise_reported_geometry.valid = true;
        _ise_reported_geometry.angle = efl_get_ise_window_angle ();
        _ise_reported_geometry.geometry.pos_x = x;
        _ise_reported_geometry.geometry.pos_y = y;
        _ise_reported_geometry.geometry.width = width;
        _ise_reported_geometry.geometry.height = height;
        set_keyboard_geometry_atom_info (_app_window, _ise_reported_geometry.geometry);
        _panel_agent->update_input_panel_event (ECORE_IMF_INPUT_PANEL_GEOMETRY_EVENT, 0);
    }
}

/**
 * @brief Show preedit slot function for PanelAgent.
 */
static void slot_show_preedit_string (void)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "...\n";
    if (_preedit_window == NULL) {
        ui_create_preedit_window ();
        int angle = efl_get_app_window_angle ();

        /* Move preedit window according to candidate window position */
        if (_candidate_window) {
            /* Get candidate window position */
            int x, y, width, height;
            ecore_evas_geometry_get (ecore_evas_ecore_evas_get (evas_object_evas_get (_candidate_window)), &x, &y, &width, &height);

            int height2 = ui_candidate_get_valid_height ();
            if (angle == 90) {
                x -= _preedit_height;
                y = _screen_height - _preedit_width;
            } else if (_candidate_angle == 270) {
                x += height2;
            } else if (_candidate_angle == 180) {
                x = _screen_width - _preedit_width;
                y += height2;
            } else {
                y -= _preedit_height;
            }
            evas_object_move (_preedit_window, x, y);
        }
    }

    if (evas_object_visible_get (_preedit_window))
        return;

    slot_show_candidate_table ();
    evas_object_show (_preedit_window);
}

/**
 * @brief Show aux slot function for PanelAgent.
 */
static void slot_show_aux_string (void)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "...\n";
    if (_candidate_window == NULL)
        ui_create_candidate_window ();

    if (_aux_area == NULL || _aux_area_visible)
        return;

    evas_object_show (_aux_area);
    _aux_area_visible = true;
    ui_candidate_window_adjust ();

    LOGD ("calling ui_candidate_show ()");
    ui_candidate_show ();
    ui_settle_candidate_window ();
    ui_candidate_delete_destroy_timer ();
}

/**
 * @brief Show candidate table slot function for PanelAgent.
 */
static void slot_show_candidate_table (void)
{
    if (_candidate_mode == SOFT_CANDIDATE_WINDOW) {
        _panel_agent->helper_candidate_show ();
        return;
    }

    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "...\n";
    if (_candidate_window == NULL)
        ui_create_candidate_window ();

    if (_candidate_state == WINDOW_STATE_SHOW &&
        (_candidate_area_1_visible || _candidate_area_2_visible)) {
        efl_set_transient_for_app_window (elm_win_xwindow_get (_candidate_window));
        return;
    }

    evas_object_show (_candidate_area_1);
    _candidate_area_1_visible = true;
    ui_candidate_window_adjust ();

    LOGD ("calling ui_candidate_show ()");
    ui_candidate_show ();
    ui_settle_candidate_window ();
    ui_candidate_delete_destroy_timer ();

#if HAVE_FEEDBACK
    int feedback_result = feedback_initialize ();

    if (FEEDBACK_ERROR_NONE == feedback_result) {
        LOGD ("Feedback initialize successful");
        feedback_initialized = true;
    } else {
        LOGW ("Feedback initialize fail : %d",feedback_result);
        feedback_initialized = false;
    }
#endif
}

/**
 * @brief Hide preedit slot function for PanelAgent.
 */
static void slot_hide_preedit_string (void)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "...\n";

    if (!_preedit_window || !evas_object_visible_get (_preedit_window))
        return;

    evas_object_hide (_preedit_window);
}

/**
 * @brief Hide aux slot function for PanelAgent.
 */
static void slot_hide_aux_string (void)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "...\n";

    if (!_aux_area || !_aux_area_visible)
        return;

    evas_object_hide (_aux_area);
    _aux_area_visible = false;
    elm_scroller_region_show (_aux_area, 0, 0, 10, 10);
    ui_candidate_window_adjust ();

    LOGD ("calling ui_candidate_hide (false, true, true)");
    ui_candidate_hide (false, true, true);
    ui_settle_candidate_window ();

    if (ui_candidate_can_be_hide ()) {
        _candidate_show_requested = false;
        LOGD ("setting _show_candidate_requested to FALSE");
    }
}

/**
 * @brief Hide candidate table slot function for PanelAgent.
 */
static void slot_hide_candidate_table (void)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "...\n";

    if (_candidate_mode == SOFT_CANDIDATE_WINDOW) {
        _panel_agent->helper_candidate_hide ();
        return;
    }

    if (!_candidate_area_1 || _candidate_state == WINDOW_STATE_WILL_HIDE)
        return;

    if (_candidate_area_1_visible || _candidate_area_2_visible) {
        bool bForce = false;
        if (_candidate_area_1_visible) {
            if (_aux_area_visible) {
                evas_object_hide (_candidate_area_1);
                _candidate_area_1_visible = false;
                evas_object_hide (_more_btn);
            } else {
                /* Let's not actually hide the _candidate_area_1 object, for the case that
                   even if the application replies CANDIDATE_WILL_HIDE_ACK a little late,
                   it is better to display the previous candidates instead of blank screen */
                _candidate_area_1_visible = false;
                bForce = true;
            }
        }
        if (_candidate_area_2_visible) {
            evas_object_hide (_candidate_area_2);
            _candidate_area_2_visible = false;
            evas_object_hide (_scroller_bg);
            evas_object_hide (_close_btn);
            _panel_agent->candidate_more_window_hide ();
        }
        ui_candidate_window_adjust ();

        LOGD ("calling ui_candidate_hide (%d, true, true)", bForce);
        ui_candidate_hide (bForce, true, true);
        ui_settle_candidate_window ();
    }

#if HAVE_FEEDBACK
    int feedback_result = feedback_deinitialize ();

    if (FEEDBACK_ERROR_NONE == feedback_result)
        LOGD ("Feedback deinitialize successful");
    else
        LOGW ("Feedback deinitialize fail : %d", feedback_result);

    feedback_initialized = false;
#endif

    if (ui_candidate_can_be_hide ()) {
        _candidate_show_requested = false;
        LOGD ("setting _show_candidate_requested to FALSE");
    }
}

/**
 * @brief Update preedit slot function for PanelAgent.
 *
 * @param str The new preedit string.
 * @param attrs The attribute list of new preedit string.
 */
static void slot_update_preedit_string (const String &str, const AttributeList &attrs, int caret)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << " string=" << str << "\n";

    if (str.length () <= 0)
        return;

    if (_preedit_window == NULL || !evas_object_visible_get (_preedit_window)) {
        slot_show_preedit_string ();
    }

    int x, y, width, height, candidate_width;
    evas_object_text_text_set (_tmp_preedit_text, str.c_str ());
    evas_object_geometry_get (_tmp_preedit_text, &x, &y, &width, &height);
    ecore_evas_geometry_get (ecore_evas_ecore_evas_get (evas_object_evas_get (_candidate_window)), &x, &y, &candidate_width, &height);
    _preedit_width = (width + ISF_PREEDIT_BORDER * 2) < candidate_width ? (width + ISF_PREEDIT_BORDER * 2) : candidate_width;

    /* Resize preedit window and avoid text blink */
    int old_width, old_height;
    evas_object_geometry_get (_preedit_window, &x, &y, &old_width, &old_height);
    if (old_width < _preedit_width) {
        evas_object_resize (_preedit_window, _preedit_width, _preedit_height);
        edje_object_part_text_set (_preedit_text, "preedit", str.c_str ());
    } else {
        edje_object_part_text_set (_preedit_text, "preedit", str.c_str ());
        evas_object_resize (_preedit_window, _preedit_width, _preedit_height);
    }

    /* Move preedit window */
    if (_candidate_angle == 90 || _candidate_angle == 180) {
        ecore_evas_geometry_get (ecore_evas_ecore_evas_get (evas_object_evas_get (_preedit_window)), &x, &y, &width, &height);
        if (_candidate_angle == 90) {
            y = _screen_height - _preedit_width;
        } else if (_candidate_angle == 180) {
            x = _screen_width - _preedit_width;
        }
        evas_object_move (_preedit_window, x, y);
    }
}

/**
 * @brief Update caret slot function for PanelAgent.
 *
 * @param caret The caret position.
 */
static void slot_update_preedit_caret (int caret)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << " caret=" << caret << "\n";
}

/**
 * @brief Set highlight text color and background color for edje object.
 *
 * @param item The edje object pointer.
 * @param nForeGround The text color.
 * @param nBackGround The background color.
 * @param bSetBack The flag for background color.
 */
static void set_highlight_color (Evas_Object *item, uint32 nForeGround, uint32 nBackGround, bool bSetBack)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "...\n";

    int r, g, b, a, r2, g2, b2, a2, r3, g3, b3, a3;
    if (edje_object_color_class_get (item, "text_color", &r, &g, &b, &a, &r2, &g2, &b2, &a2, &r3, &g3, &b3, &a3)) {
        r = SCIM_RGB_COLOR_RED (nForeGround);
        g = SCIM_RGB_COLOR_GREEN (nForeGround);
        b = SCIM_RGB_COLOR_BLUE (nForeGround);
        edje_object_color_class_set (item, "text_color", r, g, b, a, r2, g2, b2, a2, r3, g3, b3, a3);
    }
    if (bSetBack && edje_object_color_class_get (item, "rect_color", &r, &g, &b, &a, &r2, &g2, &b2, &a2, &r3, &g3, &b3, &a3)) {
        r = SCIM_RGB_COLOR_RED (nBackGround);
        g = SCIM_RGB_COLOR_GREEN (nBackGround);
        b = SCIM_RGB_COLOR_BLUE (nBackGround);
        edje_object_color_class_set (item, "rect_color", r, g, b, 255, r2, g2, b2, a2, r3, g3, b3, a3);
    }
}

/**
 * @brief Update aux slot function for PanelAgent.
 *
 * @param str The new aux string.
 * @param attrs The attribute list of new aux string.
 */
static void slot_update_aux_string (const String &str, const AttributeList &attrs)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "...\n";
    if (_candidate_window == NULL)
        ui_create_candidate_window ();

    if (!_aux_area || (str.length () <= 0))
        return;

    if (!_aux_area_visible) {
        LOGD ("calling ui_candidate_show ()");
        ui_candidate_show ();
        slot_show_aux_string ();
    }

    int x, y, width, height, item_width = 0;
    unsigned int window_width = 0, count = 0, i;

    Evas_Object *aux_edje = NULL;

    /* Get highlight item index */
    int    aux_index = -1, aux_start = 0, aux_end = 0;
    String strAux      = str;
    bool   bSetBack    = false;
    uint32 nForeGround = SCIM_RGB_COLOR (62, 207, 255);
    uint32 nBackGround = SCIM_RGB_COLOR (0, 0, 0);
    for (AttributeList::const_iterator ait = attrs.begin (); ait != attrs.end (); ++ait) {
        if (aux_index == -1 && ait->get_type () == SCIM_ATTR_DECORATE) {
            aux_index = ait->get_value ();
        } else if (ait->get_type () == SCIM_ATTR_FOREGROUND) {
            nForeGround = ait->get_value ();
        } else if (ait->get_type () == SCIM_ATTR_BACKGROUND) {
            nBackGround = ait->get_value ();
            bSetBack = true;
        }
    }

    std::vector<String> aux_list;
    scim_split_string_list (aux_list, strAux, '|');

    if (_aux_items.size () > 0) {
        for (i = 0; i < _aux_items.size (); i++)
            evas_object_del (_aux_items [i]);
        _aux_items.clear ();
    }
    if (_aux_seperates.size () > 0) {
        for (i = 0; i < _aux_seperates.size (); i++)
            evas_object_del (_aux_seperates [i]);
        _aux_seperates.clear ();
    }

    int   seperate_width  = 4;
    int   seperate_height = 52 * _height_rate;
    Evas *evas = evas_object_evas_get (_candidate_window);
    for (i = 0; i < aux_list.size (); i++) {
        if (i > 0) {
            Evas_Object *seperate_item = edje_object_add (evas);
            edje_object_file_set (seperate_item, _candidate_edje_file.c_str (), "seperate_line");
            evas_object_size_hint_min_set (seperate_item, seperate_width, seperate_height);
            elm_table_pack (_aux_table, seperate_item, 2 * i - 1, 0, 1, 1);
            evas_object_show (seperate_item);
            _aux_seperates.push_back (seperate_item);
        }

        count++;
        aux_edje = edje_object_add (evas);
        edje_object_file_set (aux_edje, _candidate_edje_file.c_str (), "aux");
        edje_object_part_text_set (aux_edje, "aux", aux_list [i].c_str ());
        edje_object_text_class_set (aux_edje, "tizen", _candidate_font_name.c_str (), _aux_font_size);
        elm_table_pack (_aux_table, aux_edje, 2 * i, 0, 1, 1);
        evas_object_event_callback_add (aux_edje, EVAS_CALLBACK_MOUSE_DOWN, ui_mouse_button_pressed_cb, GINT_TO_POINTER ((i << 8) + ISF_EFL_AUX));
        evas_object_event_callback_add (aux_edje, EVAS_CALLBACK_MOUSE_UP, ui_mouse_button_released_cb, GINT_TO_POINTER (i));
        evas_object_event_callback_add (aux_edje, EVAS_CALLBACK_MOUSE_MOVE, ui_mouse_moved_cb, GINT_TO_POINTER (ISF_EFL_AUX));
        evas_object_show (aux_edje);
        _aux_items.push_back (aux_edje);
/*      if (i == (unsigned int)aux_index)
            edje_object_signal_emit (aux_edje, "aux,state,selected", "aux");
        else
            edje_object_signal_emit (aux_edje, "aux,state,unselected", "aux");
*/
        evas_object_text_text_set (_tmp_aux_text, aux_list [i].c_str ());
        evas_object_geometry_get (_tmp_aux_text, &x, &y, &width, &height);
        item_width = width + 2*_blank_width;
        item_width = item_width > _item_min_width ? item_width : _item_min_width;
        evas_object_size_hint_min_set (aux_edje, item_width, _aux_height);
        if (aux_index == (int)i || (aux_index == -1 && i == 0)) {
            aux_start = window_width;
            aux_end   = window_width + item_width;
        }
        window_width = window_width + item_width + 4;
    }

    // Set highlight item
    for (AttributeList::const_iterator ait = attrs.begin (); ait != attrs.end (); ++ait) {
        if (ait->get_type () == SCIM_ATTR_DECORATE) {
            unsigned int index = ait->get_value ();
            if (index < _aux_items.size ())
                set_highlight_color (_aux_items [index], nForeGround, nBackGround, bSetBack);
        }
    }

    int w, h;
    elm_scroller_region_get (_aux_area, &x, &y, &w, &h);
    item_width = aux_end - aux_start;
    if (item_width > 0) {
        if (item_width >= w)
            elm_scroller_region_show (_aux_area, aux_end - w, y, w, h);
        else if (aux_end > x + w)
            elm_scroller_region_show (_aux_area, aux_end - w, y, w, h);
        else if (aux_start < x)
            elm_scroller_region_show (_aux_area, aux_start, y, w, h);
    }
    flush_memory ();
}

/**
 * @brief Update candidate/associate table.
 *
 * @param table_type The table type.
 * @param table The lookup table for candidate or associate.
 */
static void update_table (int table_type, const LookupTable &table)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << " (" << table.get_current_page_size () << ")\n";

    int item_num = table.get_current_page_size ();
    if (item_num < 0)
        return;

    String     mbs;
    WideString wcs;

    AttributeList attrs;
    int i, x, y, item_0_width = 0;

    int nLast = 0;

    int seperate_width  = 2;
    int seperate_height = 52 * _height_rate;
    int line_width      = _candidate_scroll_width;
    int line_height     = _v_padding;
    int total_width     = 0;
    int current_width   = 0;
    int line_0          = 0;
    int line_count      = 0;
    int more_item_count = 0;
    int scroll_0_width  = _candidate_scroll_0_width_min;
    int cursor_pos      = table.get_cursor_pos ();
    int cursor_line     = 0;

    if (_candidate_angle == 90 || _candidate_angle == 270)
        scroll_0_width = _screen_height - _more_btn_width - _h_padding;
    else
        scroll_0_width = _screen_width - _more_btn_width - _h_padding;

    _candidate_image_count = 0;
    _candidate_text_count = 0;
    _candidate_pop_image_count = 0;
    _candidate_display_number = 0;
    _candidate_row_items.clear ();

    Evas *evas = evas_object_evas_get (_candidate_window);
    for (i = 0; i < SCIM_LOOKUP_TABLE_MAX_PAGESIZE; ++i) {
        if (_candidate_0 [i]) {
            evas_object_del (_candidate_0 [i]);
            _candidate_0 [i] = NULL;
        }
        if (_seperate_0 [i]) {
            evas_object_del (_seperate_0 [i]);
            _seperate_0 [i] = NULL;
        }
        if (_seperate_items [i]) {
            evas_object_del (_seperate_items [i]);
            _seperate_items [i] = NULL;
        }
        if (_line_0 [i]) {
            evas_object_del (_line_0 [i]);
            _line_0 [i] = NULL;
        }
        if (_line_items [i]) {
            evas_object_del (_line_items [i]);
            _line_items [i] = NULL;
        }
    }

    for (i = 0; i < SCIM_LOOKUP_TABLE_MAX_PAGESIZE; ++i) {
        if (i < item_num) {
            bool   bHighLight  = false;
            bool   bSetBack    = false;
            uint32 nForeGround = SCIM_RGB_COLOR (249, 249, 249);
            uint32 nBackGround = SCIM_RGB_COLOR (0, 0, 0);
            attrs = table.get_attributes_in_current_page (i);
            for (AttributeList::const_iterator ait = attrs.begin (); ait != attrs.end (); ++ait) {
                if (ait->get_type () == SCIM_ATTR_DECORATE && ait->get_value () == SCIM_ATTR_DECORATE_HIGHLIGHT) {
                    bHighLight  = true;
                    nForeGround = SCIM_RGB_COLOR (62, 207, 255);
                } else if (ait->get_type () == SCIM_ATTR_FOREGROUND) {
                    bHighLight  = true;
                    nForeGround = ait->get_value ();
                } else if (ait->get_type () == SCIM_ATTR_BACKGROUND) {
                    bSetBack    = true;
                    nBackGround = ait->get_value ();
                }
            }

            wcs = table.get_candidate_in_current_page (i);
            mbs = utf8_wcstombs (wcs);

            if (!_candidate_0 [i] && total_width <= scroll_0_width) {
                _candidate_0 [i] = get_candidate (mbs, _candidate_window, &item_0_width, nForeGround, nBackGround, bHighLight, bSetBack, item_num, i);
                evas_object_event_callback_add (_candidate_0 [i], EVAS_CALLBACK_MOUSE_DOWN, ui_mouse_button_pressed_cb, GINT_TO_POINTER ((i << 8) + ISF_EFL_CANDIDATE_0));
                evas_object_event_callback_add (_candidate_0 [i], EVAS_CALLBACK_MOUSE_UP, ui_mouse_button_released_cb, GINT_TO_POINTER (i));
                evas_object_event_callback_add (_candidate_0 [i], EVAS_CALLBACK_MOUSE_MOVE, ui_mouse_moved_cb, GINT_TO_POINTER (ISF_EFL_CANDIDATE_0));

                /* Check whether this item is the last one */
                if (i == item_num - 1) {
                    if (_candidate_angle == 90 || _candidate_angle == 270)
                        scroll_0_width = _candidate_land_width;
                    else
                        scroll_0_width = _candidate_port_width;
                }

                /* Add first item */
                if (i == 0) {
                    item_0_width = item_0_width > scroll_0_width ? scroll_0_width : item_0_width;
                    evas_object_show (_candidate_0 [i]);
                    evas_object_size_hint_min_set (_candidate_0 [i], item_0_width, _item_min_height);
                    elm_table_pack (_candidate_0_table, _candidate_0 [i], 0, 0, item_0_width, _item_min_height);
                    total_width += item_0_width;
                    _candidate_display_number++;
                    continue;
                } else {
                    total_width += (item_0_width + seperate_width);
                    if (total_width <= scroll_0_width) {
                        _seperate_0 [i] = edje_object_add (evas);
                        edje_object_file_set (_seperate_0 [i], _candidate_edje_file.c_str (), "seperate_line");
                        evas_object_size_hint_min_set (_seperate_0 [i], seperate_width, seperate_height);
                        elm_table_pack (_candidate_0_table, _seperate_0 [i],
                                total_width - item_0_width - seperate_width,
                                line_0*(_item_min_height+line_height) + (_item_min_height - seperate_height)/2,
                                seperate_width, seperate_height);
                        evas_object_show (_seperate_0 [i]);
                        evas_object_show (_candidate_0 [i]);
                        evas_object_size_hint_min_set (_candidate_0 [i], item_0_width, _item_min_height);
                        elm_table_pack (_candidate_0_table, _candidate_0 [i], total_width - item_0_width, line_0*(_item_min_height+line_height), item_0_width, _item_min_height);
                        _candidate_display_number++;
                        continue;
                    } else if ((_candidate_angle == 0 || _candidate_angle == 180) &&
                            (_candidate_port_line > 1 && (line_0 + 1) < _candidate_port_line)) {
                        line_0++;
                        scroll_0_width = _candidate_scroll_0_width_min;
                        item_0_width = item_0_width > scroll_0_width ? scroll_0_width : item_0_width;
                        evas_object_show (_candidate_0 [i]);
                        evas_object_size_hint_min_set (_candidate_0 [i], item_0_width, _item_min_height);
                        elm_table_pack (_candidate_0_table, _candidate_0 [i], 0, line_0*(_item_min_height+line_height), item_0_width, _item_min_height);
                        total_width = item_0_width;
                        _candidate_display_number++;

                        _candidate_row_items.push_back (i - nLast);
                        nLast = i;
                        continue;
                    } else {
                        _candidate_row_items.push_back (i - nLast);
                        nLast = i;
                    }
                }
            }

            if (!_candidate_0 [i]) {
                _candidate_0 [i] = get_candidate (mbs, _candidate_window, &item_0_width, nForeGround, nBackGround, bHighLight, bSetBack, item_num, i);
                evas_object_event_callback_add (_candidate_0 [i], EVAS_CALLBACK_MOUSE_DOWN, ui_mouse_button_pressed_cb, GINT_TO_POINTER ((i << 8) + ISF_EFL_CANDIDATE_ITEMS));
                evas_object_event_callback_add (_candidate_0 [i], EVAS_CALLBACK_MOUSE_UP, ui_mouse_button_released_cb, GINT_TO_POINTER (i));
                evas_object_event_callback_add (_candidate_0 [i], EVAS_CALLBACK_MOUSE_MOVE, ui_mouse_moved_cb, GINT_TO_POINTER (ISF_EFL_CANDIDATE_ITEMS));
            }
            if (current_width > 0 && current_width + item_0_width > _candidate_scroll_width) {
                current_width = 0;
                line_count++;

                _candidate_row_items.push_back (i - nLast);
                nLast = i;
                if (cursor_pos >= i)
                    cursor_line++;
            }
            if (current_width == 0 && !_line_items [i]) {
                _line_items [i] = edje_object_add (evas);
                edje_object_file_set (_line_items [i], _candidate_edje_file.c_str (), "popup_line");
                evas_object_size_hint_min_set (_line_items [i], line_width, line_height);
                x = 0;
                y = line_count*(_item_min_height+line_height);
                elm_table_pack (_candidate_table, _line_items [i], x, y, line_width, line_height);
                evas_object_show (_line_items [i]);
            }
            if (current_width != 0 && !_seperate_items [i]) {
                _seperate_items [i] = edje_object_add (evas);
                edje_object_file_set (_seperate_items [i], _candidate_edje_file.c_str (), "seperate_line");
                evas_object_size_hint_min_set (_seperate_items [i], seperate_width, seperate_height);
                x = current_width;
                y = line_count*(_item_min_height+line_height) + line_height + (_item_min_height - seperate_height)/2;
                elm_table_pack (_candidate_table, _seperate_items [i], x, y, seperate_width, seperate_height);
                evas_object_show (_seperate_items [i]);
                current_width += seperate_width;
            }
            x = current_width;
            y = line_count*(_item_min_height+line_height) + line_height;
            evas_object_size_hint_min_set (_candidate_0 [i], item_0_width, _item_min_height);
            evas_object_show (_candidate_0 [i]);
            elm_table_pack (_candidate_table, _candidate_0 [i], x, y, item_0_width, _item_min_height);
            current_width += item_0_width;
            more_item_count++;
            if (candidate_expanded == false && !bHighLight)
                break;
            else
                candidate_expanded = true;
        }
    }

    for (i = 1; i < _candidate_port_line; i++) {
        if ((_candidate_angle == 0 || _candidate_angle == 180)) {
            if (_line_0 [i] == NULL) {
                _line_0 [i] = edje_object_add (evas);
                edje_object_file_set (_line_0 [i], _candidate_edje_file.c_str (), "popup_line");
                evas_object_size_hint_min_set (_line_0 [i], line_width, line_height);
                x = 0;
                y = i * (_item_min_height + line_height) - line_height;
                elm_table_pack (_candidate_0_table, _line_0 [i], x, y, line_width, line_height);
                evas_object_show (_line_0 [i]);
            }

            // Create blank line
            if (line_0 + 1 < _candidate_port_line && i > line_0) {
                int nIndex = item_num + i;
                nIndex = nIndex < SCIM_LOOKUP_TABLE_MAX_PAGESIZE ? nIndex : SCIM_LOOKUP_TABLE_MAX_PAGESIZE - 1;
                _seperate_0 [nIndex] = edje_object_add (evas);
                edje_object_file_set (_seperate_0 [nIndex], _candidate_edje_file.c_str (), "seperate_line");
                evas_object_size_hint_min_set (_seperate_0 [nIndex], seperate_width, _item_min_height);
                elm_table_pack (_candidate_0_table, _seperate_0 [nIndex],
                        0, i*(_item_min_height+line_height), seperate_width, _item_min_height);
            }
        } else if (_line_0 [i]) {
            evas_object_del (_line_0 [i]);
            _line_0 [i] = NULL;
        }
    }

    _candidate_row_items.push_back (item_num - nLast);     /* Add the number of last row */
    _panel_agent->update_displayed_candidate_number (_candidate_display_number);
    _panel_agent->update_candidate_item_layout (_candidate_row_items);
    if (more_item_count == 0) {
        ui_candidate_window_close_button_cb (NULL, NULL, NULL, NULL);
        evas_object_hide (_more_btn);
        evas_object_hide (_close_btn);
    } else if (!_candidate_area_2_visible) {
        evas_object_show (_more_btn);
        evas_object_hide (_close_btn);
    } else {
        evas_object_hide (_more_btn);
        evas_object_show (_close_btn);
    }

    int w, h;
    elm_scroller_region_get (_candidate_area_2, &x, &y, &w, &h);

    int line_h   = _item_min_height + _v_padding;
    int cursor_y = cursor_line * line_h;
    if (cursor_y < y) {
        elm_scroller_region_bring_in (_candidate_area_2, 0, cursor_y, w, h);
    } else if (cursor_y >= y + h) {
        elm_scroller_region_bring_in (_candidate_area_2, 0, cursor_y + line_h - h, w, h);
    }

    flush_memory ();
}

/**
 * @brief Update candidate table slot function for PanelAgent.
 *
 * @param table The lookup table for candidate.
 */
static void slot_update_candidate_table (const LookupTable &table)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "...\n";

    if (_candidate_mode == SOFT_CANDIDATE_WINDOW) {
        _panel_agent->update_helper_lookup_table (table);
        return ;
    }

    if (_candidate_window == NULL)
        ui_create_candidate_window ();

    if (!_candidate_window || table.get_current_page_size () < 0)
        return;

    if (evas_object_visible_get (_candidate_area_2)) {
        candidate_expanded = true;
    } else {
        candidate_expanded = false;
    }

    update_table (ISF_CANDIDATE_TABLE, table);
    _candidate_tts_focus_index = INVALID_TTS_FOCUS_INDEX;
    ui_tts_focus_rect_hide ();
}

/**
 * @brief Send selected candidate index.
 *
 * @param selected candidate string index number.
 */
static void slot_select_candidate (int index)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "...\n";
    _panel_agent->select_candidate (index);
}

/**
 * @brief Get candidate geometry slot function for PanelAgent.
 *
 * @param info The data is used to store candidate position and size.
 */
static void slot_get_candidate_geometry (struct rectinfo &info)
{
    int x      = 0;
    int y      = 0;
    int width  = 0;
    int height = 0;

    if (_candidate_mode == SOFT_CANDIDATE_WINDOW) {
        info.pos_x  = x;
        info.pos_y  = y;
        info.width  = width;
        info.height = height;
        return;
    }

    if (_candidate_window && _candidate_state == WINDOW_STATE_SHOW) {
        /* Get candidate window position */
        /*ecore_evas_geometry_get (ecore_evas_ecore_evas_get (evas_object_evas_get (_candidate_window)), &x, &y, &width, &height);*/
        /* Get exact candidate window size */
        /*int x2, y2;
          evas_object_geometry_get (_candidate_window, &x2, &y2, &width, &height);*/

        x      = _candidate_x;
        y      = _candidate_y;
        width  = _candidate_width;
        height = _candidate_height;
    }
    info.pos_x  = x;
    info.pos_y  = y;
    info.width  = width;
    info.height = height;

    LOGD ("%d %d %d %d", info.pos_x, info.pos_y, info.width, info.height);
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << " x:" << info.pos_x << " y:" << info.pos_y
                        << " width:" << info.width << " height:" << info.height << "\n";
}

/**
 * @brief Get input panel geometry slot function for PanelAgent.
 *
 * @param info The data is used to store input panel position and size.
 */
static void slot_get_input_panel_geometry (struct rectinfo &info)
{
    if (_panel_agent->get_current_toolbar_mode () == TOOLBAR_KEYBOARD_MODE) {
        info.pos_x = 0;
        info.width = 0;
        info.height = 0;

        if (_candidate_mode == FIXED_CANDIDATE_WINDOW) {
            if (_candidate_window && _candidate_state == WINDOW_STATE_SHOW) {
                info.width  = _candidate_width;
                info.height = _candidate_height;
            }
        } else if (_candidate_mode == SOFT_CANDIDATE_WINDOW) {
            info.width  = _soft_candidate_width;
            info.height = _soft_candidate_height;
        }
        int angle = efl_get_app_window_angle ();
        if (angle == 90 || angle == 270)
            info.pos_y = _screen_width - info.height;
        else
            info.pos_y = _screen_height - info.height;
    } else {
        info = get_ise_geometry ();
        if (_ise_state != WINDOW_STATE_SHOW) {
            info.width = 0;
            info.height = 0;
        } else {
            if (_candidate_mode == FIXED_CANDIDATE_WINDOW) {
                if (_candidate_window && _candidate_state == WINDOW_STATE_SHOW) {
                    int height = ui_candidate_get_valid_height ();

                    if ((_candidate_height - height) > _ise_height) {
                        info.pos_y  = info.pos_y + info.height - _candidate_height;
                        info.height = _candidate_height;
                    } else {
                        info.pos_y  -= height;
                        info.height += height;
                    }
                }
            }
        }
    }

    LOGD ("%d %d %d %d", info.pos_x, info.pos_y, info.width, info.height);
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << " x:" << info.pos_x << " y:" << info.pos_y
                        << " width:" << info.width << " height:" << info.height << "\n";
}

/**
 * @brief Get the recent input panel geometry slot function for PanelAgent.
 *
 * @param info The data is used to store input panel position and size.
 */
static void slot_get_recent_ise_geometry (struct rectinfo &info)
{
    LOGD ("slot_get_recent_ise_geometry\n");

    /* If we have geometry reported by ISE, use the geometry information */
    int angle = efl_get_app_window_angle ();
    if (angle == 0 || angle == 180) {
        if (_portrait_recent_ise_geometry.valid) {
            info = _portrait_recent_ise_geometry.geometry;
            return;
        }
    }
    else {
        if (_landscape_recent_ise_geometry.valid) {
            info = _landscape_recent_ise_geometry.geometry;
            return;
        }
    }

    info.pos_x = -1;
    info.pos_y = -1;
    info.width = -1;
    info.height = -1;
}

static bool slot_check_privilege_by_sockfd (int client_id, int cmd, String mode)
{
    int priv_ret = security_server_check_privilege_by_sockfd (client_id, "isf::manager", mode.c_str ());

    if (priv_ret != SECURITY_SERVER_API_SUCCESS) {
        ISF_SAVE_LOG ("Failed to deal with cmd(%d) due to privilege check(isf::manager %s), error code(%d)\n", cmd, mode.c_str (), priv_ret);
    }

    return priv_ret == SECURITY_SERVER_API_SUCCESS;
}

/**
 * @brief Set active ISE slot function for PanelAgent.
 *
 * @param uuid The active ISE's uuid.
 * @param changeDefault The flag for changing default ISE.
 */
static void slot_set_active_ise (const String &uuid, bool changeDefault)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << " (" << uuid << ")\n";

    /* When changing the active (default) keyboard, initialize ime_info DB if appid is invalid.
       This may be necessary if IME packages are changed while panel process is terminated. */
    pkgmgrinfo_appinfo_h handle = NULL;
    bool invalid = false;
    int ret = pkgmgrinfo_appinfo_get_appinfo (uuid.c_str (), &handle);
    if (ret != PMINFO_R_OK) {
        LOGW("appid \"%s\" is invalid.", uuid.c_str ());
        /* This might happen if IME is uninstalled while the panel process is inactive.
           The variable uuid would be invalid, so set_active_ise() would return false. */
        invalid = true;
    }
    if (handle)
        pkgmgrinfo_appinfo_destroy_appinfo (handle);

    if (invalid) {
        _initialize_ime_info ();
        set_active_ise (_initial_ise_uuid, _soft_keyboard_launched);
    }
    else if (set_active_ise (uuid, _soft_keyboard_launched) == false) {
        if (_initial_ise_uuid.compare (uuid))
            set_active_ise (_initial_ise_uuid, _soft_keyboard_launched);
    }
}

/**
 * @brief Get all ISEs list slot function for PanelAgent.
 *
 * @param list The list is used to store all ISEs.
 *
 * @return true if this operation is successful, otherwise return false.
 */
static bool slot_get_ise_list (std::vector<String> &list)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "...\n";

    bool result = false;

    std::vector<String> uuids;
    for (std::vector<ImeInfoDB>::iterator iter = _ime_info.begin(); iter != _ime_info.end(); iter++) {
        uuids.push_back(iter->appid);
    }
    if (_ime_info.size () > 0) {
        list =  uuids;
        result = true;
    }
    else {
        result = update_ise_list (list);
    }

    return result;
}

/**
 * @brief Get all Helper ISE information from ime_info DB.
 *
 * @param info This is used to store all Helper ISE information.
 *
 * @return true if this operation is successful, otherwise return false.
 */
static bool slot_get_all_helper_ise_info (HELPER_ISE_INFO &info)
{
    bool result = false;
    String active_ime_appid;

    info.appid.clear ();
    info.label.clear ();
    info.is_enabled.clear ();
    info.is_preinstalled.clear ();
    info.has_option.clear ();

    if (_ime_info.size() == 0)
        isf_pkg_select_all_ime_info_db (_ime_info);

    //active_ime_appid = scim_global_config_read (String (SCIM_GLOBAL_CONFIG_DEFAULT_ISE_UUID), String (""));
    if (_panel_agent) {
        active_ime_appid = _panel_agent->get_current_helper_uuid ();
    }

    if (_ime_info.size () > 0) {
        for (std::vector<ImeInfoDB>::iterator iter = _ime_info.begin (); iter != _ime_info.end (); iter++) {
            if (iter->mode == TOOLBAR_HELPER_MODE) {
                info.appid.push_back (iter->appid);
                info.label.push_back (iter->label);
                info.is_enabled.push_back (iter->is_enabled);
                info.is_preinstalled.push_back (iter->is_preinstalled);
                info.has_option.push_back (static_cast<uint32>(iter->has_option));
                result = true;
            }
        }
    }

    return result;
}

/**
 * @brief Update "has_option" column of ime_info DB by Application ID
 *
 * @param[in] appid Application ID of IME to enable or disable
 * @param[in] has_option @c true to have IME option(setting), otherwise @c false
 */
static void slot_set_has_option_helper_ise_info (const String &appid, bool has_option)
{
    if (appid.length() == 0) {
        LOGW("Invalid appid");
        return;
    }

    if (_ime_info.size() == 0)
        isf_pkg_select_all_ime_info_db(_ime_info);

    if (isf_db_update_has_option_by_appid(appid.c_str(), has_option)) {    // Update ime_info DB
        for (unsigned int i = 0; i < _ime_info.size (); i++) {
            if (appid == _ime_info[i].appid) {
                _ime_info[i].has_option = static_cast<uint32>(has_option);  // Update global variable
            }
        }
    }
}

/**
 * @brief Update "is_enable" column of ime_info DB by Application ID
 *
 * @param[in] appid Application ID of IME to enable or disable
 * @param[in] is_enabled @c true to enable the IME, otherwise @c false
 */
static void slot_set_enable_helper_ise_info (const String &appid, bool is_enabled)
{
    if (appid.length() == 0) {
        LOGW("Invalid appid");
        return;
    }

    if (_ime_info.size() == 0)
        isf_pkg_select_all_ime_info_db(_ime_info);

    if (isf_db_update_is_enabled_by_appid(appid.c_str(), is_enabled)) {    // Update ime_info DB
        for (unsigned int i = 0; i < _ime_info.size (); i++) {
            if (appid == _ime_info[i].appid) {
                _ime_info[i].is_enabled = static_cast<uint32>(is_enabled);  // Update global variable
            }
        }
    }
}

/**
 * @brief Finds appid with specific category
 *
 * @return 0 if success, negative value(<0) if fail. Callback is not called if return value is negative
 */
static int _find_appid_from_category (const pkgmgrinfo_appinfo_h handle, void *user_data)
{
    if (user_data) {
        char **result = static_cast<char **>(user_data);
        char *appid = NULL;
        int ret = 0;

        /* Get appid */
        ret = pkgmgrinfo_appinfo_get_appid(handle, &appid);
        if (ret == PMINFO_R_OK) {
            *result = strdup(appid);
        }
        else {
            LOGW("pkgmgrinfo_appinfo_get_appid failed!");
        }
    }
    else {
        LOGW("user_data is null!");
    }

    return -1;  // This callback is no longer called.
}

/**
 * @brief Requests to open the installed IME list application.
 */
static void slot_show_helper_ise_list (void)
{
    // Launch IME List application; e.g., org.tizen.inputmethod-setting-list
    int ret;
    app_control_h app_control;
    char *app_id = NULL;
    pkgmgrinfo_appinfo_filter_h handle;

    if (ime_list_app.length() < 1) {
        ret = pkgmgrinfo_appinfo_filter_create(&handle);
        if (ret == PMINFO_R_OK) {
            ret = pkgmgrinfo_appinfo_filter_add_string(handle, PMINFO_APPINFO_PROP_APP_CATEGORY, "http://tizen.org/category/ime-list");
            if (ret == PMINFO_R_OK) {
                ret = pkgmgrinfo_appinfo_filter_foreach_appinfo(handle, _find_appid_from_category, &app_id);
            }
            pkgmgrinfo_appinfo_filter_destroy(handle);

            if (app_id)
                ime_list_app = String(app_id);
        }
    }
    else
        app_id = strdup(ime_list_app.c_str());

    if (app_id) {
        ret = app_control_create (&app_control);
        if (ret != APP_CONTROL_ERROR_NONE) {
            LOGW("app_control_create returned %d", ret);
            free(app_id);
            return;
        }

        ret = app_control_set_operation (app_control, APP_CONTROL_OPERATION_DEFAULT);
        if (ret != APP_CONTROL_ERROR_NONE) {
            LOGW("app_control_set_operation returned %d", ret);
            app_control_destroy(app_control);
            free(app_id);
            return;
        }

        ret = app_control_set_app_id (app_control, app_id);
        if (ret != APP_CONTROL_ERROR_NONE) {
            LOGW("app_control_set_app_id returned %d", ret);
            app_control_destroy(app_control);
            free(app_id);
            return;
        }

        ret = app_control_send_launch_request(app_control, NULL, NULL);
        if (ret != APP_CONTROL_ERROR_NONE) {
            LOGW("app_control_send_launch_request returned %d, app_id=%s", ret, app_id);
            app_control_destroy(app_control);
            free(app_id);
            return;
        }

        app_control_destroy(app_control);
        SECURE_LOGD("Launch %s", app_id);
        free(app_id);
    }
    else {
      SECURE_LOGW("AppID with http://tizen.org/category/ime-list category is not available");
    }
}

/**
 * @brief Requests to open the installed IME selector application.
 */
static void slot_show_helper_ise_selector (void)
{
    // Launch IME Selector application; e.g., org.tizen.inputmethod-setting-selector
    int ret;
    app_control_h app_control;
    char *app_id = NULL;
    pkgmgrinfo_appinfo_filter_h handle;

    if (ime_selector_app.length() < 1) {
        ret = pkgmgrinfo_appinfo_filter_create(&handle);
        if (ret == PMINFO_R_OK) {
            ret = pkgmgrinfo_appinfo_filter_add_string(handle, PMINFO_APPINFO_PROP_APP_CATEGORY, "http://tizen.org/category/ime-selector");
            if (ret == PMINFO_R_OK) {
                ret = pkgmgrinfo_appinfo_filter_foreach_appinfo(handle, _find_appid_from_category, &app_id);
            }
            pkgmgrinfo_appinfo_filter_destroy(handle);

            if (app_id)
                ime_selector_app = String(app_id);
        }
    }
    else
        app_id = strdup(ime_selector_app.c_str());

    if (app_id) {
        ret = app_control_create (&app_control);
        if (ret != APP_CONTROL_ERROR_NONE) {
            LOGW("app_control_create returned %d", ret);
            free(app_id);
            return;
        }

        ret = app_control_set_operation (app_control, APP_CONTROL_OPERATION_DEFAULT);
        if (ret != APP_CONTROL_ERROR_NONE) {
            LOGW("app_control_set_operation returned %d", ret);
            app_control_destroy(app_control);
            free(app_id);
            return;
        }

        ret = app_control_set_app_id (app_control, app_id);
        if (ret != APP_CONTROL_ERROR_NONE) {
            LOGW("app_control_set_app_id returned %d", ret);
            app_control_destroy(app_control);
            free(app_id);
            return;
        }

        ret = app_control_send_launch_request(app_control, NULL, NULL);
        if (ret != APP_CONTROL_ERROR_NONE) {
            LOGW("app_control_send_launch_request returned %d, app_id=%s", ret, app_id);
            app_control_destroy(app_control);
            free(app_id);
            return;
        }

        app_control_destroy(app_control);
        SECURE_LOGD("Launch %s", app_id);
        free(app_id);
    }
    else {
        SECURE_LOGW("AppID with http://tizen.org/category/ime-selector category is not available");
    }
}

static bool slot_is_helper_ise_enabled (String appid, int &enabled)
{
    bool is_enabled = false;

    if (appid.length() == 0) {
        LOGW("Invalid appid");
        return false;
    }

    if (_ime_info.size() == 0)
        isf_pkg_select_all_ime_info_db(_ime_info);

    if (isf_db_select_is_enabled_by_appid(appid.c_str(), &is_enabled)) {
        enabled = static_cast<int>(is_enabled);
    }
    else
        return false;

    return true;
}

/**
 * @brief Get the ISE's information.
 *
 * @param uuid The ISE's uuid.
 * @param name The ISE's name.
 * @param language The ISE's language.
 * @param type The ISE's type.
 * @param option The ISE's option.
 *
 * @return true if this operation is successful, otherwise return false.
 */
static bool slot_get_ise_information (String uuid, String &name, String &language, int &type, int &option, String &module_name)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "...\n";

    if (uuid.length () > 0) {
        // update all ISE names according to the display languages
        // sometimes get_ise_information is called before vconf display language changed callback is called.
        update_ise_locale ();

        for (unsigned int i = 0; i < _ime_info.size (); i++) {
            if (uuid == _ime_info[i].appid) {
                name = _ime_info[i].label;
                language = _ime_info[i].languages;
                type  = _ime_info[i].mode;
                option = _ime_info[i].options;
                module_name = _ime_info[i].module_name;
                return true;
            }
        }
    }

    std::cerr << __func__ << " is failed!!!\n";
    return false;
}

/**
 * @brief Get keyboard ISEs list slot function for PanelAgent.
 *
 * @param name_list The list is used to store keyboard ISEs.
 *
 * @return true if this operation is successful, otherwise return false.
 */
static bool slot_get_keyboard_ise_list (std::vector<String> &name_list)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "...\n";

    isf_load_ise_information (ALL_ISE, _config);

    std::vector<String> lang_list, uuid_list;
    isf_get_all_languages (lang_list);
    isf_get_keyboard_ises_in_languages (lang_list, uuid_list, name_list, false);

    _panel_agent->update_ise_list (uuid_list);
    return true;
}

/**
 * @brief Get enable languages list slot function for PanelAgent.
 *
 * @param list The list is used to store languages.
 */
static void slot_get_language_list (std::vector<String> &list)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "...\n";

    String lang_name;
    MapStringVectorSizeT::iterator iter = _groups.begin ();

    for (; iter != _groups.end (); iter++) {
        lang_name = scim_get_language_name (iter->first);
        list.push_back (lang_name);
    }
}

/**
 * @brief Get all languages list slot function for PanelAgent.
 *
 * @param lang The list is used to store languages.
 */
static void slot_get_all_language (std::vector<String> &lang)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "...\n";

    isf_get_all_languages (lang);
}

/**
 * @brief Get specific ISE language list slot function for PanelAgent.
 *
 * @param name The ISE name.
 * @param list The list is used to store ISE languages.
 */
static void slot_get_ise_language (char *name, std::vector<String> &list)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "...\n";

    if (name == NULL)
        return;

    std::vector<String> list_tmp;
    list_tmp.clear ();
    for (unsigned int i = 0; i < _ime_info.size(); i++) {
        if (!strcmp (_ime_info[i].label.c_str (), name)) {
            scim_split_string_list (list_tmp, _ime_info[i].languages, ',');
            for (i = 0; i < list_tmp.size (); i++)
                list.push_back (scim_get_language_name (list_tmp[i]));
            return;
        }
    }
}

/**
 * @brief Get ISE information slot function for PanelAgent.
 *
 * @param uuid The ISE uuid.
 * @param info The variable is used to store ISE information.
 *
 * @return true if this operation is successful, otherwise return false.
 */
static bool slot_get_ise_info (const String &uuid, ISE_INFO &info)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "...\n";

    for (unsigned int i = 0; i < _ime_info.size (); i++) {
        if (!uuid.compare (_ime_info[i].appid)) {
            info.uuid   = _ime_info[i].appid;
            info.name   = _ime_info[i].label;
            info.icon   = _ime_info[i].iconpath;
            info.lang   = _ime_info[i].languages;
            info.option = _ime_info[i].options;
            info.type   = _ime_info[i].mode;
            return true;
        }
    }

    return false;
}

/**
 * @brief Set keyboard ISE slot function for PanelAgent.
 *
 * @param uuid The variable is ISE uuid.
 */
static void slot_set_keyboard_ise (const String &uuid)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << " uuid = " << uuid << "\n";

    std::vector<String> uuids;
    std::vector<ImeInfoDB>::iterator iter;
    for (iter = _ime_info.begin(); iter != _ime_info.end(); iter++) {
        uuids.push_back(iter->appid);
    }

    if (uuid.length () <= 0 || std::find (uuids.begin (), uuids.end (), uuid) == uuids.end ())
        return;

    String default_uuid = scim_global_config_read (String (SCIM_GLOBAL_CONFIG_DEFAULT_ISE_UUID), String (""));
    if (_ime_info[get_ise_index (default_uuid)].mode == TOOLBAR_KEYBOARD_MODE)
        return;

    uint32 ise_option = 0;
    String ise_uuid, ise_name;
    isf_get_keyboard_ise (_config, ise_uuid, ise_name, ise_option);
    if (ise_uuid == uuid)
        return;

    String language = String ("~other");/*scim_get_locale_language (scim_get_current_locale ());*/
    _config->write (String (SCIM_CONFIG_DEFAULT_IMENGINE_FACTORY) + String ("/") + language, uuid);
    _config->flush ();
    _config->reload ();

    _panel_agent->change_factory (uuid);
    _panel_agent->reload_config ();
}

/**
 * @brief Get current keyboard ISE name and uuid slot function for PanelAgent.
 *
 * @param ise_name The variable is used to store ISE name.
 * @param ise_uuid The variable is used to store ISE uuid.
 */
static void slot_get_keyboard_ise (String &ise_name, String &ise_uuid)
{
    uint32 ise_option = 0;
    isf_get_keyboard_ise (_config, ise_uuid, ise_name, ise_option);

    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << " uuid = " << ise_uuid << "\n";
}

/**
 * @brief Accept connection slot function for PanelAgent.
 *
 * @param fd The file descriptor to connect.
 */
static void slot_accept_connection (int fd)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "...\n";

    Ecore_Fd_Handler *panel_agent_read_handler = ecore_main_fd_handler_add (fd, ECORE_FD_READ, panel_agent_handler, NULL, NULL, NULL);
    _read_handler_list.push_back (panel_agent_read_handler);

    get_input_window ();
}

/**
 * @brief Close connection slot function for PanelAgent.
 *
 * @param fd The file descriptor to connect.
 */
static void slot_close_connection (int fd)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "...\n";
    int i = 0;
    std::vector<Ecore_Fd_Handler *>::iterator IterPos;

    for (IterPos = _read_handler_list.begin (); IterPos != _read_handler_list.end (); ++IterPos,++i) {
        if (ecore_main_fd_handler_fd_get (_read_handler_list[i]) == fd) {
            ecore_main_fd_handler_del (_read_handler_list[i]);
            _read_handler_list.erase (IterPos);
            break;
        }
    }
}

/**
 * @brief Exit panel process slot function for PanelAgent.
 */
static void slot_exit (void)
{
    std::cerr << __FUNCTION__ << "...\n";
    ISF_SAVE_LOG ("exit\n");

    elm_exit ();
}

static void slot_register_helper_properties (int id, const PropertyList &props)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "...\n";

    /* WMSYNC, #2 Receiving X window ID from ISE */
    /* FIXME : We should add an API to set window id of ISE */
    Property prop = props[0];
    if (prop.get_label ().compare ("XID") == 0) {
        Ecore_X_Window xwindow = atoi (prop.get_key ().c_str ());
        _ise_window = xwindow;
        LOGD ("ISE XID : %x\n", _ise_window);

        /* Just in case for the helper sent this message later than show_ise request */
        if (_ise_state == WINDOW_STATE_SHOW || _ise_state == WINDOW_STATE_WILL_SHOW) {
            efl_set_transient_for_app_window (_ise_window);
        }

        Ecore_X_Atom atom = ecore_x_atom_get ("_ISF_ISE_WINDOW");
        if (atom && _control_window && _ise_window) {
            ecore_x_window_prop_xid_set (_control_window, atom, ECORE_X_ATOM_WINDOW, &_ise_window, 1);
        }
#ifdef HAVE_NOTIFICATION
        delete_notification (&ise_selector_module_noti);
#endif
    }
}

static void slot_show_ise (void)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "...\n";

    /* If the current toolbar mode is not HELPER_MODE, do not proceed */
    if (_panel_agent->get_current_toolbar_mode () != TOOLBAR_HELPER_MODE) {
        LOGD ("Current toolbar mode should be TOOLBAR_HELPER_MODE but is %d, returning",
            _panel_agent->get_current_toolbar_mode ());
        return;
    }

    LOGD ("slot_show_ise ()\n");

    delete_ise_hide_timer ();

    /* WMSYNC, #3 Clear the existing application's conformant area and set transient_for */
    // Unset conformant area
    Ecore_X_Window current_app_window = efl_get_app_window ();
    if (_app_window != current_app_window) {
        struct rectinfo info = {0, 0, 0, 0};
        info.pos_y = _screen_width > _screen_height ? _screen_width : _screen_height;
        set_keyboard_geometry_atom_info (_app_window, info);
        ecore_x_event_mask_unset (_app_window, ECORE_X_EVENT_MASK_WINDOW_FOCUS_CHANGE);
        LOGD ("Conformant reset for window %x\n", _app_window);
        _app_window = current_app_window;

        /* If the target window has changed but our ISE is still in visible state,
           update the keyboard geometry information */
        if (_ise_state == WINDOW_STATE_SHOW) {
            set_keyboard_geometry_atom_info (_app_window, get_ise_geometry ());
        }
    }

    /* If the candidate was already in SHOW state, respect the current angle */
    if (_candidate_state != WINDOW_STATE_SHOW) {
        /* FIXME : Need to check if candidate_angle and window_angle should be left as separated */
        _candidate_angle = efl_get_app_window_angle ();
    }
    /* If the ise was already in SHOW state, respect the current angle */
    if (_ise_state != WINDOW_STATE_SHOW) {
        _ise_angle = efl_get_app_window_angle ();
    }

    ecore_x_event_mask_set (_app_window, ECORE_X_EVENT_MASK_WINDOW_FOCUS_CHANGE);
    efl_set_transient_for_app_window (_ise_window);

    /* Make clipboard window to have transient_for information on ISE window,
       so that the clipboard window will always be above ISE window */
    Ecore_X_Window clipboard_window = efl_get_clipboard_window ();
    if (_ise_window && clipboard_window) {
        ecore_x_icccm_transient_for_set (clipboard_window, _ise_window);
    }

    /* If our ISE was already in SHOW state, skip state transition to WILL_SHOW */
    if (_ise_state != WINDOW_STATE_SHOW) {
        _ise_state = WINDOW_STATE_WILL_SHOW;
    }
}

static void slot_hide_ise (void)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "...\n";

    LOGD ("slot_hide_ise ()");

    if (!_ise_hide_timer)
        hide_ise ();
}

static void slot_will_hide_ack (void)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "...\n";

    /* WMSYNC, #8 Let the Window Manager to actually hide keyboard window */
    // WILL_HIDE_REQUEST_DONE Ack to WM
    Ecore_X_Window root_window = ecore_x_window_root_get (_control_window);
    ecore_x_e_virtual_keyboard_off_prepare_done_send (root_window, _control_window);
    LOGD ("_ecore_x_e_virtual_keyboard_off_prepare_done_send (%x, %x)\n",
            root_window, _control_window);
    if (_panel_agent->get_current_toolbar_mode () == TOOLBAR_HELPER_MODE) {
        LOGD ("calling ui_candidate_hide (true, false)");
        ui_candidate_hide (true, false);
    }

    /* WILL_HIDE_ACK means that the application finished redrawing the autoscroll area,
       now hide the candidate window right away if it is also in WILL_HIDE state */
    if (_candidate_state == WINDOW_STATE_WILL_HIDE) {
        candidate_window_hide ();
    }

    if (_off_prepare_done_timer) {
        ecore_timer_del (_off_prepare_done_timer);
        _off_prepare_done_timer = NULL;
    }
}

static void slot_candidate_will_hide_ack (void)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "...\n";

    LOGD ("candidate_will_hide_ack");
    if (_candidate_state == WINDOW_STATE_WILL_HIDE) {
        candidate_window_hide ();
    }
}

static void slot_set_keyboard_mode (int mode)
{
    LOGD ("slot_set_keyboard_mode called (TOOLBAR_MODE : %d)\n",mode);

    change_keyboard_mode ((TOOLBAR_MODE_T)mode);
    show_soft_keyboard ();
}

static void slot_get_ise_state (int &state)
{
    if (_ise_state == WINDOW_STATE_SHOW ||
        ((_panel_agent->get_current_toolbar_mode () == TOOLBAR_KEYBOARD_MODE) && (_candidate_state == WINDOW_STATE_SHOW))) {
        state = ECORE_IMF_INPUT_PANEL_STATE_SHOW;
    } else {
        /* Currently we don't have WILL_HIDE / HIDE state distinction in Ecore_IMF */
        switch (_ise_state) {
            case WINDOW_STATE_SHOW :
                state = ECORE_IMF_INPUT_PANEL_STATE_SHOW;
                break;
            case WINDOW_STATE_WILL_SHOW :
                state = ECORE_IMF_INPUT_PANEL_STATE_WILL_SHOW;
                break;
            case WINDOW_STATE_WILL_HIDE :
                state = ECORE_IMF_INPUT_PANEL_STATE_HIDE;
                break;
            case WINDOW_STATE_HIDE :
                state = ECORE_IMF_INPUT_PANEL_STATE_HIDE;
                break;
            default :
                state = ECORE_IMF_INPUT_PANEL_STATE_HIDE;
        };
    }
    LOGD ("state = %d", state);
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << " state = " << state << "\n";
}

static void slot_start_default_ise (void)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "...\n";
    if ((_panel_agent->get_current_toolbar_mode () == TOOLBAR_HELPER_MODE)) {
        if (_launch_ise_on_request && !_soft_keyboard_launched) {
            String uuid  = _config->read (SCIM_CONFIG_DEFAULT_HELPER_ISE, String (""));

            LOGD ("Start helper (%s)", uuid.c_str ());

            set_keyboard_engine (String (SCIM_COMPOSE_KEY_FACTORY_UUID));
            if (_panel_agent->start_helper (uuid))
                _soft_keyboard_launched = true;
        }
    }
}

static void slot_stop_default_ise (void)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "...\n";

    if (_launch_ise_on_request && _soft_keyboard_launched) {
        String uuid = _panel_agent->get_current_helper_uuid ();

        if (uuid.length () > 0) {
            _panel_agent->hide_helper (uuid);
            _panel_agent->stop_helper (uuid);
            _soft_keyboard_launched = false;
            LOGD ("stop helper (%s)", uuid.c_str ());
        }
    }
}

//////////////////////////////////////////////////////////////////////
// End of PanelAgent-Functions
//////////////////////////////////////////////////////////////////////


/**
 * @brief Callback function for ecore fd handler.
 *
 * @param data The data to pass to this callback.
 * @param fd_handler The ecore fd handler.
 *
 * @return ECORE_CALLBACK_RENEW
 */
static Eina_Bool panel_agent_handler (void *data, Ecore_Fd_Handler *fd_handler)
{
    if (fd_handler == NULL)
        return ECORE_CALLBACK_RENEW;

    int fd = ecore_main_fd_handler_fd_get (fd_handler);
    for (unsigned int i = 0; i < _read_handler_list.size (); i++) {
        if (fd_handler == _read_handler_list [i]) {
            if (!_panel_agent->filter_event (fd)) {
                std::cerr << "_panel_agent->filter_event () is failed!!!\n";
                ecore_main_fd_handler_del (fd_handler);

                ISF_SAVE_LOG ("_panel_agent->filter_event (fd=%d) is failed!!!\n", fd);
            }
            return ECORE_CALLBACK_RENEW;
        }
    }
    std::cerr << "panel_agent_handler () has received exception event!!!\n";
    _panel_agent->filter_exception_event (fd);
    ecore_main_fd_handler_del (fd_handler);

    ISF_SAVE_LOG ("Received exception event (fd=%d)!!!\n", fd);
    return ECORE_CALLBACK_RENEW;
}

#if defined(HAVE_SYSTEMD)
static Eina_Bool _terminate_timer_cb(void *data)
{
    Ecore_Ipc_Server *server = (Ecore_Ipc_Server *)data;
    if (server)
        ecore_ipc_server_del (server);
    elm_exit ();
    return ECORE_CALLBACK_CANCEL;
}
#endif

/**
 * @brief Handler function for HelperManager input.
 *
 * @param data The data to pass to this callback.
 * @param fd_handler The Ecore Fd handler.
 *
 * @return ECORE_CALLBACK_RENEW
 */
static Eina_Bool helper_manager_input_handler (void *data, Ecore_Fd_Handler *fd_handler)
{
    if (_panel_agent->has_helper_manager_pending_event ()) {
        if (!_panel_agent->filter_helper_manager_event ()) {
            std::cerr << "_panel_agent->filter_helper_manager_event () is failed!!!\n";
            LOGE ("_panel_agent->filter_helper_manager_event () is failed!!!");
#if defined(HAVE_SYSTEMD)
            Ecore_Ipc_Server *server = ecore_ipc_server_connect (ECORE_IPC_LOCAL_SYSTEM, (char *)"scim-helper-broker", 0, NULL);
            if (server) {
                const char *message = "request_to_terminate_scim";
                LOGD("Request to terminate scim...");
                ecore_ipc_server_send (server, 2, 4, 0, 0, 0, message, strlen (message));
            }

            /* isf-panel-process will be terminated when the parent scim process is terminated.
               This is a defensive timer callback function to terminate itself in case something goes wrong.
               The _terminate_timer_cb() function wouldn't be called. */
            ecore_timer_add(2.0, _terminate_timer_cb, server);
            return ECORE_CALLBACK_CANCEL;
#else
            elm_exit ();
#endif
        }
    } else {
        std::cerr << "_panel_agent->has_helper_manager_pending_event () is failed!!!\n";
        LOGE ("_panel_agent->has_helper_manager_pending_event () is failed!!!");
    }

    return ECORE_CALLBACK_RENEW;
}

/**
 * @brief Callback function for abnormal signal.
 *
 * @param sig The signal.
 */
static void signalhandler (int sig)
{
    std::cerr << __FUNCTION__ << " Signal=" << sig << "\n";
    ISF_SAVE_LOG ("Signal=%d\n", sig);

    unregister_edbus_signal_handler ();
    elm_exit ();
}

#if HAVE_VCONF
static void update_ise_locale ()
{
    char *lang_str = vconf_get_str (VCONFKEY_LANGSET);
    if (lang_str && _locale_string.compare(lang_str) == 0) {
        free (lang_str);
        return;
    }

    LOGD ("update all ISE names according to display language\n");
    set_language_and_locale ();

    int ret = 0;
    bool exist = false;
    char *label = NULL;
    pkgmgrinfo_appinfo_h handle = NULL;
    bool need_to_init_db = false;

    /* Read DB from ime_info table */
    isf_load_ise_information(ALL_ISE, _config);

    for (unsigned int i = 0; i < _ime_info.size (); i++) {
        ret = pkgmgrinfo_appinfo_get_appinfo(_ime_info[i].appid.c_str(), &handle);
        if (ret == PMINFO_R_OK) {
            ret = pkgmgrinfo_appinfo_is_category_exist(handle, "http://tizen.org/category/ime", &exist);
            if (ret == PMINFO_R_OK && exist) {
                ret = pkgmgrinfo_appinfo_get_label(handle, &label);
                if (ret == PMINFO_R_OK && label) {
                    _ime_info[i].label = String(label);
                    /* Update label column in ime_info db table */
                    if (isf_db_update_label_by_appid(_ime_info[i].appid.c_str(), label)) {
                        _ime_info[i].label = label;
                    }
                }
            }
            else {
                // The appid is invalid.. Need to initialize ime_info DB.
                need_to_init_db = true;
            }
            pkgmgrinfo_appinfo_destroy_appinfo(handle);
        }
        else {
            // The appid is invalid.. Need to initialize ime_info DB.
            need_to_init_db = true;
        }
    }

    if (need_to_init_db) {
        _initialize_ime_info ();
    }

    if (lang_str) {
        isf_db_update_disp_lang(lang_str);
        _locale_string = String (lang_str);
        free (lang_str);
    }
}

/**
 * @brief Set language and locale.
 *
 * @return void
 */
static void set_language_and_locale (void)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "...\n";

    char *lang_str = vconf_get_str (VCONFKEY_LANGSET);

    if (lang_str) {
        elm_language_set (lang_str);

        setenv ("LANG", lang_str, 1);
        setlocale (LC_MESSAGES, lang_str);
        free (lang_str);
    } else {
        setenv ("LANG", "en_US.utf8", 1);
        setlocale (LC_MESSAGES, "en_US.utf8");
    }
}

/**
 * @brief Callback function for display language change.
 *
 * @param key The key node.
 * @param data The data to pass to this callback.
 *
 * @return void
 */
static void display_language_changed_cb (keynode_t *key, void* data)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "...\n";

    set_language_and_locale ();

    /* Update all ISE names according to display language */
    update_ise_locale ();

    String default_uuid = scim_global_config_read (String (SCIM_GLOBAL_CONFIG_DEFAULT_ISE_UUID), _initial_ise_uuid);
    unsigned int ise_idx = get_ise_index (default_uuid);

    if (ise_idx < _ime_info.size ()) {
        String default_name = _ime_info[ise_idx].label;
        _panel_agent->set_current_ise_name (default_name);
        _config->reload ();
    }
}
#endif

/**
 * @brief Change keyboard mode.
 *
 * @param mode The keyboard mode.
 *
 * @return void
 */
static void change_keyboard_mode (TOOLBAR_MODE_T mode)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "...\n";

    uint32 option = 0;
    String uuid, name;
    unsigned int val = 0;
    bool _support_hw_keyboard_mode = false;

    String helper_uuid = _config->read (SCIM_CONFIG_DEFAULT_HELPER_ISE, String (""));
    String default_uuid = scim_global_config_read (String (SCIM_GLOBAL_CONFIG_DEFAULT_ISE_UUID), String (""));
    _support_hw_keyboard_mode = scim_global_config_read (String (SCIM_GLOBAL_CONFIG_SUPPORT_HW_KEYBOARD_MODE), _support_hw_keyboard_mode);

    if (mode == TOOLBAR_KEYBOARD_MODE && _support_hw_keyboard_mode) {
        if (_panel_agent->get_current_toolbar_mode () == TOOLBAR_KEYBOARD_MODE) {
            LOGD ("HARDWARE_KEYBOARD_MODE return");
            return;
        }

        LOGD ("HARDWARE KEYBOARD MODE");
        _config->write (ISF_CONFIG_HARDWARE_KEYBOARD_DETECT, 1);
        _config->flush ();

        if (_ime_info[get_ise_index(default_uuid)].mode == TOOLBAR_HELPER_MODE) {
            /* Get the keyboard ISE */
            isf_get_keyboard_ise (_config, uuid, name, option);
            if (option & SCIM_IME_NOT_SUPPORT_HARDWARE_KEYBOARD) {
                uuid = String (SCIM_COMPOSE_KEY_FACTORY_UUID);
                std::cerr << __FUNCTION__ << ": Keyboard ISE (" << name << ") can not support hardware keyboard!!!\n";
            }
            /* Try to find reasonable keyboard ISE according to helper ISE language */
            if (uuid == String (SCIM_COMPOSE_KEY_FACTORY_UUID)) {
                String helper_language = _ime_info[get_ise_index(default_uuid)].languages;
                if (helper_language.length () > 0) {
                    std::vector<String> ise_langs;
                    scim_split_string_list (ise_langs, helper_language);
                    for (size_t i = 0; i < _groups[ise_langs[0]].size (); ++i) {
                        int j = _groups[ise_langs[0]][i];
                        if (_ime_info[j].appid != uuid && _ime_info[j].mode == TOOLBAR_KEYBOARD_MODE) {
                            uuid = _ime_info[j].appid;
                            break;
                        }
                    }
                }
            }
        }
        else {
            uuid = default_uuid;
        }
        _soft_candidate_width = 0;
        _soft_candidate_height = 0;
        _ise_state = WINDOW_STATE_HIDE;
        _panel_agent->set_current_toolbar_mode (TOOLBAR_KEYBOARD_MODE);
        _panel_agent->hide_helper (helper_uuid);
        _panel_agent->reload_config ();

        /* Check whether stop soft keyboard */
        if (_focus_in && (_ime_info[get_ise_index (helper_uuid)].options & ISM_HELPER_PROCESS_KEYBOARD_KEYEVENT)) {
            /* If focus in and soft keyboard can support hardware key event, then don't stop it */
            ;
        } else if (_launch_ise_on_request && _soft_keyboard_launched) {
            _panel_agent->stop_helper (helper_uuid);
            _soft_keyboard_launched = false;
        }

        ecore_x_event_mask_set (efl_get_quickpanel_window (), ECORE_X_EVENT_MASK_WINDOW_PROPERTY);

#ifdef HAVE_NOTIFICATION
        notification_status_message_post (_("Input detected from hardware keyboard"));

        /* Read configuations for notification app (isf-kbd-mode-changer) */
        String kbd_mode_changer = scim_global_config_read (String (SCIM_GLOBAL_CONFIG_DEFAULT_KBD_MODE_CHANGER_PROGRAM), String (""));
        hwkbd_module_noti.launch_app = kbd_mode_changer;
        LOGD ("Create kbd_mode_changer notification with : %s", kbd_mode_changer.c_str ());
        create_notification (&hwkbd_module_noti);
#endif

        /* Set input detected property for isf setting */
        val = 1;
        ecore_x_window_prop_card32_set (_control_window, ecore_x_atom_get (PROP_X_EXT_KEYBOARD_INPUT_DETECTED), &val, 1);
        ecore_x_window_prop_card32_set (ecore_x_window_root_first_get (), ecore_x_atom_get (PROP_X_EXT_KEYBOARD_INPUT_DETECTED), &val, 1);
    } else if (mode == TOOLBAR_HELPER_MODE) {
        LOGD ("SOFTWARE KEYBOARD MODE");
        /* When switching back to S/W keyboard mode, let's hide candidate window first */
        LOGD ("calling ui_candidate_hide (true, true, true)");
        ui_candidate_hide (true, true, true);
        _config->write (ISF_CONFIG_HARDWARE_KEYBOARD_DETECT, 0);
        _config->flush ();
        if (_panel_agent->get_current_toolbar_mode () == TOOLBAR_KEYBOARD_MODE) {
            uuid = helper_uuid.length () > 0 ? helper_uuid : _initial_ise_uuid;
            if (_launch_ise_on_request) {
                if (set_active_ise (uuid, false) == false) {
                    if (_initial_ise_uuid.compare(uuid))
                        set_active_ise (_initial_ise_uuid, false);
                }
            }
            else {
                if (set_active_ise (uuid, true) == false) {
                    if (_initial_ise_uuid.compare(uuid))
                        set_active_ise (_initial_ise_uuid, true);
                }
            }
        }

#ifdef HAVE_NOTIFICATION
        delete_notification (&hwkbd_module_noti);
#endif

        /* Set input detected property for isf setting */
        val = 0;
        ecore_x_window_prop_card32_set (_control_window, ecore_x_atom_get (PROP_X_EXT_KEYBOARD_INPUT_DETECTED), &val, 1);
        ecore_x_window_prop_card32_set (ecore_x_window_root_first_get (), ecore_x_atom_get (PROP_X_EXT_KEYBOARD_INPUT_DETECTED), &val, 1);
    }
    _config->reload ();
}

#if HAVE_BLUETOOTH
/**
 * @brief Callback function for the connection state of Bluetooth Keyboard
 *
 * @param result The result of changing the connection state
 * @param connected The state to be changed. true means connected state, Otherwise, false.
 * @param remote_address The remote address
 * @param user_data The user data passed from the callback registration function
 *
 * @return void
 */
static void _bt_cb_hid_state_changed (int result, bool connected, const char *remote_address, void *user_data)
{
    if (connected == false) {
       LOGD ("Bluetooth keyboard disconnected");
       if (_panel_agent->get_current_toolbar_mode () == TOOLBAR_KEYBOARD_MODE) {
           change_keyboard_mode (TOOLBAR_HELPER_MODE);
        }
    }
}
#endif

/**
 * @brief Callback function for ECORE_X_EVENT_WINDOW_PROPERTY.
 *
 * @param data Data to pass when it is called.
 * @param ev_type The event type.
 * @param ev The information for current message.
 *
 * @return ECORE_CALLBACK_PASS_ON
 */
static Eina_Bool x_event_window_property_cb (void *data, int ev_type, void *event)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "...\n";

    Ecore_X_Event_Window_Property *ev = (Ecore_X_Event_Window_Property *)event;

    if (ev == NULL)
        return ECORE_CALLBACK_PASS_ON;

    if (ev->win == _input_win && ev->atom == ecore_x_atom_get (PROP_X_EXT_KEYBOARD_EXIST)) {
        SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "...\n";
        unsigned int val = 0;
        if (ecore_x_window_prop_card32_get (_input_win, ecore_x_atom_get (PROP_X_EXT_KEYBOARD_EXIST), &val, 1) > 0) {
            if (val == 0) {
                _panel_agent->reset_keyboard_ise ();
                change_keyboard_mode (TOOLBAR_HELPER_MODE);
                set_keyboard_geometry_atom_info (_app_window, get_ise_geometry ());
                _panel_agent->update_input_panel_event (ECORE_IMF_INPUT_PANEL_GEOMETRY_EVENT, 0);
                show_soft_keyboard ();
            }
        }
    } else if (ev->atom == ECORE_X_ATOM_E_VIRTUAL_KEYBOARD_STATE) {
        if (ev->win == _control_window) {
            /* WMSYNC, #6 The keyboard window is displayed fully so set the conformant geometry */
            LOGD ("ECORE_X_ATOM_E_VIRTUAL_KEYBOARD_STATE : win : %p, atom : %d\n", ev->win, ev->atom);
            Ecore_X_Virtual_Keyboard_State state;
            state = ecore_x_e_virtual_keyboard_state_get (ev->win);
            if (state == ECORE_X_VIRTUAL_KEYBOARD_STATE_ON) {
                LOGD ("ECORE_X_VIRTUAL_KEYBOARD_STATE_ON\n");
                _ise_state = WINDOW_STATE_SHOW;

                /* Make sure that we have the same rotation angle with the keyboard window */
                if (_ise_window) {
                    _candidate_angle = efl_get_ise_window_angle ();
                    _ise_angle = efl_get_ise_window_angle ();
                }

                if (_candidate_show_requested) {
                    LOGD ("calling ui_candidate_show (true)");
                    ui_candidate_show (true);
                } else {
                    if (_candidate_area_1_visible) {
                        LOGD ("calling ui_candidate_show (false)");
                        ui_candidate_show (false);
                    }
                }

                set_keyboard_geometry_atom_info (_app_window, get_ise_geometry ());
                _panel_agent->update_input_panel_event (ECORE_IMF_INPUT_PANEL_GEOMETRY_EVENT, 0);
                _panel_agent->update_input_panel_event (
                        ECORE_IMF_INPUT_PANEL_STATE_EVENT, ECORE_IMF_INPUT_PANEL_STATE_SHOW);

                vconf_set_int (VCONFKEY_ISF_INPUT_PANEL_STATE, VCONFKEY_ISF_INPUT_PANEL_STATE_SHOW);

                if (_panel_agent->get_current_toolbar_mode () == TOOLBAR_HELPER_MODE) {
                    if (get_ise_count (TOOLBAR_HELPER_MODE, true) >= 2) {
                        ecore_x_event_mask_set (efl_get_quickpanel_window (), ECORE_X_EVENT_MASK_WINDOW_PROPERTY);
#ifdef HAVE_NOTIFICATION
                        String ise_name;
                        unsigned int idx = get_ise_index (_panel_agent->get_current_helper_uuid ());
                        if (idx < _ime_info.size ())
                            ise_name = _ime_info[idx].label;

                        ise_selector_module_noti.content = ise_name.c_str ();

                        /* Find IME Selector appid for notification */
                        if (ime_selector_app.length() < 1) {
                            char *app_id = NULL;
                            pkgmgrinfo_appinfo_filter_h handle;
                            int ret = pkgmgrinfo_appinfo_filter_create(&handle);
                            if (ret == PMINFO_R_OK) {
                                ret = pkgmgrinfo_appinfo_filter_add_string(handle, PMINFO_APPINFO_PROP_APP_CATEGORY, "http://tizen.org/category/ime-selector");
                                if (ret == PMINFO_R_OK) {
                                    ret = pkgmgrinfo_appinfo_filter_foreach_appinfo(handle, _find_appid_from_category, &app_id);
                                }
                                pkgmgrinfo_appinfo_filter_destroy(handle);

                                if (app_id) {
                                    ime_selector_app = String(app_id);
                                    free (app_id);
                                }
                            }
                        }
                        if (ime_selector_app.length() > 0) {
                            ise_selector_module_noti.launch_app = ime_selector_app;
                            LOGD ("Create ise_selector notification with : %s", ime_selector_app.c_str ());
                            create_notification (&ise_selector_module_noti);
                        }
                        else
                            LOGW("AppID with http://tizen.org/category/ime-selector category is not available");
#endif
                    }
                }

                _updated_hide_state_geometry = false;

                ecore_x_e_virtual_keyboard_state_set (_ise_window, ECORE_X_VIRTUAL_KEYBOARD_STATE_ON);
            } else if (state == ECORE_X_VIRTUAL_KEYBOARD_STATE_OFF) {
                /* WMSYNC, #9 The keyboard window is hidden fully so send HIDE state */
                LOGD ("ECORE_X_VIRTUAL_KEYBOARD_STATE_OFF\n");
                // For now don't send HIDE signal here
                //_panel_agent->update_input_panel_event (
                //    ECORE_IMF_INPUT_PANEL_STATE_EVENT, ECORE_IMF_INPUT_PANEL_STATE_HIDE);
                _ise_state = WINDOW_STATE_HIDE;
                _ise_angle = -1;
                if (!_updated_hide_state_geometry) {
                    /* When the ISE gets hidden by the window manager forcefully without OFF_PREPARE,
                       the application might not have updated its autoscroll area */
                    set_keyboard_geometry_atom_info (_app_window, get_ise_geometry ());
                    _panel_agent->update_input_panel_event (ECORE_IMF_INPUT_PANEL_GEOMETRY_EVENT, 0);
                    _panel_agent->update_input_panel_event (
                            ECORE_IMF_INPUT_PANEL_STATE_EVENT, ECORE_IMF_INPUT_PANEL_STATE_HIDE);

                    _updated_hide_state_geometry = true;
                }
                if (_panel_agent->get_current_toolbar_mode () == TOOLBAR_HELPER_MODE) {
                    LOGD ("calling ui_candidate_hide (true, false)");
                    ui_candidate_hide (true, false);
                } else {
                    ui_settle_candidate_window ();
                }

                vconf_set_int (VCONFKEY_ISF_INPUT_PANEL_STATE, VCONFKEY_ISF_INPUT_PANEL_STATE_HIDE);

#ifdef HAVE_NOTIFICATION
                delete_notification (&ise_selector_module_noti);
#endif

                _ise_reported_geometry.valid = false;

                ecore_x_e_virtual_keyboard_state_set (_ise_window, ECORE_X_VIRTUAL_KEYBOARD_STATE_OFF);
            }
            ui_settle_candidate_window ();
        }
    } else if (ev->atom == ECORE_X_ATOM_E_ILLUME_ROTATE_WINDOW_ANGLE) {
        if (ev->win == efl_get_quickpanel_window ()) {
            int angle = efl_get_quickpanel_window_angle ();
            LOGD ("ev->win : %p, change window angle : %d\n", ev->win, angle);
        }
    }

    return ECORE_CALLBACK_PASS_ON;
}

/**
 * @brief Callback function for X event client message.
 *
 * @param data Data to pass when it is called.
 * @param type The event type.
 * @param event The information for current message.
 *
 * @return ECORE_CALLBACK_RENEW
 */
static Eina_Bool x_event_client_message_cb (void *data, int type, void *event)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "...\n";

    Ecore_X_Event_Client_Message *ev = (Ecore_X_Event_Client_Message *)event;

    if (ev == NULL)
        return ECORE_CALLBACK_RENEW;

    if ((ev->win == _control_window)) {
        if (ev->message_type == ECORE_X_ATOM_E_VIRTUAL_KEYBOARD_ON_PREPARE_REQUEST) {
            /* WMSYNC, #4 Send WILL_SHOW event when the keyboard window is about to displayed */
            LOGD ("_ECORE_X_ATOM_E_VIRTUAL_KEYBOARD_ON_PREPARE_REQUEST\n");

            /* WMSYNC, #5 Let the Window Manager to actually show keyboard window */
            // WILL_SHOW_REQUEST_DONE Ack to WM
            Ecore_X_Window root_window = ecore_x_window_root_get (_control_window);
            ecore_x_e_virtual_keyboard_on_prepare_done_send (root_window, _control_window);
            LOGD ("_ecore_x_e_virtual_keyboard_on_prepare_done_send (%x, %x)\n",
                    root_window, _control_window);

            _panel_agent->update_input_panel_event (
                    ECORE_IMF_INPUT_PANEL_STATE_EVENT, ECORE_IMF_INPUT_PANEL_STATE_WILL_SHOW);
            ui_create_candidate_window ();

            vconf_set_int (VCONFKEY_ISF_INPUT_PANEL_STATE, VCONFKEY_ISF_INPUT_PANEL_STATE_WILL_SHOW);
        } else if (ev->message_type == ECORE_X_ATOM_E_VIRTUAL_KEYBOARD_OFF_PREPARE_REQUEST) {
            _ise_state = WINDOW_STATE_WILL_HIDE;
            /* WMSYNC, #7 Send WILL_HIDE event when the keyboard window is about to hidden */
            LOGD ("_ECORE_X_ATOM_E_VIRTUAL_KEYBOARD_OFF_PREPARE_REQUEST\n");
            // Clear conformant geometry information first

            if (_off_prepare_done_timer) {
                ecore_timer_del (_off_prepare_done_timer);
                _off_prepare_done_timer = NULL;
            }
            _off_prepare_done_timer = ecore_timer_add (1.0, off_prepare_done_timeout, NULL);

            _ise_reported_geometry.valid = false;
            set_keyboard_geometry_atom_info (_app_window, get_ise_geometry ());
            _panel_agent->update_input_panel_event (ECORE_IMF_INPUT_PANEL_GEOMETRY_EVENT, 0);
            _updated_hide_state_geometry = true;

            /* If the input panel is getting hidden because of hw keyboard mode while
               the candidate window is still opened, it is considered to be an
               "input panel being resized" event instead of "input panel being hidden",
               since the candidate window will work as an "input panel" afterwards */
            bool send_input_panel_hide_event = true;
            if (_panel_agent->get_current_toolbar_mode () == TOOLBAR_KEYBOARD_MODE) {
                LOGD ("_candidate_state : %d", _candidate_state);
                if (_candidate_state == WINDOW_STATE_SHOW) {
                    send_input_panel_hide_event = false;
                }
            }
            if (send_input_panel_hide_event) {
                _panel_agent->update_input_panel_event (
                        ECORE_IMF_INPUT_PANEL_STATE_EVENT, ECORE_IMF_INPUT_PANEL_STATE_HIDE);
            }
            // For now don't send WILL_HIDE signal here
            //_panel_agent->update_input_panel_event (
            //    ECORE_IMF_INPUT_PANEL_STATE_EVENT, ECORE_IMF_INPUT_PANEL_STATE_WILL_HIDE);
            // Instead send HIDE signal
            vconf_set_int (VCONFKEY_ISF_INPUT_PANEL_STATE, VCONFKEY_ISF_INPUT_PANEL_STATE_WILL_HIDE);
        } else if (ev->message_type == ECORE_X_ATOM_E_WINDOW_ROTATION_CHANGE_PREPARE) {
            /* WMSYNC, #10 Register size hints for candidate window and set conformant geometry */
            // PRE_ROTATE_DONE Ack to WM
            _candidate_angle = ev->data.l[1];
            _ise_angle = ev->data.l[1];
            LOGD ("ECORE_X_ATOM_E_WINDOW_ROTATION_CHANGE_PREPARE : %d\n", _candidate_angle);

            if (_candidate_angle == 90 || _candidate_angle == 270) {
                ui_candidate_window_resize (_candidate_land_width, _candidate_land_height_min);
            } else {
                ui_candidate_window_resize (_candidate_port_width, _candidate_port_height_min);
            }
            if (_ise_state == WINDOW_STATE_SHOW) {
                set_keyboard_geometry_atom_info (_app_window, get_ise_geometry ());
                _panel_agent->update_input_panel_event (ECORE_IMF_INPUT_PANEL_GEOMETRY_EVENT, 0);
            }
            ui_settle_candidate_window ();
            ui_candidate_window_rotate (_candidate_angle);
            Ecore_X_Window root_window = ecore_x_window_root_get (_control_window);
            LOGD ("ecore_x_e_window_rotation_change_prepare_done_send (%d)\n", _candidate_angle);
            ecore_x_e_window_rotation_change_prepare_done_send (root_window,
                    _control_window, _candidate_angle);
        } else if (ev->message_type == ECORE_X_ATOM_E_WINDOW_ROTATION_CHANGE_REQUEST) {
            int ise_angle = (int)ev->data.l[1];
            LOGD ("ECORE_X_ATOM_E_WINDOW_ROTATION_CHANGE_REQUEST for ISE WINDOW : ISE angle : %d, Candidate angle : %d\n", ise_angle, _candidate_angle);
            _candidate_angle = ise_angle;
            _ise_angle = ise_angle;
            if (_ise_state == WINDOW_STATE_SHOW) {
                set_keyboard_geometry_atom_info (_app_window, get_ise_geometry ());
                _panel_agent->update_input_panel_event (ECORE_IMF_INPUT_PANEL_GEOMETRY_EVENT, 0);
                ui_settle_candidate_window ();
            }
        }
    } else if (ev->win == elm_win_xwindow_get (_candidate_window)) {
        if (ev->message_type == ECORE_X_ATOM_E_WINDOW_ROTATION_CHANGE_REQUEST || ev->message_type == ECORE_X_ATOM_E_ILLUME_ROTATE_ROOT_ANGLE) {
            /* WMSYNC, #11 Actual rotate the candidate window */
            if (ev->message_type == ECORE_X_ATOM_E_WINDOW_ROTATION_CHANGE_REQUEST) {
                _candidate_angle = (int)ev->data.l[1];
                ui_candidate_window_rotate (_candidate_angle);
                LOGD ("ECORE_X_ATOM_E_WINDOW_ROTATION_CHANGE_REQUEST : %d\n", _candidate_angle);
            } else if (ev->message_type == ECORE_X_ATOM_E_ILLUME_ROTATE_ROOT_ANGLE &&
                _ise_state != WINDOW_STATE_SHOW) {
                ecore_x_e_window_rotation_app_set (elm_win_xwindow_get (_candidate_window), EINA_TRUE);
                _candidate_angle = (int)ev->data.l[0];
                if (_candidate_angle == 90 || _candidate_angle == 270) {
                    evas_object_resize (_candidate_window, _candidate_land_width,_candidate_land_height_min);
                } else {
                    evas_object_resize (_candidate_window, _candidate_port_width,_candidate_port_height_min);
                }
                ui_candidate_window_rotate (_candidate_angle);
                ui_settle_candidate_window ();
                ecore_x_e_window_rotation_app_set (elm_win_xwindow_get (_candidate_window), EINA_FALSE);
                LOGD ("ECORE_X_ATOM_E_ILLUME_ROTATE_ROOT_ANGLE : %d\n", _candidate_angle);
            }
            SCIM_DEBUG_MAIN (3) << __FUNCTION__ << " : ANGLE (" << _candidate_angle << ")\n";
        }
    }

    /* Screen reader feature */
    if (ev->message_type == ECORE_X_ATOM_E_ILLUME_ACCESS_CONTROL) {
        static int last_pos_x = -10000;
        static int last_pos_y = -10000;

        if (_candidate_window) {
            if ((unsigned int)ev->data.l[0] == elm_win_xwindow_get (_candidate_window)) {
                if ((unsigned int)ev->data.l[1] == ECORE_X_ATOM_E_ILLUME_ACCESS_ACTION_ACTIVATE) {
                    // 1 finger double tap
                    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "    1 finger double tap focus index = " << _candidate_tts_focus_index << "\n";
                    ui_mouse_click (_candidate_tts_focus_index);
                } else if ((unsigned int)ev->data.l[1] == ECORE_X_ATOM_E_ILLUME_ACCESS_ACTION_READ) {
                    // 1 finger tap
                    // 1 finger touch & move
                    last_pos_x = ev->data.l[2];
                    last_pos_y = ev->data.l[3];
                    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "    1 finger touch & move (" << last_pos_x << ", " << last_pos_y << ")\n";
                    ui_mouse_over (last_pos_x, last_pos_y);
                } else if ((unsigned int)ev->data.l[1] == ECORE_X_ATOM_E_ILLUME_ACCESS_ACTION_READ_NEXT ||
                           (unsigned int)ev->data.l[1] == ECORE_X_ATOM_E_ILLUME_ACCESS_ACTION_READ_PREV) {
                    if ((unsigned int)ev->data.l[1] == ECORE_X_ATOM_E_ILLUME_ACCESS_ACTION_READ_NEXT) {
                        // flick right
                        SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "    1 finger flick right\n";
                        if (evas_object_visible_get (_more_btn) && _candidate_tts_focus_index == (int)(_candidate_display_number - 1))
                            _candidate_tts_focus_index = _candidate_display_number == _candidate_row_items[0] ? MORE_BUTTON_INDEX : 0;
                        else if (evas_object_visible_get (_more_btn) && _candidate_tts_focus_index == (int)(_candidate_row_items[0] - 1))
                            _candidate_tts_focus_index = MORE_BUTTON_INDEX;
                        else if (evas_object_visible_get (_close_btn) && _candidate_tts_focus_index == (int)(_candidate_row_items[0] - 1))
                            _candidate_tts_focus_index = CLOSE_BUTTON_INDEX;
                        else if (_candidate_tts_focus_index == MORE_BUTTON_INDEX)
                            _candidate_tts_focus_index = _candidate_display_number == _candidate_row_items[0] ? 0 : _candidate_row_items[0];
                        else if (_candidate_tts_focus_index == CLOSE_BUTTON_INDEX)
                            _candidate_tts_focus_index = _candidate_row_items[0];
                        else if (_candidate_tts_focus_index >= 0 && _candidate_tts_focus_index < (g_isf_candidate_table.get_current_page_size () - 1))
                            _candidate_tts_focus_index++;
                        else if (_candidate_tts_focus_index == (g_isf_candidate_table.get_current_page_size () - 1))
                            _candidate_tts_focus_index = 0;
                    } else {
                        // flick left
                        SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "    1 finger flick left\n";
                        if (evas_object_visible_get (_more_btn) && _candidate_tts_focus_index == 0)
                            _candidate_tts_focus_index = _candidate_display_number == _candidate_row_items[0] ? MORE_BUTTON_INDEX : _candidate_display_number - 1;
                        else if (evas_object_visible_get (_more_btn) && _candidate_tts_focus_index == (int)_candidate_row_items[0])
                            _candidate_tts_focus_index = MORE_BUTTON_INDEX;
                        else if (evas_object_visible_get (_close_btn) && _candidate_tts_focus_index == (int)_candidate_row_items[0])
                            _candidate_tts_focus_index = CLOSE_BUTTON_INDEX;
                        else if (_candidate_tts_focus_index == MORE_BUTTON_INDEX)
                            _candidate_tts_focus_index = _candidate_row_items[0] - 1;
                        else if (_candidate_tts_focus_index == CLOSE_BUTTON_INDEX)
                            _candidate_tts_focus_index = _candidate_row_items[0] - 1;
                        else if (_candidate_tts_focus_index > 0 && _candidate_tts_focus_index < g_isf_candidate_table.get_current_page_size ())
                            _candidate_tts_focus_index--;
                        else if (_candidate_tts_focus_index == 0)
                            _candidate_tts_focus_index = g_isf_candidate_table.get_current_page_size () - 1;
                    }

                    int x = 0, y = 0, w = 0, h = 0;
                    _wait_stop_event = false;
                    if (candidate_expanded) {
                        int total = 0;
                        int cursor_line = 0;
                        for (unsigned int i = 0; i < _candidate_row_items.size (); i++) {
                            total += _candidate_row_items [i];
                            if (total > (int)_candidate_display_number && _candidate_tts_focus_index >= total)
                                cursor_line++;
                        }

                        elm_scroller_region_get (_candidate_area_2, &x, &y, &w, &h);

                        int line_h   = _item_min_height + _v_padding;
                        int cursor_y = cursor_line * line_h;
                        if (cursor_y < y) {
                            elm_scroller_region_bring_in (_candidate_area_2, 0, cursor_y, w, h);
                            _wait_stop_event = true;
                        } else if (cursor_y >= y + h) {
                            elm_scroller_region_bring_in (_candidate_area_2, 0, cursor_y + line_h - h, w, h);
                            _wait_stop_event = true;
                        }
                    }

                    x = y = w = h = 0;
                    String strTts = String ("");
                    if (_candidate_tts_focus_index >= 0 && _candidate_tts_focus_index < g_isf_candidate_table.get_current_page_size ()) {
                        strTts = utf8_wcstombs (g_isf_candidate_table.get_candidate_in_current_page (_candidate_tts_focus_index));
                        if (_candidate_0 [_candidate_tts_focus_index])
                            evas_object_geometry_get (_candidate_0 [_candidate_tts_focus_index], &x, &y, &w, &h);
                    } else if (_candidate_tts_focus_index == MORE_BUTTON_INDEX) {
                        strTts = String (_("more button"));
                        evas_object_geometry_get (_more_btn, &x, &y, &w, &h);
                    } else if (_candidate_tts_focus_index == CLOSE_BUTTON_INDEX) {
                        strTts = String (_("close button"));
                        evas_object_geometry_get (_close_btn, &x, &y, &w, &h);
                    } else {
                        LOGW ("TTS focus index = %d", _candidate_tts_focus_index);
                        ui_tts_focus_rect_hide ();
                    }

                    if (strTts.length () > 0)
                        ui_play_tts (strTts.c_str ());
                    if (w > 0 && h > 0) {
                        if (!_wait_stop_event)
                            ui_tts_focus_rect_show (x, y, w, h);
                        else
                            ui_tts_focus_rect_hide ();
                    }
                }
            }
        }
    }

    return ECORE_CALLBACK_RENEW;
}

Eina_Bool check_focus_out_by_popup_win ()
{
    Ecore_X_Window focus_win = ecore_x_window_focus_get ();
    Eina_Bool ret = EINA_FALSE;
    Ecore_X_Window_Type win_type = ECORE_X_WINDOW_TYPE_UNKNOWN;

    if (!ecore_x_netwm_window_type_get (focus_win, &win_type))
        return EINA_FALSE;

    LOGD ("win type : %d\n", win_type);

    if (win_type == ECORE_X_WINDOW_TYPE_POPUP_MENU ||
        win_type == ECORE_X_WINDOW_TYPE_NOTIFICATION) {
        ret = EINA_TRUE;
    }

    return ret;
}

/**
 * @brief Callback function for focus out event of application window
 *
 * @param data Data to pass when it is called.
 *
 * @return ECORE_CALLBACK_RENEW
 */
static Eina_Bool x_event_window_focus_out_cb (void *data, int ev_type, void *event)
{
    Ecore_X_Event_Window_Focus_Out *e = (Ecore_X_Event_Window_Focus_Out*)event;

    if (e && e->win == _app_window) {
        if (_panel_agent->get_current_toolbar_mode () == TOOLBAR_HELPER_MODE) {
            if (check_focus_out_by_popup_win ())
                return ECORE_CALLBACK_RENEW;

#if ENABLE_MULTIWINDOW_SUPPORT
            unsigned int layout = 0;
            LOGD ("Application window focus OUT!\n");
            delete_ise_hide_timer ();

            // Check multi window mode
            if (ecore_x_window_prop_card32_get (efl_get_app_window (), ECORE_X_ATOM_E_WINDOW_DESKTOP_LAYOUT, &layout, 1) != -1) {
                if (layout == 0 || layout == 1) {
                    // Split mode
                    LOGD ("Multi window mode. start timer to hide IME\n");

                    // Use timer not to hide and show IME again in focus-out and focus-in event between applications
                    _ise_hide_timer = ecore_timer_add (ISF_ISE_HIDE_DELAY, ise_hide_timeout, NULL);
                }
            }

            if (!_ise_hide_timer) {
                LOGD ("Panel hides ISE\n");
                _panel_agent->hide_helper (_panel_agent->get_current_helper_uuid ());
                slot_hide_ise ();
                ui_candidate_hide (true, false, false);
            }
#else
            LOGD ("Application window focus OUT! Panel hides ISE");
            _panel_agent->hide_helper (_panel_agent->get_current_helper_uuid ());
            slot_hide_ise ();
            ui_candidate_hide (true, false, false);
#endif
        }
    }

    return ECORE_CALLBACK_RENEW;
}

/**
 * @brief : Checks whether the window manager is launched or not
 * @return true if window manager launched, else false
 */
static bool check_wm_ready (void)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "...\n";

#ifdef WAIT_WM
    int try_count = 0;
    while (check_file (ISF_SYSTEM_WM_READY_FILE) == false) {
        if (ISF_SYSTEM_WM_WAIT_COUNT <= (try_count++)) return false;
        usleep (ISF_SYSTEM_WAIT_DELAY);
    }
#endif

    return true;
}

/**
 * @brief : Launches default soft keyboard for performance enhancement (It's not mandatory)
 */
static void launch_default_soft_keyboard (keynode_t *key, void* data)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "...\n";

    /* Start default ISE */
    change_keyboard_mode (TOOLBAR_HELPER_MODE);
}

static String sanitize_string (const char *str, int maxlen = 32)
{
    String ret;
    static char acceptables[] =
        "abcdefghijklmnopqrstuvwxyz"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "1234567890_-.@:";

    char *newstr = NULL;
    if (maxlen > 0) {
        newstr = new char[maxlen + 1];
    }
    int len = 0;
    if (newstr) {
        memset (newstr, 0x00, sizeof (char) * (maxlen + 1));

        if (str) {
            while (len < maxlen && str[len] != '\0' && strchr (acceptables, str[len]) != NULL) {
                newstr[len] = str[len];
                len++;
            }
            ret = newstr;
        }
        delete [] newstr;
    }
    return ret;
}

int main (int argc, char *argv [])
{
    struct tms    tiks_buf;
    _clock_start = times (&tiks_buf);

    int           i;
    int           ret             = 0;

    bool          daemon          = false;
    bool          should_resident = true;

    int           new_argc        = 0;
    char        **new_argv        = new char * [40];
    int           display_name_c  = 0;
    ConfigModule *config_module   = NULL;
    String        config_name     = String ("socket");
    String        display_name    = String ();
    char          buf[256]        = {0};

    Ecore_Fd_Handler *panel_agent_read_handler = NULL;
    Ecore_Fd_Handler *helper_manager_handler   = NULL;
    Ecore_Event_Handler *xclient_message_handler  = NULL;
    Ecore_Event_Handler *xwindow_property_handler = NULL;
    Ecore_Event_Handler *xwindow_focus_out_handler = NULL;

    perm_app_set_privilege ("isf", NULL, NULL);

    check_time ("\nStarting ISF Panel EFL...... ");
    ISF_SAVE_LOG ("Starting ISF Panel EFL......\n");

    DebugOutput::disable_debug (SCIM_DEBUG_AllMask);
    DebugOutput::enable_debug (SCIM_DEBUG_MainMask);

    /* Parse command options */
    i = 0;
    while (i < argc) {
        if (++i >= argc)
            break;

        if (String ("-c") == argv [i] || String ("--config") == argv [i]) {
            if (++i >= argc) {
                std::cerr << "no argument for option " << argv [i-1] << "\n";
                ret = -1;
                goto cleanup;
            }
            config_name = argv [i];
            continue;
        }

        if (String ("-h") == argv [i] || String ("--help") == argv [i]) {
            std::cout << "Usage: " << argv [0] << " [option]...\n\n"
                 << "The options are: \n"
                 << "  --display DISPLAY    Run on display DISPLAY.\n"
                 << "  -c, --config NAME    Uses specified Config module.\n"
                 << "  -d, --daemon         Run " << argv [0] << " as a daemon.\n"
                 << "  -ns, --no-stay       Quit if no connected client.\n"
#if ENABLE_DEBUG
                 << "  -v, --verbose LEVEL  Enable debug info, to specific LEVEL.\n"
                 << "  -o, --output FILE    Output debug information into FILE.\n"
#endif
                 << "  -h, --help           Show this help message.\n";
            delete []new_argv;
            return 0;
        }

        if (String ("-d") == argv [i] || String ("--daemon") == argv [i]) {
            daemon = true;
            continue;
        }

        if (String ("-ns") == argv [i] || String ("--no-stay") == argv [i]) {
            should_resident = false;
            continue;
        }

        if (String ("-v") == argv [i] || String ("--verbose") == argv [i]) {
            if (++i >= argc) {
                std::cerr << "no argument for option " << argv [i-1] << "\n";
                ret = -1;
                goto cleanup;
            }
            DebugOutput::set_verbose_level (atoi (argv [i]));
            continue;
        }

        if (String ("-o") == argv [i] || String ("--output") == argv [i]) {
            if (++i >= argc) {
                std::cerr << "No argument for option " << argv [i-1] << "\n";
                ret = -1;
                goto cleanup;
            }
            DebugOutput::set_output (argv [i]);
            continue;
        }

        if (String ("--display") == argv [i]) {
            if (++i >= argc) {
                std::cerr << "No argument for option " << argv [i-1] << "\n";
                ret = -1;
                goto cleanup;
            }
            display_name = sanitize_string (argv [i]);
            continue;
        }

        if (String ("--") == argv [i])
            break;

        std::cerr << "Invalid command line option: " << argv [i] << "\n";
        delete []new_argv;
        return 0;
    } /* End of command line parsing. */

    new_argv [new_argc ++] = argv [0];

    /* Store the rest argvs into new_argv. */
    for (++i; i < argc && new_argc < 37; ++i) {
        new_argv [new_argc ++] = argv [i];
    }

    /* Make up DISPLAY env. */
    if (display_name.length ()) {
        new_argv [new_argc ++] = const_cast <char*> ("--display");
        display_name_c = new_argc;
        new_argv [new_argc ++] = strdup (display_name.c_str ());

        setenv ("DISPLAY", display_name.c_str (), 1);
    }

    new_argv [new_argc] = 0;

    if (!config_name.length ()) {
        std::cerr << "No Config module is available!\n";
        ret = -1;
        goto cleanup;
    }

    if (config_name != "dummy") {
        /* Load config module */
        config_module = new ConfigModule (config_name);

        if (!config_module || !config_module->valid ()) {
            std::cerr << "Can not load " << config_name << " Config module.\n";
            ret = -1;
            goto cleanup;
        }
    } else {
        _config = new DummyConfig ();
    }

    /* Get current display. */
    {
        const char *p = getenv ("DISPLAY");
        if (p)
            display_name = String (p);
    }

    snprintf (buf, sizeof (buf), "config_name=%s display_name=%s", config_name.c_str (), display_name.c_str ());
    check_time (buf);

    try {
        if (!initialize_panel_agent (config_name, display_name, should_resident)) {
            check_time ("Failed to initialize Panel Agent!");
            std::cerr << "Failed to initialize Panel Agent!\n";
            ISF_SAVE_LOG ("Failed to initialize Panel Agent!\n");

            ret = -1;
            goto cleanup;
        }
    } catch (scim::Exception & e) {
        std::cerr << e.what () << "\n";
        ret = -1;
        goto cleanup;
    }
    check_time ("initialize_panel_agent");

    /* Create config instance */
    if (_config.null () && config_module && config_module->valid ())
        _config = config_module->create_config ();
    if (_config.null ()) {
        std::cerr << "Failed to create Config instance from " << config_name << " Config module.\n";
        ret = -1;
        goto cleanup;
    }
    check_time ("create config instance");

    /* Initialize global variables and pointers for candidate items and etc. */
    for (i = 0; i < SCIM_LOOKUP_TABLE_MAX_PAGESIZE; i++) {
        _candidate_0 [i]     = NULL;
        _seperate_0 [i]      = NULL;
        _seperate_items [i]  = NULL;
        _line_0 [i]          = NULL;
        _line_items [i]      = NULL;
        _candidate_text [i]  = NULL;
        _candidate_image [i] = NULL;
        _candidate_pop_image [i] = NULL;
    }

    try {
        _panel_agent->send_display_name (display_name);
    } catch (scim::Exception & e) {
        std::cerr << e.what () << "\n";
        ret = -1;
        goto cleanup;
    }

    if (daemon) {
        check_time ("ISF Panel EFL run as daemon");
        scim_daemon ();
    }

    /* Connect the configuration reload signal. */
    _config->signal_connect_reload (slot (config_reload_cb));

    if (!check_wm_ready ()) {
        std::cerr << "[ISF-PANEL-EFL] WM ready timeout\n";
        ISF_SAVE_LOG ("Window Manager ready timeout\n");
    } else {
        LOGD ("Window Manager is in ready state\n");
    }

    elm_init (argc, argv);
    check_time ("elm_init");

    elm_policy_set (ELM_POLICY_THROTTLE, ELM_POLICY_THROTTLE_NEVER);

    if (!efl_create_control_window ()) {
        LOGW ("Failed to create control window\n");
        goto cleanup;
    }

    efl_get_screen_resolution (_screen_width, _screen_height);

    _width_rate       = (float)(_screen_width / 720.0);
    _height_rate      = (float)(_screen_height / 1280.0);
    _blank_width      = (int)(_blank_width * _width_rate);
    _item_min_width   = (int)(_item_min_width * _width_rate);
    _item_min_height  = (int)(_item_min_height * _height_rate);
    _candidate_width  = (int)(_candidate_port_width * _width_rate);
    _candidate_height = (int)(_candidate_port_height_min * _height_rate);
    _indicator_height = (int)(_indicator_height * _height_rate);

    _aux_font_size       = (int)(_aux_font_size * (_width_rate < _height_rate ? _width_rate : _height_rate));
    _candidate_font_size = (int)(_candidate_font_size * (_width_rate < _height_rate ? _width_rate : _height_rate));

    /* Load ISF configuration */
    load_config ();
    check_time ("load_config");

    helper_manager_handler   = ecore_main_fd_handler_add (_panel_agent->get_helper_manager_id (), ECORE_FD_READ, helper_manager_input_handler, NULL, NULL, NULL);
    panel_agent_read_handler = ecore_main_fd_handler_add (_panel_agent->get_server_id (), ECORE_FD_READ, panel_agent_handler, NULL, NULL, NULL);
    _read_handler_list.push_back (panel_agent_read_handler);
    check_time ("run_panel_agent");

    set_language_and_locale ();

#if HAVE_VCONF
    /* Add callback function for input language and display language */
    vconf_notify_key_changed (VCONFKEY_LANGSET, display_language_changed_cb, NULL);
#endif

    if (0 != register_edbus_signal_handler ())
        LOGW ("register edbus signal fail");

    try {
        /* Update ISE list */
        std::vector<String> list;
        update_ise_list (list);

        /* Load initial ISE information */
        _initial_ise_uuid = scim_global_config_read (String (SCIM_GLOBAL_CONFIG_INITIAL_ISE_UUID), String (SCIM_COMPOSE_KEY_FACTORY_UUID));

        /* Check if SCIM_CONFIG_DEFAULT_HELPER_ISE is available. If it's not, set it as _initial_ise_uuid.
           e.g., This might be necessary when the platform is upgraded from 2.3 to 2.4. */
        String helper_uuid = _config->read (SCIM_CONFIG_DEFAULT_HELPER_ISE, String (""));
        if (helper_uuid.length() > 0 && _initial_ise_uuid.length() > 0 && helper_uuid != _initial_ise_uuid) {
            bool match = false;
            for (unsigned int u = 0; u < _ime_info.size (); u++) {
                if (_ime_info[u].mode == TOOLBAR_HELPER_MODE && helper_uuid == _ime_info[u].appid) {
                    match = true;
                    break;
                }
            }
            if (!match) {
                _config->write (String (SCIM_CONFIG_DEFAULT_HELPER_ISE), _initial_ise_uuid);
            }
        }

        /* Launches default soft keyboard when all conditions are satisfied */
        launch_default_soft_keyboard ();

        /* Update the name of each ISE according to display language */
        update_ise_locale ();
    } catch (scim::Exception & e) {
        std::cerr << e.what () << "\n";
    }

    xclient_message_handler  = ecore_event_handler_add (ECORE_X_EVENT_CLIENT_MESSAGE, x_event_client_message_cb, NULL);
    xwindow_property_handler = ecore_event_handler_add (ECORE_X_EVENT_WINDOW_PROPERTY, x_event_window_property_cb, NULL);
    xwindow_focus_out_handler = ecore_event_handler_add (ECORE_X_EVENT_WINDOW_FOCUS_OUT, x_event_window_focus_out_cb, NULL);

#if HAVE_BLUETOOTH
    /* Register the callback function of Bluetooth connection */
     ret = bt_initialize ();
     if (ret != BT_ERROR_NONE)
         LOGW ("Fail to init Bluetooth");

     ret = bt_hid_host_initialize (_bt_cb_hid_state_changed, NULL);
     if (ret != BT_ERROR_NONE)
         LOGW ("bt_hid_host_initialize failed");
#endif

    _system_scale = elm_config_scale_get ();

    /* Set elementary scale */
    if (_screen_width) {
        _app_scale = _screen_width / 720.0;
        elm_config_scale_set (_app_scale);
        char buf[16] = {0};
        snprintf (buf, sizeof (buf), "%4.3f", _app_scale);
        setenv ("ELM_SCALE", buf, 1);
    }

    signal (SIGQUIT, signalhandler);
    signal (SIGTERM, signalhandler);
    signal (SIGINT,  signalhandler);
    signal (SIGHUP,  signalhandler);

    check_time ("EFL Panel launch time");

    elm_run ();

    _config->flush ();
    ret = 0;

#if HAVE_BLUETOOTH
    /* deinitialize the callback function of Bluetooth connection */
    ret = bt_hid_host_deinitialize ();
    if (ret != BT_ERROR_NONE)
        LOGW ("bt_hid_host_deinitialize failed: %d", ret);

    ret = bt_deinitialize ();
    if (ret != BT_ERROR_NONE)
        LOGW ("bt_deinitialize failed: %d", ret);
#endif

    if (xclient_message_handler) {
        ecore_event_handler_del (xclient_message_handler);
        xclient_message_handler = NULL;
    }

    if (xwindow_property_handler) {
        ecore_event_handler_del (xwindow_property_handler);
        xwindow_property_handler = NULL;
    }

    if (xwindow_focus_out_handler) {
        ecore_event_handler_del (xwindow_focus_out_handler);
        xwindow_focus_out_handler = NULL;
    }

    if (helper_manager_handler) {
        ecore_main_fd_handler_del (helper_manager_handler);
        helper_manager_handler = NULL;
    }

    for (unsigned int ii = 0; ii < _read_handler_list.size (); ++ii) {
        ecore_main_fd_handler_del (_read_handler_list[ii]);
    }
    _read_handler_list.clear ();

#if HAVE_VCONF
    /* Remove callback function for input language and display language */
    vconf_ignore_key_changed (VCONFKEY_LANGSET, display_language_changed_cb);
#endif

cleanup:
    ui_candidate_delete_check_size_timer ();
    ui_candidate_delete_longpress_timer ();
    ui_candidate_delete_destroy_timer ();
#if HAVE_PKGMGR_INFO
    if (pkgmgr) {
        package_manager_destroy (pkgmgr);
        pkgmgr = NULL;
    }
#endif
    ui_close_tts ();

    unregister_edbus_signal_handler ();

    if (!_config.null ())
        _config.reset ();
    if (config_module)
        delete config_module;
    if (_panel_agent) {
        try {
            _panel_agent->stop ();
        } catch (scim::Exception & e) {
            std::cerr << "Exception is thrown from _panel_agent->stop (), error is " << e.what () << "\n";
        }
        delete _panel_agent;
    }
    if ((display_name_c > 0) && new_argv [display_name_c]) {
        free (new_argv [display_name_c]);
    }
    delete []new_argv;

    ISF_SAVE_LOG ("ret=%d\n", ret);
    if (ret == 0) {
        std::cerr << "Successfully exited.\n";
        return 0;
    } else {
        std::cerr << "Abnormally exited.\n";
        return -1;
    }
}

/*
vi:ts=4:nowrap:expandtab
*/
