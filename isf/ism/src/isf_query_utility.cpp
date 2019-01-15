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

#define Uses_SCIM_DEBUG
#define Uses_SCIM_IMENGINE
#define Uses_SCIM_IMENGINE_MODULE
#define Uses_SCIM_HELPER_MODULE
#define Uses_SCIM_CONFIG
#define Uses_SCIM_CONFIG_MODULE
#define Uses_SCIM_CONFIG_PATH
#define Uses_C_LOCALE
#define Uses_SCIM_UTILITY
#define Uses_SCIM_PANEL_AGENT


#include <sys/stat.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <dlog.h>
#include "scim_private.h"
#include "scim.h"
#include "isf_query_utility.h"
#include "scim_helper.h"
#include <db-util.h>


using namespace scim;


/////////////////////////////////////////////////////////////////////////////
// Declaration of macro.
/////////////////////////////////////////////////////////////////////////////
#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG "ISF_QUERY"

/*!
 * \note
 * DB Table schema
 *
 * ime_info
 * +-------+-------+-------+----------+------+-------+-------+------+---------+------------+-----------------+------------+-----------+
 * | appid | label | pkgid |  pkgtype | exec | mname | mpath | mode | options | is_enabled | is_preinstalled | has_option | disp_lang |
 * +-------+-------+-------+----------+------+-------+-------+------+---------+------------+-----------------+------------+-----------+
 *
 * CREATE TABLE ime_info (appid TEXT PRIMARY KEY NOT NULL, label TEXT, pkgid TEXT, pkgtype TEXT, exec TEXT mname TEXT, mpath TEXT, mode INTEGER, options INTEGER, is_enabled INTEGER, is_preinstalled INTEGER, has_option INTEGER, disp_lang TEXT);
 *
 */

///////////////////////////////////////////////////////////////////////////////////////////////////
// DATABASE
///////////////////////////////////////////////////////////////////////////////////////////////////
#define DB_PATH "/opt/usr/dbspace/.ime_info.db"
static struct {
    const char* pPath;
    sqlite3* pHandle;
} databaseInfo = {
    DB_PATH, NULL
};

static inline int _begin_transaction(void)
{
    sqlite3_stmt* pStmt = NULL;

    int ret = sqlite3_prepare_v2(databaseInfo.pHandle, "BEGIN TRANSACTION", -1, &pStmt, NULL);
    if (ret != SQLITE_OK)
    {
        LOGE("sqlite3_prepare_v2: %s", sqlite3_errmsg(databaseInfo.pHandle));
        return EXIT_FAILURE;
    }

    if (sqlite3_step(pStmt) != SQLITE_DONE)
    {
        LOGE("sqlite3_step: %s", sqlite3_errmsg(databaseInfo.pHandle));
        sqlite3_finalize(pStmt);
        return EXIT_FAILURE;
    }

    sqlite3_finalize(pStmt);
    return EXIT_SUCCESS;
}

static inline int _rollback_transaction(void)
{
    sqlite3_stmt* pStmt = NULL;

    int ret = sqlite3_prepare_v2(databaseInfo.pHandle, "ROLLBACK TRANSACTION", -1, &pStmt, NULL);
    if (ret != SQLITE_OK)
    {
        LOGE("sqlite3_prepare_v2: %s", sqlite3_errmsg(databaseInfo.pHandle));
        return EXIT_FAILURE;
    }

    if (sqlite3_step(pStmt) != SQLITE_DONE)
    {
        LOGE("sqlite3_step: %s", sqlite3_errmsg(databaseInfo.pHandle));
        sqlite3_finalize(pStmt);
        return EXIT_FAILURE;
    }

    sqlite3_finalize(pStmt);
    return EXIT_SUCCESS;
}

static inline int _commit_transaction(void)
{
    sqlite3_stmt* pStmt = NULL;

    int ret = sqlite3_prepare_v2(databaseInfo.pHandle, "COMMIT TRANSACTION", -1, &pStmt, NULL);
    if (ret != SQLITE_OK)
    {
        LOGE("sqlite3_prepare_v2: %s", sqlite3_errmsg(databaseInfo.pHandle));
        return EXIT_FAILURE;
    }

    if (sqlite3_step(pStmt) != SQLITE_DONE)
    {
        LOGE("sqlite3_step: %s", sqlite3_errmsg(databaseInfo.pHandle));
        sqlite3_finalize(pStmt);
        return EXIT_FAILURE;
    }

    sqlite3_finalize(pStmt);
    return EXIT_SUCCESS;
}

static inline int _db_create_ime_info(void)
{
    char* pException = NULL;
    static const char* pQuery = "CREATE TABLE ime_info (appid TEXT PRIMARY KEY NOT NULL, label TEXT, pkgid TEXT, pkgtype TEXT, exec TEXT, mname TEXT, mpath TEXT, mode INTEGER, options INTEGER, is_enabled INTEGER, is_preinstalled INTEGER, has_option INTEGER, disp_lang TEXT);";

    if (sqlite3_exec(databaseInfo.pHandle, pQuery, NULL, NULL, &pException) != SQLITE_OK) {
        LOGE("sqlite3_exec: %s", pException);
        sqlite3_free(pException);
        return -EIO;
    }

    if (sqlite3_changes(databaseInfo.pHandle) == 0) {
        LOGD("The database is not changed.");
    }

    return 0;
}

static inline void _db_create_table(void)
{
    _begin_transaction();

    int ret = _db_create_ime_info();
    if (ret < 0) {
        _rollback_transaction();
        return;
    }

    _commit_transaction();
}

static inline int _db_init(void)
{
    struct stat stat;

    int ret = db_util_open(databaseInfo.pPath, &databaseInfo.pHandle, DB_UTIL_REGISTER_HOOK_METHOD);
    if (ret != SQLITE_OK) {
        ISF_SAVE_LOG("db_util_open returned code: %d\n", ret);
        LOGE("db_util_open returned code: %d", ret);
        return -EIO;
    }

    if (lstat(databaseInfo.pPath, &stat) < 0) {
        char buf_err[256];
        LOGE("%s", strerror_r (errno, buf_err, sizeof (buf_err)));
        db_util_close(databaseInfo.pHandle);
        databaseInfo.pHandle = NULL;
        return -EIO;
    }

    if (!S_ISREG(stat.st_mode)) {
        LOGE("S_ISREG failed.");
        db_util_close(databaseInfo.pHandle);
        databaseInfo.pHandle = NULL;
        return -EINVAL;
    }

    if (!stat.st_size) {
        LOGE("The RPM file has not been installed properly.");
        _db_create_table();
    }

    return 0;
}

static inline int _db_connect(void)
{
    if (!databaseInfo.pHandle) {
        int ret = _db_init();
        if (ret < 0)
        {
            ISF_SAVE_LOG("Failed to initialize db. return code=%d\n", ret);
            LOGE("Failed to initialize db. return code=%d", ret);
            return -EIO;
        }
    }

    return 0;
}

