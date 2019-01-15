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

#define Uses_SCIM_TRANSACTION
#define Uses_ISF_IMCONTROL_CLIENT
#define Uses_SCIM_PANEL_AGENT
#define Uses_SCIM_IMENGINE_MODULE
#define Uses_SCIM_HELPER_MODULE

#include <string.h>
#include <vconf.h>
#include "scim.h"


namespace scim
{

typedef Signal1<void, int> IMControlClientSignalVoid;

#if ENABLE_LAZY_LAUNCH
static bool check_panel (const String &display)
{
    SocketAddress address;
    SocketClient client;

    uint32 magic;

    address.set_address (scim_get_default_panel_socket_address (display));

    if (!client.connect (address))
        return false;

    if (!scim_socket_open_connection (magic,
        String ("ConnectionTester"),
        String ("Panel"),
        client,
        1000)) {
        return false;
    }

    return true;
}

static bool
check_socket_frontend (void)
{
    SCIM_DEBUG_FRONTEND(1) << __FUNCTION__ << "...\n";

    SocketAddress address;
    SocketClient client;

    uint32 magic;

    address.set_address (scim_get_default_socket_frontend_address ());

    if (!client.connect (address)) {
        std::cerr << "check_socket_frontend's connect () failed.\n";
        return false;
    }

    if (!scim_socket_open_connection (magic,
                                      String ("ConnectionTester"),
                                      String ("SocketFrontEnd"),
                                      client,
                                      500)) {
        std::cerr << "check_socket_frontend's scim_socket_open_connection () failed.\n";
        return false;
    }

    return true;
}

static int
launch_socket_frontend ()
{
    SCIM_DEBUG_FRONTEND(1) << __FUNCTION__ << "...\n";

    std::vector<String>     engine_list;
    std::vector<String>     helper_list;
    std::vector<String>     load_engine_list;

    std::vector<String>::iterator it;

    std::cerr << "Launching a ISF daemon with Socket FrontEnd...\n";
    //get modules list
    scim_get_imengine_module_list (engine_list);
    scim_get_helper_module_list (helper_list);

    for (it = engine_list.begin (); it != engine_list.end (); it++) {
        if (*it != "socket")
            load_engine_list.push_back (*it);
    }
    for (it = helper_list.begin (); it != helper_list.end (); it++)
        load_engine_list.push_back (*it);

    return scim_launch (true,
        "simple",
        (load_engine_list.size () > 0 ? scim_combine_string_list (load_engine_list, ',') : "none"),
        "socket",
        NULL);
}
#endif

class IMControlClient::IMControlClientImpl
{
    SocketClient                                m_socket_imclient2panel;
    SocketClient                                m_socket_panel2imclient;
    int                                         m_socket_timeout;
    uint32                                      m_socket_i2p_magic_key;
    uint32                                      m_socket_p2i_magic_key;
    Transaction                                 m_trans;

    IMControlClientSignalVoid                   m_signal_show_ise;
    IMControlClientSignalVoid                   m_signal_hide_ise;

public:
    IMControlClientImpl ()
          : m_socket_timeout (scim_get_default_socket_timeout ()),
            m_socket_i2p_magic_key (0),
            m_socket_p2i_magic_key (0) {
    }

    int open_connection (void) {
        const char *p = getenv ("DISPLAY");
        String display;
        if (p) display = String (p);

        SocketAddress addr (scim_get_default_panel_socket_address (display));

        if (m_socket_imclient2panel.is_connected ()) close_connection ();

        bool ret=false, ret2=false;
        int count = 0;

        /* Try three times. */
        while (1) {
            ret = m_socket_imclient2panel.connect (addr);
            ret2 = m_socket_panel2imclient.connect (addr);
            if (!ret) {
                scim_usleep (100000);
#if ENABLE_LAZY_LAUNCH
                if (!check_socket_frontend ())
                    launch_socket_frontend ();
                if (!check_panel (display))
                    scim_launch_panel (true, "socket", display, NULL);
#endif
                for (int i = 0; i < 200; ++i) {
                    if (m_socket_imclient2panel.connect (addr)) {
                        ret = true;
                        break;
                    }
                    scim_usleep (100000);
                }
            }

            if (ret && scim_socket_open_connection (m_socket_i2p_magic_key, String ("IMControl_Active"), String ("Panel"), m_socket_imclient2panel, m_socket_timeout)) {
                if (ret2 && scim_socket_open_connection (m_socket_p2i_magic_key, String ("IMControl_Passive"), String ("Panel"), m_socket_panel2imclient, m_socket_timeout))
                    break;
            }
            m_socket_imclient2panel.close ();
            m_socket_panel2imclient.close ();

            if (count++ >= 3) break;

            scim_usleep (100000);
        }

        return m_socket_imclient2panel.get_id ();
    }

