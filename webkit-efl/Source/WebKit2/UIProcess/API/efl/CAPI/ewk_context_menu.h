/*
 * Copyright (C) 2012 Samsung Electronics
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

/**
 * @file    ewk_context_menu.h
 * @brief   This file describes the Ewk Context Menu API.
 */

#ifndef ewk_context_menu_h
#define ewk_context_menu_h

#include <tizen.h>
#include <Eina.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup WEBVIEW
 * @{
 */

/**
 * \enum    _Ewk_Context_Menu_Item_Tag
 * @brief   Enumeration that provides the tags of items in the context menu.
 * @since_tizen 2.3
 */
enum _Ewk_Context_Menu_Item_Tag{
    EWK_CONTEXT_MENU_ITEM_TAG_NO_ACTION = 0,
    EWK_CONTEXT_MENU_ITEM_TAG_OPEN_LINK_IN_NEW_WINDOW,  /**< Open link in new window */
    EWK_CONTEXT_MENU_ITEM_TAG_DOWNLOAD_LINK_TO_DISK,    /**< Download link to disk */
    EWK_CONTEXT_MENU_ITEM_TAG_COPY_LINK_TO_CLIPBOARD,   /**< Copy link to clipboard */
    EWK_CONTEXT_MENU_ITEM_TAG_OPEN_IMAGE_IN_NEW_WINDOW, /**< Open image in new window */
    EWK_CONTEXT_MENU_ITEM_TAG_OPEN_IMAGE_IN_CURRENT_WINDOW, /**< Open image in current window */
    EWK_CONTEXT_MENU_ITEM_TAG_DOWNLOAD_IMAGE_TO_DISK,   /**< Download image to disk */
    EWK_CONTEXT_MENU_ITEM_TAG_COPY_IMAGE_TO_CLIPBOARD,  /**< Copy image to clipboard */
    EWK_CONTEXT_MENU_ITEM_TAG_OPEN_FRAME_IN_NEW_WINDOW,
    EWK_CONTEXT_MENU_ITEM_TAG_COPY, /**< Copy */
    EWK_CONTEXT_MENU_ITEM_TAG_GO_BACK,
    EWK_CONTEXT_MENU_ITEM_TAG_GO_FORWARD,
    EWK_CONTEXT_MENU_ITEM_TAG_STOP,
    EWK_CONTEXT_MENU_ITEM_TAG_RELOAD,
    EWK_CONTEXT_MENU_ITEM_TAG_CUT,  /**< Cut */
    EWK_CONTEXT_MENU_ITEM_TAG_PASTE,    /**< Paste */
    EWK_CONTEXT_MENU_ITEM_TAG_SPELLING_GUESS,
    EWK_CONTEXT_MENU_ITEM_TAG_NO_GUESSES_FOUND,
    EWK_CONTEXT_MENU_ITEM_TAG_IGNORE_SPELLING,
    EWK_CONTEXT_MENU_ITEM_TAG_LEARN_SPELLING,
    EWK_CONTEXT_MENU_ITEM_TAG_OTHER,
    EWK_CONTEXT_MENU_ITEM_TAG_SEARCH_IN_SPOTLIGHT,
    EWK_CONTEXT_MENU_ITEM_TAG_SEARCH_WEB,   /**< Search web */
    EWK_CONTEXT_MENU_ITEM_TAG_LOOK_UP_IN_DICTIONARY,
    EWK_CONTEXT_MENU_ITEM_TAG_OPEN_WITH_DEFAULT_APPLICATION,
    EWK_CONTEXT_MENU_ITEM_TAG_PDF_ACTUAL_SIZE,
    EWK_CONTEXT_MENU_ITEM_TAG_PDF_ZOOM_IN,
    EWK_CONTEXT_MENU_ITEM_TAG_PDF_ZOOM_OUT,
    EWK_CONTEXT_MENU_ITEM_TAG_PDF_AUTO_SIZE,
    EWK_CONTEXT_MENU_ITEM_TAG_PDF_SINGLE_PAGE,
    EWK_CONTEXT_MENU_ITEM_TAG_PDF_FACTING_PAGES,
    EWK_CONTEXT_MENU_ITEM_TAG_PDF_CONTINUOUS,
    EWK_CONTEXT_MENU_ITEM_TAG_PDF_NEXT_PAGE,
    EWK_CONTEXT_MENU_ITEM_TAG_PDF_PREVIOUS_PAGE,
    EWK_CONTEXT_MENU_ITEM_TAG_OPEN_LINK,    /**< Open link */
    EWK_CONTEXT_MENU_ITEM_TAG_IGNORE_GRAMMAR,
    EWK_CONTEXT_MENU_ITEM_TAG_SPELLING_MENU,
    EWK_CONTEXT_MENU_ITEM_TAG_SHOW_SPELLING_PANEL,
    EWK_CONTEXT_MENU_ITEM_TAG_CHECK_SPELLING,
    EWK_CONTEXT_MENU_ITEM_TAG_CHECK_SPELLING_WHILE_TYPING,
    EWK_CONTEXT_MENU_ITEM_TAG_CHECK_GRAMMAR_WITH_SPELLING,
    EWK_CONTEXT_MENU_ITEM_TAG_FONT_MENU,
    EWK_CONTEXT_MENU_ITEM_TAG_SHOW_FONTS,
    EWK_CONTEXT_MENU_ITEM_TAG_BOLD,
    EWK_CONTEXT_MENU_ITEM_TAG_ITALIC,
    EWK_CONTEXT_MENU_ITEM_TAG_UNDERLINE,
    EWK_CONTEXT_MENU_ITEM_TAG_OUTLINE,
    EWK_CONTEXT_MENU_ITEM_TAG_STYLES,
    EWK_CONTEXT_MENU_ITEM_TAG_SHOW_COLORS,
    EWK_CONTEXT_MENU_ITEM_TAG_SPEECH_MENU,
    EWK_CONTEXT_MENU_ITEM_TAG_START_SPEAKING,
    EWK_CONTEXT_MENU_ITEM_TAG_STOP_SPEAKING,
    EWK_CONTEXT_MENU_ITEM_TAG_WRITING_DIRECTION_MENU,
    EWK_CONTEXT_MENU_ITEM_TAG_DEFAULT_DIRECTION,
    EWK_CONTEXT_MENU_ITEM_TAG_LEFT_TO_RIGHT,
    EWK_CONTEXT_MENU_ITEM_TAG_RIGHT_TO_LEFT,
    EWK_CONTEXT_MENU_ITEM_TAG_PDF_SINGLE_PAGE_SCROLLING,
    EWK_CONTEXT_MENU_ITEM_TAG_PDF_FACING_PAGES_SCROLLING,
    EWK_CONTEXT_MENU_ITEM_TAG_INSPECT_ELEMENT,  /**< Inspect element */
    EWK_CONTEXT_MENU_ITEM_TAG_TEXT_DIRECTION_MENU,
    EWK_CONTEXT_MENU_ITEM_TAG_TEXT_DIRECTION_DEFAULT,
    EWK_CONTEXT_MENU_ITEM_TAG_TEXT_DIRECTION_LEFT_TO_RIGHT,
    EWK_CONTEXT_MENU_ITEM_TAG_TEXT_DIRECTION_RIGHT_TO_LEFT,
    EWK_CONTEXT_MENU_ITEM_TAG_CORRECT_SPELLING_AUTOMATICALLY,
    EWK_CONTEXT_MENU_ITEM_TAG_SUBSTITUTIONS_MENU,
    EWK_CONTEXT_MENU_ITEM_TAG_SHOW_SUBSTITUTIONS,
    EWK_CONTEXT_MENU_ITEM_TAG_SMART_COPY_PASTE,
    EWK_CONTEXT_MENU_ITEM_TAG_SMART_QUOTES,
    EWK_CONTEXT_MENU_ITEM_TAG_SMART_DASHES,
    EWK_CONTEXT_MENU_ITEM_TAG_SMART_LINKS,
    EWK_CONTEXT_MENU_ITEM_TAG_TEXT_REPLACEMENT,
    EWK_CONTEXT_MENU_ITEM_TAG_TRANSFORMATIONS_MENU,
    EWK_CONTEXT_MENU_ITEM_TAG_MAKE_UPPER_CASE,
    EWK_CONTEXT_MENU_ITEM_TAG_MAKE_LOWER_CASE,
    EWK_CONTEXT_MENU_ITEM_TAG_CAPITALIZE,
    EWK_CONTEXT_MENU_ITEM_TAG_CHANGE_BACK,
    EWK_CONTEXT_MENU_ITEM_TAG_OPEN_MEDIA_IN_NEW_WINDOW,
    EWK_CONTEXT_MENU_ITEM_TAG_COPY_MEDIA_LINK_TO_CLIPBOARD,
    EWK_CONTEXT_MENU_ITEM_TAG_TOGGLE_MEDIA_CONTROLS,
    EWK_CONTEXT_MENU_ITEM_TAG_TOGGLE_MEDIA_LOOP,
    EWK_CONTEXT_MENU_ITEM_TAG_ENTER_VIDEO_FULLSCREEN,
    EWK_CONTEXT_MENU_ITEM_TAG_MEDIA_PLAY_PAUSE,
    EWK_CONTEXT_MENU_ITEM_TAG_MEDIA_MUTE,
    EWK_CONTEXT_MENU_ITEM_TAG_DICTATION_ALTERNATIVE,
    EWK_CONTEXT_MENU_ITEM_TAG_SELECT_ALL,   /**< Select all */
    EWK_CONTEXT_MENU_ITEM_TAG_SELECT_WORD,  /**< Select word */
    EWK_CONTEXT_MENU_ITEM_TAG_TEXT_SELECTION_MODE,  /**< Text selection mode */
    EWK_CONTEXT_MENU_ITEM_TAG_CLIPBOARD,    /**< Clipboard */
    EWK_CONTEXT_MENU_ITEM_TAG_DRAG,
    EWK_CONTEXT_MENU_ITEM_TAG_TRANSLATE,
    EWK_CONTEXT_MENU_ITEM_TAG_COPY_LINK_DATA,   /**< Copy link data */
    EWK_CONTEXT_MENU_ITEM_BASE_APPLICATION_TAG = 10000  /**< If app want to add customized item, use enum value after #EWK_CONTEXT_MENU_ITEM_BASE_APPLICATION_TAG */
};

/**
 * @brief The structure type that creates a type name for #_Ewk_Context_Menu_Item_Tag.
 * @since_tizen 2.3
 */
typedef uint32_t Ewk_Context_Menu_Item_Tag;

/**
 * @brief The structure type that creates a type name for #Ewk_Context_Menu.
 * @since_tizen 2.3
 */
typedef struct _Ewk_Context_Menu Ewk_Context_Menu;

/**
 * @brief The structure type that creates a type name for #Ewk_Context_Menu_Item.
 * @since_tizen 2.3
 */
typedef struct _Ewk_Context_Menu_Item Ewk_Context_Menu_Item;

/**
 * @brief Counts the number of context menu items.
 *
 * @since_tizen 2.3
 *
 * @param[in] menu The context menu object
 *
 * @return The number of current context menu items
 */
EXPORT_API unsigned ewk_context_menu_item_count(Ewk_Context_Menu* menu);

/**
 * @brief Returns the nth item in a context menu.
 *
 * @since_tizen 2.3
 *
 * @param[in] menu The context menu object
 * @param[in] n The number of the item
 *
 * @return The nth item of the context menu
 */
EXPORT_API Ewk_Context_Menu_Item* ewk_context_menu_nth_item_get(Ewk_Context_Menu* menu, unsigned int n);

/**
 * @brief Removes the context menu item from the context menu object.
 *
 * @since_tizen 2.3
 *
 * @param[in] menu The context menu object
 * @param[in] item The context menu item to remove
 *
 * @return @c EINA_TRUE on a successful request,\n
 *         otherwise @c EINA_FALSE on failure
 */
EXPORT_API Eina_Bool ewk_context_menu_item_remove(Ewk_Context_Menu* menu, Ewk_Context_Menu_Item* item);

/**
 * @brief Adds the context menu item to the context menu object.
 *
 * @since_tizen 2.3
 *
 * @param[in] menu The context menu object
 * @param[in] tag  The tag of the context menu item
 * @param[in] title The title of the context menu item
 * @param[in] enabled If @c true the context menu item is enabled,\n
 *                    otherwise @c false
 *
 * @return @c EINA_TRUE on a successful request,\n
 *         otherwise @c EINA_FALSE on failure
 */
EXPORT_API Eina_Bool ewk_context_menu_item_append_as_action(Ewk_Context_Menu* menu, Ewk_Context_Menu_Item_Tag tag, const char* title, Eina_Bool enabled);

/**
 * @brief Adds the context menu item to the context menu object.
 *
 * @since_tizen 2.3
 *
 * @param[in] menu  The context menu object
 * @param[in] tag   The tag of the context menu item
 * @param[in] title The title of the context menu item
 * @param[in] icon_file The path of the icon to be set on the context menu item
 * @param[in] enabled If @c true the context menu item is enabled,\n
 *                    otherwise @c false
 *
 * @return @c EINA_TRUE on a successful request,\n
 *         otherwise @c EINA_FALSE on failure
 */
EXPORT_API Eina_Bool ewk_context_menu_item_append(Ewk_Context_Menu* menu, Ewk_Context_Menu_Item_Tag tag, const char* title, const char* icon_file, Eina_Bool enabled);

/**
 * @brief Returns the tag of the context menu item.
 *
 * @since_tizen 2.3
 *
 * @param[in] item The context menu item object
 *
 * @return The tag of the context menu item
 */
EXPORT_API Ewk_Context_Menu_Item_Tag ewk_context_menu_item_tag_get(Ewk_Context_Menu_Item* item);

/**
 * @brief Returns the link URL string of the context menu item.
 *
 * @since_tizen 2.3
 *
 * @param[in] item The context menu item object
 *
 * @return The current link URL string on success,\n
 *         otherwise @c 0 on failure
 */
EXPORT_API const char* ewk_context_menu_item_link_url_get(Ewk_Context_Menu_Item* item);

/**
 * @brief Returns the image URL string of the context menu item.
 *
 * @since_tizen 2.3
 *
 * @param[in] item The context menu item object
 *
 * @return The current image URL string on success,\n
 *         otherwise @c 0 on failure
 */
EXPORT_API const char* ewk_context_menu_item_image_url_get(Ewk_Context_Menu_Item* item);

/**
* @}
*/

#ifdef __cplusplus
}
#endif

#endif // ewk_context_menu_h