static inline int _db_disconnect(void)
{
    if (!databaseInfo.pHandle)
        return 0;

    db_util_close(databaseInfo.pHandle);

    databaseInfo.pHandle = NULL;

    return 0;
}

/**
 * @brief Select all ime_info table.
 *
 * @param ime_info The list to store ImeInfoDB
 *
 * @return the number of selected row.
 */
static int _db_select_all_ime_info(std::vector<ImeInfoDB> &ime_info)
{
    int ret = 0, i = 0;
    ImeInfoDB info;
    sqlite3_stmt* pStmt = NULL;
    static const char* pQuery = "SELECT * FROM ime_info;";
    char *db_text;

    do {
        if (i == 0) {
            ret = sqlite3_prepare_v2(databaseInfo.pHandle, pQuery, -1, &pStmt, NULL);
            if (ret != SQLITE_OK) {
                LOGE("sqlite3_prepare_v2: %s", sqlite3_errmsg(databaseInfo.pHandle));
                return 0;
            }
        }

        ret = sqlite3_step(pStmt);
        if (ret == SQLITE_ROW) {
            db_text = (char *)sqlite3_column_text(pStmt, 0);
            info.appid = String(db_text ? db_text : "");

            db_text = (char *)sqlite3_column_text(pStmt, 1);
            info.label = String(db_text ? db_text : "");

            info.languages = "en";

            info.iconpath = "";

            db_text = (char *)sqlite3_column_text(pStmt, 2);
            info.pkgid = String(db_text ? db_text : "");

            db_text = (char *)sqlite3_column_text(pStmt, 3);
            info.pkgtype = String(db_text ? db_text : "");

            db_text = (char *)sqlite3_column_text(pStmt, 4);
            info.exec = String(db_text ? db_text : "");

            db_text = (char *)sqlite3_column_text(pStmt, 5);
            info.module_name = String(db_text ? db_text : "");

            db_text = (char *)sqlite3_column_text(pStmt, 6);
            info.module_path = String(db_text ? db_text : "");

            info.mode = (TOOLBAR_MODE_T)sqlite3_column_int(pStmt, 7);
            info.options = (uint32)sqlite3_column_int(pStmt, 8);
            info.is_enabled = (uint32)sqlite3_column_int(pStmt, 9);
            info.is_preinstalled = (uint32)sqlite3_column_int(pStmt, 10);
            info.has_option = sqlite3_column_int(pStmt, 11);

            db_text = (char *)sqlite3_column_text(pStmt, 12);
            info.display_lang = String(db_text ? db_text : "");

            SECURE_LOGD("appid=\"%s\", label=\"%s\", pkgid=\"%s\", pkgtype=\"%s\", exec=\"%s\", mname=\"%s\", mpath=\"%s\", mode=%d, options=%u, is_enabled=%u, is_preinstalled=%u, has_option=%d, disp_lang=\"%s\"",
                info.appid.c_str(),
                info.label.c_str(),
                info.pkgid.c_str(),
                info.pkgtype.c_str(),
                info.exec.c_str(),
                info.module_name.c_str(),
                info.module_path.c_str(),
                info.mode,
                info.options,
                info.is_enabled,
                info.is_preinstalled,
                info.has_option,
                info.display_lang.c_str());

            ime_info.push_back(info);
            i++;
        }
    } while (ret == SQLITE_ROW);

    if (ret != SQLITE_DONE) {
        LOGE("sqlite3_step: %s", sqlite3_errmsg(databaseInfo.pHandle));
    }

    sqlite3_reset(pStmt);
    sqlite3_clear_bindings(pStmt);
    sqlite3_finalize(pStmt);
    return i;
}

/**
 * @brief Select ime_info table with appid.
 *
 * @param appid appid to select ime_info row.
 * @param pImeInfo The pointer of ImeInfoDB.
 *
 * @return 1 if it is successful, otherwise return 0.
 */
static int _db_select_ime_info_by_appid(const char *appid, ImeInfoDB *pImeInfo)
{
    int ret = 0, i = 0;
    sqlite3_stmt* pStmt = NULL;
    static const char* pQuery = "SELECT * FROM ime_info WHERE appid = ?;";
    char *db_text;

    if (!appid || !pImeInfo) {
        return 0;
    }

    pImeInfo->appid.clear();
    pImeInfo->label.clear();
    pImeInfo->languages.clear();
    pImeInfo->iconpath.clear();
    pImeInfo->pkgid.clear();
    pImeInfo->pkgtype.clear();
    pImeInfo->exec.clear();
    pImeInfo->module_name.clear();
    pImeInfo->module_path.clear();
    pImeInfo->mode = TOOLBAR_KEYBOARD_MODE;
    pImeInfo->options = 0;
    pImeInfo->is_enabled = 0;
    pImeInfo->is_preinstalled = 0;
    pImeInfo->has_option = -1;
    pImeInfo->display_lang.clear();

    ret = sqlite3_prepare_v2(databaseInfo.pHandle, pQuery, -1, &pStmt, NULL);
    if (ret != SQLITE_OK) {
        LOGE("%s", sqlite3_errmsg(databaseInfo.pHandle));
        return 0;
    }

    ret = sqlite3_bind_text(pStmt, 1, appid, -1, SQLITE_TRANSIENT);
    if (ret != SQLITE_OK) {
        LOGE("%s", sqlite3_errmsg(databaseInfo.pHandle));
        goto out;
    }

    ret = sqlite3_step(pStmt);
    if (ret != SQLITE_ROW) {
        LOGE("%s", sqlite3_errmsg(databaseInfo.pHandle));
        goto out;
    }
    i = 1;

    db_text = (char*)sqlite3_column_text(pStmt, 0);
    pImeInfo->appid = String(db_text ? db_text : "");

    db_text = (char*)sqlite3_column_text(pStmt, 1);
    pImeInfo->label = String(db_text ? db_text : "");

    pImeInfo->languages = "en";

    pImeInfo->iconpath = "";

    db_text = (char*)sqlite3_column_text(pStmt, 2);
    pImeInfo->pkgid = String(db_text ? db_text : "");

    db_text = (char*)sqlite3_column_text(pStmt, 3);
    pImeInfo->pkgtype = String(db_text ? db_text : "");

    db_text = (char*)sqlite3_column_text(pStmt, 4);
    pImeInfo->exec = String(db_text ? db_text : "");

    db_text = (char*)sqlite3_column_text(pStmt, 5);
    pImeInfo->module_name = String(db_text ? db_text : "");

    db_text = (char*)sqlite3_column_text(pStmt, 6);
    pImeInfo->module_path = String(db_text ? db_text : "");

    pImeInfo->mode = (TOOLBAR_MODE_T)sqlite3_column_int(pStmt, 7);
    pImeInfo->options = (uint32)sqlite3_column_int(pStmt, 8);
    pImeInfo->is_enabled = (uint32)sqlite3_column_int(pStmt, 9);
    pImeInfo->is_preinstalled = (uint32)sqlite3_column_int(pStmt, 10);
    pImeInfo->has_option = sqlite3_column_int(pStmt, 11);

    db_text = (char*)sqlite3_column_text(pStmt, 12);
    pImeInfo->display_lang = String(db_text ? db_text : "");

    SECURE_LOGD("appid=\"%s\", label=\"%s\", pkgid=\"%s\", pkgtype=\"%s\", exec=\"%s\", mname=\"%s\", mpath=\"%s\", mode=%d, options=%u, is_enabled=%u, is_preinstalled=%u, has_option=%d, disp_lang=\"%s\"",
        pImeInfo->appid.c_str(),
        pImeInfo->label.c_str(),
        pImeInfo->pkgid.c_str(),
        pImeInfo->pkgtype.c_str(),
        pImeInfo->exec.c_str(),
        pImeInfo->module_name.c_str(),
        pImeInfo->module_path.c_str(),
        pImeInfo->mode,
        pImeInfo->options,
        pImeInfo->is_enabled,
        pImeInfo->is_preinstalled,
        pImeInfo->has_option,
        pImeInfo->display_lang.c_str());

out:
    sqlite3_reset(pStmt);
    sqlite3_clear_bindings(pStmt);
    sqlite3_finalize(pStmt);
    return i;
}

