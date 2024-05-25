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
#include <WiFi.h>
#include <UrlEncode.h>
#include "./ThreadMessaging.h"
#include "../AppContext.h"
#include "../../../secret.h"

#define CONNECT_TIMEOUT 30 // in unit of seconds

#define TCP_RX_BUFFER_SIZE 1024

namespace freertos
{
    ////////////////////////////////////////////////////////////////////////////////////////////
    // for release
    static const char apiHost[] = CALLMEBOT_HOST;
    static const char apiPath[] = CALLMEBOT_PATH;
    static const int apiPort = CALLMEBOT_PORT;

    // for test only
    // static const char apiHost[] = "www.google.com";
    // static const char apiPath[] = "/search?q=";
    // static const int apiPort = CALLMEBOT_PORT;

    ////////////////////////////////////////////////////////////////////////////////////////////
    ThreadMessaging *ThreadMessaging::_instance = nullptr;

    uint8_t ThreadMessaging::_shareRxBuf[TCP_RX_BUFFER_SIZE];

////////////////////////////////////////////////////////////////////////////////////////////
// Thread
////////////////////////////////////////////////////////////////////////////////////////////
// #define RUNNING_CORE 0 // dedicate core 0 for Thread
// #define RUNNING_CORE 1 // dedicate core 1 for Thread
#define RUNNING_CORE ARDUINO_RUNNING_CORE

#define TASK_NAME "ThreadMessaging"
#define TASK_STACK_SIZE 2048
#define TASK_PRIORITY 3
#define TASK_QUEUE_SIZE 128 // message queue size for app task

    static uint8_t ucQueueStorageArea[TASK_QUEUE_SIZE * sizeof(Message)];
    static StaticQueue_t xStaticQueue;

    static StackType_t xStack[TASK_STACK_SIZE];
    static StaticTask_t xTaskBuffer;
    ////////////////////////////////////////////////////////////////////////////////////////////

    ThreadMessaging::ThreadMessaging() : ThreadBase(TASK_QUEUE_SIZE, ucQueueStorageArea, &xStaticQueue),
                                         _wifi(this),
                                         _isInternetReady(false),
                                         _tcpClient(),
                                         _isHttpStatusLineReceived(false),
                                         _clientState(Ready),
                                         _timer1Hz("Timer 1Hz",
                                                   pdMS_TO_TICKS(1000),
                                                   [](TimerHandle_t xTimer)
                                                   {
                                                       if (_instance)
                                                       {
                                                           auto context = reinterpret_cast<AppContext *>(_instance->context());
                                                           if (context && context->threadMessaging)
                                                           {
                                                               static_cast<freertos::ThreadMessaging *>(context->threadMessaging)->postEvent(EventSystem, SysSoftwareTimer, 0, (uint32_t)xTimer);
                                                           }
                                                       }
                                                   }),
                                         handlerMap()
    {
        _instance = this;

        handlerMap = {
            __EVENT_MAP(ThreadMessaging, EventSystem),
            __EVENT_MAP(ThreadMessaging, EventSendMessage),
            __EVENT_MAP(ThreadMessaging, EventWifiStatus),
            __EVENT_MAP(ThreadMessaging, EventNull), // {EventNull, &ThreadMessaging::handlerEventNull},
        };
    }

    __EVENT_FUNC_DEFINITION(ThreadMessaging, EventSendMessage, msg) // void ThreadMessaging::handlerEventSendMessage(const Message &msg)
    {
        LOG_DEBUG("EventSendMessage(", msg.event, "), iParam = ", msg.iParam, ", uParam = ", msg.uParam, ", lParam = ", msg.lParam);

        auto appCtx = static_cast<AppContext *>(context());
        if (!appCtx || !appCtx->queueMain)
        {
            return;
        }

        if (_isInternetReady)
        {
            postEvent(appCtx->queueMain, EventMessageStatus, MessageStatus::Sending);
            sendWhatsapp(msg.iParam == IpcNpuTenderDetected ? "doorbell: tenant" : "doorbell: alert - stranger!");
        }
        else
        {
            postEvent(appCtx->queueMain, EventMessageStatus, MessageStatus::SentFail);
        }
    }

