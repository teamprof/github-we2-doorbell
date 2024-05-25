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
#include "./ThreadNpu.h"
#include "../AppContext.h"

////////////////////////////////////////////////////////////////////////////////////////////
#define NO_OBJECT_COUNT 10 // (number of seconds) x (timer frequency): e.g. 5 seconds x 2 Hz = 10

////////////////////////////////////////////////////////////////////////////////////////////
#define AI_TARGET_TENANT 0
#define AI_TARGET_STRANGER 1

#define AI_SCORE_THRESHOLD 60

////////////////////////////////////////////////////////////////////////////////////////////
// Thread
////////////////////////////////////////////////////////////////////////////////////////////
// #define RUNNING_CORE 0 // dedicate core 0 for Thread
// #define RUNNING_CORE 1 // dedicate core 1 for Thread
#define RUNNING_CORE ARDUINO_RUNNING_CORE

#define TASK_NAME "ThreadNpu"
#define TASK_STACK_SIZE 2048
#define TASK_PRIORITY 3
#define TASK_QUEUE_SIZE 128 // message queue size for app task

////////////////////////////////////////////////////////////////////////////////////////////
namespace freertos
{

    ////////////////////////////////////////////////////////////////////////////////////////////
    ThreadNpu *ThreadNpu::_instance = nullptr;

    static uint8_t ucQueueStorageArea[TASK_QUEUE_SIZE * sizeof(Message)];
    static StaticQueue_t xStaticQueue;

    static StackType_t xStack[TASK_STACK_SIZE];
    static StaticTask_t xTaskBuffer;
    ////////////////////////////////////////////////////////////////////////////////////////////

    ThreadNpu::ThreadNpu() : ThreadBase(TASK_QUEUE_SIZE, ucQueueStorageArea, &xStaticQueue),
                             _ai(),
                             _isNpuRunning(false),
                             _timer1Hz("Timer 1Hz",
                                       pdMS_TO_TICKS(1000),
                                       [](TimerHandle_t xTimer)
                                       {
                                           if (_instance)
                                           {
                                               auto context = reinterpret_cast<AppContext *>(_instance->context());
                                               if (context && context->threadNpu)
                                               {
                                                   static_cast<freertos::ThreadNpu *>(context->threadNpu)->postEvent(EventSystem, SysSoftwareTimer, 0, (uint32_t)xTimer);
                                               }
                                           }
                                       }),
                             _timer2Hz("Timer 2Hz",
                                       pdMS_TO_TICKS(500),
                                       [](TimerHandle_t xTimer)
                                       {
                                           if (_instance)
                                           {
                                               auto context = reinterpret_cast<AppContext *>(_instance->context());
                                               if (context && context->threadNpu)
                                               {
                                                   static_cast<freertos::ThreadNpu *>(context->threadNpu)->postEvent(EventSystem, SysSoftwareTimer, 0, (uint32_t)xTimer);
                                               }
                                           }
                                       }),
                             handlerMap()
    {
        _instance = this;

        handlerMap = {
            __EVENT_MAP(ThreadNpu, EventIpc),
            __EVENT_MAP(ThreadNpu, EventSystem),
            __EVENT_MAP(ThreadNpu, EventNull), // {EventNull, &ThreadMessaging::handlerEventNull},
        };
    }

