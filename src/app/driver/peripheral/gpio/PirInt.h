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
#include <FunctionalInterrupt.h>
#include "../../../ArduProfFreeRTOS.h"
#include "../../../pins.h"
#include "../../../AppEvent.h"

#ifdef GPIO_PIN
#undef GPIO_PIN
#endif

/////////////////////////////////////////////////////////////
#define GPIO_PIN PIN_PIR_INT

class PirInt : public Gpio
{
public:
    PirInt() : Gpio(GPIO_PIN, INPUT)
    {
    }

    void enableInterrupt(ardufreertos::MessageQueue *msgQueue)
    {
        // static int _value = digitalRead(GPIO_PIN);

        attachIntr(
            RISING, [](void *ptr)
            {
                int value = digitalRead(GPIO_PIN);
                if (value == HIGH)
                { // workaround of ensuring Rising interrupt
                    auto msgQueue = static_cast<ardufreertos::MessageQueue *>(ptr);
                    msgQueue->postEvent(EventGpioISR, GPIO_PIN, value, millis());
                }

                // int newValue = digitalRead(GPIO_PIN);
                // if (_value != newValue)
                // {
                //     _value = newValue;
                //     auto msgQueue = static_cast<ardufreertos::MessageQueue *>(ptr);
                //     msgQueue->postEvent(EventGpioISR, GPIO_PIN, newValue, millis());
                //     // msgQueue->postEvent(EventGpioISR, GPIO_PIN, digitalRead(GPIO_PIN), millis());
                // }
                //
            },
            msgQueue);
        // attachIntr(RISING, std::bind(&PirInt::isr, this));
    }

    void disableInterrupt(void)
    {
        detachIntr();
    }

    bool isActive(void)
    {
        return (read() == HIGH);
    }

private:
    // void isr(void)
    // {
    //     // sendMessageFromIsrToTask(EventGpioISR, _PIN, digitalRead(_PIN), millis());
    // }
};

#undef GPIO_PIN
