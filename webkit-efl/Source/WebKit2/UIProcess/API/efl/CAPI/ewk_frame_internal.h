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

#ifndef ewk_frame_internal_h
#define ewk_frame_internal_h

#include <tizen.h>
#include <Eina.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef const void* Ewk_Frame_Ref;

/**
 * Gets whether the frame is main frame.
 *
 * @param frame frame object
 *
 * @return @c EINA_TRUE if the frame is main frame or @c EINA_FALSE otherwise
 */
EXPORT_API Eina_Bool ewk_frame_is_main_frame(Ewk_Frame_Ref frame);

#ifdef __cplusplus
}
#endif
#endif // ewk_frame_internal_h
