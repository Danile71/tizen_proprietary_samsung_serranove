/*
 * Copyright (C) 2013 Samsung Electronics
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
#include "EdgeEffect.h"

#include "ewk_settings_private.h"

#if ENABLE(TIZEN_EDGE_SUPPORT)

EdgeEffect::EdgeEffect(Evas_Object* view)
    : m_view(view)
    , m_edgeObject(0)
{
    ASSERT(m_view);

    evas_object_event_callback_add(m_view, EVAS_CALLBACK_RESIZE, viewResizeCallback, this);
    evas_object_event_callback_add(m_view, EVAS_CALLBACK_MOVE, viewMoveCallback, this);
    evas_object_event_callback_add(m_view, EVAS_CALLBACK_SHOW, viewShowCallback, this);
    evas_object_event_callback_add(m_view, EVAS_CALLBACK_HIDE, viewHideCallback, this);
}

EdgeEffect::~EdgeEffect()
{
    if (m_edgeObject)
        evas_object_del(m_edgeObject);

    evas_object_event_callback_del(m_view, EVAS_CALLBACK_RESIZE, viewResizeCallback);
    evas_object_event_callback_del(m_view, EVAS_CALLBACK_MOVE, viewMoveCallback);
    evas_object_event_callback_del(m_view, EVAS_CALLBACK_SHOW, viewShowCallback);
    evas_object_event_callback_del(m_view, EVAS_CALLBACK_HIDE, viewHideCallback);
}

void EdgeEffect::viewResizeCallback(void* data, Evas*, Evas_Object* view, void*)
{
    int width;
    int height;
    evas_object_geometry_get(view, 0, 0, &width, &height);
    (static_cast<EdgeEffect*>(data))->resizeObject(width, height);
}

void EdgeEffect::viewMoveCallback(void* data, Evas*, Evas_Object* view, void*)
{
    int x;
    int y;
    evas_object_geometry_get(view, &x, &y, 0, 0);
    (static_cast<EdgeEffect*>(data))->moveObject(x, y);
}

void EdgeEffect::viewShowCallback(void* data, Evas*, Evas_Object* view, void*)
{
    (static_cast<EdgeEffect*>(data))->showObject();
}

void EdgeEffect::viewHideCallback(void* data, Evas*, Evas_Object* view, void*)
{
    (static_cast<EdgeEffect*>(data))->hideObject();
}

Evas_Object* EdgeEffect::edgeObject()
{
    if (!m_edgeObject) {
        m_edgeObject = edje_object_add(evas_object_evas_get(m_view));
        if (!m_edgeObject)
            return 0;

        if (!edje_object_file_set(m_edgeObject, EDGE_EDJE_FILE, EDJE_EFFECT_GROUP))
            return 0;

        evas_object_smart_member_add(m_edgeObject, m_view);

        int x;
        int y;
        int width;
        int height;
        evas_object_geometry_get(m_view, &x, &y, &width, &height);
        evas_object_move(m_edgeObject, x, y);
        evas_object_resize(m_edgeObject, width, height);
        evas_object_show(m_edgeObject);
        evas_object_pass_events_set(m_edgeObject, EINA_TRUE);
        top_show = EINA_FALSE;
        bottom_show = EINA_FALSE;
        left_show = EINA_FALSE;
        right_show = EINA_FALSE;
        m_edgeMove = false;
    }
    return m_edgeObject;
}

void EdgeEffect::show(const String& source)
{
    Ewk_Settings* settings = ewk_view_settings_get(m_view);
    if (settings && !settings->edgeEffectEnabled())
        return;

    Evas_Object* object = edgeObject();
    if (!object)
        return;

    if (source.contains("top")) {
        if (!top_show) {
            edje_object_signal_emit(object, "elm,edge,top", "elm");
            top_show = EINA_TRUE;
        } else
            edje_object_signal_emit(object, "elm,edge,move", "top");
    }
    if (source.contains("bottom")) {
        if (!bottom_show) {
            edje_object_signal_emit(object, "elm,edge,bottom", "elm");
            bottom_show = EINA_TRUE;
        } else
            edje_object_signal_emit(object, "elm,edge,move", "bottom");
    }
    if (source.contains("left")) {
        if (!left_show) {
            edje_object_signal_emit(object, "elm,edge,left", "elm");
            left_show = EINA_TRUE;
        } else
            edje_object_signal_emit(object, "elm,edge,move", "left");
    }
    if (source.contains("right")) {
        if (!right_show) {
            edje_object_signal_emit(object, "elm,edge,right", "elm");
            right_show = EINA_TRUE;
        } else
            edje_object_signal_emit(object, "elm,edge,move", "right");
    }

    edje_object_message_signal_process(object);
}

void EdgeEffect::hide(const String& source)
{
    Ewk_Settings* settings = ewk_view_settings_get(m_view);
    if (settings && !settings->edgeEffectEnabled())
        return;

    Evas_Object* object = edgeObject();
    if (!object)
        return;

    if (source.contains("top") && top_show) {
        edje_object_signal_emit(object, "edge,hide", source.utf8().data());
        top_show = EINA_FALSE;
    }
    if (source.contains("bottom") && bottom_show) {
        edje_object_signal_emit(object, "edge,hide", source.utf8().data());
        bottom_show = EINA_FALSE;
    }
    if (source.contains("left") && left_show) {
        edje_object_signal_emit(object, "edge,hide", source.utf8().data());
        left_show = EINA_FALSE;
    }
    if (source.contains("right") && right_show) {
        edje_object_signal_emit(object, "edge,hide", source.utf8().data());
        right_show = EINA_FALSE;
    }

    edje_object_message_signal_process(object);
}

void EdgeEffect::hide()
{
    Ewk_Settings* settings = ewk_view_settings_get(m_view);
    if (settings && !settings->edgeEffectEnabled())
        return;

    Evas_Object* object = edgeObject();
    if (!object)
        return;

    if (top_show){
        top_show = EINA_FALSE;
        edje_object_signal_emit(object, "edge,hide", "edge,top");
        edje_object_message_signal_process(object);
    }
    if (bottom_show){
        bottom_show = EINA_FALSE;
        edje_object_signal_emit(object, "edge,hide", "edge,bottom");
        edje_object_message_signal_process(object);
    }
    if (left_show){
        left_show = EINA_FALSE;
        edje_object_signal_emit(object, "edge,hide", "edge,left");
        edje_object_message_signal_process(object);
    }
    if (right_show){
        right_show = EINA_FALSE;
        edje_object_signal_emit(object, "edge,hide", "edge,right");
        edje_object_message_signal_process(object);
    }
}

void EdgeEffect::resizeObject(int width, int height)
{
    if (!m_edgeObject)
        return;

    evas_object_resize(m_edgeObject, width, height);
}

void EdgeEffect::moveObject(int x, int y)
{
    if (!m_edgeObject)
        return;

    evas_object_move(m_edgeObject, x, y);
}

void EdgeEffect::showObject()
{
    if (!m_edgeObject)
        return;

    evas_object_show(m_edgeObject);
}

void EdgeEffect::hideObject()
{
    if (!m_edgeObject)
        return;

    evas_object_hide(m_edgeObject);
}

#endif // #if ENABLE(TIZEN_EDGE_SUPPORT)