    void close_connection (void) {
        m_socket_imclient2panel.close ();
        m_socket_panel2imclient.close ();
        m_socket_i2p_magic_key = 0;
        m_socket_p2i_magic_key = 0;
    }

    bool is_connected (void) const {
        return (m_socket_imclient2panel.is_connected () && m_socket_panel2imclient.is_connected ());
    }

    int  get_panel2imclient_connection_number  (void) const {
        return m_socket_panel2imclient.get_id ();
    }

    bool prepare (void) {
        if (!m_socket_imclient2panel.is_connected ()) return false;

        m_trans.clear ();
        m_trans.put_command (SCIM_TRANS_CMD_REQUEST);
        m_trans.put_data (m_socket_i2p_magic_key);

        return true;
    }

    bool send (void) {
        if (!m_socket_imclient2panel.is_connected ()) return false;
        if (m_trans.get_data_type () != SCIM_TRANS_DATA_UNKNOWN)
            return m_trans.write_to_socket (m_socket_imclient2panel, 0x4d494353);
        return false;
    }

    bool set_active_ise_by_uuid (const char* uuid) {
        int cmd;
        m_trans.put_command (ISM_TRANS_CMD_SET_ACTIVE_ISE_BY_UUID);
        m_trans.put_data (uuid, strlen (uuid)+1);
        m_trans.write_to_socket (m_socket_imclient2panel);
        if (!m_trans.read_from_socket (m_socket_imclient2panel, m_socket_timeout)) {
            std::cerr << __func__ << " read_from_socket() may be timeout \n";
            return false;
        }

        if (m_trans.get_command (cmd) && cmd == SCIM_TRANS_CMD_REPLY &&
                m_trans.get_command (cmd) && cmd == SCIM_TRANS_CMD_OK) {
        } else {
            std::cerr << __func__ << " get_command() or get_data() may fail!!!\n";
            return false;
        }

        return true;
    }

    bool set_initial_ise_by_uuid (const char* uuid) {
        int cmd;
        m_trans.put_command (ISM_TRANS_CMD_SET_INITIAL_ISE_BY_UUID);
        m_trans.put_data (uuid, strlen (uuid)+1);
        m_trans.write_to_socket (m_socket_imclient2panel);
        if (!m_trans.read_from_socket (m_socket_imclient2panel, m_socket_timeout)) {
            std::cerr << __func__ << " read_from_socket() may be timeout \n";
            return false;
        }

        if (m_trans.get_command (cmd) && cmd == SCIM_TRANS_CMD_REPLY &&
                m_trans.get_command (cmd) && cmd == SCIM_TRANS_CMD_OK) {
        } else {
            std::cerr << __func__ << " get_command() or get_data() may fail!!!\n";
            return false;
        }

        return true;
    }

    bool get_active_ise (String &uuid) {
        int    cmd;
        String strTemp;

        m_trans.put_command (ISM_TRANS_CMD_GET_ACTIVE_ISE);
        m_trans.write_to_socket (m_socket_imclient2panel);
        if (!m_trans.read_from_socket (m_socket_imclient2panel, m_socket_timeout)) {
            std::cerr << __func__ << " read_from_socket() may be timeout \n";
            return false;
        }

        if (m_trans.get_command (cmd) && cmd == SCIM_TRANS_CMD_REPLY &&
                m_trans.get_command (cmd) && cmd == SCIM_TRANS_CMD_OK &&
                m_trans.get_data (strTemp)) {
            uuid = strTemp;
        } else {
            std::cerr << __func__ << " get_command() or get_data() may fail!!!\n";
            return false;
        }

        return true;
    }

