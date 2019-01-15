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

#ifndef ewk_text_style_product_h
#define ewk_text_style_product_h

#include <Eina.h>
#include <Evas.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @internal
 * @addtogroup WEBVIEW
 * @{
 */

/**
 * @brief Enumeration for text style state.
 * @since_tizen 2.3
 */
typedef enum {
    EWK_TEXT_STYLE_STATE_FALSE, /**< false */
    EWK_TEXT_STYLE_STATE_TRUE,  /**<  true */
    EWK_TEXT_STYLE_STATE_MIXED  /**< mixed */
} Ewk_Text_Style_State;

/**
 * @brief Creates a type name for #Ewk_Text_Style
 * @since_tizen 2.3
 */
typedef struct _Ewk_Text_Style Ewk_Text_Style;

/**
 * @brief Gets state whether underline style is applied for the whole selection.
 *
 * @since_tizen 2.3
 *
 * @param[in] text_style text style object
 *
 * @return @c #EWK_TEXT_STYLE_STATE_TRUE if underline style is fully applied for the whole selection\n
 *         @c #EWK_TEXT_STYLE_STATE_FALSE if underline style is not fully applied or on failure for the whole selection\n
 *         @c #EWK_TEXT_STYLE_STATE_MIXED if underline style is mixed for the whole selection
 */
EXPORT_API Ewk_Text_Style_State ewk_text_style_underline_get(Ewk_Text_Style* text_style);

/**
 * @brief Gets state whether italic style is applied for the whole selection.
 *
 * @since_tizen 2.3
 *
 * @param[in] text_style text style object
 *
 * @return @c #EWK_TEXT_STYLE_STATE_TRUE if italic style is fully applied for the whole selection\n
 *         @c #EWK_TEXT_STYLE_STATE_FALSE if italic style is not fully applied or on failure for the whole selection\n
 *         @c #EWK_TEXT_STYLE_STATE_MIXED if italic style is mixed for the whole selection
 */
EXPORT_API Ewk_Text_Style_State ewk_text_style_italic_get(Ewk_Text_Style* text_style);

/**
 * @brief Gets state whether bold style is applied for the whole selection.
 *
 * @since_tizen 2.3
 *
 * @param[in] text_style text style object
 *
 * @return @c #EWK_TEXT_STYLE_STATE_TRUE if bold style is fully applied for the whole selection\n
 *         @c #EWK_TEXT_STYLE_STATE_FALSE if bold style is not fully applied or on failure for the whole selection\n
 *         @c #EWK_TEXT_STYLE_STATE_MIXED if bold style is mixed for the whole selection
 */
EXPORT_API Ewk_Text_Style_State ewk_text_style_bold_get(Ewk_Text_Style* text_style);

/**
 * @brief Gets state whether ordered list style is applied for the whole selection.
 *
 * @since_tizen 2.3
 *
 * @param[in] text_style text style object
 *
 * @return @c #EWK_TEXT_STYLE_STATE_TRUE if ordered list style is fully applied for the whole selection\n
 *         @c #EWK_TEXT_STYLE_STATE_FALSE if ordered list style is not fully applied or on failure for the whole selection\n
 *         @c #EWK_TEXT_STYLE_STATE_MIXED if ordered list style is mixed for the whole selection
 */
EXPORT_API Ewk_Text_Style_State ewk_text_style_ordered_list_get(Ewk_Text_Style* text_style);

/**
 * @brief Gets state whether unordered list style is applied for the whole selection.
 *
 * @since_tizen 2.3
 *
 * @param[in] text_style text style object
 *
 * @return @c #EWK_TEXT_STYLE_STATE_TRUE if unordered list style is fully applied for the whole selection\n
 *         @c #EWK_TEXT_STYLE_STATE_FALSE if unordered list style is not fully applied or on failure for the whole selection\n
 *         @c #EWK_TEXT_STYLE_STATE_MIXED if unordered list style is mixed for the whole selection
 */
EXPORT_API Ewk_Text_Style_State ewk_text_style_unordered_list_get(Ewk_Text_Style* text_style);

/**
 * @brief Gets selection position.
 *
 * @since_tizen 2.3
 *
 * @param[in] text_style text style object
 * @param[in] start_point start point(left-bottom) of selection
 * @param[in] end_point end point(right-bottom) of selection
 *
 * @return @c EINA_TRUE on success or @c EINA_FALSE on failure
 */
EXPORT_API Eina_Bool ewk_text_style_position_get(Ewk_Text_Style* text_style, Evas_Point* start_point, Evas_Point* end_point);

/**
 * @brief Gets background color for selection.
 *
 * @since_tizen 2.3
 *
 * @param[in] text_style text style object
 * @param[in] r red color
 * @param[in] g green color
 * @param[in] b blue color
 * @param[in] a alpha
 *
 * @return @c EINA_TRUE on success or @c EINA_FALSE on failure
 */
EXPORT_API Eina_Bool ewk_text_style_bg_color_get(Ewk_Text_Style* text_style, int* r, int* g, int* b, int* a);

/**
 * @brief Gets font color for selection.
 *
 * @since_tizen 2.3
 *
 * @param[in] text_style text style object
 * @param[in] r red color
 * @param[in] g green color
 * @param[in] b blue color
 * @param[in] a alpha
 *
 * @return @c EINA_TRUE on success or @c EINA_FALSE on failure
 */
EXPORT_API Eina_Bool ewk_text_style_color_get(Ewk_Text_Style* text_style, int* r, int* g, int* b, int* a);

/**
 * @brief Gets font size for selection.
 *
 * @since_tizen 2.3
 *
 * @param[in] text_style text style object
 *
 * @return font size
 */
EXPORT_API const char* ewk_text_style_font_size_get(Ewk_Text_Style* text_style);

/**
 * @brief Gets composition state for selection.
 *
 * @since_tizen 2.3
 *
 * @param[in] text_style text style object
 *
 * @return @c EINA_TRUE if style has composition
 *         @c EINA_FALSE if not or on failure
 */
EXPORT_API Eina_Bool ewk_text_style_has_composition_get(Ewk_Text_Style* text_style);

/**
 * @brief Gets state whether align is center for the whole selection.
 *
 * @since_tizen 2.3
 *
 * @param[in] text_style text style object
 *
 * @return @c #EWK_TEXT_STYLE_STATE_TRUE if align is fully center for the whole selection\n
 *         @c #EWK_TEXT_STYLE_STATE_FALSE if align is not fully center for the whole selection\n
 *         @c #EWK_TEXT_STYLE_STATE_MIXED if if align is mixed for the whole selection range
 */
EXPORT_API Ewk_Text_Style_State ewk_text_style_align_center_get(Ewk_Text_Style* text_style);

/**
 * @brief Gets state whether align is left for the whole selection.
 *
 * @since_tizen 2.3
 *
 * @param[in] text_style text style object
 *
 * @return @c #EWK_TEXT_STYLE_STATE_TRUE if align is fully left for the whole selection\n
 *         @c #EWK_TEXT_STYLE_STATE_FALSE if align is not fully left for the whole selection\n
 *         @c #EWK_TEXT_STYLE_STATE_MIXED if if align is mixed for the whole selection
 */
EXPORT_API Ewk_Text_Style_State ewk_text_style_align_left_get(Ewk_Text_Style* text_style);

/**
 * @brief Gets state whether align is right for the whole selection.
 *
 * @since_tizen 2.3
 *
 * @param[in] text_style text style object
 *
 * @return @c #EWK_TEXT_STYLE_STATE_TRUE if align is fully right for the whole selection\n
 *         @c #EWK_TEXT_STYLE_STATE_FALSE if align is not fully right for the whole selection\n
 *         @c #EWK_TEXT_STYLE_STATE_MIXED if if align is mixed for the whole selection
 */
EXPORT_API Ewk_Text_Style_State ewk_text_style_align_right_get(Ewk_Text_Style* text_style);

/**
 * @brief Gets state whether align is full for the whole selection.
 *
 * @since_tizen 2.3
 *
 * @param[in] text_style text style object
 *
 * @return @c #EWK_TEXT_STYLE_STATE_TRUE if align is fully full for the whole selection\n
 *         @c #EWK_TEXT_STYLE_STATE_FALSE if align is not fully full for the whole selection\n
 *         @c #EWK_TEXT_STYLE_STATE_MIXED if if align is mixed for the whole selection
 */
EXPORT_API Ewk_Text_Style_State ewk_text_style_align_full_get(Ewk_Text_Style* text_style);

/**
* @}
*/

#ifdef __cplusplus
}
#endif

#endif // ewk_text_style_product_h
