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
#include "./QueueMain.h"
#include "../AppContext.h"
#include "../AppDef.h"

////////////////////////////////////////////////////////////////////////////////////////////
#define NO_OBJECT_COUNT 10 // (number of seconds) x (timer frequency): e.g. 5 seconds x 2 Hz = 10

////////////////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////////////////
#define TASK_QUEUE_SIZE 2048 // message queue size for app task

#define LOW_POWER_COUNT 5       // in unit of seconds
#define NO_OBJECT_COUNT (5 * 2) // (expiry seconds) x (timer frequency)

namespace freertos
{
    ////////////////////////////////////////////////////////////////////////////////////////////
    QueueMain *QueueMain::_instance = nullptr;

    ////////////////////////////////////////////////////////////////////////////////////////////
    static uint8_t ucQueueStorageArea[TASK_QUEUE_SIZE * sizeof(Message)];
    static StaticQueue_t xStaticQueue;

    /////////////////////////////////////////////////////////////////////////////
    QueueMain::QueueMain() : ardufreertos::MessageBus(TASK_QUEUE_SIZE, ucQueueStorageArea, &xStaticQueue),
                             _isInternetConnected(false),
                             _isMessageSending(false),
                             _idleCount(0),
                             _noObjCount(0),
                             _lastNpuResult(IpcNpuNoObjectDetected),
                             _isNpuRunning(false),
                             _debounceTimer(queue(), EventSystem, SysSoftwareTimer),
                             _buttonBoot(queue()),
                             _pirInt(),
                             _timer1Hz("Timer 1Hz",
                                       pdMS_TO_TICKS(1000),
                                       [](TimerHandle_t xTimer)
                                       {
                                           if (_instance)
                                           {
                                               auto context = reinterpret_cast<AppContext *>(_instance->context());
                                               if (context && context->queueMain)
                                               {
                                                   static_cast<freertos::QueueMain *>(context->queueMain)->postEvent(EventSystem, SysSoftwareTimer, 0, (uint32_t)xTimer);
                                               }
                                           }
                                       }),

                             handlerMap()
    {
        _instance = this;

        handlerMap = {
            __EVENT_MAP(QueueMain, EventIpc),
            __EVENT_MAP(QueueMain, EventMessageStatus),
            __EVENT_MAP(QueueMain, EventInternetStatus),
            __EVENT_MAP(QueueMain, EventGpioISR),
            __EVENT_MAP(QueueMain, EventSystem),
            __EVENT_MAP(QueueMain, EventNull), // {EventNull, &QueueMain::handlerEventNull},
        };
    }

    void QueueMain::start(void *ctx)
    {
        // LOG_TRACE("on core ", xPortGetCoreID(), ", xPortGetFreeHeapSize()=", xPortGetFreeHeapSize());
        MessageBus::start(ctx);

        LOG_TRACE("CPU Frequency: ", getCpuFrequencyMhz(), " MHz");

        _timer1Hz.start();
        // _timer1Hz.stop();

        _buttonBoot.init(EventSystem, SysButtonClick, SysButtonDoubleClick, SysButtonLongPress);
        _debounceTimer.attachButton(&_buttonBoot);

        LOG_TRACE("_pirInt.enableInterrupt()");
        _pirInt.enableInterrupt(this);
    }

    void QueueMain::onMessage(const Message &msg)
    {
        auto func = handlerMap[msg.event];
        if (func)
        {
            (this->*func)(msg);
        }
        else
        {
            LOG_TRACE("Unsupported event=", msg.event, ", iParam=", msg.iParam, ", uParam=", msg.uParam, ", lParam=", msg.lParam);
        }
    }

