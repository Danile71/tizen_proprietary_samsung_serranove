/*
   Copyright (C) 2012, 2013 Samsung Electronics

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
#include "ewk_geolocation_provider.h"

#if ENABLE(TIZEN_GEOLOCATION)
#include "WKGeolocationManager.h"
#include "ewk_geolocation_provider_private.h"
#include "ewk_view_private.h"

Ewk_Geolocation_Provider::Ewk_Geolocation_Provider(WKContextRef contextRef)
    : m_locationManagerStarted(false)
    , m_locationManager(0)
    , m_wkContext(contextRef)
{
    ASSERT(m_wkContext.get());
    WKGeolocationManagerRef geolocationManagerRef = WKContextGetGeolocationManager(m_wkContext.get());
    ASSERT(geolocationManagerRef);

    WKGeolocationProvider provider = {
        kWKGeolocationProviderCurrentVersion,
        this,
        startUpdating,
        stopUpdating,
    };

    WKGeolocationManagerSetProvider(geolocationManagerRef, &provider);
}

Ewk_Geolocation_Provider::~Ewk_Geolocation_Provider()
{
    stopUpdating(0, this);
    WKGeolocationManagerSetProvider(wkGeolocationManager(), 0);
}

void Ewk_Geolocation_Provider::watchValidity(const GeolocationValidityCallbackData& callbackData)
{
    ASSERT(callbackData);
    if (m_validityListeners.contains(callbackData.userData))
        return;

    m_validityListeners.add(callbackData.userData, callbackData);
}

void Ewk_Geolocation_Provider::unwatchValidity(void* userData)
{
    ASSERT(userData);
    m_validityListeners.remove(userData);

    if (m_locationManagerStarted && m_validityListeners.isEmpty()) {
        TIZEN_LOGI("no validityListeners now. So stop updating... ");
        stopUpdating(0, this);
    }
}

WKGeolocationManagerRef Ewk_Geolocation_Provider::wkGeolocationManager()
{
    return WKContextGetGeolocationManager(m_wkContext.get());
}

static void ewkGeolocationProviderPositionChangedCallback(double latitude, double longitude, double altitude, time_t timestamp, void* user_data)
{
    Ewk_Geolocation_Provider* ewkGeolocationProvider = static_cast<Ewk_Geolocation_Provider*>(user_data);
    if(!timestamp || !ewkGeolocationProvider)
        return;

    location_accuracy_level_e level;
    double horizontal = 0;
    double vertical = 0;
    double climb = 0;
    double direction = std::numeric_limits<double>::quiet_NaN();
    double speed = 0;
    time_t velocityTimestamp;

    location_error_e ret = static_cast<location_error_e>(location_manager_get_last_accuracy(ewkGeolocationProvider->locationManager(), &level, &horizontal, &vertical));
    if (ret != LOCATIONS_ERROR_NONE)
        TIZEN_LOGE("location_manager_get_last_accuracy error(0x%08x)", ret);
    ret = static_cast<location_error_e>(location_manager_get_velocity(ewkGeolocationProvider->locationManager(), &climb, &direction, &speed, &velocityTimestamp));
    if (ret != LOCATIONS_ERROR_NONE)
        TIZEN_LOGE("location_manager_get_velocity error(0x%08x)", ret);

    if (speed)
        speed = speed * 5.0 / 18.0;
    else
        direction = std::numeric_limits<double>::quiet_NaN();
    if (vertical < 0)
        vertical = 0;

    WKRetainPtr<WKGeolocationPositionRef> position(AdoptWK, WKGeolocationPositionCreate_b(timestamp, latitude, longitude, horizontal, isnan(altitude) ? false : true, altitude, true, vertical, true, direction, true, speed));
    WKGeolocationManagerProviderDidChangePosition(ewkGeolocationProvider->wkGeolocationManager(), position.get());
}

static void ewkGeolocationProviderServiceStateUpdatedCallback(location_service_state_e state, void *userData)
{
    Ewk_Geolocation_Provider* ewkGeolocationProvider = static_cast<Ewk_Geolocation_Provider*>(userData);

    if (state == LOCATIONS_SERVICE_DISABLED) {
        TIZEN_LOGE("Privacy is disabled. will stop updating");
        ewkGeolocationProvider->locationManagerStopped();
        WKGeolocationManagerProviderDidFailToDeterminePosition(ewkGeolocationProvider->wkGeolocationManager());
    }
}

void Ewk_Geolocation_Provider::errorInternal()
{
    TIZEN_LOGI("GPS Setting is still off");
    WKGeolocationManagerProviderDidFailToDeterminePosition(wkGeolocationManager());
}

void Ewk_Geolocation_Provider::locationManagerStopped()
{
    TIZEN_LOGI("Location manager is stopped from outside");
    location_manager_destroy(m_locationManager);
    m_locationManagerStarted = false;
}

void Ewk_Geolocation_Provider::startUpdatingInternal()
{
    if (m_locationManagerStarted)
        return;

    if (m_validityListeners.isEmpty()) {
        TIZEN_LOGE("No valid view exists");
        WKGeolocationManagerProviderDidFailToDeterminePosition(wkGeolocationManager());
        return;
    }

    ValidityListenerMap::const_iterator validtyListener = m_validityListeners.begin();
    if (!validtyListener->second.callback(validtyListener->second.userData)) {
        TIZEN_LOGE("No valid view exists");
        WKGeolocationManagerProviderDidFailToDeterminePosition(wkGeolocationManager());
        return;
    }

    location_error_e ret = static_cast<location_error_e>(location_manager_create(LOCATIONS_METHOD_HYBRID, &m_locationManager));
    if (ret != LOCATIONS_ERROR_NONE) {
        TIZEN_LOGE("location_manager_create error (0x%08x)", ret);
        WKGeolocationManagerProviderDidFailToDeterminePosition(wkGeolocationManager());
        return;
    }

    ret = static_cast<location_error_e>(location_manager_set_service_state_changed_cb(m_locationManager, ewkGeolocationProviderServiceStateUpdatedCallback, const_cast<Ewk_Geolocation_Provider*>(this)));
    if (ret != LOCATIONS_ERROR_NONE){
        TIZEN_LOGE("location_manager_set_service_state_changed_cb error(0x%08x)", ret);
        location_manager_destroy(m_locationManager);
        WKGeolocationManagerProviderDidFailToDeterminePosition(wkGeolocationManager());
        return;
    }

    ret = static_cast<location_error_e>(location_manager_set_position_updated_cb(m_locationManager, ewkGeolocationProviderPositionChangedCallback, 1, const_cast<Ewk_Geolocation_Provider*>(this)));
    if (ret != LOCATIONS_ERROR_NONE){
        TIZEN_LOGE("location_manager_set_position_updated_cb error(0x%08x)", ret);
        location_manager_destroy(m_locationManager);
        WKGeolocationManagerProviderDidFailToDeterminePosition(wkGeolocationManager());
        return;
    }

    ret = static_cast<location_error_e>(location_manager_start(m_locationManager));
    if (ret != LOCATIONS_ERROR_NONE) {
        TIZEN_LOGE("location_manager_start error(0x%08x)", ret);
        location_manager_unset_position_updated_cb(m_locationManager);
        location_manager_destroy(m_locationManager);
        WKGeolocationManagerProviderDidFailToDeterminePosition(wkGeolocationManager());
        return;
    }

    m_locationManagerStarted = true;
}

void Ewk_Geolocation_Provider::startUpdating(WKGeolocationManagerRef, const void* clientInfo)
{
    ASSERT(clientInfo);
    Ewk_Geolocation_Provider* ewkGeolocationProvider = static_cast<Ewk_Geolocation_Provider*>(const_cast<void*>(clientInfo));

    ewkGeolocationProvider->startUpdatingInternal();
}

void Ewk_Geolocation_Provider::stopUpdating(WKGeolocationManagerRef, const void* clientInfo)
{
    ASSERT(clientInfo);
    Ewk_Geolocation_Provider* ewkGeolocationProvider = static_cast<Ewk_Geolocation_Provider*>(const_cast<void*>(clientInfo));

    if (!ewkGeolocationProvider->m_locationManagerStarted) {
        TIZEN_LOGE("already stopped or not started");
        return;
    }

    location_error_e ret = static_cast<location_error_e>(location_manager_unset_service_state_changed_cb(ewkGeolocationProvider->m_locationManager));
    if (ret != LOCATIONS_ERROR_NONE)
        TIZEN_LOGE("location_manager_unset_service_state_changed_updated_cb error(0x%08x)", ret);

    ret = static_cast<location_error_e>(location_manager_unset_position_updated_cb(ewkGeolocationProvider->m_locationManager));
    if (ret != LOCATIONS_ERROR_NONE)
        TIZEN_LOGE("location_manager_unset_position_updated_cb error(0x%08x)", ret);
    
    ret = static_cast<location_error_e>(location_manager_stop(ewkGeolocationProvider->m_locationManager));
    if (ret != LOCATIONS_ERROR_NONE)
        TIZEN_LOGE("location_manager_stop error(0x%08x)", ret);

    ret = static_cast<location_error_e>(location_manager_destroy(ewkGeolocationProvider->m_locationManager));
    if (ret != LOCATIONS_ERROR_NONE)
        TIZEN_LOGE("location_manager_destroy error(0x%08x)", ret);

    ewkGeolocationProvider->m_locationManagerStarted = false;
}
#endif // ENABLE(TIZEN_GEOLOCATION)