    __EVENT_FUNC_DEFINITION(ThreadNpu, EventIpc, msg) // void ThreadNpu::handlerEventIpc(const Message &msg)
    {
        // LOG_TRACE("EventIpc(", msg.event, "), iParam = ", msg.iParam, ", uParam = ", msg.uParam, ", lParam = ", msg.lParam);
        IpcParam ipcParam = static_cast<IpcParam>(msg.iParam);
        switch (ipcParam)
        {
        case IpcNpuStart:
            LOG_TRACE("IpcNpuStart");
            if (!_isNpuRunning)
            {
                _isNpuRunning = true;
                _timer2Hz.start();
            }
            break;

        case IpcNpuStop:
            LOG_TRACE("IpcNpuStop");
            _isNpuRunning = false;
            _timer2Hz.stop();
            break;

        default:
            LOG_TRACE("unsupported iParam=", msg.iParam);
            break;
        }
    }
    __EVENT_FUNC_DEFINITION(ThreadNpu, EventSystem, msg) // void ThreadNpu::handlerEventSystem(const Message &msg)
    {
        // LOG_TRACE("EventSystem(", msg.event, "), iParam = ", msg.iParam, ", uParam = ", msg.uParam, ", lParam = ", msg.lParam);
        enum SystemTriggerSource src = static_cast<SystemTriggerSource>(msg.iParam);
        switch (src)
        {
        case SysSoftwareTimer:
            handlerSoftwareTimer((TimerHandle_t)(msg.lParam));
            break;
        default:
            LOG_TRACE("unsupported SystemTriggerSource=", src);
            break;
        }
    }
    __EVENT_FUNC_DEFINITION(ThreadNpu, EventNull, msg) // void ThreadNpu::handlerEventNull(const Message &msg)
    {
        LOG_TRACE("EventNull(", msg.event, "), iParam = ", msg.iParam, ", uParam = ", msg.uParam, ", lParam = ", msg.lParam);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////
    void ThreadNpu::onMessage(const Message &msg)
    {
        // LOG_TRACE("event=", msg.event, ", iParam=", msg.iParam, ", uParam=", msg.uParam, ", lParam=", msg.lParam);
        auto func = handlerMap[msg.event];
        if (func)
        {
            (this->*func)(msg);
        }
        else
        {
            LOG_TRACE("Unsupported event = ", msg.event, ", iParam = ", msg.iParam, ", uParam = ", msg.uParam, ", lParam = ", msg.lParam);
        }
    }

    void ThreadNpu::start(void *ctx)
    {
        // LOG_TRACE("start() on core ", xPortGetCoreID(), ", xPortGetFreeHeapSize()=", xPortGetFreeHeapSize());
        ThreadBase::start(ctx);

        _taskHandle = xTaskCreateStaticPinnedToCore(
            [](void *instance)
            { static_cast<ThreadNpu *>(instance)->run(); },
            TASK_NAME,
            TASK_STACK_SIZE, // This stack size can be checked & adjusted by reading the Stack Highwater
            this,
            TASK_PRIORITY, // Priority, with 3 (configMAX_PRIORITIES - 1) being the highest, and 0 being the lowest.
            xStack,
            &xTaskBuffer,
            RUNNING_CORE);
    }

    void ThreadNpu::setup(void)
    {
        // LOG_TRACE("setup() on core ", xPortGetCoreID(), ", xPortGetFreeHeapSize()=", xPortGetFreeHeapSize());
        ThreadBase::setup();

        _ai.begin();

        _timer1Hz.start();
        // _timer1Hz.stop();

        // _timer2Hz.start();
        // _timer2Hz.stop();

        // vTaskDelay(pdMS_TO_TICKS(1000));
    }

    void ThreadNpu::run(void)
    {
        // LOG_TRACE("run() on core ", xPortGetCoreID(), ", xPortGetFreeHeapSize()=", xPortGetFreeHeapSize());
        ThreadBase::run();
    }

    void ThreadNpu::delayInit(void)
    {
        // LOG_TRACE("delayInit() on core ", xPortGetCoreID(), ", xPortGetFreeHeapSize()=", xPortGetFreeHeapSize());

        //////////////////////////////////////////////////////////////
        // add time consuming init code here
        //////////////////////////////////////////////////////////////
    }

    void ThreadNpu::handlerSoftwareTimer(TimerHandle_t xTimer)
    {
        if (xTimer == _timer1Hz.timer())
        {
            // LOG_TRACE("_timer1Hz");
        }
        else if (xTimer == _timer2Hz.timer())
        {
            // LOG_TRACE("_timer2Hz");
            if (_isNpuRunning)
            {
                doInference();
            }
            else
            {
                _timer2Hz.stop();
            }
        }
        else
        {
            LOG_TRACE("unsupported timer handle=0x%04x", (uint32_t)(xTimer));
        }
    }

    void ThreadNpu::doInference(void)
    {
        if (!_ai.invoke() && _ai.boxes().size() > 0)
        {
            // LOG_TRACE("perf: prepocess=", _ai.perf().prepocess, ", inference=", _ai.perf().inference, ", postpocess=", _ai.perf().postprocess);
            // LOG_TRACE("_ai.boxes().size()=", _ai.boxes().size(), ", .classes().size()=", _ai.classes().size(), ", .points().size()=", _ai.points().size(), ", .keypoints().size()=", _ai.keypoints().size());

            auto appCtx = static_cast<AppContext *>(context());
            for (int i = 0; i < _ai.boxes().size(); i++)
            {
                auto target = _ai.boxes()[i].target;
                auto score = _ai.boxes()[i].score;
                // LOG_TRACE("Box[", i, "], target=", _ai.boxes()[i].target, ", score=", _ai.boxes()[i].score,
                //           ", x=", _ai.boxes()[i].x, ", y=", _ai.boxes()[i].y, ", w=", _ai.boxes()[i].w, ", h=", _ai.boxes()[i].h);

                if (score >= AI_SCORE_THRESHOLD)
                {
                    postEvent(appCtx->queueMain, EventIpc, target == AI_TARGET_TENANT ? IpcNpuTenderDetected : IpcNpuStrangerDetected, score);
                }
                else
                {
                    postEvent(appCtx->queueMain, EventIpc, IpcNpuObjectUnclassified);
                }
            }
            for (int i = 0; i < _ai.classes().size(); i++)
            {
                LOG_TRACE("Class[", i, "], target=", _ai.classes()[i].target, ", score=", _ai.classes()[i].score);
            }

            for (int i = 0; i < _ai.points().size(); i++)
            {
                LOG_TRACE("Point[", i, "], target=", _ai.points()[i].target, ", score=", _ai.points()[i].score, ", x=", _ai.points()[i].x, ", y=", _ai.points()[i].y);
            }
            for (int i = 0; i < _ai.keypoints().size(); i++)
            {
                LOG_TRACE("keypoint[", i, "], target=", _ai.keypoints()[i].box.target, ", score=", _ai.keypoints()[i].box.score,
                          ", box:[x=", _ai.keypoints()[i].box.x, ", y=", _ai.keypoints()[i].box.y, ", w=", _ai.keypoints()[i].box.w, ", h=", _ai.keypoints()[i].box.h, "]");
                LOG_TRACE("points:[");
                for (int j = 0; j < _ai.keypoints()[i].points.size(); j++)
                {
                    LOG_TRACE("\t[", _ai.keypoints()[i].points[j].x, ", ", _ai.keypoints()[i].points[j].y, "]");
                }
                LOG_TRACE("]");
            }
        }
        else
        {
            auto appCtx = static_cast<AppContext *>(context());
            postEvent(appCtx->queueMain, EventIpc, IpcNpuNoObjectDetected);
        }
    }

} // namespace freertos