/**
 * @brief Select ime_info mname data.
 *
 * @param mname The list to store module name
 * @param mode  ISE type
 *
 * @return the number of selected row.
 */
static int _db_select_module_name_by_mode(TOOLBAR_MODE_T mode, std::vector<String> &mname)
{
    int ret = 0, i = 0;
    sqlite3_stmt* pStmt = NULL;
    static const char* pQuery = "SELECT mname FROM ime_info WHERE mode = ?;";

    do {
        if (i == 0) {
            ret = sqlite3_prepare_v2(databaseInfo.pHandle, pQuery, -1, &pStmt, NULL);
            if (ret != SQLITE_OK) {
                LOGE("%s", sqlite3_errmsg(databaseInfo.pHandle));
                return 0;
            }

            ret = sqlite3_bind_int(pStmt, 1, mode);
            if (ret != SQLITE_OK) {
                LOGE("%s", sqlite3_errmsg(databaseInfo.pHandle));
                goto out;
            }
        }

        ret = sqlite3_step(pStmt);
        if (ret == SQLITE_ROW) {
            char *db_text = (char *)sqlite3_column_text(pStmt, 0);
            mname.push_back(String(db_text ? db_text : ""));
            SECURE_LOGD("%s: \"%s\"", (mode? "Helper": "IMEngine"), mname.back().c_str());
            i++;
        }
    } while (ret == SQLITE_ROW);

    if (ret != SQLITE_DONE) {
        LOGE("sqlite3_step: %s", sqlite3_errmsg(databaseInfo.pHandle));
    }

out:
    sqlite3_reset(pStmt);
    sqlite3_clear_bindings(pStmt);
    sqlite3_finalize(pStmt);
    return i;
}

/**
 * @brief Select ime_info mpath data.
 *
 * @param mpath The list to store module path
 * @param mode  ISE type
 *
 * @return the number of selected row.
 */
static int _db_select_module_path_by_mode(TOOLBAR_MODE_T mode, std::vector<String> &mpath)
{
    int ret = 0, i = 0;
    sqlite3_stmt* pStmt = NULL;
    static const char* pQuery = "SELECT mpath FROM ime_info WHERE mode = ?;";

    ret = sqlite3_prepare_v2(databaseInfo.pHandle, pQuery, -1, &pStmt, NULL);
    if (ret != SQLITE_OK) {
        LOGE("%s", sqlite3_errmsg(databaseInfo.pHandle));
        return 0;
    }

    ret = sqlite3_bind_int(pStmt, 1, mode);
    if (ret != SQLITE_OK) {
        LOGE("%s", sqlite3_errmsg(databaseInfo.pHandle));
        goto out;
    }

    do {
        ret = sqlite3_step(pStmt);
        if (ret == SQLITE_ROW) {
            char *db_text = (char *)sqlite3_column_text(pStmt, 0);
            mpath.push_back(String(db_text ? db_text : ""));
            SECURE_LOGD("%s: \"%s\"", (mode? "Helper": "IMEngine"), mpath.back().c_str());
            i++;
        }
    } while (ret == SQLITE_ROW);

    if (ret != SQLITE_DONE) {
        LOGW("sqlite3_step returned %d, mode=%d, %s", ret, mode, sqlite3_errmsg(databaseInfo.pHandle));
    }

out:
    sqlite3_reset(pStmt);
    sqlite3_clear_bindings(pStmt);
    sqlite3_finalize(pStmt);
    return i;
}

/**
 * @brief Select appids by pkgid in ime_info table.
 *
 * @param pkgid pkgid to get appid data.
 * @param appids    The list to store appid; there might be multiple appids for one pkgid.
 *
 * @return the number of selected row.
 */
static int _db_select_appids_by_pkgid(const char *pkgid, std::vector<String> &appids)
{
    int ret = 0, i = 0;
    sqlite3_stmt* pStmt = NULL;
    static const char* pQuery = "SELECT appid FROM ime_info WHERE pkgid = ?;";

    ret = sqlite3_prepare_v2(databaseInfo.pHandle, pQuery, -1, &pStmt, NULL);
    if (ret != SQLITE_OK) {
        LOGE("%s", sqlite3_errmsg(databaseInfo.pHandle));
        return 0;
    }

    ret = sqlite3_bind_text(pStmt, 1, pkgid, -1, SQLITE_TRANSIENT);
    if (ret != SQLITE_OK) {
        LOGE("%s", sqlite3_errmsg(databaseInfo.pHandle));
        goto out;
    }

    do {
        ret = sqlite3_step(pStmt);
        if (ret == SQLITE_ROW) {
            char *db_text = (char *)sqlite3_column_text(pStmt, 0);
            appids.push_back(String(db_text ? db_text : ""));
            SECURE_LOGD("appid=\"%s\"", appids.back().c_str());
            i++;
        }
    } while (ret == SQLITE_ROW);

    if (ret != SQLITE_DONE) {
        LOGE("sqlite3_step: %s", sqlite3_errmsg(databaseInfo.pHandle));
    }

out:
    sqlite3_reset(pStmt);
    sqlite3_clear_bindings(pStmt);
    sqlite3_finalize(pStmt);
    return i;
}

/**
 * @brief Select "is_enabled" by appid in ime_info table.
 *
 * @param[in] appid appid in ime_info table.
 * @param[out] is_enabled Helper ISE enabled or not.
 *
 * @return 1 if it is successful, otherwise return 0.
 */
