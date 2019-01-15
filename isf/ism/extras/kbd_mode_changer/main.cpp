/*
 * ISF(Input Service Framework)
 *
 * ISF is based on SCIM 1.4.7 and extended for supporting more mobile fitable.
 * Copyright (c) 2012-2014 Samsung Electronics Co., Ltd.
 *
 * Contact: Wonkeun Oh <wonkeun.oh@samsung.com>, Jihoon Kim <jihoon48.kim@samsung.com>
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

#include <Ecore_X.h>
#include <Elementary.h>
#include "main.h"

typedef enum
{
    TOOLBAR_KEYBOARD_MODE = 0,  /* Hardware keyboard ISE */
    TOOLBAR_HELPER_MODE         /* Software keyboard ISE */
} TOOLBAR_MODE_T;

#define E_PROP_DEVICEMGR_INPUTWIN                        "DeviceMgr Input Window"
#define E_PROP_DEVICEMGR_CONTROLWIN                      "_ISF_CONTROL_WINDOW"
#define PROP_X_EXT_KEYBOARD_INPUT_DETECTED               "HW Keyboard Input Started"
#define PROP_X_EXT_KEYBOARD_EXIST                        "X External Keyboard Exist"

bool app_create (void *user_data)
{
    LOGD ("app create");
    return true;
}

void app_control (app_control_h app_control, void *user_data)
{
    LOGD ("%s", __func__);

    Ecore_X_Atom       prop_x_keyboard_input_detected = 0;
    TOOLBAR_MODE_T     kbd_mode = TOOLBAR_HELPER_MODE;
    Ecore_X_Window     _control_win = 0;
    Ecore_X_Window     _input_win = 0;
    unsigned int val = 0;

    LOGD ("isf_extra_hwkbd_module start");

    Ecore_X_Atom atom = ecore_x_atom_get (E_PROP_DEVICEMGR_CONTROLWIN);
    if (ecore_x_window_prop_xid_get (ecore_x_window_root_first_get (), atom, ECORE_X_ATOM_WINDOW, &_control_win, 1) >= 0) {
        if (!prop_x_keyboard_input_detected)
            prop_x_keyboard_input_detected = ecore_x_atom_get (PROP_X_EXT_KEYBOARD_INPUT_DETECTED);

        if (ecore_x_window_prop_card32_get (_control_win, prop_x_keyboard_input_detected, &val, 1) > 0) {
            if (val == 1) {
                kbd_mode = TOOLBAR_KEYBOARD_MODE;
            } else {
                kbd_mode = TOOLBAR_HELPER_MODE;
            }
        } else {
            kbd_mode = TOOLBAR_HELPER_MODE;
        }

        // get the input window
        atom = ecore_x_atom_get (E_PROP_DEVICEMGR_INPUTWIN);
        if (ecore_x_window_prop_xid_get (ecore_x_window_root_first_get (), atom, ECORE_X_ATOM_WINDOW, &_input_win, 1) >= 0) {
            //Set the window property value;
            if (kbd_mode == TOOLBAR_KEYBOARD_MODE) {
                val = 0;
                ecore_x_window_prop_card32_set (_input_win, ecore_x_atom_get (PROP_X_EXT_KEYBOARD_EXIST), &val, 1);
                LOGD ("keyboard mode is changed HW -> SW by isf-kbd-mode-changer");
            }
        }
    }

    ui_app_exit ();
}

void app_terminate (void *user_data)
{
    LOGD ("app terminated");
}

int main (int argc, char *argv [])
{
    ui_app_lifecycle_callback_s event_callback = {0,};

    event_callback.create = app_create;
    event_callback.terminate = app_terminate;
    event_callback.app_control = app_control;

    LOGD ("start org.tizen.isf-kbd-mode-changer");

    return ui_app_main (argc, argv, &event_callback, NULL);
}