    bool get_ise_list (int* count, char*** iselist) {
        int cmd;
        uint32 count_temp = 0;
        char **buf = NULL;
        size_t len;
        char *buf_temp = NULL;

        m_trans.put_command (ISM_TRANS_CMD_GET_ISE_LIST);
        m_trans.write_to_socket (m_socket_imclient2panel);
        if (!m_trans.read_from_socket (m_socket_imclient2panel, m_socket_timeout)) {
            std::cerr << __func__ << " read_from_socket() may be timeout \n";
            return false;
        }

        if (m_trans.get_command (cmd) && cmd == SCIM_TRANS_CMD_REPLY &&
                m_trans.get_command (cmd) && cmd == SCIM_TRANS_CMD_OK &&
                m_trans.get_data (count_temp) ) {
            if (count)
                *count = count_temp;
        } else {
            if (count)
                *count = 0;
            std::cerr << __func__ << " get_command() or get_data() may fail!!!\n";
            return false;
        }

        if (iselist) {
            if (count_temp > 0) {
                buf = (char**)calloc (1, count_temp * sizeof (char*));
                if (buf) {
                    for (uint32 i = 0; i < count_temp; i++) {
                        if (m_trans.get_data (&buf_temp, len)) {
                            if (buf_temp) {
                                buf[i] = strdup (buf_temp);
                                delete [] buf_temp;
                            }
                        }
                    }
                }
            }

            *iselist = buf;
        }

        return true;
    }

    bool get_ise_info (const char* uuid, String &name, String &language, int &type, int &option, String &module_name)
    {
        int    cmd;
        uint32 tmp_type, tmp_option;
        String tmp_name, tmp_language, tmp_module_name;

        m_trans.put_command (ISM_TRANS_CMD_GET_ISE_INFORMATION);
        m_trans.put_data (String (uuid));
        m_trans.write_to_socket (m_socket_imclient2panel);
        if (!m_trans.read_from_socket (m_socket_imclient2panel, m_socket_timeout)) {
            std::cerr << __func__ << " read_from_socket() may be timeout \n";
            return false;
        }

        if (m_trans.get_command (cmd) && cmd == SCIM_TRANS_CMD_REPLY &&
                m_trans.get_command (cmd) && cmd == SCIM_TRANS_CMD_OK &&
                m_trans.get_data (tmp_name) && m_trans.get_data (tmp_language) &&
                m_trans.get_data (tmp_type) && m_trans.get_data (tmp_option) && m_trans.get_data (tmp_module_name)) {
            name     = tmp_name;
            language = tmp_language;
            type     = tmp_type;
            option   = tmp_option;
            module_name = tmp_module_name;

            return true;
        } else {
            std::cerr << __func__ << " get_command() or get_data() may fail!!!\n";
            return false;
        }
    }

    bool reset_ise_option (void) {
        int cmd;

        m_trans.put_command (ISM_TRANS_CMD_RESET_ISE_OPTION);
        m_trans.write_to_socket (m_socket_imclient2panel);
        if (!m_trans.read_from_socket (m_socket_imclient2panel, m_socket_timeout)) {
            std::cerr << __func__ << " read_from_socket() may be timeout \n";
            return false;
        }

        if (m_trans.get_command (cmd) && cmd == SCIM_TRANS_CMD_REPLY &&
                m_trans.get_command (cmd) && cmd == SCIM_TRANS_CMD_OK) {
            return true;
        } else {
            std::cerr << __func__ << " get_command() is failed!!!\n";
            return false;
        }
    }

    void set_active_ise_to_default (void) {
        m_trans.put_command (ISM_TRANS_CMD_RESET_DEFAULT_ISE);
    }

    void show_ise_selector (void) {
        m_trans.put_command (ISM_TRANS_CMD_SHOW_ISF_CONTROL);
    }

    void show_ise_option_window (void) {
        m_trans.put_command (ISM_TRANS_CMD_SHOW_ISE_OPTION_WINDOW);
    }

