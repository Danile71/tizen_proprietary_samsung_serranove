/*
   Copyright (C) 2012 Samsung Electronics

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
#include "config.h"

#if ENABLE(TIZEN_OPEN_PANEL)
#include "OpenPanel.h"

#include "ewk_view.h"
#include "JavaScriptPopup.h"
#include "WKString.h"
#include <app_control_internal.h>
#include <Ecore_X.h>
#include <Elementary.h>
#include <WebCore/LocalizedStrings.h>

namespace WebKit {

OpenPanel::OpenPanel(Evas_Object* ewkView)
    : m_ewkView(ewkView)
{
}

OpenPanel::~OpenPanel()
{
    close();
}

static void serviceResultCb(app_control_h request, app_control_h reply, app_control_result_e result, void* data)
{
    OpenPanel* openPanel = static_cast<OpenPanel*>(data);

    TIZEN_LOGI("result(%d)", result);

    if (!openPanel->ewkView()) {
        TIZEN_LOGE("view is already deleted. so do nothing");
        return;
    }

    if (result == APP_CONTROL_RESULT_SUCCEEDED) {
        char* operation;
        int ret = app_control_get_operation(request, &operation);
        if (ret) {
            TIZEN_LOGE("app_control_ERROR (0x%08x)", ret);
            ewk_view_open_panel_reply(openPanel->ewkView(), 0, false);
            return;
        }

        Eina_List* selectedFiles = 0;
        char** resultFileNames = 0;
        int count;
        ret = app_control_get_extra_data_array(reply, APP_CONTROL_DATA_SELECTED, &resultFileNames, &count);
        if (ret == APP_CONTROL_ERROR_NONE && count > 0) {
            for (int i = 0; i < count; ++i)
                selectedFiles = eina_list_append(selectedFiles, resultFileNames[i]);
            free(resultFileNames);
        } else {
            TIZEN_LOGE("app_control_get_extra_data_array fail. app_control_ERROR (0x%08x). retry app_control_get_extra_data", ret);
            char* resultFileName = 0;
            ret = app_control_get_extra_data(reply, APP_CONTROL_DATA_SELECTED, &resultFileName);
            if (ret == APP_CONTROL_ERROR_NONE)
                selectedFiles = eina_list_append(selectedFiles, resultFileName);
            else
                TIZEN_LOGE("app_control_ERROR (0x%08x)", ret);
        }
        free(operation);

        if (selectedFiles) {
            ewk_view_open_panel_reply(openPanel->ewkView(), selectedFiles, true);

            void* data = 0;
            EINA_LIST_FREE(selectedFiles, data)
                free(static_cast<char*>(data));
        } else {
            TIZEN_LOGE("selectedFiles is null");
            ewk_view_open_panel_reply(openPanel->ewkView(), 0, false);
        }
    } else
        ewk_view_open_panel_reply(openPanel->ewkView(), 0, false);
}

bool OpenPanel::launchApplicationForType(OpenPanelOperationType operationType, OpenPanelType panelType, bool allow_multiple_files)
{
    app_control_h svcHandle = 0;
    if (app_control_create(&svcHandle) < 0 || !svcHandle) {
        TIZEN_LOGE("app_control_create is fail");
        return false;
    }

    if (operationType == OPEN_PANEL_PICK) {
        app_control_set_operation(svcHandle, APP_CONTROL_OPERATION_PICK);
        const char* extraApps[1] = {"com.samsung.camera-appcontrol"};
        int ret = app_control_add_extra_data_array(svcHandle, "http://tizen.org/appcontrol/data/selector_extra_list", extraApps, 1);
        if (ret != APP_CONTROL_ERROR_NONE)
            TIZEN_LOGE("app_control_add_extra_data_array(http://tizen.org/appcontrol/data/selector_extra_list) FAIL returned(%d)", ret);
    } else
        app_control_set_operation(svcHandle, APP_CONTROL_OPERATION_CREATE_CONTENT);

    if (panelType == OPEN_PANEL_IMAGE_TYPE)
        app_control_set_mime(svcHandle, "image/*");
    else if (panelType == OPEN_PANEL_VIDEO_TYPE)
        app_control_set_mime(svcHandle, "video/*");
    else if (panelType == OPEN_PANEL_AUDIO_TYPE)
        app_control_set_mime(svcHandle, "audio/*");
    else if (panelType == OPEN_PANEL_ALL_TYPE)
        app_control_set_mime(svcHandle, "*/*");

    if (allow_multiple_files)
        app_control_add_extra_data(svcHandle, "http://tizen.org/appcontrol/data/selection_mode", "multiple");
    else
        app_control_add_extra_data(svcHandle, "http://tizen.org/appcontrol/data/selection_mode", "single");

    app_control_set_window(svcHandle, elm_win_xwindow_get(elm_object_parent_widget_get(m_ewkView)));
    int ret = app_control_send_launch_request(svcHandle, serviceResultCb, this);
    if (ret != APP_CONTROL_ERROR_NONE) {
        if (ret == APP_CONTROL_ERROR_APP_NOT_FOUND)
            ewkViewGetPageClient(m_ewkView)->viewImpl()->javascriptPopup->alert(openPanelErrorText().utf8().data());

        app_control_destroy(svcHandle);
        TIZEN_LOGE("app_control_ERROR (0x%08x)", ret);
        return false;
    }

    app_control_destroy(svcHandle);

    return true;
}


