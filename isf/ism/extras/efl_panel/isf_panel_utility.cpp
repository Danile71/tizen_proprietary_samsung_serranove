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
#define Uses_SCIM_PANEL_AGENT
#define Uses_SCIM_IMENGINE_MODULE
#define Uses_SCIM_HELPER_MODULE
#define Uses_SCIM_COMPOSE_KEY


#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dlog.h>
#include <errno.h>
#include "scim.h"
#include "scim_private.h"
#include "scim_stl_map.h"
#include "isf_panel_efl.h"
#include "isf_panel_utility.h"
#include "isf_query_utility.h"
#include <pkgmgr-info.h>
#include "isf_pkg.h"

/////////////////////////////////////////////////////////////////////////////
// Declaration of macro.
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// Declaration of global variables.
/////////////////////////////////////////////////////////////////////////////
MapStringVectorSizeT         _groups;
std::vector<ImeInfoDB> _ime_info;

/////////////////////////////////////////////////////////////////////////////
// Declaration of internal variables.
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Declaration of internal functions.
/////////////////////////////////////////////////////////////////////////////

/**
 * @brief Get all ISEs support languages.
 *
 * @param all_langs The list to store all languages.
 */
void isf_get_all_languages (std::vector<String> &all_langs)
{
    String lang_name;

    for (MapStringVectorSizeT::iterator it = _groups.begin (); it != _groups.end (); ++it) {
        lang_name = scim_get_language_name_english (it->first);
        all_langs.push_back (lang_name);
    }
}

/**
 * @brief Get keyboard ISE
 *
 * @param config The config pointer for loading keyboard ISE.
 * @param ise_uuid The keyboard ISE uuid.
 * @param ise_name The keyboard ISE name.
 * @param ise_option The keyboard ISE option.
 */
void isf_get_keyboard_ise (const ConfigPointer &config, String &ise_uuid, String &ise_name, uint32 &ise_option)
{
    String uuid = config->read (String (SCIM_CONFIG_DEFAULT_IMENGINE_FACTORY) + String ("/") + String ("~other"), String (""));
    if (ise_uuid.length () > 0)
        uuid = ise_uuid;
    for (unsigned int i = 0; i < _ime_info.size (); i++) {
        if (uuid == _ime_info[i].appid) {
            ise_uuid = _ime_info[i].appid;
            ise_name = _ime_info[i].label;
            ise_option = _ime_info[i].options;
            return;
        }
    }
    ise_name = String ("");
    ise_uuid = String ("");
}

/**
 * @brief Get all keyboard ISEs for the specific languages.
 *
 * @param lang_list The specific languages list.
 * @param uuid_list The list to store keyboard ISEs' UUID.
 * @param name_list The list to store keyboard ISEs' name.
 * @param bCheckOption The flag to check whether support hardware keyboard.
 */
void isf_get_keyboard_ises_in_languages (const std::vector<String> &lang_list,
                                         std::vector<String>       &uuid_list,
                                         std::vector<String>       &name_list,
                                         bool                       bCheckOption)
{
    String lang_name;

    for (MapStringVectorSizeT::iterator it = _groups.begin (); it != _groups.end (); ++it) {
        lang_name = scim_get_language_name_english (it->first);

        if (std::find (lang_list.begin (), lang_list.end (), lang_name) != lang_list.end ()) {
            for (size_t i = 0; i < it->second.size (); i++) {
                if (_ime_info[it->second[i]].mode != TOOLBAR_KEYBOARD_MODE)
                    continue;
                if (bCheckOption && (_ime_info[it->second[i]].options & SCIM_IME_NOT_SUPPORT_HARDWARE_KEYBOARD))
                    continue;
                if (std::find (uuid_list.begin (), uuid_list.end (), _ime_info[it->second[i]].appid) == uuid_list.end ()) {
                    uuid_list.push_back (_ime_info[it->second[i]].appid);
                    name_list.push_back (_ime_info[it->second[i]].label);
                }
            }
        }
    }
}

/**
 * @brief Get all helper ISEs for the specific languages.
 *
 * @param lang_list The specific languages list.
 * @param uuid_list The list to store helper ISEs' UUID.
 * @param name_list The list to store helper ISEs' name.
 */
void isf_get_helper_ises_in_languages (const std::vector<String> &lang_list, std::vector<String> &uuid_list, std::vector<String> &name_list)
{
    String lang_name;

    for (MapStringVectorSizeT::iterator it = _groups.begin (); it != _groups.end (); ++it) {
        lang_name = scim_get_language_name_english (it->first);
        if (std::find (lang_list.begin (), lang_list.end (), lang_name) != lang_list.end ()) {
            for (size_t i = 0; i < it->second.size (); i++) {

                if (_ime_info[it->second[i]].mode != TOOLBAR_HELPER_MODE)
                    continue;
                // Avoid to add the same ISE
                if (std::find (uuid_list.begin (), uuid_list.end (), _ime_info[it->second[i]].appid) == uuid_list.end ()) {
                    uuid_list.push_back (_ime_info[it->second[i]].appid);
                    name_list.push_back (_ime_info[it->second[i]].label);
                }
            }
        }
    }
}

/**
 * @brief Load all ISEs information and ISF default languages.
 *
 * @param type The load ISE type.
 * @param config The config pointer for loading keyboard ISE.
 */
void isf_load_ise_information (LOAD_ISE_TYPE type, const ConfigPointer &config)
{
    /* Load IME info */
    if (_ime_info.size() == 0)
        isf_pkg_select_all_ime_info_db(_ime_info);

    /* Update _groups */
    _groups.clear();
    std::vector<String> ise_langs;
    for (size_t i = 0; i < _ime_info.size (); ++i) {
        scim_split_string_list(ise_langs, _ime_info[i].languages);
        for (size_t j = 0; j < ise_langs.size (); j++) {
            if (std::find (_groups[ise_langs[j]].begin (), _groups[ise_langs[j]].end (), i) == _groups[ise_langs[j]].end ()) {
                _groups[ise_langs[j]].push_back (i);
            }
        }
        ise_langs.clear ();
    }
}

/**
 * @brief Get normalized language name.
 *
 * @param src_str The language name before normalized.
 *
 * @return normalized language name.
 */
String isf_get_normalized_languages (String src_str)
{
    if (src_str.length () == 0)
        return String ("en");

    std::vector<String> str_list, dst_list;
    scim_split_string_list (str_list, src_str);

    for (size_t i = 0; i < str_list.size (); i++)
        dst_list.push_back (scim_get_normalized_language (str_list[i]));

    String dst_str =  scim_combine_string_list (dst_list);

    return dst_str;
}

/*
vi:ts=4:nowrap:ai:expandtab
*/
