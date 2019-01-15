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

#ifndef ewk_text_style_h
#define ewk_text_style_h

#include <Eina.h>
#include <Evas.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    EWK_TEXT_STYLE_STATE_FALSE,
    EWK_TEXT_STYLE_STATE_TRUE,
    EWK_TEXT_STYLE_STATE_MIXED
} Ewk_Text_Style_State;

typedef struct _Ewk_Text_Style Ewk_Text_Style;

EXPORT_API Ewk_Text_Style_State ewk_text_style_underline_get(Ewk_Text_Style* text_style);
EXPORT_API Ewk_Text_Style_State ewk_text_style_italic_get(Ewk_Text_Style* text_style);
EXPORT_API Ewk_Text_Style_State ewk_text_style_bold_get(Ewk_Text_Style* text_style);
EXPORT_API Ewk_Text_Style_State ewk_text_style_ordered_list_get(Ewk_Text_Style* text_style);
EXPORT_API Ewk_Text_Style_State ewk_text_style_unordered_list_get(Ewk_Text_Style* text_style);
EXPORT_API Eina_Bool ewk_text_style_position_get(Ewk_Text_Style* text_style, Evas_Point* start_point, Evas_Point* end_point);
EXPORT_API Eina_Bool ewk_text_style_bg_color_get(Ewk_Text_Style* textStyle, int* r, int* g, int* b, int* a);
EXPORT_API Eina_Bool ewk_text_style_color_get(Ewk_Text_Style* textStyle, int* r, int* g, int* b, int* a);
EXPORT_API const char* ewk_text_style_font_size_get(Ewk_Text_Style* textStyle);
EXPORT_API Eina_Bool ewk_text_style_has_composition_get(Ewk_Text_Style* textStyle);
EXPORT_API Ewk_Text_Style_State ewk_text_style_align_center_get(Ewk_Text_Style* textStyle);
EXPORT_API Ewk_Text_Style_State ewk_text_style_align_left_get(Ewk_Text_Style* textStyle);
EXPORT_API Ewk_Text_Style_State ewk_text_style_align_right_get(Ewk_Text_Style* textStyle);
EXPORT_API Ewk_Text_Style_State ewk_text_style_align_full_get(Ewk_Text_Style* textStyle);

#ifdef __cplusplus
}
#endif

#endif // ewk_text_style_h