static int _db_select_is_enabled_by_appid(const char *appid, bool *is_enabled)
{
    int ret = 0, i = 0;
    sqlite3_stmt* pStmt = NULL;
    static const char* pQuery = "SELECT is_enabled FROM ime_info WHERE appid = ?;";

    if (!appid || !is_enabled)
        return i;

    ret = sqlite3_prepare_v2(databaseInfo.pHandle, pQuery, -1, &pStmt, NULL);
    if (ret != SQLITE_OK) {
        LOGE("%s", sqlite3_errmsg(databaseInfo.pHandle));
        return i;
    }

    ret = sqlite3_bind_text(pStmt, 1, appid, -1, SQLITE_TRANSIENT);
    if (ret != SQLITE_OK) {
        LOGE("%s", sqlite3_errmsg(databaseInfo.pHandle));
        goto out;
    }

    ret = sqlite3_step(pStmt);
    if (ret != SQLITE_ROW) {
        LOGE("%s", sqlite3_errmsg(databaseInfo.pHandle));
    }
    else {
        *is_enabled = (bool)sqlite3_column_int(pStmt, 0);
        i = 1;
        SECURE_LOGD("is_enabled=%d by appid=\"%s\"", *is_enabled, appid);
    }

out:
    sqlite3_reset(pStmt);
    sqlite3_clear_bindings(pStmt);
    sqlite3_finalize(pStmt);
    return i;
}

/**
 * @brief Gets the count of the same module name in ime_info table.
 *
 * @param module_name Module name
 *
 * @return the number of selected row.
 */
static int _db_select_count_by_module_name(const char *module_name)
{
    int ret = 0, i = 0;
    sqlite3_stmt* pStmt = NULL;
    static const char* pQuery = "SELECT COUNT(*) FROM ime_info WHERE mname = ?;";

    ret = sqlite3_prepare_v2(databaseInfo.pHandle, pQuery, -1, &pStmt, NULL);
    if (ret != SQLITE_OK) {
        LOGE("%s", sqlite3_errmsg(databaseInfo.pHandle));
        return 0;
    }

    ret = sqlite3_bind_text(pStmt, 1, module_name, -1, SQLITE_TRANSIENT);
    if (ret != SQLITE_OK) {
        LOGE("%s", sqlite3_errmsg(databaseInfo.pHandle));
        goto out;
    }

    ret = sqlite3_step(pStmt);
    if (ret != SQLITE_ROW) {
        LOGE("sqlite3_step: %s", sqlite3_errmsg(databaseInfo.pHandle));
    }
    else {
        i = sqlite3_column_int(pStmt, 0);
    }

out:
    sqlite3_reset(pStmt);
    sqlite3_clear_bindings(pStmt);
    sqlite3_finalize(pStmt);
    return i;
}

/**
 * @brief Update label data by appid in ime_info table.
 *
 * @param appid appid in ime_info table.
 * @param label label to be updated in ime_info table.
 *
 * @return 1 if it is successful, otherwise return 0.
 */
static int _db_update_label_by_appid(const char *appid, const char *label)
{
    int ret = 0;
    sqlite3_stmt* pStmt = NULL;
    static const char* pQuery = "UPDATE ime_info SET label = ? WHERE appid = ?;";

    if (appid == NULL || label == NULL) {
        LOGE("input is NULL.");
        return 0;
    }

    ret = sqlite3_prepare_v2(databaseInfo.pHandle, pQuery, -1, &pStmt, NULL);
    if (ret != SQLITE_OK) {
        LOGE("sqlite3_prepare_v2: %s", sqlite3_errmsg(databaseInfo.pHandle));
        return 0;
    }

    ret = sqlite3_bind_text(pStmt, 1, label, -1, SQLITE_TRANSIENT);
    if (ret != SQLITE_OK) {
        LOGE("sqlite3_bind_text: %s", sqlite3_errmsg(databaseInfo.pHandle));
        ret = 0;
        goto out;
    }

    ret = sqlite3_bind_text(pStmt, 2, appid, -1, SQLITE_TRANSIENT);
    if (ret != SQLITE_OK) {
        LOGE("sqlite3_bind_text: %s", sqlite3_errmsg(databaseInfo.pHandle));
        ret = 0;
        goto out;
    }

    ret = sqlite3_step(pStmt);
    if (ret != SQLITE_DONE) {
        LOGE("sqlite3_step returned %d, %s", ret, sqlite3_errmsg(databaseInfo.pHandle));
        ret = 0;
    }
    else {
        SECURE_LOGD("UPDATE ime_info SET label = %s WHERE appid = %s;", label, appid);
        ret = 1;
    }

out:
    sqlite3_reset(pStmt);
    sqlite3_clear_bindings(pStmt);
    sqlite3_finalize(pStmt);
    return ret;
}

/**
 * @brief Update disp_lang data in ime_info table.
 *
 * @param disp_lang display language code
 *
 * @return 1 if it is successful, otherwise return 0.
 */
static int _db_update_disp_lang(const char *disp_lang)
{
    int ret = 0;
    sqlite3_stmt* pStmt = NULL;
    static const char* pQuery = "UPDATE ime_info SET disp_lang = ?;";

    if (disp_lang == NULL) {
        LOGE("input is NULL.");
        return 0;
    }

    ret = sqlite3_prepare_v2(databaseInfo.pHandle, pQuery, -1, &pStmt, NULL);
    if (ret != SQLITE_OK) {
        LOGE("sqlite3_prepare_v2: %s", sqlite3_errmsg(databaseInfo.pHandle));
        return 0;
    }

    ret = sqlite3_bind_text(pStmt, 1, disp_lang, -1, SQLITE_TRANSIENT);
    if (ret != SQLITE_OK) {
        LOGE("sqlite3_bind_text: %s", sqlite3_errmsg(databaseInfo.pHandle));
        ret = 0;
        goto out;
    }

    ret = sqlite3_step(pStmt);
    if (ret != SQLITE_DONE) {
        LOGE("sqlite3_step returned %d, %s", ret, sqlite3_errmsg(databaseInfo.pHandle));
        ret = 0;
    }
    else {
        SECURE_LOGD("UPDATE ime_info SET disp_lang = %s;", disp_lang);
        ret = 1;
    }

out:
    sqlite3_reset(pStmt);
    sqlite3_clear_bindings(pStmt);
    sqlite3_finalize(pStmt);
    return ret;
}


/**
 * @brief Update "is_enabled" data by appid in ime_info table.
 *
 * @param appid appid in ime_info table.
 * @param is_enabled Helper ISE enabled or not.
 *
 * @return 1 if it is successful, otherwise return 0.
 */