    /////////////////////////////////////////////////////////////////////////////
    __EVENT_FUNC_DEFINITION(QueueMain, EventIpc, msg) // void QueueMain::handlerEventIpc(const Message &msg)
    {
        // LOG_TRACE("EventIpc(", msg.event, "), iParam = ", msg.iParam, ", uParam = ", msg.uParam, ", lParam = ", msg.lParam);
        switch (msg.iParam)
        {
        case IpcNpuNoObjectDetected:
        case IpcNpuObjectUnclassified:
            LOG_TRACE(msg.iParam == IpcNpuNoObjectDetected ? "IpcNpuNoObjectDetected" : "IpcNpuObjectUnclassified");
            if (_noObjCount++ >= NO_OBJECT_COUNT && !_pirInt.isActive())
            {
                _isNpuRunning = false;

                auto appCtx = static_cast<AppContext *>(context());
                postEvent(appCtx->threadNpu, EventIpc, IpcNpuStop);
                _noObjCount = 0;
                _lastNpuResult = msg.iParam;

                LOG_TRACE("_pirInt.enableInterrupt()");
                _pirInt.enableInterrupt(this);
            }
            break;
        case IpcNpuStrangerDetected:
            LOG_TRACE("IpcNpuStrangerDetected, _lastNpuResult=", _lastNpuResult);
            _noObjCount = 0;
            if (!_isMessageSending && _lastNpuResult != IpcNpuStrangerDetected)
            {
                _lastNpuResult = IpcNpuStrangerDetected;
                auto appCtx = static_cast<AppContext *>(context());
                postEvent(appCtx->threadMessaging, EventSendMessage, IpcNpuStrangerDetected);
            }
            break;
        case IpcNpuTenderDetected:
            LOG_TRACE("IpcNpuTenderDetected, _lastNpuResult=", _lastNpuResult);
            _noObjCount = 0;
            if (!_isMessageSending && _lastNpuResult != IpcNpuTenderDetected)
            {
                _lastNpuResult = IpcNpuTenderDetected;
                auto appCtx = static_cast<AppContext *>(context());
                postEvent(appCtx->threadMessaging, EventSendMessage, IpcNpuTenderDetected);
            }
            break;
        default:
            LOG_TRACE("unsupported iParam: ", msg.iParam);
            break;
        }
    }
    __EVENT_FUNC_DEFINITION(QueueMain, EventMessageStatus, msg) // void QueueMain::handlerEventMessageStatus(const Message &msg)
    {
        // LOG_TRACE("EventMessageStatus(", msg.event, "), iParam = ", msg.iParam, ", uParam = ", msg.uParam, ", lParam = ", msg.lParam);
        MessageStatus status = (MessageStatus)msg.iParam;
        switch (status)
        {
        case MessageStatus::Sending:
            LOG_WARN("MessageStatus::Sending");
            _isMessageSending = true;
            _idleCount = 0;
            break;

        case MessageStatus::SentSuccess:
            LOG_WARN("MessageStatus::SentSuccess");
            _isMessageSending = false;
            break;

        case MessageStatus::SentFail:
            _isMessageSending = false;
            LOG_WARN("MessageStatus::SentFail");
            break;

        default:
            LOG_TRACE("unsupported MessageStatus: ", status);
            _isMessageSending = false;
            break;
        }
    }
    __EVENT_FUNC_DEFINITION(QueueMain, EventInternetStatus, msg) // void QueueMain::handlerEventInternetStatus(const Message &msg)
    {
        // LOG_TRACE("EventInternetStatus(", msg.event, "), iParam = ", msg.iParam, ", uParam = ", msg.uParam, ", lParam = ", msg.lParam);
        _isInternetConnected = msg.iParam == InternetStatus::Connect;
        LOG_TRACE("_isInternetConnected=", _isInternetConnected ? "true" : "false");
    }

    __EVENT_FUNC_DEFINITION(QueueMain, EventGpioISR, msg) // void QueueMain::handlerEventGpioISR(const Message &msg)
    {
        // LOG_TRACE("EventGpioISR(", msg.event, "), iParam = ", msg.iParam, ", uParam = ", msg.uParam, ", lParam = ", msg.lParam);

        uint8_t pin = msg.iParam;
        uint8_t value = msg.uParam;
        if (pin == _pirInt.getPin())
        {
            if (_isNpuRunning)
            {
                return;
            }

            _isNpuRunning = true;
            LOG_TRACE("_pirInt.disableInterrupt()");
            _pirInt.disableInterrupt();

            auto appCtx = static_cast<AppContext *>(context());
            postEvent(appCtx->threadNpu, EventIpc, IpcNpuStart);
        }
        else if (pin == _buttonBoot.getPin())
        {
            uint32_t ms = msg.lParam;
            _buttonBoot.onEventIsr(value, ms);
        }
        else
        {
            LOG_TRACE("unsupported button: GPIO", pin);
        }
    }

