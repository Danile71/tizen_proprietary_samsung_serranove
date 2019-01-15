/*
 * Copyright (C) 2012 Samsung Electronics
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

#ifndef EflScreenUtilities_h
#define EflScreenUtilities_h

#include <wtf/text/WTFString.h>

#if ENABLE(TIZEN_VIEWPORT_META_TAG)
#include "IntSize.h"
#endif

#if ENABLE(TIZEN_WEBKIT2_CONTEXT_X_WINDOW)
typedef unsigned Ecore_X_Window;
#endif

namespace WebCore {

void applyFallbackCursor(Ecore_Evas*, const char*);
int getDPI();
int getPixelDepth(const Evas*);
bool isUsingEcoreX(const Evas*);

#if ENABLE(TIZEN_VIEWPORT_META_TAG)
IntSize getDefaultScreenResolution();
int getMobileDPI();
#endif

#if ENABLE(TIZEN_WEBKIT2_CONTEXT_X_WINDOW)
// FIXME: By root window's orientation state sync issue, get orientation info with x window
// If application begins with landscape mode, default root window's orientation is
// not completely synchronized with application window's orientation.
// So, we can avoid this problem with getting application's x window directly.
void setXWindow(Ecore_X_Window xWindow);
Ecore_X_Window getXWindow();
bool isMiniWindowMode(Ecore_X_Window xWindow = 0);
bool isMultiWindowMode(Ecore_X_Window xWindow = 0);
#endif

} // namespace WebCore

#endif // EflScreenUtilities_h