bool OpenPanel::launchFileSystem(OpenPanelType panelType, bool allow_multiple_files)
{
    app_control_h svcHandle = 0;
    if (app_control_create(&svcHandle) < 0 || !svcHandle) {
        TIZEN_LOGE("app_control_create is fail");
        return false;
    }

    app_control_set_operation(svcHandle, APP_CONTROL_OPERATION_PICK);

    if (panelType == OPEN_PANEL_IMAGE_TYPE)
        app_control_set_mime(svcHandle, "image/*");
    else if (panelType == OPEN_PANEL_VIDEO_TYPE)
        app_control_set_mime(svcHandle, "video/*");
    else if (panelType == OPEN_PANEL_AUDIO_TYPE)
        app_control_set_mime(svcHandle, "audio/*");
    else
        app_control_set_mime(svcHandle, "*/*");

    if (allow_multiple_files)
        app_control_add_extra_data(svcHandle, "http://tizen.org/appcontrol/data/selection_mode", "multiple");
    else
        app_control_add_extra_data(svcHandle, "http://tizen.org/appcontrol/data/selection_mode", "single");
    app_control_set_app_id(svcHandle, "myfile-efl-lite");

    app_control_set_window(svcHandle, elm_win_xwindow_get(elm_object_parent_widget_get(m_ewkView)));
    int ret = app_control_send_launch_request(svcHandle, serviceResultCb, this);
    if (ret != APP_CONTROL_ERROR_NONE) {
        if (ret == APP_CONTROL_ERROR_APP_NOT_FOUND)
            ewkViewGetPageClient(m_ewkView)->viewImpl()->javascriptPopup->alert(openPanelErrorText().utf8().data());
        app_control_destroy(svcHandle);
        TIZEN_LOGE("app_control_ERROR (0x%08x)", ret);
        return false;
    }

    app_control_destroy(svcHandle);
    return true;
}

bool OpenPanel::openPanel(Evas_Object* ewkView, Eina_Bool allow_multiple_files, Eina_List* accepted_mime_types, const char* capture, void* userData)
{
    char *accept_type = NULL;
    unsigned int n = eina_list_count(accepted_mime_types);
    if (n > 0) accept_type = (char *) eina_list_nth(accepted_mime_types, n-1);

// FIXME: plese note that the following code makes decision about opening a
// specific type of panel based on the mime-type that was last in the list of
// accepted mime types. This looks like an error - should a bug be submitted for
// this functionality, this method is the prime candidate for fixing.

    OpenPanelType panelType;
    OpenPanelOperationType operationType;
    if (capture && strcmp(capture, "notexist")) {
        operationType = OPEN_PANEL_CREATE_CONTENT;
         if (accept_type == NULL) {
             if (!strcmp(capture, "camera"))
                panelType = OPEN_PANEL_IMAGE_TYPE;
            else if (!strcmp(capture, "camcorder"))
                panelType = OPEN_PANEL_VIDEO_TYPE;
            else if (!strcmp(capture, "microphone"))
                panelType = OPEN_PANEL_AUDIO_TYPE;
            else
                panelType = OPEN_PANEL_ALL_TYPE;
        } else {
             if (strstr(accept_type, "image"))
                panelType = OPEN_PANEL_IMAGE_TYPE;
            else if (strstr(accept_type, "video"))
                panelType = OPEN_PANEL_VIDEO_TYPE;
            else if (strstr(accept_type, "audio"))
                panelType = OPEN_PANEL_AUDIO_TYPE;
            else
                panelType = OPEN_PANEL_ALL_TYPE;
        }
        if (allow_multiple_files || panelType == OPEN_PANEL_ALL_TYPE || !strcmp(capture, "filesystem"))
            return launchFileSystem(panelType, allow_multiple_files);
    } else {
        operationType = OPEN_PANEL_PICK;
        if (accept_type) {
            if (strstr(accept_type, "image"))
                panelType = OPEN_PANEL_IMAGE_TYPE;
            else if (strstr(accept_type, "video"))
                panelType = OPEN_PANEL_VIDEO_TYPE;
            else if (strstr(accept_type, "audio"))
                panelType = OPEN_PANEL_AUDIO_TYPE;
            else
                panelType = OPEN_PANEL_ALL_TYPE;
        } else
            panelType = OPEN_PANEL_ALL_TYPE;
    }

    return launchApplicationForType(operationType, panelType, allow_multiple_files);
}


Evas_Object* OpenPanel::ewkView()
{
    return m_ewkView;
}

void OpenPanel::close()
{
    if (m_ewkView) {
        ewk_view_open_panel_reply(m_ewkView, NULL, EINA_FALSE);
        m_ewkView = 0;
    }
}

} // namespace WebKit

#endif // ENABLE(TIZEN_OPEN_PANEL)

