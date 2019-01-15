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

#ifndef __ISF_QUERY_UTILITY_H__
#define __ISF_QUERY_UTILITY_H__

using namespace scim;


/////////////////////////////////////////////////////////////////////////////
// Declaration of macro.
/////////////////////////////////////////////////////////////////////////////

typedef struct {
    String name;
    String uuid;    // Replaced by appid.
    String module;
    String language;
    String icon;
    TOOLBAR_MODE_T mode;
    uint32 option;
    String locales;
} ISEINFO;

typedef struct {
    String appid;   // Replace the previous uuid.
    String label;
    String languages;
    String iconpath;
    String pkgid;
    String pkgtype;
    String exec;
    String module_name;
    String module_path;
    TOOLBAR_MODE_T mode;
    uint32 options;
    uint32 is_enabled;
    uint32 is_preinstalled;
    int has_option;
    String display_lang;
} ImeInfoDB;

EXAPI int isf_db_select_all_ime_info(std::vector<ImeInfoDB> &ime_info);
EXAPI int isf_db_select_ime_info_by_appid(const char *appid, ImeInfoDB *pImeInfo);
EXAPI int isf_db_select_module_name_by_mode(TOOLBAR_MODE_T mode, std::vector<String> &mname);
EXAPI int isf_db_select_module_path_by_mode(TOOLBAR_MODE_T mode, std::vector<String> &mpath);
EXAPI int isf_db_select_appids_by_pkgid(const char *pkgid, std::vector<String> &appids);
EXAPI int isf_db_select_is_enabled_by_appid(const char *appid, bool *is_enabled);
EXAPI int isf_db_select_count_by_module_name(const char *module_name);
EXAPI int isf_db_update_label_by_appid(const char *appid, const char *label);
EXAPI int isf_db_update_disp_lang(const char *disp_lang);
EXAPI int isf_db_update_is_enabled_by_appid(const char *appid, bool is_enabled);
EXAPI int isf_db_update_has_option_by_appid(const char *appid, bool has_option);
EXAPI int isf_db_update_ime_info(ImeInfoDB *ime_db);
EXAPI int isf_db_insert_ime_info(ImeInfoDB *ime_db);
EXAPI int isf_db_delete_ime_info_by_pkgid(const char *pkgid);
EXAPI int isf_db_delete_ime_info_by_appid(const char *appid);
EXAPI int isf_db_delete_ime_info(void);

#endif /* __ISF_QUERY_UTILITY_H__ */

/*
vi:ts=4:ai:nowrap:expandtab
*/