    __EVENT_FUNC_DEFINITION(ThreadMessaging, EventWifiStatus, msg) // void ThreadMessaging::handlerEventWifiStatus(const Message &msg)
    {
        // LOG_INFO("EventWifiStatus(", msg.event, "), iParam = ", msg.iParam, ", uParam = ", msg.uParam, ", lParam = ", msg.lParam);

        // const char *str = WifiBase::getEventString((WiFiEvent_t)(msg.iParam));
        // LOG_INFO(str);

        WiFiEvent_t event = (WiFiEvent_t)(msg.iParam);
        switch (event)
        {
        case ARDUINO_EVENT_WIFI_READY:
            LOG_INFO("WiFi interface ready");
            break;
        case ARDUINO_EVENT_WIFI_SCAN_DONE:
            LOG_INFO("Completed scan for access points");
            break;
        case ARDUINO_EVENT_WIFI_STA_START:
            LOG_INFO("WiFi client started");
            break;
        case ARDUINO_EVENT_WIFI_STA_STOP:
            LOG_INFO("WiFi clients stopped");
            break;
        case ARDUINO_EVENT_WIFI_STA_CONNECTED:
            LOG_INFO("Connected to WiFi point");
            LOG_INFO("RSSI=", WiFi.RSSI(), " dBm");
            break;
        case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
            LOG_INFO("Disconnected from WiFi access point");
            break;
        case ARDUINO_EVENT_WIFI_STA_AUTHMODE_CHANGE:
            LOG_INFO("Authentication mode of access point has changed");
            break;
        case ARDUINO_EVENT_WIFI_STA_GOT_IP:
        {
            LOG_INFO("Obtained IP: ", WiFi.localIP(), ", gateway IP: ", WiFi.gatewayIP(), ", dnsIP: ", WiFi.dnsIP(), ", subnetMask: ", WiFi.subnetMask());
            _isInternetReady = true;
            auto appCtx = static_cast<AppContext *>(context());
            if (appCtx && appCtx->queueMain)
            {
                appCtx->queueMain->postEvent(EventInternetStatus, InternetStatus::Connect);
            }
            break;
        }
        case ARDUINO_EVENT_WIFI_STA_LOST_IP:
        {
            LOG_INFO("Lost IP address and IP address is reset to 0");
            _isInternetReady = false;
            auto appCtx = static_cast<AppContext *>(context());
            if (appCtx && appCtx->queueMain)
            {
                appCtx->queueMain->postEvent(EventInternetStatus, InternetStatus::Disconnect);
            }
            break;
        }
        case ARDUINO_EVENT_WPS_ER_SUCCESS:
            LOG_INFO("WiFi Protected Setup (WPS): succeeded in enrollee mode");
            break;
        case ARDUINO_EVENT_WPS_ER_FAILED:
            LOG_INFO("WiFi Protected Setup (WPS): failed in enrollee mode");
            break;
        case ARDUINO_EVENT_WPS_ER_TIMEOUT:
            LOG_INFO("WiFi Protected Setup (WPS): timeout in enrollee mode");
            break;
        case ARDUINO_EVENT_WPS_ER_PIN:
            LOG_INFO("WiFi Protected Setup (WPS): pin code in enrollee mode");
            break;
        case ARDUINO_EVENT_WIFI_AP_START:
            LOG_INFO("WiFi access point started");
            break;
        case ARDUINO_EVENT_WIFI_AP_STOP:
            LOG_INFO("WiFi access point stopped");
            break;
        case ARDUINO_EVENT_WIFI_AP_STACONNECTED:
            LOG_INFO("Client connected");
            break;
        case ARDUINO_EVENT_WIFI_AP_STADISCONNECTED:
            LOG_INFO("Client disconnected");
            break;
        case ARDUINO_EVENT_WIFI_AP_STAIPASSIGNED:
            LOG_INFO("Assigned IP address to client");
            break;
        case ARDUINO_EVENT_WIFI_AP_PROBEREQRECVED:
            LOG_INFO("Received probe request");
            break;
        case ARDUINO_EVENT_WIFI_AP_GOT_IP6:
            LOG_INFO("AP IPv6 is preferred");
            break;
        case ARDUINO_EVENT_WIFI_STA_GOT_IP6:
            LOG_INFO("STA IPv6 is preferred");
            break;

        default:
            LOG_INFO("unknown event: ", event);
            break;
        }
    }
    __EVENT_FUNC_DEFINITION(ThreadMessaging, EventSystem, msg) // void ThreadMessaging::handlerEventSystem(const Message &msg)
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
    __EVENT_FUNC_DEFINITION(ThreadMessaging, EventNull, msg) // void ThreadMessaging::handlerEventNull(const Message &msg)
    {
        LOG_DEBUG("EventNull(", msg.event, "), iParam = ", msg.iParam, ", uParam = ", msg.uParam, ", lParam = ", msg.lParam);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////
    void ThreadMessaging::onMessage(const Message &msg)
    {
        // LOG_DEBUG("event=", msg.event, ", iParam=", msg.iParam, ", uParam=", msg.uParam, ", lParam=", msg.lParam);
        auto func = handlerMap[msg.event];
        if (func)
        {
            (this->*func)(msg);
        }
        else
        {
            LOG_DEBUG("Unsupported event = ", msg.event, ", iParam = ", msg.iParam, ", uParam = ", msg.uParam, ", lParam = ", msg.lParam);
        }
    }

    void ThreadMessaging::start(void *ctx)
    {
        // LOG_TRACE("start() on core ", xPortGetCoreID(), ", xPortGetFreeHeapSize()=", xPortGetFreeHeapSize());
        ThreadBase::start(ctx);

        _taskHandle = xTaskCreateStaticPinnedToCore(
            [](void *instance)
            { static_cast<ThreadMessaging *>(instance)->run(); },
            TASK_NAME,
            TASK_STACK_SIZE, // This stack size can be checked & adjusted by reading the Stack Highwater
            this,
            TASK_PRIORITY, // Priority, with 3 (configMAX_PRIORITIES - 1) being the highest, and 0 being the lowest.
            xStack,
            &xTaskBuffer,
            RUNNING_CORE);
    }

    void ThreadMessaging::setup(void)
    {
        // LOG_TRACE("setup() on core ", xPortGetCoreID(), ", xPortGetFreeHeapSize()=", xPortGetFreeHeapSize());
        ThreadBase::setup();

        // WiFi.mode(WIFI_STA); // WiFi.mode() requires large stack, therefore, init it in .ino setup()
        WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

        // vTaskDelay(pdMS_TO_TICKS(2000));
    }

    void ThreadMessaging::run(void)
    {
        // LOG_TRACE("run() on core ", xPortGetCoreID(), ", xPortGetFreeHeapSize()=", xPortGetFreeHeapSize());
        ThreadBase::run();
    }

    void ThreadMessaging::delayInit(void)
    {
        // LOG_TRACE("delayInit() on core ", xPortGetCoreID(), ", xPortGetFreeHeapSize()=", xPortGetFreeHeapSize());

        //////////////////////////////////////////////////////////////
        // add time consuming init code here
        //////////////////////////////////////////////////////////////
    }

    void ThreadMessaging::sendWhatsapp(const char *s)
    {
        auto appCtx = static_cast<AppContext *>(context());
        if (appCtx && appCtx->queueMain)
        {
            prepareText(s);

            if (_tcpClient.connect(apiHost, apiPort))
            {
                LOG_TRACE("connecting to server=", apiHost, ", port=", apiPort);
                _clientState = Connecting;
                _connectTimeout = 0;
                _isHttpStatusLineReceived = false;
                _timer1Hz.start();
            }
            else
            {
                LOG_TRACE("Fail to connect server=", apiHost, ", port=", apiPort);
            }
        }
    }

    void ThreadMessaging::responseSendMessage(MessageStatus status)
    {
        _timer1Hz.stop();
        _clientState = Ready;

        auto appCtx = static_cast<AppContext *>(context());
        configASSERT(appCtx && appCtx->queueMain);
        appCtx->queueMain->postEvent(EventMessageStatus, status);
    }

    bool ThreadMessaging::readHttpResponse(int *ptrResponseCode)
    {
        // static const int TCP_RX_BUFFER_SIZE = 1024;
        // static uint8_t _shareRxBuf[TCP_RX_BUFFER_SIZE];

        int tcpSize = _tcpClient.available();
        while (tcpSize > 0)
        {
            // LOG_DEBUG("tcpSize=", tcpSize);
            if (tcpSize >= sizeof(_shareRxBuf))
            {
                tcpSize = sizeof(_shareRxBuf) - 1; // truncate data if oversize
            }
            _tcpClient.read(_shareRxBuf, tcpSize);
            _shareRxBuf[tcpSize] = '\0';
            LOG_DEBUG("Received ", tcpSize, " bytes from ", _tcpClient.remoteIP(), ":", _tcpClient.remotePort());
            // LOG_DEBUG("Content: ", (const char *)_shareRxBuf);

            if (!_isHttpStatusLineReceived)
            {
                char *strResponseCode = strstr((char *)_shareRxBuf, "HTTP/");
                if (strResponseCode)
                {
                    _isHttpStatusLineReceived = true;

                    int responseCode;
                    sscanf(strResponseCode, "HTTP/%*d.%*d %d", &responseCode);
                    LOG_DEBUG("HTTP Status Code: ", responseCode);

                    *ptrResponseCode = responseCode;
                    return true;
                }
            }

            tcpSize = _tcpClient.available();
        }
        // LOG_DEBUG("tcpSize=", tcpSize);
        return false;
    }

    void ThreadMessaging::writeText(const char *s)
    {
        LOG_DEBUG("_messageText=", _messageText);

        // Make a HTTP request:
        _tcpClient.print("GET ");
        _tcpClient.print(_messageText);
        _tcpClient.println(" HTTP/1.1");
        _tcpClient.print("Host: ");
        _tcpClient.println(apiHost);
        _tcpClient.println("Connection: close");
        _tcpClient.println();
    }

    void ThreadMessaging::prepareText(const char *s)
    {
        // make text data to be sent
        String encoded = urlEncode(s);
        snprintf(_messageText, sizeof(_messageText), "%s%s", apiPath, encoded.c_str());
        LOG_TRACE("_messageText=", _messageText);
    }

    void ThreadMessaging::handlerSoftwareTimer(TimerHandle_t xTimer)
    {
        if (xTimer == _timer1Hz.timer())
        {
            // LOG_TRACE("_timer1Hz");
            switch (_clientState)
            {
            case Ready:
                break;

            case Connecting:
            {
                if (_tcpClient.connected())
                {
                    LOG_TRACE("Connected server: IP=", _tcpClient.remoteIP(), ", port=", _tcpClient.remotePort());
                    writeText(_messageText);
                    _clientState = Connected;
                    _connectTimeout = 0;
                }
                else if (_connectTimeout++ >= CONNECT_TIMEOUT)
                {
                    LOG_TRACE("Connecting: timeout! _connectTimeout=", _connectTimeout, " seconds");
                    responseSendMessage(MessageStatus::SentFail);
                    _tcpClient.stop();
                }
                else
                {
                    LOG_TRACE("Connecting: time=", _connectTimeout, " seconds");
                }
            }
            break;

            case Connected:
            {
                int responseCode;
                if (readHttpResponse(&responseCode))
                {
                    if (responseCode == 200)
                    {
                        responseSendMessage(MessageStatus::SentSuccess);
                    }
                    else
                    {
                        LOG_TRACE("Connected: HTTP server response statusCode=", responseCode);
                        responseSendMessage(MessageStatus::SentFail);
                    }
                    _tcpClient.stop();
                }
                else if (_connectTimeout++ >= CONNECT_TIMEOUT)
                {
                    LOG_TRACE("Connected: timeout! _connectTimeout=", _connectTimeout, " seconds");
                    responseSendMessage(MessageStatus::SentFail);
                    _tcpClient.stop();
                }
                else
                {
                    LOG_TRACE("Connected: time=", _connectTimeout, " seconds");
                }
                break;
            }

            default:
                LOG_TRACE("unsupported _clientState=%d", (uint32_t)(_clientState));
                break;
            }
        }
        else
        {
            LOG_TRACE("unsupported timer handle=0x%04x", (uint32_t)(xTimer));
        }
    }

} // namespace freertos