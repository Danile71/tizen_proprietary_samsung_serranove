/*
 * Copyright (C) 2015 Samsung Electronics All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */
#include "config.h"
#include "WebBaseAccessibilityTizen.h"

#if HAVE(ACCESSIBILITY)

#include <atk-bridge.h>
#include <unistd.h>

G_DEFINE_TYPE(WebBaseAccessibility, web_base_accessibility, ATK_TYPE_OBJECT)

static void web_base_accessibility_init(WebBaseAccessibility*)
{
}

static void web_base_accessibility_class_init(WebBaseAccessibilityClass* klass)
{
}

static const char* toolkitName()
{
    return "WebKit";
}

static const char* toolkitVersion()
{
    return "1.0";
}

static AtkObject* root()
{
    static AtkObject* root = NULL;
    if (!root)
        root = ATK_OBJECT(g_object_new(WEB_BASE_ACCESSIBILITY_TYPE, NULL));

    return root;
}

WebAccessibility::WebAccessibility()
    : m_isAccessibilityEnabled(false)
{
}

WebAccessibility& WebAccessibility::instance()
{
    static WebAccessibility webAccessibility;
    return webAccessibility;
}

void WebAccessibility::init()
{
    // ToDo: Just a temporary fix to avoid setting up atk-bridge on root accout.
    // Will be fixed
    if (getuid() == 0)
        return;

    bridgeInit();
}

void WebAccessibility::cleanup() const
{
    bridgeCleanup();
}

void WebAccessibility::bridgeInit()
{
    AtkUtilClass* atkUtilClass = ATK_UTIL_CLASS(g_type_class_ref(ATK_TYPE_UTIL));
    atkUtilClass->get_toolkit_name  = toolkitName;
    atkUtilClass->get_toolkit_version = toolkitVersion;
    atkUtilClass->get_root = root;

    // Init atk bridge
    atk_bridge_adaptor_init(NULL, NULL);
    m_isAccessibilityEnabled = true;
}

void WebAccessibility::bridgeCleanup() const
{
    if (!m_isAccessibilityEnabled)
        return;

    atk_bridge_adaptor_cleanup();
}

#endif // HAVE(ACCESSIBILITY)