    __EVENT_FUNC_DEFINITION(QueueMain, EventSystem, msg) // void QueueMain::handlerEventSystem(const Message &msg)
    {
        // LOG_TRACE("EventSystem(", msg.event, "), iParam = ", msg.iParam, ", uParam = ", msg.uParam, ", lParam = ", msg.lParam);
        enum SystemTriggerSource src = static_cast<SystemTriggerSource>(msg.iParam);
        switch (src)
        {
        case SysSoftwareTimer:
            handlerSoftwareTimer((TimerHandle_t)(msg.lParam));
            break;
        case SysButtonClick:
        {
            handlerButtonClick(msg);
            break;
        }
        case SysButtonDoubleClick:
        {
            handlerButtonDoubleClick(msg);
            break;
        }
        case SysButtonLongPress:
        {
            handlerButtonLongPress(msg);
            break;
        }
        default:
            LOG_TRACE("unsupported SystemTriggerSource=", src);
            break;
        }
    }

    // define EventNull handler
    __EVENT_FUNC_DEFINITION(QueueMain, EventNull, msg) // void QueueMain::handlerEventNull(const Message &msg)
    {
        LOG_TRACE("EventNull(", msg.event, "), iParam=", msg.iParam, ", uParam=", msg.uParam, ", lParam=", msg.lParam);
    }
    /////////////////////////////////////////////////////////////////////////////

    void QueueMain::handlerSoftwareTimer(TimerHandle_t xTimer)
    {
        if (xTimer == _debounceTimer.timer())
        {
            // LOG_TRACE("debounceTimer::timer()");
            _debounceTimer.onEventTimer();
        }
        else if (xTimer == _timer1Hz.timer())
        {
            // LOG_TRACE("_timer1Hz");
            // LOG_TRACE("_pirInt.read() retutns ", _pirInt.read());

            if (_isInternetConnected &&
                !_isMessageSending &&
                !_pirInt.isActive() &&
                (_idleCount++ >= LOW_POWER_COUNT))
            {
                _idleCount = 0;
            }
            else
            {
                // LOG_TRACE("_idleCount=", _idleCount);
            }
        }
        else
        {
            LOG_TRACE("unsupported timer handle=0x%04x", (uint32_t)(xTimer));
        }
    }

    void QueueMain::debounce(uint32_t start, uint32_t ms)
    {
        // simple debounce
        uint32_t currentMs = millis();
        uint32_t delta = (start < currentMs) ? currentMs - start : start - currentMs;
        if (delta < DEBOUNCE_TIME)
        {
            vTaskDelay(pdMS_TO_TICKS(DEBOUNCE_TIME - delta));
        }
    }

    void QueueMain::handlerButtonClick(const Message &msg)
    {
        int16_t pin = msg.uParam;
        if (pin == _buttonBoot.getPin())
        {
            LOG_TRACE("ButtonClick: buttonBoot");

            // for testing only
            // auto appCtx = static_cast<AppContext *>(context());
            // if (appCtx && appCtx->threadNpu)
            // {
            //     _isNpuRunning = true;

            //     LOG_TRACE("_pirInt.disableInterrupt()");
            //     _pirInt.disableInterrupt();

            //     appCtx->threadNpu->postEvent(EventIpc, IpcNpuStart);
            // }
        }
        else
        {
            LOG_TRACE("SysButtonClick: unsupported pin=", pin);
        }
    }
    void QueueMain::handlerButtonDoubleClick(const Message &msg)
    {
        int16_t pin = msg.uParam;
        if (pin == _buttonBoot.getPin())
        {
            LOG_TRACE("SysButtonDoubleClick: buttonBoot");
        }
        else
        {
            LOG_TRACE("SysButtonDoubleClick: unsupported pin=", pin);
        }
    }
    void QueueMain::handlerButtonLongPress(const Message &msg)
    {
        int16_t pin = msg.uParam;
        if (pin == _buttonBoot.getPin())
        {
            LOG_TRACE("SysButtonLongPress: buttonBoot");
        }
        else
        {
            LOG_TRACE("SysButtonLongPress: unsupported pin=", pin);
        }
    }

    /////////////////////////////////////////////////////////////////////////////

} // namespace freertos
