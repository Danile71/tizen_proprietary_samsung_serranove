/*
 * Copyright (c) 2013 Samsung Electronics Co., Ltd All Rights Reserved
 *
 * Licensed under the Apache License, Version 2.0 (the License);
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef __TIZEN_WEBKIT_EFL_DOC_H__
#define __TIZEN_WEBKIT_EFL_DOC_H__

/**
 * @ingroup  CAPI_WEB_FRAMEWORK
 * @defgroup WEBVIEW WebView
 * @brief    The WebView API provides functions to display web pages and control web pages.
 *
 * @section  WEBVIEW_HEADER Required Header
 *   \#include <EWebKit.h>
 *
 * @section  WEBVIEW_OVERVIEW Overview
 * The WebView API provides functions to display web pages and control web pages. It is based on the WebKit engine, which is one of the most popular layout engines to render web pages.
 *
 * @section  WEBVIEW_SMART_OBJECT Smart object
 * It is WebKit main smart object. This object provides view related APIs of WebKit2 to EFL object.\n
 * The following signals (see evas_object_smart_callback_add()) are emitted:
 * <table>
 *     <tr>
 *         <th> Signals </th>
 *         <th> Type </th>
 *         <th> Description </th>
 *     </tr>
 *     <tr>
 *         <td> close,window </td>
 *         <td> void </td>
 *         <td> Window is closed </td>
 *     </tr>
 *     <tr>
 *         <td> contextmenu,customize </td>
 *         <td> Ewk_Context_Menu* </td>
 *         <td> Requested context menu items can be customized by app side </td>
 *     </tr>
 *     <tr>
 *         <td> contextmenu,selected </td>
 *         <td> Ewk_Context_Menu_Item* </td>
 *         <td> A context menu item is selected </td>
 *     </tr>
 *     <tr>
 *         <td> create,window </td>
 *         <td> Evas_Object** </td>
 *         <td> A new window is created </td>
 *     </tr>
 *     <tr>
 *         <td> fullscreen,enterfullscreen </td>
 *         <td> bool* </td>
 *         <td> Reports to enter fullscreen </td>
 *     </tr>
 *     <tr>
 *         <td> fullscreen,exitfullscreen </td>
 *         <td> void </td>
 *         <td> Reports to exit fullscreen </td>
 *     </tr>
 *     <tr>
 *         <td> load,committed </td>
 *         <td> void </td>
 *         <td> Reports load committed </td>
 *     </tr>
 *     <tr>
 *         <td> load,error </td>
 *         <td> Ewk_Error* </td>
 *         <td> Reports load error </td>
 *     </tr>
 *     <tr>
 *         <td> load,finished </td>
 *         <td> void </td>
 *         <td> Reports load finished </td>
 *     </tr>
 *     <tr>
 *         <td> load,progress </td>
 *         <td> double* </td>
 *         <td> Load progress has changed </td>
 *     </tr>
 *     <tr>
 *         <td> load,started </td>
 *         <td> void </td>
 *         <td> Reports load started </td>
 *     </tr>
 *     <tr>
 *         <td> policy,navigation,decide </td>
 *         <td> Ewk_Policy_Decision* </td>
 *         <td> A navigation policy decision should be taken </td>
 *     </tr>
 *     <tr>
 *         <td> policy,newwindow,decide </td>
 *         <td> Ewk_Policy_Decision* </td>
 *         <td> A new window policy decision should be taken </td>
 *     </tr>
 *     <tr>
 *         <td> policy,response,decide </td>
 *         <td> Ewk_Policy_Decision* </td>
 *         <td> A response policy decision should be taken </td>
 *     </tr>
 *     <tr>
 *         <td> text,found </td>
 *         <td> unsigned* </td>
 *         <td> The requested text was found and it gives the number of matches </td>
 *     </tr>
 *     <tr>
 *         <td> title,changed </td>
 *         <td> const char* </td>
 *         <td> Title of the main frame was changed </td>
 *     </tr>
  *     <tr>
 *         <td> url,changed </td>
 *         <td> const char* </td>
 *         <td> Url of the main frame was changed </td>
 *     </tr>
 * </table>
 */

/**
 * @ingroup WEBVIEW
 * @brief The structure type that creates a type name for #Ewk_Context.
 * @since_tizen 2.3
 */
typedef struct Ewk_Context Ewk_Context;

/**
 * @ingroup WEBVIEW
 * @brief The structure type that creates a type name for #Ewk_Settings.
 * @since_tizen 2.3
 */
typedef struct Ewk_Settings Ewk_Settings;

/**
 * @ingroup WEBVIEW
 * Enum values used to specify search options.
 * @brief  Enumeration that provides the option to find text.
 * @details It contains enum values used to specify search options.
 * @since_tizen 2.3
 */
enum Ewk_Find_Options {
    EWK_FIND_OPTIONS_NONE, /**< No search flags, this means a case sensitive, no wrap, forward only search */
    EWK_FIND_OPTIONS_CASE_INSENSITIVE = 1 << 0, /**< Case insensitive search */
    EWK_FIND_OPTIONS_AT_WORD_STARTS = 1 << 1, /**< Search text only at the beginning of the words */
    EWK_FIND_OPTIONS_TREAT_MEDIAL_CAPITAL_AS_WORD_START = 1 << 2, /**< Treat capital letters in the middle of words as word start */
    EWK_FIND_OPTIONS_BACKWARDS = 1 << 3, /**< Search backwards */
    EWK_FIND_OPTIONS_WRAP_AROUND = 1 << 4, /**< If not present the search stops at the end of the document */
    EWK_FIND_OPTIONS_SHOW_OVERLAY = 1 << 5, /**< Show overlay */
    EWK_FIND_OPTIONS_SHOW_FIND_INDICATOR = 1 << 6, /**< Show indicator */
    EWK_FIND_OPTIONS_SHOW_HIGHLIGHT = 1 << 7 /**< Show highlight */
};

/**
 * @ingroup WEBVIEW
 * @brief Enumeration that creates a type name for the #Ewk_Find_Options.
 * @since_tizen 2.3
 */
typedef enum Ewk_Find_Options Ewk_Find_Options;

/**
 * @ingroup WEBVIEW
 * @brief The structure type that creates a type name for #Ewk_Policy_Decision.
 * @since_tizen 2.3
 */
typedef struct _Ewk_Policy_Decision Ewk_Policy_Decision;

/**
 * @ingroup WEBVIEW
 * @brief The structure type that creates a type name for #Ewk_Error.
 * @since_tizen 2.3
 */
typedef struct Ewk_Error Ewk_Error;

#endif /* __TIZEN_WEBKIT_EFL_DOC_H__ */