    bool get_all_helper_ise_info (HELPER_ISE_INFO &info) {
        int cmd;
        std::vector<String> appid;
        std::vector<String> label;
        std::vector<uint32> is_enabled;
        std::vector<uint32> is_preinstalled;
        std::vector<uint32> has_option;

        info.appid.clear ();
        info.label.clear ();
        info.is_enabled.clear ();
        info.is_preinstalled.clear ();
        info.has_option.clear ();

        m_trans.put_command (ISM_TRANS_CMD_GET_ALL_HELPER_ISE_INFO);
        m_trans.write_to_socket (m_socket_imclient2panel);
        if (!m_trans.read_from_socket (m_socket_imclient2panel, m_socket_timeout)) {
            std::cerr << __func__ << " read_from_socket() may be timeout \n";
            return false;
        }

        if (m_trans.get_command (cmd) && cmd == SCIM_TRANS_CMD_REPLY &&
                m_trans.get_command (cmd) && cmd == SCIM_TRANS_CMD_OK &&
                m_trans.get_data (appid) ) {
        } else {
            std::cerr << __func__ << " get_command() or get_data() may fail!!!\n";
            return false;
        }

        if (appid.size () > 0) {
            m_trans.get_data (label);
            m_trans.get_data (is_enabled);
            m_trans.get_data (is_preinstalled);
            m_trans.get_data (has_option);
            if (appid.size () == label.size () && appid.size () == is_enabled.size () && appid.size () == is_preinstalled.size () && appid.size () == has_option.size ()) {
                info.appid = appid;
                info.label = label;
                info.is_enabled = is_enabled;
                info.is_preinstalled = is_preinstalled;
                info.has_option = has_option;
                return true;
            }
        }

        return false;
    }

    bool set_enable_helper_ise_info (const char *appid, bool is_enabled) {
        int cmd;

        if (!appid)
            return false;

        m_trans.put_command (ISM_TRANS_CMD_SET_ENABLE_HELPER_ISE_INFO);
        m_trans.put_data (String (appid));
        m_trans.put_data (static_cast<uint32>(is_enabled));
        m_trans.write_to_socket (m_socket_imclient2panel);
        if (!m_trans.read_from_socket (m_socket_imclient2panel, m_socket_timeout)) {
            std::cerr << __func__ << " read_from_socket() may be timeout \n";
            return false;
        }

        if (m_trans.get_command (cmd) && cmd == SCIM_TRANS_CMD_REPLY &&
            m_trans.get_command (cmd) && cmd == SCIM_TRANS_CMD_OK) {
            return true;
        } else {
            std::cerr << __func__ << " get_command() or get_data() may fail!!!\n";
            return false;
        }
    }

    bool show_helper_ise_list (void) {
        int    cmd;
        m_trans.put_command (ISM_TRANS_CMD_SHOW_HELPER_ISE_LIST);
        m_trans.write_to_socket (m_socket_imclient2panel);
        if (!m_trans.read_from_socket (m_socket_imclient2panel, m_socket_timeout)) {
            std::cerr << __func__ << " read_from_socket() may be timeout \n";
            return false;
        }

        if (m_trans.get_command (cmd) && cmd == SCIM_TRANS_CMD_REPLY &&
            m_trans.get_command (cmd) && cmd == SCIM_TRANS_CMD_OK) {
            return true;
        } else {
            std::cerr << __func__ << " get_command() or get_data() may fail!!!\n";
            return false;
        }
    }

    bool show_helper_ise_selector (void) {
        int    cmd;
        m_trans.put_command (ISM_TRANS_CMD_SHOW_HELPER_ISE_SELECTOR);
        m_trans.write_to_socket (m_socket_imclient2panel);
        if (!m_trans.read_from_socket (m_socket_imclient2panel, m_socket_timeout)) {
            std::cerr << __func__ << " read_from_socket() may be timeout \n";
            return false;
        }

        if (m_trans.get_command (cmd) && cmd == SCIM_TRANS_CMD_REPLY &&
            m_trans.get_command (cmd) && cmd == SCIM_TRANS_CMD_OK) {
            return true;
        } else {
            std::cerr << __func__ << " get_command() or get_data() may fail!!!\n";
            return false;
        }
    }

    bool is_helper_ise_enabled (const char* appid, int &enabled)
    {
        int    cmd;
        uint32 tmp_enabled;

        m_trans.put_command (ISM_TRANS_CMD_IS_HELPER_ISE_ENABLED);
        m_trans.put_data (String (appid));
        m_trans.write_to_socket (m_socket_imclient2panel);
        if (!m_trans.read_from_socket (m_socket_imclient2panel, m_socket_timeout)) {
            std::cerr << __func__ << " read_from_socket() may be timeout \n";
            return false;
        }

        if (m_trans.get_command (cmd) && cmd == SCIM_TRANS_CMD_REPLY &&
                m_trans.get_command (cmd) && cmd == SCIM_TRANS_CMD_OK &&
                m_trans.get_data (tmp_enabled)) {
            enabled = static_cast<int>(tmp_enabled);
            return true;
        } else {
            std::cerr << __func__ << " get_command() or get_data() may fail!!!\n";
            return false;
        }
    }

