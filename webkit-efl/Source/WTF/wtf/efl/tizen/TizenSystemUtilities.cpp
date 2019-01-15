/*
 * Copyright (C) 2013 Samsung Electronics
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "config.h"
#include "TizenSystemUtilities.h"
#include "CurrentTime.h"
#include <wtf/text/WTFString.h>

#include <E_DBus.h>

namespace WTF {

int invokeDbusMethodSync(const char* destination, const char* path, const char* name, const char* method, const char* signal, const char* parameters[])
{
    DBusConnection* connection = dbus_bus_get(DBUS_BUS_SYSTEM, 0);
    if (!connection) {
#if ENABLE(TIZEN_DLOG_SUPPORT)
        TIZEN_LOGE("dbus_bus_get error");
#endif
        return -EBADMSG;
    }

    DBusMessage* message = dbus_message_new_method_call(destination, path, name, method);
    if (!message) {
#if ENABLE(TIZEN_DLOG_SUPPORT)
        TIZEN_LOGE("dbus_message_new_method_call(%s:%s-%s) error", path, name, method);
#endif
        return -EBADMSG;
    }

    DBusMessageIter iter;
    dbus_message_iter_init_append(message, &iter);

    int returnedValue = 0;
    if (signal && parameters) {
        int i = 0;
        int int_type;
        uint64_t int64_type;
        for (char* ch = const_cast<char*>(signal); *ch != '\0'; ++i, ++ch) {
            switch (*ch) {
            case 'i':
                int_type = atoi(parameters[i]);
                dbus_message_iter_append_basic(&iter, DBUS_TYPE_INT32, &int_type);
                break;
            case 'u':
                int_type = atoi(parameters[i]);
                dbus_message_iter_append_basic(&iter, DBUS_TYPE_UINT32, &int_type);
                break;
            case 't':
                int64_type = atoi(parameters[i]);
                dbus_message_iter_append_basic(&iter, DBUS_TYPE_UINT64, &int64_type);
                break;
            case 's':
                dbus_message_iter_append_basic(&iter, DBUS_TYPE_STRING, parameters[i]);
                break;
            default:
                returnedValue = -EINVAL;
            }
        }
    }

    if (returnedValue < 0) {
#if ENABLE(TIZEN_DLOG_SUPPORT)
        TIZEN_LOGE("append_variant error(%d)", returnedValue);
#endif
        dbus_message_unref(message);
        return -EBADMSG;
    }

    DBusError error;
    dbus_error_init(&error);

    DBusMessage* reply = dbus_connection_send_with_reply_and_block(connection, message, -1/*Default Timeout(25*1000)*/, &error);
    dbus_message_unref(message);
    if (!reply) {
#if ENABLE(TIZEN_DLOG_SUPPORT)
        TIZEN_LOGE("dbus_connection_send error(%s:%s)", error.name, error.message);
#endif
        dbus_error_free(&error);
        return -EBADMSG;
    }

    int result = 0;
    returnedValue = dbus_message_get_args(reply, &error, DBUS_TYPE_INT32, &result, DBUS_TYPE_INVALID);
    dbus_message_unref(reply);
    if (!returnedValue) {
#if ENABLE(TIZEN_DLOG_SUPPORT)
        TIZEN_LOGE("no message : [%s:%s]", error.name, error.message);
#endif
        dbus_error_free(&error);
        return -EBADMSG;
    }

    return result;
}

#if ENABLE(TIZEN_BROWSER_DASH_MODE)
static double s_dashModeSetTime = 0;
double dashModeSetTime()
{
    return s_dashModeSetTime;
}

void setWebkitDashMode(DashMode mode, int flag)
{
    const char* destination = "org.tizen.system.deviced";
    const char* path = "/Org/Tizen/System/DeviceD/PmQos";
    const char* name = "org.tizen.system.deviced.PmQos";
    const char* method;
    char* modeName = "";

    switch(mode) {
    case DashModeBrowserJavaScript:
        modeName = "BrowserJavaScript";
        break;
    case DashModeBrowserScroll:
        modeName = "BrowserScroll";
        break;
    case DashModeGpuBoost:
        modeName = "GpuBoost";
        break;
    case DashModeBrowserDash:
    case DashModeBrowserLoading:
    case DashModeNone:
    default:
        TIZEN_LOGE("invalidMode(%d:%d)", mode, flag);
        return;
    }

    const char* parameters[1];
    char val[32];
    snprintf(val, sizeof(val), "%d", flag);
    parameters[0] = val;
    method = modeName;

    int ret = invokeDbusMethodSync(destination, path, name, method, "i", parameters);
    if (ret) {
#if ENABLE(TIZEN_DLOG_SUPPORT)
        TIZEN_LOGE("setWebkitDashMode(%d:%d) returns %d", mode, flag, ret);
#endif
    }
}
#endif

static String getTokenKey(FILE* file)
{
    if (file) {
        char buffer[128];
        memset(buffer, 0, 128);

        if (fscanf(file, "%s", buffer) > 0)
            return String(buffer);
    }

    return String();
}

size_t getFreeMemory()
{
    const char* TIZEN_MEMORY_INFO_PATH = "/proc/meminfo";
    size_t memFree = 0, memCached = 0;
    FILE* fSystemMemoryInfo = fopen(TIZEN_MEMORY_INFO_PATH, "r");

    if (fSystemMemoryInfo) {
        while (!feof(fSystemMemoryInfo)) {
            String strToken = getTokenKey(fSystemMemoryInfo);
            if (strToken == "MemFree:")
                memFree = getTokenKey(fSystemMemoryInfo).toInt();
            else if (strToken == "Cached:") {
                memCached = getTokenKey(fSystemMemoryInfo).toInt();
                break;
            }
        }
        fclose(fSystemMemoryInfo);
    }

    return (memFree + memCached);
}

} // namespace WTF
