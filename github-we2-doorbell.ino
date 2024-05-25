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
#include <Arduino.h>
#include <WiFi.h>
#include "./src/app/ArduProfFreeRTOS.h"
#include "./src/app/AppContext.h"
#include "./src/app/AppLog.h"
#include "./src/app/thread/QueueMain.h"
#include "./src/app/thread/ThreadNpu.h"
#include "./src/app/thread/ThreadMessaging.h"
#include "./src/app/util/EspUtil.h"

/////////////////////////////////////////////////////////////////////////////
static AppContext appContext = {0};

static void initGlobalVar(void)
{
    WiFi.macAddress(appContext.mac);
    LOG_TRACE("WiFi MAC address: ", DebugLogBase::HEX, appContext.mac[0], "-", appContext.mac[1], "-", appContext.mac[2], "-", appContext.mac[3], "-", appContext.mac[4], "-", appContext.mac[5]);
}

static void createTasks(void)
{
    static freertos::QueueMain queueMain;
    static freertos::ThreadNpu threadNpu;
    static freertos::ThreadMessaging threadMessaging;

    appContext.queueMain = &queueMain;
    appContext.threadNpu = &threadNpu;
    appContext.threadMessaging = &threadMessaging;

    static_cast<freertos::QueueMain *>(appContext.queueMain)->start(&appContext);
    static_cast<freertos::ThreadNpu *>(appContext.threadNpu)->start(&appContext);
    static_cast<freertos::ThreadMessaging *>(appContext.threadMessaging)->start(&appContext);
}

void printChipInfo(void)
{
    PRINTLN("===============================================================================");
    PRINTLN("ESP.Reset reason=", getEspResetReasonString());
    PRINTLN("chipModel=", ESP.getChipModel(), ", chipRevision=", ESP.getChipRevision());
    PRINTLN("Number of cores=", ESP.getChipCores(), ", SDK version=", ESP.getSdkVersion());
    PRINTLN("ArduProf version: ", ARDUPROF_VER);
    PRINTLN("===============================================================================");
}

void setup(void)
{
    Serial.begin(115200);
    while (!Serial)
    {
        delay(100);
    }
    Serial.println("initialized Serial");

    /////////////////////////////////////////////////////////////////////////////
    // Serial is used for DebugLog
    /////////////////////////////////////////////////////////////////////////////
    // set log output to serial port, and init log params such as log_level
    LOG_SET_LEVEL(DefaultLogLevel);
    LOG_SET_DELIMITER("");
    LOG_ATTACH_SERIAL(Serial); // debug log on TX0 (CH340)
    // LOG_TRACE("initialized Serial/UART0 for debug log");
    printChipInfo();

    // WiFi.mode() requires large stack, therefore, init it in .ino setup()
    WiFi.mode(WIFI_STA);

    initGlobalVar();
    createTasks();

    // LOG_TRACE("setup done");

    // Now the task scheduler, which takes over control of scheduling individual tasks, is automatically started.
}

void loop(void)
{
    // static_cast<freertos::QueueMain *>(appContext.queueMain)->messageLoop(1000);
    static_cast<freertos::QueueMain *>(appContext.queueMain)->messageLoopForever(); // never return
}