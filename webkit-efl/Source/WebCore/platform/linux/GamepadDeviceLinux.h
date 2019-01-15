/*
 * Copyright (C) 2012 Zan Dobersek <zandobersek@gmail.com>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 */

#ifndef GamepadDeviceLinux_h
#define GamepadDeviceLinux_h

#if ENABLE(GAMEPAD)

#include <linux/joystick.h>
#include <wtf/Vector.h>
#include <wtf/text/WTFString.h>

namespace WebCore {

#if ENABLE(TIZEN_GAMEPAD)
enum _GamepadButtonCode
{
    _GAMEPAD_BUTTON_INVALID = -1,
    _GAMEPAD_BUTTON_1,
    _GAMEPAD_BUTTON_2,
    _GAMEPAD_BUTTON_3,
    _GAMEPAD_BUTTON_4,
    _GAMEPAD_BUTTON_L1_SHOULDER,
    _GAMEPAD_BUTTON_R1_SHOULDER,
    _GAMEPAD_BUTTON_L2_TRIGGER,
    _GAMEPAD_BUTTON_R2_TRIGGER,
    _GAMEPAD_BUTTON_SELECT,
    _GAMEPAD_BUTTON_START,
    _GAMEPAD_BUTTON_LEFT_ANALOG_STICK_BUTTON,
    _GAMEPAD_BUTTON_RIGHT_ANALOG_STICK_BUTTON,
    _GAMEPAD_BUTTON_DPAD_UP,
    _GAMEPAD_BUTTON_DPAD_DOWN,
    _GAMEPAD_BUTTON_DPAD_LEFT,
    _GAMEPAD_BUTTON_DPAD_RIGHT,
    _GAMEPAD_BUTTON_GAME,
    _GAMEPAD_BUTTON_MAX,
};

enum _GamepadAxeCode
{
    _GAMEPAD_AXE_INVALID = -1,
    _GAMEPAD_AXE_LEFT_X,
    _GAMEPAD_AXE_LEFT_Y,
    _GAMEPAD_AXE_RIGHT_X,
    _GAMEPAD_AXE_RIGHT_Y,
    _GAMEPAD_AXE_MAX,
};
#endif

class GamepadDeviceLinux {
public:
    bool connected() { return m_connected; };

    String id() { return m_deviceName; }
    unsigned long long timestamp() { return m_lastTimestamp; }

    unsigned axesCount() { return m_axes.size(); }
    float* axesData() { return m_axes.data(); }

    unsigned buttonsCount() { return m_buttons.size(); }
    float* buttonsData() { return m_buttons.data(); }

protected:
    GamepadDeviceLinux(String deviceFile);
    ~GamepadDeviceLinux();

#if ENABLE(TIZEN_GAMEPAD)
    void updateForEvent(struct input_event);
#else
    void updateForEvent(struct js_event);
#endif
    int m_fileDescriptor;

private:
    float normalizeAxisValue(short value);
    float normalizeButtonValue(short value);
#if ENABLE(TIZEN_GAMEPAD)
    void processAxeEvent(unsigned int code, int value);
    void processButtonEvent(unsigned int code, int value);
#endif

    bool m_connected;
    String m_deviceName;
    unsigned long long m_lastTimestamp;

    Vector<float> m_axes;
    Vector<float> m_buttons;
};

} // namespace WebCore

#endif // ENABLE(GAMEPAD)

#endif // GamepadDeviceLinux_h
