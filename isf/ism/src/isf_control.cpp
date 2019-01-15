/*
 * ISF(Input Service Framework)
 *
 * ISF is based on SCIM 1.4.7 and extended for supporting more mobile fitable.
 * Copyright (c) 2012-2014 Samsung Electronics Co., Ltd.
 *
 * Contact: Haifeng Deng <haifeng.deng@samsung.com>, Hengliang Luo <hl.luo@samsung.com>
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
#define Uses_SCIM_TRANSACTION
#define Uses_ISF_IMCONTROL_CLIENT
#define Uses_SCIM_COMPOSE_KEY
#define Uses_SCIM_PANEL_AGENT

#include <string.h>
#include "scim.h"
#include "isf_control.h"

#include <dlog.h>
#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG "ISF_CONTROL"

using namespace scim;


EXAPI int isf_control_set_active_ise_by_uuid (const char *uuid)
{
    if (uuid == NULL)
        return -1;

    IMControlClient imcontrol_client;
    int ret = 0;
    if (!imcontrol_client.open_connection ())
        return -1;

    imcontrol_client.prepare ();
    if (!imcontrol_client.set_active_ise_by_uuid (uuid))
        ret = -1;

    imcontrol_client.close_connection ();

    return ret;
}

EXAPI int isf_control_get_active_ise (char **uuid)
{
    if (uuid == NULL)
        return -1;

    String strUuid;
    IMControlClient imcontrol_client;
    int ret = 0;

    if (!imcontrol_client.open_connection ())
        return -1;

    imcontrol_client.prepare ();
    if (!imcontrol_client.get_active_ise (strUuid)) {
        ret = -1;
    }

    imcontrol_client.close_connection ();

    if (ret == -1)
        return -1;

    *uuid = strUuid.length () ? strdup (strUuid.c_str ()) : strdup ("");

    return strUuid.length ();
}

EXAPI int isf_control_get_ise_list (char ***uuid_list)
{
    if (uuid_list == NULL)
        return -1;

    int count;
    IMControlClient imcontrol_client;

    if (!imcontrol_client.open_connection ())
        return -1;

    imcontrol_client.prepare ();
    if (!imcontrol_client.get_ise_list (&count, uuid_list))
        count = -1;

    imcontrol_client.close_connection ();

    return count;
}

EXAPI int isf_control_get_ise_info (const char *uuid, char **name, char **language, ISE_TYPE_T *type, int *option)
{
    return isf_control_get_ise_info_and_module_name (uuid, name, language, type, option, NULL);
}

EXAPI int isf_control_get_ise_info_and_module_name (const char *uuid, char **name, char **language, ISE_TYPE_T *type, int *option, char **module_name)
{
    if (uuid == NULL)
        return -1;

    int nType   = 0;
    int nOption = 0;
    int ret = 0;
    String strName, strLanguage, strModuleName;

    IMControlClient imcontrol_client;
    if (!imcontrol_client.open_connection ())
        return -1;

    imcontrol_client.prepare ();
    if (!imcontrol_client.get_ise_info (uuid, strName, strLanguage, nType, nOption, strModuleName)) {
        ret = -1;
    }

    imcontrol_client.close_connection ();

    if (ret == -1)
        return ret;

    if (name != NULL)
        *name = strName.length () ? strdup (strName.c_str ()) : strdup ("");

    if (language != NULL)
        *language = strLanguage.length () ? strdup (strLanguage.c_str ()) : strdup ("");

    if (type != NULL)
        *type = (ISE_TYPE_T)nType;

    if (option != NULL)
        *option = nOption;

    if (module_name != NULL)
        *module_name = strModuleName.length () ? strdup (strModuleName.c_str ()) : strdup ("");

    return ret;
}

EXAPI int isf_control_set_active_ise_to_default (void)
{
    IMControlClient imcontrol_client;
    int ret = 0;

    if (!imcontrol_client.open_connection ())
        return -1;

    imcontrol_client.prepare ();
    imcontrol_client.set_active_ise_to_default ();
    if (!imcontrol_client.send ())
        ret = -1;

    imcontrol_client.close_connection ();

    return ret;
}

EXAPI int isf_control_reset_ise_option (void)
{
    IMControlClient imcontrol_client;
    int ret = 0;
    if (!imcontrol_client.open_connection ())
        return -1;

    imcontrol_client.prepare ();
    if (!imcontrol_client.reset_ise_option ())
        ret = -1;

    imcontrol_client.close_connection ();

    return ret;
}

EXAPI int isf_control_set_initial_ise_by_uuid (const char *uuid)
{
    if (uuid == NULL)
        return -1;

    int ret = 0;
    IMControlClient imcontrol_client;
    if (!imcontrol_client.open_connection ())
        return -1;

    imcontrol_client.prepare ();
    if (!imcontrol_client.set_initial_ise_by_uuid (uuid))
        ret = -1;

    imcontrol_client.close_connection ();

    return ret;
}

EXAPI int isf_control_get_initial_ise (char **uuid)
{
    if (uuid == NULL)
        return -1;

    String strUuid = scim_global_config_read (String (SCIM_GLOBAL_CONFIG_INITIAL_ISE_UUID), String (SCIM_COMPOSE_KEY_FACTORY_UUID));

    *uuid = strUuid.length () ? strdup (strUuid.c_str ()) : strdup ("");

    return strUuid.length ();
}

EXAPI int isf_control_show_ise_selector (void)
{
    return isf_control_show_ime_selector();
}

EXAPI int isf_control_get_ise_count (ISE_TYPE_T type)
{
    char **iselist = NULL;
    int all_ise_count, ise_count = 0;
    ISE_TYPE_T isetype;

    all_ise_count = isf_control_get_ise_list (&iselist);
    if (all_ise_count < 0) {
        if (iselist)
            free (iselist);
        return -1;
    }

    for (int i = 0; i < all_ise_count; i++) {
        if (iselist[i]) {
            isf_control_get_ise_info (iselist[i], NULL, NULL, &isetype, NULL);
            if (isetype == type) {
                ise_count++;
            }

            free (iselist[i]);
        }
    }

    if (iselist)
        free (iselist);

    return ise_count;
}

EXAPI int isf_control_show_ise_option_window (void)
{
    IMControlClient imcontrol_client;
    int ret = 0;
    if (!imcontrol_client.open_connection ())
        return -1;

    imcontrol_client.prepare ();
    imcontrol_client.show_ise_option_window ();
    if (!imcontrol_client.send ())
        ret = -1;

    imcontrol_client.close_connection ();

    return ret;
}

EXAPI int isf_control_get_all_ime_info (ime_info_s **info)
{
    int count = -1, i = 0;
    int ret = 0;
    HELPER_ISE_INFO helper_info;
    ime_info_s *ime_info = NULL;

    if (info == NULL)
        return count;

    IMControlClient imcontrol_client;
    if (!imcontrol_client.open_connection ())
        return -1;

    imcontrol_client.prepare ();
    if (!imcontrol_client.get_all_helper_ise_info (helper_info))
        ret = -1;

    imcontrol_client.close_connection ();

    if (ret == -1)
        return -1;

    count = static_cast<int>(helper_info.label.size());
    if (count > 0) {
        ime_info = (ime_info_s *)calloc (count, sizeof (ime_info_s));
        if (ime_info) {
            for (i = 0; i < count; i++) {
                snprintf(ime_info[i].appid, sizeof (ime_info[i].appid), "%s", helper_info.appid[i].c_str());
                snprintf(ime_info[i].label, sizeof (ime_info[i].label), "%s", helper_info.label[i].c_str());
                ime_info[i].is_enabled = static_cast<bool>(helper_info.is_enabled[i]);
                ime_info[i].is_preinstalled = static_cast<bool>(helper_info.is_preinstalled[i]);
                ime_info[i].has_option = static_cast<int>(helper_info.has_option[i]);
            }
            *info = ime_info;
        }
        else {
            count = -1;
        }
    }

    return count;
}

EXAPI int isf_control_open_ime_option_window (void)
{
    return isf_control_show_ise_option_window ();
}

EXAPI int isf_control_get_active_ime (char **appid)
{
    if (appid == NULL)
        return -1;

    String strUuid;
    int ret = 0;
    IMControlClient imcontrol_client;
    if (!imcontrol_client.open_connection ()) {
        LOGW("open_connection failed");
        return -1;
    }

    imcontrol_client.prepare ();
    if (!imcontrol_client.get_active_ise (strUuid)) {
        LOGW("get_active_ise failed");
        ret = -1;
    }

    imcontrol_client.close_connection ();

    if (ret == -1)
        return -1;

    *appid = strUuid.length () ? strdup (strUuid.c_str ()) : NULL;

    if (*appid == NULL) {
        LOGW("appid is invalid");
        return -1;
    }
    else
        return strUuid.length ();
}

EXAPI int isf_control_set_active_ime (const char *appid)
{
    return isf_control_set_active_ise_by_uuid(appid);
}

EXAPI int isf_control_set_enable_ime (const char *appid, bool is_enabled)
{
    if (!appid)
        return -1;

    IMControlClient imcontrol_client;
    int ret = 0;

    if (!imcontrol_client.open_connection ())
        return -1;

    imcontrol_client.prepare ();

    if (!imcontrol_client.set_enable_helper_ise_info (appid, is_enabled))
        ret = -1;

    imcontrol_client.close_connection ();

    return ret;
}

EXAPI int isf_control_show_ime_list (void)
{
    IMControlClient imcontrol_client;
    int ret = 0;

    if (!imcontrol_client.open_connection ())
        return -1;

    imcontrol_client.prepare ();
    if (!imcontrol_client.show_helper_ise_list ()) {
        LOGW("show_helper_ise_list failed");
        ret = -1;
    }

    imcontrol_client.close_connection ();

    return ret;
}

EXAPI int isf_control_show_ime_selector (void)
{
    IMControlClient imcontrol_client;
    int ret = 0;

    if (!imcontrol_client.open_connection ())
        return -1;

    imcontrol_client.prepare ();
    if (!imcontrol_client.show_helper_ise_selector ()) {
        LOGW("show_helper_ise_selector failed");
        ret = -1;
    }

    imcontrol_client.close_connection ();

    return ret;
}

EXAPI int isf_control_is_ime_enabled (const char *appid, bool *enabled)
{
    if (!appid || !enabled || strlen(appid) < 1) {
        LOGW("Invalid parameter");
        return -1;
    }

    int nEnabled = -1;
    int ret = 0;

    IMControlClient imcontrol_client;
    if (!imcontrol_client.open_connection ()) {
        LOGW("open_connection failed");
        return -1;
    }

    imcontrol_client.prepare ();
    if (!imcontrol_client.is_helper_ise_enabled (appid, nEnabled)) {
        LOGW("is_helper_ise_enabled failed");
        ret = -1;
    }

    imcontrol_client.close_connection ();

    if (ret == -1)
        return -1;

    if (nEnabled < 0) {
        LOGW("Failed; appid(%s), nEnabled=%d", appid, nEnabled);
        return -1;
    }
    else
        *enabled = static_cast<bool>(nEnabled);

    return ret;
}

EXAPI int isf_control_get_recent_ime_geometry (int *x, int *y, int *w, int *h)
{
    int ime_x = -1, ime_y = -1, ime_w = -1, ime_h = -1;

    IMControlClient imcontrol_client;
    int ret = 0;

    if (!imcontrol_client.open_connection ())
        return -1;

    imcontrol_client.prepare ();
    if (!imcontrol_client.get_recent_ime_geometry (&ime_x, &ime_y, &ime_w, &ime_h))
        ret = -1;

    imcontrol_client.close_connection ();

    if (ret == -1)
        return -1;

    if (x)
        *x = ime_x;

    if (y)
        *y = ime_y;

    if (w)
        *w = ime_w;

    if (h)
        *h = ime_h;

    return ret;
}

/*
vi:ts=4:nowrap:ai:expandtab
*/
