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

#ifndef WebBaseAccessibilityTizen_h
#define WebBaseAccessibilityTizen_h

#if HAVE(ACCESSIBILITY)

#include <atk/atk.h>

G_BEGIN_DECLS

#define WEB_BASE_ACCESSIBILITY_TYPE (web_base_accessibility_get_type())
#define WEB_BASE_ACCESSIBILITY(obj) \
    (G_TYPE_CHECK_INSTANCE_CAST((obj), \
                                 WEB_BASE_ACCESSIBILITY_TYPE, \
                                 WebBaseAccessibility))

#define WEB_BASE_ACCESSIBILITY_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_CAST((klass), \
                             WEB_BASE_ACCESSIBILITY_TYPE, \
                             WebBaseAccessibilityClass))

#define IS_WEB_BASE_ACCESSIBILITY(obj) \
    (G_TYPE_CHECK_INSTANCE_TYPE((obj), WEB_BASE_ACCESSIBILITY_TYPE))

#define IS_WEB_BASE_ACCESSIBILITY_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_TYPE((klass), WEB_BASE_ACCESSIBILITY_TYPE))

#define WEB_BASE_ACCESSIBILITY_GET_CLASS(obj) \
    (G_TYPE_INSTANCE_GET_CLASS((obj), \
                               WEB_BASE_ACCESSIBILITY_TYPE, \
                               WebBaseAccessibilityClass))

typedef struct _WebBaseAccessibility WebBaseAccessibility;
typedef struct _WebBaseAccessibilityClass WebBaseAccessibilityClass;

struct _WebBaseAccessibility {
    AtkObject parent;
};

struct _WebBaseAccessibilityClass {
    AtkObjectClass parentClass;
};

GType web_base_accessibility_get_type();

G_END_DECLS

class WebAccessibility
{
public:
    static WebAccessibility& instance();
    void init();
    void cleanup() const;
private:
    WebAccessibility();

    void bridgeInit();
    void bridgeCleanup() const;

    bool m_isAccessibilityEnabled;
};

#endif // HAVE(ACCESSIBILITY)
#endif // WebBaseAccessibilityTizen_h
