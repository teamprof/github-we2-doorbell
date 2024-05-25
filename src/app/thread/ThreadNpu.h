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
#include <Seeed_Arduino_SSCMA.h>
#include <map>
#include "../ArduProfFreeRTOS.h"
#include "../AppEvent.h"

namespace freertos
{
    class ThreadNpu : public ardufreertos::ThreadBase
    {
    public:
        ThreadNpu();
        virtual void start(void *);

    protected:
        typedef void (ThreadNpu::*handlerFunc)(const Message &);
        std::map<int16_t, handlerFunc> handlerMap;
        virtual void onMessage(const Message &msg);

        virtual void run(void);

    private:
        static ThreadNpu *_instance;

        SSCMA _ai;
        bool _isNpuRunning;

        TaskHandle_t _taskInitHandle;

        ardufreertos::PeriodicTimer _timer2Hz;
        ardufreertos::PeriodicTimer _timer1Hz;

        virtual void setup(void);
        virtual void delayInit(void);

        void handlerSoftwareTimer(TimerHandle_t xTimer);
        void doInference(void);

        ///////////////////////////////////////////////////////////////////////
        // declare event handler
        ///////////////////////////////////////////////////////////////////////
        __EVENT_FUNC_DECLARATION(EventIpc)
        __EVENT_FUNC_DECLARATION(EventSystem)
        __EVENT_FUNC_DECLARATION(EventNull) // void handlerEventNull(const Message &msg);
    };
} // namespace freertos