static int _db_update_is_enabled_by_appid(const char *appid, bool is_enabled)
{
    int ret = 0;
    sqlite3_stmt* pStmt = NULL;
    static const char* pQuery = "UPDATE ime_info SET is_enabled = ? WHERE appid = ?;";

    if (appid == NULL) {
        LOGE("input is NULL.");
        return 0;
    }

    ret = sqlite3_prepare_v2(databaseInfo.pHandle, pQuery, -1, &pStmt, NULL);
    if (ret != SQLITE_OK) {
        LOGE("sqlite3_prepare_v2: %s", sqlite3_errmsg(databaseInfo.pHandle));
        return 0;
    }

    ret = sqlite3_bind_int(pStmt, 1, (int)is_enabled);
    if (ret != SQLITE_OK) {
        LOGE("%s", sqlite3_errmsg(databaseInfo.pHandle));
        goto out;
    }

    ret = sqlite3_bind_text(pStmt, 2, appid, -1, SQLITE_TRANSIENT);
    if (ret != SQLITE_OK) {
        LOGE("sqlite3_bind_text: %s", sqlite3_errmsg(databaseInfo.pHandle));
        ret = 0;
        goto out;
    }

    ret = sqlite3_step(pStmt);
    if (ret != SQLITE_DONE) {
        LOGE("sqlite3_step returned %d, %s", ret, sqlite3_errmsg(databaseInfo.pHandle));
        ret = 0;
    }
    else {
        SECURE_LOGD("UPDATE ime_info SET enabled = %d WHERE appid = %s;", is_enabled, appid);
        ret = 1;
    }

out:
    sqlite3_reset(pStmt);
    sqlite3_clear_bindings(pStmt);
    sqlite3_finalize(pStmt);
    return ret;
}

/**
 * @brief Update "has_option" data by appid in ime_info table.
 *
 * @param appid appid in ime_info table.
 * @param has_option Helper ISE enabled or not.
 *
 * @return 1 if it is successful, otherwise return 0.
 */
static int _db_update_has_option_by_appid(const char *appid, bool has_option)
{
    int ret = 0;
    sqlite3_stmt* pStmt = NULL;
    static const char* pQuery = "UPDATE ime_info SET has_option = ? WHERE appid = ?;";

    if (appid == NULL) {
        LOGE("input is NULL.");
        return 0;
    }

    ret = sqlite3_prepare_v2(databaseInfo.pHandle, pQuery, -1, &pStmt, NULL);
    if (ret != SQLITE_OK) {
        LOGE("sqlite3_prepare_v2: %s", sqlite3_errmsg(databaseInfo.pHandle));
        return 0;
    }

    ret = sqlite3_bind_int(pStmt, 1, (int)has_option);
    if (ret != SQLITE_OK) {
        LOGE("%s", sqlite3_errmsg(databaseInfo.pHandle));
        goto out;
    }

    ret = sqlite3_bind_text(pStmt, 2, appid, -1, SQLITE_TRANSIENT);
    if (ret != SQLITE_OK) {
        LOGE("sqlite3_bind_text: %s", sqlite3_errmsg(databaseInfo.pHandle));
        ret = 0;
        goto out;
    }

    ret = sqlite3_step(pStmt);
    if (ret != SQLITE_DONE) {
        LOGE("sqlite3_step returned %d, %s", ret, sqlite3_errmsg(databaseInfo.pHandle));
        ret = 0;
    }
    else {
        SECURE_LOGD("UPDATE ime_info SET enabled = %d WHERE appid = %s;", has_option, appid);
        ret = 1;
    }

out:
    sqlite3_reset(pStmt);
    sqlite3_clear_bindings(pStmt);
    sqlite3_finalize(pStmt);
    return ret;
}

/**
 * @brief Update data to ime_info table except for a few columns.
 *
 * @param ime_db The pointer of ImeInfoDB
 *
 * @return the number of updated data.
 */
static int _db_update_ime_info(ImeInfoDB *ime_db)
{
    int ret = 0, i = 0, has_option = 0;
    sqlite3_stmt* pStmt = NULL;
    static const char* pQuery = "UPDATE ime_info SET label = ?, exec = ?, mname = ?, mpath = ?, has_option = ? WHERE appid = ?;";

    if (!ime_db) {
        LOGE("Input parameter is null");
        return 0;
    }

    ret = sqlite3_prepare_v2(databaseInfo.pHandle, pQuery, -1, &pStmt, NULL);
    if (ret != SQLITE_OK) {
        ISF_SAVE_LOG("sqlite3_prepare_v2: %s\n", sqlite3_errmsg(databaseInfo.pHandle));
        LOGE("sqlite3_prepare_v2: %s", sqlite3_errmsg(databaseInfo.pHandle));
        return i;
    }

    ret = sqlite3_bind_text(pStmt, 1, ime_db->label.c_str(), -1, SQLITE_TRANSIENT);
    if (ret != SQLITE_OK) {
        LOGE("sqlite3_bind_text: %s", sqlite3_errmsg(databaseInfo.pHandle));
        goto out;
    }

    ret = sqlite3_bind_text(pStmt, 2, ime_db->exec.c_str(), -1, SQLITE_TRANSIENT);
    if (ret != SQLITE_OK) {
        LOGE("sqlite3_bind_text: %s", sqlite3_errmsg(databaseInfo.pHandle));
        goto out;
    }

    ret = sqlite3_bind_text(pStmt, 3, ime_db->module_name.c_str(), -1, SQLITE_TRANSIENT);
    if (ret != SQLITE_OK) {
        LOGE("sqlite3_bind_text: %s", sqlite3_errmsg(databaseInfo.pHandle));
        goto out;
    }

    ret = sqlite3_bind_text(pStmt, 4, ime_db->module_path.c_str(), -1, SQLITE_TRANSIENT);
    if (ret != SQLITE_OK) {
        LOGE("sqlite3_bind_text: %s", sqlite3_errmsg(databaseInfo.pHandle));
        goto out;
    }

    if (ime_db->pkgtype.compare("wgt") == 0 || ime_db->pkgtype.compare("tpk") == 0)
        has_option = -1;
    else if (ime_db->mode == TOOLBAR_HELPER_MODE)
        has_option = 1;
    else
        has_option = 0;

    ret = sqlite3_bind_int(pStmt, 5, has_option);
    if (ret != SQLITE_OK) {
        LOGE("%s", sqlite3_errmsg(databaseInfo.pHandle));
        goto out;
    }

    ret = sqlite3_bind_text(pStmt, 6, ime_db->appid.c_str(), -1, SQLITE_TRANSIENT);
    if (ret != SQLITE_OK) {
        LOGE("sqlite3_bind_text: %s", sqlite3_errmsg(databaseInfo.pHandle));
        goto out;
    }

    ret = sqlite3_step(pStmt);
    if (ret != SQLITE_DONE) {
        ISF_SAVE_LOG("sqlite3_step returned %d, appid=%s, %s\n", ret, ime_db->appid.c_str(), sqlite3_errmsg(databaseInfo.pHandle));
        LOGW("sqlite3_step returned %d, appid=%s, %s", ret, ime_db->appid.c_str(), sqlite3_errmsg(databaseInfo.pHandle));
        ret = SQLITE_ERROR;
        goto out;
    }
    else {
        SECURE_LOGD("Update \"%s\" \"%s\" \"%s\" \"%s\" \"%s\"",
            ime_db->appid.c_str(), ime_db->label.c_str(), ime_db->exec.c_str(), ime_db->module_name.c_str(), ime_db->module_path.c_str());
        ret = SQLITE_OK;
        i++;
    }
    sqlite3_reset(pStmt);
    sqlite3_clear_bindings(pStmt);

out:
    if (ret != SQLITE_OK) {
        sqlite3_reset(pStmt);
        sqlite3_clear_bindings(pStmt);
    }
    sqlite3_finalize(pStmt);

    return i;
}