    bool get_recent_ime_geometry (int *x, int *y, int *w, int *h)
    {
        int    cmd;
        uint32 tmp_x, tmp_y, tmp_w, tmp_h;

        m_trans.put_command (ISM_TRANS_CMD_GET_RECENT_ISE_GEOMETRY);
        m_trans.write_to_socket (m_socket_imclient2panel);
        if (!m_trans.read_from_socket (m_socket_imclient2panel, m_socket_timeout)) {
            std::cerr << __func__ << " read_from_socket() may be timeout \n";
            return false;
        }

        if (m_trans.get_command (cmd) && cmd == SCIM_TRANS_CMD_REPLY &&
                m_trans.get_command (cmd) && cmd == SCIM_TRANS_CMD_OK &&
                m_trans.get_data (tmp_x) && m_trans.get_data (tmp_y) &&
                m_trans.get_data (tmp_w) && m_trans.get_data (tmp_h)) {
            if (x)
                *x = (int)tmp_x;

            if (y)
                *y = (int)tmp_y;

            if (w)
                *w = (int)tmp_w;

            if (h)
                *h = (int)tmp_h;

            if ((int)tmp_w == -1 && (int)tmp_h == -1)
                return false;

            return true;
        } else {
            std::cerr << __func__ << " get_command() or get_data() may fail!!!\n";
            return false;
        }
    }
};

IMControlClient::IMControlClient ()
        : m_impl (new IMControlClientImpl ())
{
}

IMControlClient::~IMControlClient ()
{
    delete m_impl;
}

int
IMControlClient::open_connection (void)
{
    return m_impl->open_connection ();
}

void
IMControlClient::close_connection (void)
{
    m_impl->close_connection ();
}

bool IMControlClient::is_connected (void) const
{
    return m_impl->is_connected ();
}

int IMControlClient::get_panel2imclient_connection_number (void) const
{
    return m_impl->get_panel2imclient_connection_number ();
}

bool
IMControlClient::prepare (void)
{
    return m_impl->prepare ();
}

bool
IMControlClient::send (void)
{
    return m_impl->send ();
}

bool IMControlClient::set_active_ise_by_uuid (const char* uuid)
{
    return m_impl->set_active_ise_by_uuid (uuid);
}

bool IMControlClient::set_initial_ise_by_uuid (const char* uuid)
{
    return m_impl->set_initial_ise_by_uuid (uuid);
}

bool IMControlClient::get_active_ise (String &uuid)
{
    return m_impl->get_active_ise (uuid);
}

bool IMControlClient::get_ise_list (int* count, char*** iselist)
{
    return m_impl->get_ise_list (count, iselist);
}

bool IMControlClient::get_ise_info (const char* uuid, String &name, String &language, int &type, int &option, String &module_name)
{
    return m_impl->get_ise_info (uuid, name, language, type, option, module_name);
}

bool IMControlClient::reset_ise_option (void)
{
    return m_impl->reset_ise_option ();
}

void IMControlClient::set_active_ise_to_default (void)
{
    m_impl->set_active_ise_to_default ();
}

void IMControlClient::show_ise_selector (void)
{
    m_impl->show_ise_selector ();
}

void IMControlClient::show_ise_option_window (void)
{
    m_impl->show_ise_option_window ();
}

bool IMControlClient::get_all_helper_ise_info (HELPER_ISE_INFO &info)
{
    return m_impl->get_all_helper_ise_info (info);
}

bool IMControlClient::set_enable_helper_ise_info (const char *appid, bool is_enabled)
{
    return m_impl->set_enable_helper_ise_info (appid, is_enabled);
}

bool IMControlClient::show_helper_ise_list (void)
{
    return m_impl->show_helper_ise_list ();
}

bool IMControlClient::show_helper_ise_selector (void)
{
    return m_impl->show_helper_ise_selector ();
}

bool IMControlClient::is_helper_ise_enabled (const char* appid, int &enabled)
{
    return m_impl->is_helper_ise_enabled (appid, enabled);
}

bool IMControlClient::get_recent_ime_geometry (int *x, int *y, int *w, int *h)
{
    return m_impl->get_recent_ime_geometry (x, y, w, h);
}
};

/*
vi:ts=4:nowrap:ai:expandtab
*/
