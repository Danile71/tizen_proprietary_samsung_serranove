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
 *
 */

#ifndef External_h
#define External_h

#if ENABLE(TIZEN_SEARCH_PROVIDER)

#include "DOMWindowProperty.h"
#include <wtf/Forward.h>
#include <wtf/PassRefPtr.h>
#include <wtf/RefCounted.h>
#include <wtf/RefPtr.h>

namespace WebCore {

class Frame;

typedef int ExceptionCode;

class External : public RefCounted<External>, public DOMWindowProperty{
public:
    static PassRefPtr<External> create(Frame* frame) { return adoptRef(new External(frame)); }
    virtual ~External();

    void addSearchProvider(const String& engineURL);
    unsigned long isSearchProviderInstalled(const String& engineURL);

private:
    explicit External(Frame*);
};

} // namespace WebCore

#endif // ENABLE(TIZEN_SEARCH_PROVIDER)

#endif // External_h