/**
 * @brief Insert data to ime_info table.
 *
 * @param ime_db The pointer of ImeInfoDB
 *
 * @return the number of inserted data.
 */
static int _db_insert_ime_info(ImeInfoDB *ime_db)
{
    int ret = 0, i = 0;
    sqlite3_stmt* pStmt = NULL;
    static const char* pQuery = "INSERT INTO ime_info (appid, label, pkgid, pkgtype, exec, mname, mpath, mode, options, is_enabled, is_preinstalled, has_option, disp_lang) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);";

    if (!ime_db) {
        LOGE("Input parameter is null");
        return 0;
    }

    ret = sqlite3_prepare_v2(databaseInfo.pHandle, pQuery, -1, &pStmt, NULL);
    if (ret != SQLITE_OK) {
        ISF_SAVE_LOG("sqlite3_prepare_v2: %s\n", sqlite3_errmsg(databaseInfo.pHandle));
        LOGE("sqlite3_prepare_v2: %s", sqlite3_errmsg(databaseInfo.pHandle));
        return i;
    }

    ret = sqlite3_bind_text(pStmt, 1, ime_db->appid.c_str(), -1, SQLITE_TRANSIENT);
    if (ret != SQLITE_OK) {
        LOGE("sqlite3_bind_text: %s", sqlite3_errmsg(databaseInfo.pHandle));
        goto out;
    }

    ret = sqlite3_bind_text(pStmt, 2, ime_db->label.c_str(), -1, SQLITE_TRANSIENT);
    if (ret != SQLITE_OK) {
        LOGE("sqlite3_bind_text: %s", sqlite3_errmsg(databaseInfo.pHandle));
        goto out;
    }

    ret = sqlite3_bind_text(pStmt, 3, ime_db->pkgid.c_str(), -1, SQLITE_TRANSIENT);
    if (ret != SQLITE_OK) {
        LOGE("sqlite3_bind_text: %s", sqlite3_errmsg(databaseInfo.pHandle));
        goto out;
    }

    ret = sqlite3_bind_text(pStmt, 4, ime_db->pkgtype.c_str(), -1, SQLITE_TRANSIENT);
    if (ret != SQLITE_OK) {
        LOGE("sqlite3_bind_text: %s", sqlite3_errmsg(databaseInfo.pHandle));
        goto out;
    }

    ret = sqlite3_bind_text(pStmt, 5, ime_db->exec.c_str(), -1, SQLITE_TRANSIENT);
    if (ret != SQLITE_OK) {
        LOGE("sqlite3_bind_text: %s", sqlite3_errmsg(databaseInfo.pHandle));
        goto out;
    }

    ret = sqlite3_bind_text(pStmt, 6, ime_db->module_name.c_str(), -1, SQLITE_TRANSIENT);
    if (ret != SQLITE_OK) {
        LOGE("sqlite3_bind_text: %s", sqlite3_errmsg(databaseInfo.pHandle));
        goto out;
    }

    ret = sqlite3_bind_text(pStmt, 7, ime_db->module_path.c_str(), -1, SQLITE_TRANSIENT);
    if (ret != SQLITE_OK) {
        LOGE("sqlite3_bind_text: %s", sqlite3_errmsg(databaseInfo.pHandle));
        goto out;
    }

    ret = sqlite3_bind_int(pStmt, 8, ime_db->mode);
    if (ret != SQLITE_OK) {
        LOGE("sqlite3_bind_int: %s", sqlite3_errmsg(databaseInfo.pHandle));
        goto out;
    }

    ret = sqlite3_bind_int(pStmt, 9, (int)ime_db->options);
    if (ret != SQLITE_OK) {
        LOGE("sqlite3_bind_int: %s", sqlite3_errmsg(databaseInfo.pHandle));
        goto out;
    }

    ret = sqlite3_bind_int(pStmt, 10, (int)ime_db->is_enabled);
    if (ret != SQLITE_OK) {
        LOGE("sqlite3_bind_int: %s", sqlite3_errmsg(databaseInfo.pHandle));
        goto out;
    }

    ret = sqlite3_bind_int(pStmt, 11, (int)ime_db->is_preinstalled);
    if (ret != SQLITE_OK) {
        LOGE("sqlite3_bind_int: %s", sqlite3_errmsg(databaseInfo.pHandle));
        goto out;
    }

    ret = sqlite3_bind_int(pStmt, 12, ime_db->has_option);
    if (ret != SQLITE_OK) {
        LOGE("sqlite3_bind_int: %s", sqlite3_errmsg(databaseInfo.pHandle));
        goto out;
    }

    ret = sqlite3_bind_text(pStmt, 13, ime_db->display_lang.c_str(), -1, SQLITE_TRANSIENT);
    if (ret != SQLITE_OK) {
        LOGE("sqlite3_bind_text: %s", sqlite3_errmsg(databaseInfo.pHandle));
        goto out;
    }

    ret = sqlite3_step(pStmt);
    if (ret != SQLITE_DONE) {
        LOGW("sqlite3_step returned %d, appid=%s, %s", ret, ime_db->appid.c_str(), sqlite3_errmsg(databaseInfo.pHandle));
        ret = SQLITE_ERROR;
        goto out;
    }
    else {
        SECURE_LOGD("Insert \"%s\" \"%s\" \"%s\" \"%s\" \"%s\" \"%s\" \"%s\" %d %u %u %u %d \"%s\"",
            ime_db->appid.c_str(), ime_db->label.c_str(), ime_db->pkgid.c_str(), ime_db->pkgtype.c_str(),
            ime_db->module_name.c_str(), ime_db->exec.c_str(), ime_db->module_path.c_str(), ime_db->mode,
            ime_db->options, ime_db->is_enabled, ime_db->is_preinstalled, ime_db->has_option, ime_db->display_lang.c_str());
        ret = SQLITE_OK;
        i++;
    }
    sqlite3_reset(pStmt);
    sqlite3_clear_bindings(pStmt);

out:
    if (ret != SQLITE_OK) {
        sqlite3_reset(pStmt);
        sqlite3_clear_bindings(pStmt);
    }
    sqlite3_finalize(pStmt);

    return i;
}

