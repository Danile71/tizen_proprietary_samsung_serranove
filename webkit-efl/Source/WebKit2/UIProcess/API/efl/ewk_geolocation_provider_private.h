/*
   Copyright (C) 2013 Samsung Electronics

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

#ifndef ewk_geolocation_provider_private_h
#define ewk_geolocation_provider_private_h

#include "WKGeolocationManager.h"
#include "WKRetainPtr.h"
#include "ewk_context.h"
#include "ewk_geolocation_provider.h"
#include <WebKit2/WKBase.h>
#include <location/locations.h>
#include <wtf/HashMap.h>

struct GeolocationValidityCallbackData {
    Ewk_Geolocation_Validity_Cb callback;
    void* userData;

    GeolocationValidityCallbackData()
        : callback(0)
        , userData(0)
    { }

    GeolocationValidityCallbackData(Ewk_Geolocation_Validity_Cb _callback, void* _userData)
        : callback(_callback)
        , userData(_userData)
    { }
};

typedef HashMap<void*, GeolocationValidityCallbackData> ValidityListenerMap;

class Ewk_Geolocation_Provider {
public:
    static PassOwnPtr<Ewk_Geolocation_Provider> create(WKContextRef contextRef)
    {
        return adoptPtr(new Ewk_Geolocation_Provider(contextRef));
    }
    ~Ewk_Geolocation_Provider();

    void watchValidity(const GeolocationValidityCallbackData&);
    void unwatchValidity(void*);
    void errorInternal();
    void locationManagerStopped();
    void startUpdatingInternal();
    location_manager_h locationManager() { return m_locationManager; }
    WKGeolocationManagerRef wkGeolocationManager();

private:
    explicit Ewk_Geolocation_Provider(WKContextRef contextRef);

    static void startUpdating(WKGeolocationManagerRef, const void* clientInfo);
    static void stopUpdating(WKGeolocationManagerRef, const void* clientInfo);

    bool m_locationManagerStarted;
    location_manager_h m_locationManager;
    WKRetainPtr<WKContextRef> m_wkContext;
    ValidityListenerMap m_validityListeners;
};

#endif // ewk_geolocation_provider_private_h
