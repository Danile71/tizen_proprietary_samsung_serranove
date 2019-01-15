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

#include "config.h"
#include "GamepadDeviceLinux.h"

#if ENABLE(GAMEPAD)

#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <wtf/text/CString.h>

namespace WebCore {

GamepadDeviceLinux::GamepadDeviceLinux(String deviceFile)
    : m_fileDescriptor(-1)
    , m_connected(false)
    , m_lastTimestamp(0)
{
    // FIXME: Log errors when returning early.
    m_fileDescriptor = open(deviceFile.utf8().data(), O_RDONLY | O_NONBLOCK);
    if (m_fileDescriptor == -1) {
        TIZEN_LOGE("device open fail! (%d)", errno);
        return;
    }

#if ENABLE(TIZEN_GAMEPAD)
    char deviceName[1024];
    if (ioctl(m_fileDescriptor, EVIOCGNAME(sizeof(deviceName)), deviceName) < 0)
        return;
    m_deviceName = String(deviceName).simplifyWhiteSpace();

    m_axes.fill(0.0, _GAMEPAD_AXE_MAX);
    m_buttons.fill(0.0, _GAMEPAD_BUTTON_MAX);
#else
    char deviceName[1024];
    if (ioctl(m_fileDescriptor, JSIOCGNAME(sizeof(deviceName)), deviceName) < 0)
        return;
    m_deviceName = String(deviceName).simplifyWhiteSpace();

    uint8_t numberOfAxes;
    uint8_t numberOfButtons;
    if (ioctl(m_fileDescriptor, JSIOCGAXES, &numberOfAxes) < 0 || ioctl(m_fileDescriptor, JSIOCGBUTTONS, &numberOfButtons) < 0)
        return;
    m_axes.fill(0.0, numberOfAxes);
    m_buttons.fill(0.0, numberOfButtons);
#endif
}

GamepadDeviceLinux::~GamepadDeviceLinux()
{
    if (m_fileDescriptor != -1)
        close(m_fileDescriptor);
}

#if ENABLE(TIZEN_GAMEPAD)
void GamepadDeviceLinux::processButtonEvent(unsigned int code, int value)
{
    int gamepadButtonCode = _GAMEPAD_BUTTON_INVALID;
    switch(code)
    {
        case BTN_A:
            gamepadButtonCode = _GAMEPAD_BUTTON_1;
            break;
        case BTN_B:
            gamepadButtonCode = _GAMEPAD_BUTTON_2;
            break;
        case BTN_X:
            gamepadButtonCode = _GAMEPAD_BUTTON_3;
            break;
        case BTN_Y:
            gamepadButtonCode = _GAMEPAD_BUTTON_4;
            break;
        case BTN_TL:
            gamepadButtonCode = _GAMEPAD_BUTTON_L1_SHOULDER;
            break;
        case BTN_TR:
            gamepadButtonCode = _GAMEPAD_BUTTON_R1_SHOULDER;
            break;
        case BTN_TL2:
            gamepadButtonCode = _GAMEPAD_BUTTON_L2_TRIGGER;
            break;
        case BTN_TR2:
            gamepadButtonCode = _GAMEPAD_BUTTON_R2_TRIGGER;
            break;
        case BTN_SELECT:
            gamepadButtonCode = _GAMEPAD_BUTTON_SELECT;
            break;
        case BTN_START:
            gamepadButtonCode = _GAMEPAD_BUTTON_START;
            break;
        case BTN_THUMBL:
            gamepadButtonCode = _GAMEPAD_BUTTON_LEFT_ANALOG_STICK_BUTTON;
            break;
        case BTN_THUMBR:
            gamepadButtonCode = _GAMEPAD_BUTTON_RIGHT_ANALOG_STICK_BUTTON;
            break;;
        case 0x13f: // button "PLAY"
            gamepadButtonCode = _GAMEPAD_BUTTON_GAME;
            break;
        default:
            break;
    }

    if (gamepadButtonCode != _GAMEPAD_BUTTON_INVALID)
        m_buttons[gamepadButtonCode] = normalizeButtonValue(value);
}

void GamepadDeviceLinux::processAxeEvent(unsigned int code, int value)
{
    int gamepadAxeCode = _GAMEPAD_AXE_INVALID;
    switch(code)
    {
        case ABS_X:
            gamepadAxeCode = _GAMEPAD_AXE_LEFT_X;
            break;
        case ABS_Y:
            gamepadAxeCode = _GAMEPAD_AXE_LEFT_Y;
            break;
        case ABS_RX:
            gamepadAxeCode = _GAMEPAD_AXE_RIGHT_X;
            break;
        case ABS_RY:
            gamepadAxeCode = _GAMEPAD_AXE_RIGHT_Y;
            break;
        default:
            break;
    }

    if (gamepadAxeCode != _GAMEPAD_AXE_INVALID) {
        m_axes[gamepadAxeCode] = normalizeAxisValue(value);
        return;
    }

    int gamepadButtonCode = _GAMEPAD_BUTTON_INVALID;
    switch(code)
    {
        case ABS_HAT0X:
            if (value == -1)
                gamepadButtonCode = _GAMEPAD_BUTTON_DPAD_LEFT;
            else if (value == 1)
                gamepadButtonCode = _GAMEPAD_BUTTON_DPAD_RIGHT;
            else if (!value) {
                m_buttons[_GAMEPAD_BUTTON_DPAD_LEFT] = normalizeButtonValue(value);
                m_buttons[_GAMEPAD_BUTTON_DPAD_RIGHT] = normalizeButtonValue(value);
            }
            break;
        case ABS_HAT0Y:
            if (value == -1)
                gamepadButtonCode = _GAMEPAD_BUTTON_DPAD_UP;
            else if (value == 1)
                gamepadButtonCode = _GAMEPAD_BUTTON_DPAD_DOWN;
            else if (!value) {
                m_buttons[_GAMEPAD_BUTTON_DPAD_UP] = normalizeButtonValue(value);
                m_buttons[_GAMEPAD_BUTTON_DPAD_DOWN] = normalizeButtonValue(value);
            }
            break;
        default:
            break;
    }

    if (gamepadButtonCode != _GAMEPAD_BUTTON_INVALID)
        m_buttons[gamepadButtonCode] = normalizeButtonValue(value);
}

void GamepadDeviceLinux::updateForEvent(struct input_event event)
{
    if (event.type != EV_KEY && event.type != EV_ABS)
        return;

    if (!m_connected)
        m_connected = true;

    if (event.type == EV_KEY)
        processButtonEvent(event.code, event.value);
    else if (event.type == EV_ABS)
        processAxeEvent(event.code, event.value);
}
#else
void GamepadDeviceLinux::updateForEvent(struct js_event event)
{
    if (!(event.type & JS_EVENT_AXIS || event.type & JS_EVENT_BUTTON))
        return;

    // Mark the device as connected only if it is not yet connected, the event is not an initialization
    // and the value is not 0 (indicating a genuine interaction with the device).
    if (!m_connected && !(event.type & JS_EVENT_INIT) && event.value)
        m_connected = true;

    if (event.type & JS_EVENT_AXIS)
        m_axes[event.number] = normalizeAxisValue(event.value);
    else if (event.type & JS_EVENT_BUTTON)
        m_buttons[event.number] = normalizeButtonValue(event.value);

    m_lastTimestamp = event.time;
}
#endif

float GamepadDeviceLinux::normalizeAxisValue(short value)
{
    // Normalize from range [0, 255] into range [-1.0, 1.0]
    return (value - 127.5f) / 127.5f;
}

float GamepadDeviceLinux::normalizeButtonValue(short value)
{
    // Normalize from range [0, 1] into range [0.0, 1.0]
    return value < 0 ? value / -1.0f : value / 1.0f;
}

} // namespace WebCore

#endif // ENABLE(GAMEPAD)