/**
 * @brief Delete ime_info data with pkgid.
 *
 * @param pkgid pkgid to delete ime_info table.
 *
 * @return 1 if it is successful, otherwise return 0.
 */
static int _db_delete_ime_info_by_pkgid(const char *pkgid)
{
    int i = 0, ret = 0;
    sqlite3_stmt* pStmt = NULL;
    static const char* pQuery = "DELETE FROM ime_info WHERE pkgid = ?;"; // There might be more than one appid for one pkgid.

    ret = sqlite3_prepare_v2(databaseInfo.pHandle, pQuery, -1, &pStmt, NULL);
    if (ret != SQLITE_OK) {
        LOGE("%s", sqlite3_errmsg(databaseInfo.pHandle));
        return i;
    }

    ret = sqlite3_bind_text(pStmt, 1, pkgid, -1, SQLITE_TRANSIENT);
    if (ret != SQLITE_OK) {
        LOGE("sqlite3_bind_text: %s", sqlite3_errmsg(databaseInfo.pHandle));
        goto out;
    }

    ret = sqlite3_step(pStmt);
    if (ret != SQLITE_DONE) {
        LOGE("sqlite3_step returned %d, %s", ret, sqlite3_errmsg(databaseInfo.pHandle));
    }
    else {
        // If there is no pkgid to delete, ret is still SQLITE_DONE.
        SECURE_LOGD("DELETE FROM ime_info WHERE pkgid = %s;", pkgid);
        i = 1;
    }

out:
    sqlite3_reset(pStmt);
    sqlite3_clear_bindings(pStmt);
    sqlite3_finalize(pStmt);
    return i;
}

/**
 * @brief Delete ime_info data with appid.
 *
 * @param appid appid to delete ime_info table.
 *
 * @return 1 if it is successful, otherwise return 0.
 */
static int _db_delete_ime_info_by_appid(const char *appid)
{
    int i = 0, ret = 0;
    sqlite3_stmt* pStmt = NULL;
    static const char* pQuery = "DELETE FROM ime_info WHERE appid = ?;";

    ret = sqlite3_prepare_v2(databaseInfo.pHandle, pQuery, -1, &pStmt, NULL);
    if (ret != SQLITE_OK) {
        LOGE("%s", sqlite3_errmsg(databaseInfo.pHandle));
        return i;
    }

    ret = sqlite3_bind_text(pStmt, 1, appid, -1, SQLITE_TRANSIENT);
    if (ret != SQLITE_OK) {
        LOGE("sqlite3_bind_text: %s", sqlite3_errmsg(databaseInfo.pHandle));
        goto out;
    }

    ret = sqlite3_step(pStmt);
    if (ret != SQLITE_DONE) {
        LOGE("sqlite3_step returned %d, %s", ret, sqlite3_errmsg(databaseInfo.pHandle));
    }
    else {
        // If there is no appid to delete, ret is still SQLITE_DONE.
        SECURE_LOGD("DELETE FROM ime_info WHERE appid = %s;", appid);
        i = 1;
    }

out:
    sqlite3_reset(pStmt);
    sqlite3_clear_bindings(pStmt);
    sqlite3_finalize(pStmt);
    return i;
}

/**
 * @brief Delete all ime_info data.
 *
 * @return 1 if it is successful, otherwise return 0.
 */
static int _db_delete_ime_info(void)
{
    int i = 0, ret = 0;
    sqlite3_stmt* pStmt = NULL;
    static const char* pQuery = "DELETE FROM ime_info;"; // There might be more than one appid for one pkgid.

    ret = sqlite3_prepare_v2(databaseInfo.pHandle, pQuery, -1, &pStmt, NULL);
    if (ret != SQLITE_OK) {
        LOGE("%s", sqlite3_errmsg(databaseInfo.pHandle));
        return i;
    }

    ret = sqlite3_step(pStmt);
    if (ret != SQLITE_DONE) {
        LOGE("sqlite3_step returned %d, %s", ret, sqlite3_errmsg(databaseInfo.pHandle));
    }
    else {
        // If there is no pkgid to delete, ret is still SQLITE_DONE.
        SECURE_LOGD("DELETE FROM ime_info;");
        i = 1;
    }

    sqlite3_reset(pStmt);
    sqlite3_clear_bindings(pStmt);
    sqlite3_finalize(pStmt);
    return i;
}

/**
 * @brief Select all ime_info table.
 *
 * @param ime_info The list to store ImeInfoDB
 *
 * @return the number of selected row.
 */
EXAPI int isf_db_select_all_ime_info(std::vector<ImeInfoDB> &ime_info)
{
    int ret = 0;

    ime_info.clear();

    if (_db_connect() == 0) {
        ret = _db_select_all_ime_info(ime_info);
        _db_disconnect();
    }
    else
        LOGW("failed");

    return ret;
}

/**
 * @brief Select ime_info table with appid.
 *
 * @param appid Application ID to get ime_info data
 * @param pImeInfo The pointer of ImeInfoDB.
 *
 * @return 1 if it is successful, otherwise return 0.
 */
EXAPI int isf_db_select_ime_info_by_appid(const char *appid, ImeInfoDB *pImeInfo)
{
    int ret = 0;

    if (!appid || !pImeInfo) {
        LOGW("invalid parameter.");
        return 0;
    }

    if (_db_connect() == 0) {
        ret = _db_select_ime_info_by_appid(appid, pImeInfo);
        _db_disconnect();
    }
    else
        LOGW("failed");

    return ret;
}

/**
 * @brief Select ime_info mname data.
 *
 * @param mode  ISE type
 * @param mname The list to store module name
 *
 * @return the number of selected row.
 */
EXAPI int isf_db_select_module_name_by_mode(TOOLBAR_MODE_T mode, std::vector<String> &mname)
{
    int ret = 0;

    mname.clear();

    if (_db_connect() == 0) {
        ret = _db_select_module_name_by_mode(mode, mname);
        _db_disconnect();
    }
    else
        LOGW("failed");

    return ret;
}

/**
 * @brief Select ime_info mpath data.
 *
 * @param mode  ISE type
 * @param mpath The list to store module path
 *
 * @return the number of selected row.
 */
EXAPI int isf_db_select_module_path_by_mode(TOOLBAR_MODE_T mode, std::vector<String> &mpath)
{
    int ret = 0;

    mpath.clear();

    if (_db_connect() == 0) {
        ret = _db_select_module_path_by_mode(mode, mpath);
        _db_disconnect();
    }
    else
        LOGW("failed");

    return ret;
}

