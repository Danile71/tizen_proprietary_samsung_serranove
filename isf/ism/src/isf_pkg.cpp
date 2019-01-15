/* ISF is based on SCIM 1.4.7 and extended for supporting more mobile fitable. */

/*
 * Smart Common Input Method
 *
 * Copyright (c) 2002-2005 James Su <suzhe@tsinghua.org.cn>
 *
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA  02111-1307  USA
 *
 * $Id: scim_launcher.cpp,v 1.9 2005/06/15 00:19:08 suzhe Exp $
 *
 */

#define Uses_SCIM_HELPER

#include <unistd.h>
#include "scim_private.h"
#include "scim.h"
#include <pkgmgr-info.h>
#include <scim_panel_common.h>
#include "isf_query_utility.h"
#include "isf_pkg.h"
#include <dlog.h>

#ifdef LOG_TAG
# undef LOG_TAG
#endif
#define LOG_TAG             "ISF_PKG"

using namespace scim;


/**
 * @brief Read data from ime category manifest and insert initial db
 *
 * @param handle    pkgmgrinfo_appinfo_h pointer
 * @param user_data The data to pass to this callback.
 *
 * @return 0 if success, negative value(<0) if fail. Callback is not called if return value is negative.
 */
int isf_pkg_ime_app_list_cb (const pkgmgrinfo_appinfo_h handle, void *user_data)
{
    int ret = 0;
    char *appid = NULL, *pkgid = NULL, *pkgtype = NULL, *exec = NULL, *label = NULL, *path = NULL;
    pkgmgrinfo_pkginfo_h  pkginfo_handle = NULL;
    ImeInfoDB ime_db;
    int *result = static_cast<int*>(user_data); // 0: not IME, 1: Inserted, 2: Updated

    if (result) /* in this case, need to check category */ {
        bool exist = true;
        ret = pkgmgrinfo_appinfo_is_category_exist (handle, "http://tizen.org/category/ime", &exist);
        if (ret != PMINFO_R_OK || !exist) {
            return 0;
        }
    }

    /* appid */
    ret = pkgmgrinfo_appinfo_get_appid (handle, &appid);
    if (ret == PMINFO_R_OK)
        ime_db.appid = String (appid ? appid : "");
    else {
        ISF_SAVE_LOG ("appid is not available! error code=%d\n", ret);
        LOGW ("appid is not available! error code=%d", ret);
        return 0;
    }

    ime_db.iconpath = "";

    /* pkgid */
    ret = pkgmgrinfo_appinfo_get_pkgid (handle, &pkgid);
    if (ret == PMINFO_R_OK)
        ime_db.pkgid = String (pkgid ? pkgid : "");
    else {
        ISF_SAVE_LOG ("pkgid is not available! error code=%d\n", ret);
        LOGW ("pkgid is not available! error code=%d", ret);
        return 0;
    }

    /* pkgtype */
    ret = pkgmgrinfo_appinfo_get_pkgtype (handle, &pkgtype);
    if (ret == PMINFO_R_OK)
        ime_db.pkgtype = String (pkgtype ? pkgtype : "");
    else {
        ISF_SAVE_LOG ("pkgtype is not available! error code=%d\n", ret);
        LOGW ("pkgtype is not available! error code=%d", ret);
        return 0;
    }

    /* exec path */
    ret = pkgmgrinfo_appinfo_get_exec (handle, &exec);
    if (ret == PMINFO_R_OK)
        ime_db.exec = String (exec ? exec : "");
    else {
        ISF_SAVE_LOG ("exec is not available! error code=%d\n", ret);
        LOGW ("exec is not available! error code=%d", ret);
        return 0;
    }

    /* label */
    ret = pkgmgrinfo_appinfo_get_label (handle, &label);
    if (ret == PMINFO_R_OK)
        ime_db.label = String (label ? label : "");

    /* get pkgmgrinfo_pkginfo_h */
    ret = pkgmgrinfo_pkginfo_get_pkginfo (pkgid, &pkginfo_handle);
    if (ret == PMINFO_R_OK && pkginfo_handle) {
        /* pkgrootpath */
        ret = pkgmgrinfo_pkginfo_get_root_path (pkginfo_handle, &path);
    }

    ime_db.languages = "en";
    ime_db.display_lang = "";

    if (ime_db.pkgtype.compare ("rpm") == 0 &&   //1 Inhouse IMEngine ISE(IME)
        ime_db.exec.find ("scim-launcher") != String::npos)  // Some IMEngine's pkgid doesn't have "ise-engine" prefix.
    {
        ime_db.mode = TOOLBAR_KEYBOARD_MODE;
        ime_db.options = 0;
        ime_db.module_path = String (SCIM_MODULE_PATH) + String (SCIM_PATH_DELIM_STRING) + String (SCIM_BINARY_VERSION)
            + String (SCIM_PATH_DELIM_STRING) + String ("IMEngine");
        ime_db.module_name = ime_db.pkgid;
        ime_db.is_enabled = 1;
        ime_db.is_preinstalled = 1;
        ime_db.has_option = 0; // It doesn't matter. No option for IMEngine...
    }
    else {
        ime_db.mode = TOOLBAR_HELPER_MODE;
        if (ime_db.pkgtype.compare ("rpm") == 0) //1 Inhouse Helper ISE(IME)
        {
#ifdef _TV
            ime_db.options = SCIM_HELPER_STAND_ALONE | SCIM_HELPER_NEED_SCREEN_INFO | SCIM_HELPER_AUTO_RESTART | ISM_HELPER_PROCESS_KEYBOARD_KEYEVENT;
#else
            ime_db.options = SCIM_HELPER_STAND_ALONE | SCIM_HELPER_NEED_SCREEN_INFO | SCIM_HELPER_AUTO_RESTART;
#endif
            ime_db.module_path = String (SCIM_MODULE_PATH) + String (SCIM_PATH_DELIM_STRING) + String (SCIM_BINARY_VERSION)
                + String (SCIM_PATH_DELIM_STRING) + String ("Helper");
            ime_db.module_name = ime_db.pkgid;
            ime_db.is_enabled = 1;
            ime_db.is_preinstalled = 1;
            ime_db.has_option = 1;  // Let's assume the inhouse IME always has an option menu.
        }
#ifdef _WEARABLE
        else if (ime_db.pkgtype.compare ("wgt") == 0)    //1 Download Web IME
        {
            ime_db.options = SCIM_HELPER_STAND_ALONE | SCIM_HELPER_NEED_SCREEN_INFO | SCIM_HELPER_AUTO_RESTART
                | SCIM_HELPER_NEED_SPOT_LOCATION_INFO | ISM_HELPER_PROCESS_KEYBOARD_KEYEVENT | ISM_HELPER_WITHOUT_IMENGINE;
            ime_db.module_path = String (SCIM_MODULE_PATH) + String (SCIM_PATH_DELIM_STRING) + String (SCIM_BINARY_VERSION)
                + String (SCIM_PATH_DELIM_STRING) + String ("Helper");
            ime_db.module_name = String ("ise-web-helper-agent");
            if (ime_db.exec.compare (0, 5, "/usr/") == 0) {
                ime_db.is_enabled = 1;
                ime_db.is_preinstalled = 1;
            }
            else {
               ime_db.is_enabled = 0;
               ime_db.is_preinstalled = 0;
            }
            ime_db.has_option = -1; // At this point, we can't know IME has an option (setting) or not; -1 means unknown.
        }
#endif
        else if (ime_db.pkgtype.compare ("tpk") == 0)    //1 Download Native IME
        {
            ime_db.options = SCIM_HELPER_STAND_ALONE | SCIM_HELPER_NEED_SCREEN_INFO | SCIM_HELPER_AUTO_RESTART
                | ISM_HELPER_PROCESS_KEYBOARD_KEYEVENT | ISM_HELPER_WITHOUT_IMENGINE;
            if (path)
                ime_db.module_path = String (path) + String ("/lib");
            else
                ime_db.module_path = String ("/opt/usr/apps/") + ime_db.pkgid + String ("/lib");
            ime_db.module_name = String ("lib") + ime_db.exec.substr (ime_db.exec.find_last_of (SCIM_PATH_DELIM) + 1);
            if (ime_db.exec.compare (0, 5, "/usr/") == 0) {
                ime_db.is_enabled = 1;
                ime_db.is_preinstalled = 1;
            }
            else {
                ime_db.is_enabled = 0;
                ime_db.is_preinstalled = 0;
            }
            ime_db.has_option = -1; // At this point, we can't know IME has an option (setting) or not; -1 means unknown.
        }
        else {
            ISF_SAVE_LOG ("Unsupported pkgtype(%s)\n", ime_db.pkgtype.c_str ());
            if (pkginfo_handle) {
                pkgmgrinfo_pkginfo_destroy_pkginfo (pkginfo_handle);
                pkginfo_handle = NULL;
            }
            return 0;
        }
    }

    ret = isf_db_insert_ime_info (&ime_db);
    if (ret < 1) {
        if (result) {
            ret = isf_db_update_ime_info (&ime_db);
            if (ret) {
                *result = 2;    // Updated
            }
        }
        if (ret < 1) {
            ISF_SAVE_LOG("isf_db_%s_ime_info failed(%d). appid=%s pkgid=%s\n", (result? "update" : "insert"), ret, ime_db.appid.c_str(), ime_db.pkgid.c_str());
        }
    }
    else if (result && ret) {
        *result = 1;    // Inserted
    }

    if (pkginfo_handle) {
        pkgmgrinfo_pkginfo_destroy_pkginfo (pkginfo_handle);
        pkginfo_handle = NULL;
    }

    return 0;
}


