/* Copyright 2024 teamprof.net@gmail.com
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this
 * software and associated documentation files (the "Software"), to deal in the Software
 * without restriction, including without limitation the rights to use, copy, modify,
 * merge, publish, distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
 * PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#pragma once
#include <stdint.h>

enum AppEvent : int16_t
{
    /////////////////////////////////////////////////////////////////////////////
    EventNull = 0,

    EventGpioISR = 10, // iParam=pin, uParam=value, lParam=millis()
    EventSystem,       // iParam=SystemTriggerSource

    ///////////////////////////////////////////////////////////////////////
    // Inter-process event
    ///////////////////////////////////////////////////////////////////////
    EventIpc = 300, // iParam=IpcParam

    ///////////////////////////////////////////////////////////////////////
    EventWifiStatus = 400, // iParam = WiFiEvent_t
    EventInternetStatus,   // iParam = InternetStatus
    EventSendMessage,
    EventMessageStatus, // iParam = MessageStatus

    /////////////////////////////////////////////////////////////////////////////
};

enum SystemTriggerSource : int16_t
{
    SysInitDone = 0,
    SysSoftwareTimer, // lParam=xTimer:uint32_t
    SysVbusDetect,    // uParam=isVbusDetected:bool
    SysLowBattery,
    SysButtonClick,       // uParam=pin number
    SysButtonDoubleClick, // uParam=pin number
    SysButtonLongPress,   // uParam=pin number
    SysSerial,            // lParam=ptr to Serial
};

typedef enum _InternetStatus : int16_t
{
    Disconnect = 0,
    Connect,
} InternetStatus;

typedef enum _MessageStatus : int16_t
{
    UnknownStatus = 0,
    Sending,

    SentSuccess,
    SentFail,
} MessageStatus;

typedef enum _IpcParam : int16_t
{
    IpcNull = 0,
    IpcNpuStart,
    IpcNpuStop,

    IpcNpuNoObjectDetected,
    IpcNpuObjectUnclassified,
    IpcNpuStrangerDetected,
    IpcNpuTenderDetected,
} IpcParam;