/**
 * @brief Select appids by pkgid in ime_info table.
 *
 * @param pkgid pkgid to get appid data.
 * @param appids    The list to store appid; there might be multiple appids for one pkgid.
 *
 * @return the number of selected row.
 */
EXAPI int isf_db_select_appids_by_pkgid(const char *pkgid, std::vector<String> &appids)
{
    int ret = 0;

    appids.clear();

    if (!pkgid) {
        LOGW("pkgid is null.");
        return ret;
    }

    if (_db_connect() == 0) {
        ret = _db_select_appids_by_pkgid(pkgid, appids);
        _db_disconnect();
    }
    else
        LOGW("failed");

    return ret;
}

/**
 * @brief Select "is_enabled" by appid in ime_info table.
 *
 * @param[in] appid appid in ime_info table.
 * @param[out] is_enabled Helper ISE enabled or not.
 *
 * @return 1 if it is successful, otherwise return 0.
 */
EXAPI int isf_db_select_is_enabled_by_appid(const char *appid, bool *is_enabled)
{
    int ret = 0;

    if (!appid || !is_enabled) {
        LOGW("Input parameter is null.");
        return ret;
    }

    if (_db_connect() == 0) {
        ret = _db_select_is_enabled_by_appid(appid, is_enabled);
        _db_disconnect();
    }
    else
        LOGW("failed");

    return ret;
}


/**
 * @brief Gets the count of the same module name in ime_info table.
 *
 * @param module_name Module name
 *
 * @return the number of selected row.
 */
EXAPI int isf_db_select_count_by_module_name(const char *module_name)
{
    int ret = 0;

    if (!module_name) {
        LOGW("module_name is null.");
        return ret;
    }

    if (_db_connect() == 0) {
        ret = _db_select_count_by_module_name(module_name);
        _db_disconnect();
    }
    else
        LOGW("failed");

    return ret;
}

/**
 * @brief Update label data by appid in ime_info table.
 *
 * @param appid appid in ime_info table.
 * @param label label to be updated in ime_info table.
 *
 * @return 1 if it is successful, otherwise return 0.
 */
EXAPI int isf_db_update_label_by_appid(const char *appid, const char *label)
{
    int ret = 0;

    if (!appid || !label) {
        LOGW("Input parameter is null.");
        return ret;
    }

    if (_db_connect() == 0) {
        ret = _db_update_label_by_appid(appid, label);
        _db_disconnect();
    }
    else
        LOGW("failed");

    return ret;
}

/**
 * @brief Update disp_lang data in ime_info table.
 *
 * @param disp_lang display language code
 *
 * @return 1 if it is successful, otherwise return 0.
 */
EXAPI int isf_db_update_disp_lang(const char *disp_lang)
{
    int ret = 0;

    if (!disp_lang) {
        LOGW("Input parameter is null.");
        return ret;
    }

    if (_db_connect() == 0) {
        ret = _db_update_disp_lang(disp_lang);
        _db_disconnect();
    }
    else
        LOGW("failed");

    return ret;
}

/**
 * @brief Update "is_enabled" data by appid in ime_info table.
 *
 * @param appid appid in ime_info table.
 * @param is_enabled Helper ISE enabled or not.
 *
 * @return 1 if it is successful, otherwise return 0.
 */
EXAPI int isf_db_update_is_enabled_by_appid(const char *appid, bool is_enabled)
{
    int ret = 0;

    if (!appid) {
        LOGW("Input parameter is null.");
        return ret;
    }

    if (_db_connect() == 0) {
        ret = _db_update_is_enabled_by_appid(appid, is_enabled);
        _db_disconnect();
    }
    else
        LOGW("failed");

    return ret;
}

/**
 * @brief Update "has_option" data by appid in ime_info table.
 *
 * @param appid appid in ime_info table.
 * @param enabled Helper ISE enabled or not.
 *
 * @return 1 if it is successful, otherwise return 0.
 */
EXAPI int isf_db_update_has_option_by_appid(const char *appid, bool has_option)
{
    int ret = 0;

    if (!appid) {
        LOGW("Input parameter is null.");
        return ret;
    }

    if (_db_connect() == 0) {
        ret = _db_update_has_option_by_appid(appid, has_option);
        _db_disconnect();
    }
    else
        LOGW("failed");

    return ret;
}

/**
 * @brief Update data to ime_info table except for a few columns.
 *
 * @param ime_db The pointer of ImeInfoDB
 *
 * @return the number of updated data.
 */
EXAPI int isf_db_update_ime_info(ImeInfoDB *ime_db)
{
    int ret = 0;

    if (!ime_db) {
        LOGW("Input parameter is null.");
        return ret;
    }

    if (_db_connect() == 0) {
        ret = _db_update_ime_info(ime_db);
        _db_disconnect();
    }
    else
        LOGW("failed");

    return ret;
}

/**
 * @brief Insert data to ime_info table.
 *
 * @param ime_db The pointer of ImeInfoDB
 *
 * @return the number of inserted data.
 */
EXAPI int isf_db_insert_ime_info(ImeInfoDB *ime_db)
{
    int ret = 0;

    if (!ime_db) {
        return ret;
    }

    if (_db_connect() == 0) {
        ret = _db_insert_ime_info(ime_db);
        _db_disconnect();
    }
    else {
        ISF_SAVE_LOG("_db_connect() failed\n");
    }

    return ret;
}

/**
 * @brief Delete ime_info data with pkgid.
 *
 * @param pkgid pkgid to delete ime_info table.
 *
 * @return 1 if it is successful, otherwise return 0.
 */
EXAPI int isf_db_delete_ime_info_by_pkgid(const char *pkgid)
{
    int ret = 0;

    if (!pkgid) {
        LOGW("Input parameter is null.");
        return ret;
    }

    if (_db_connect() == 0) {
        ret = _db_delete_ime_info_by_pkgid(pkgid);
        _db_disconnect();
    }
    else
        LOGW("failed");

    return ret;
}

/**
 * @brief Delete ime_info data with appid.
 *
 * @param appid appid to delete ime_info table.
 *
 * @return 1 if it is successful, otherwise return 0.
 */
EXAPI int isf_db_delete_ime_info_by_appid(const char *appid)
{
    int ret = 0;

    if (!appid) {
        LOGW("Input parameter is null.");
        return ret;
    }

    if (_db_connect() == 0) {
        ret = _db_delete_ime_info_by_appid(appid);
        _db_disconnect();
    }
    else
        LOGW("failed");

    return ret;
}

/**
 * @brief Delete all ime_info data.
 *
 * @return 1 if it is successful, otherwise return 0.
 */
EXAPI int isf_db_delete_ime_info(void)
{
    int ret = 0;

    if (_db_connect() == 0) {
        ret = _db_delete_ime_info();
        _db_disconnect();
    }
    else
        LOGW("failed");

    return ret;
}

/*
vi:ts=4:nowrap:ai:expandtab
*/