/**
 * @brief Reload ime_info DB. This needs to be called when ime_info table is empty.
 */
void isf_pkg_reload_ime_info_db(void)
{
    pkgmgrinfo_appinfo_filter_h handle;
    int ret = pkgmgrinfo_appinfo_filter_create (&handle);
    if (ret == PMINFO_R_OK) {
        ret = pkgmgrinfo_appinfo_filter_add_string (handle, PMINFO_APPINFO_PROP_APP_CATEGORY, "http://tizen.org/category/ime");
        if (ret == PMINFO_R_OK) {
            ret = pkgmgrinfo_appinfo_filter_foreach_appinfo (handle, isf_pkg_ime_app_list_cb, NULL);
        }
        else {
            ISF_SAVE_LOG ("pkgmgrinfo_appinfo_filter_add_string failed(%d)\n", ret);
        }
        pkgmgrinfo_appinfo_filter_destroy (handle);
    }
    else {
        ISF_SAVE_LOG ("pkgmgrinfo_appinfo_filter_create failed(%d)\n", ret);
    }
}

/**
 * @brief Select all ime_info table. If ime_info table is empty, this will reload data through pkgmgr-info.
 *
 * @param ime_info The list to store ImeInfoDB
 *
 * @return the number of selected row.
 */
int isf_pkg_select_all_ime_info_db(std::vector<ImeInfoDB> &ime_info)
{
    int result = isf_db_select_all_ime_info(ime_info);
    if (result == 0) {   // Probably ime_info DB's already added by scim process. But isf_db_delete_ime_info() can delete it, so try to make one here.
        isf_pkg_reload_ime_info_db();

        return isf_db_select_all_ime_info(ime_info);
    }
    return result;
}

/*
vi:ts=4:ai:nowrap:expandtab
*/

