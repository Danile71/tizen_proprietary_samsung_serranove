/*
 * Copyright (C) 2012 Samsung Electronics
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this program; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "config.h"

#include "EWebKit2.h"
#include "TestController.h"
#include <stdlib.h>

#if PLATFORM(TIZEN)
#include <Elementary.h>
#endif

int main(int argc, char** argv)
{
    if (!ewk_init())
        return 1;

#if PLATFORM(TIZEN)
    if (!elm_init(0, 0))
        return 1;
#endif

    // Prefer the not installed web and plugin processes.
    WTR::TestController controller(argc, const_cast<const char**>(argv));

#if PLATFORM(TIZEN)
    elm_shutdown();
#else
    edje_shutdown();
    ecore_evas_shutdown();
#endif

    ewk_shutdown();

    return 0;
}

