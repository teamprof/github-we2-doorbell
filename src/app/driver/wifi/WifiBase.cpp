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
#include "../../ArduProfFreeRTOS.h"
#include "../../AppEvent.h"
#include "WifiBase.h"

#define MaxThreadNum 8

WifiBase *WifiBase::instance = nullptr;

void WiFiStationConnected(arduino_event_id_t event, arduino_event_info_t info)
{
    // LOG_DEBUG("Connected to AP successfully!");
}

WifiBase::WifiBase(ardufreertos::ThreadBase *thread) : thread(thread)
{
    if (instance == nullptr)
    {
        instance = this;

        // WiFi.mode(WIFI_STA);

        WiFi.onEvent(onEvent);
        // WiFi.onEvent([](WiFiEvent_t event)
        //              {
        //                 if (instance != nullptr)
        //                 {
        //                     instance->onEvent(event);
        //                 } });

        WiFi.onEvent(WiFiStationConnected, ARDUINO_EVENT_WIFI_STA_CONNECTED);

        // WiFi.onEvent(WiFiGotIP, SYSTEM_EVENT_STA_GOT_IP);
        // WiFi.onEvent(WiFiStationDisconnected, SYSTEM_EVENT_STA_DISCONNECTED);
        // https://randomnerdtutorials.com/esp32-useful-wi-fi-functions-arduino/
    }
}

// WifiBase *WifiBase::createInstance(ThreadBase *thread)
// {
//     if (instance == nullptr)
//     {
//         instance = new WifiBase(thread);
//     }
//     return instance;
// }

bool WifiBase::connect(const char *ssid, const char *password)
{
    WiFi.disconnect(true);
    WiFi.begin(ssid, password);
    return true;
}

void WifiBase::onEvent(WiFiEvent_t event)
{
    if (instance == nullptr || instance->thread == nullptr)
    {
        LOG_DEBUG(getEventString(event));
        return;
    }
    instance->thread->postEvent(EventWifiStatus, event);
}

const char *WifiBase::getEventString(WiFiEvent_t event)
{
    static char buf[32];

    switch (event)
    {
    case ARDUINO_EVENT_WIFI_READY:
        return ("WiFi interface ready");
        break;
    case ARDUINO_EVENT_WIFI_SCAN_DONE:
        return ("Completed scan for access points");
        break;
    case ARDUINO_EVENT_WIFI_STA_START:
        return ("WiFi client started");
        break;
    case ARDUINO_EVENT_WIFI_STA_STOP:
        return ("WiFi clients stopped");
        break;
    case ARDUINO_EVENT_WIFI_STA_CONNECTED:
        return ("Connected to WiFi point");
        break;
    case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
        return ("Disconnected from WiFi access point");
        break;
    case ARDUINO_EVENT_WIFI_STA_AUTHMODE_CHANGE:
        return ("Authentication mode of access point has changed");
        break;
    case ARDUINO_EVENT_WIFI_STA_GOT_IP:
        return ("Obtained IP");
        break;
    case ARDUINO_EVENT_WIFI_STA_LOST_IP:
        return ("Lost IP address and IP address is reset to 0");
        break;
    case ARDUINO_EVENT_WPS_ER_SUCCESS:
        return ("WiFi Protected Setup (WPS): succeeded in enrollee mode");
        break;
    case ARDUINO_EVENT_WPS_ER_FAILED:
        return ("WiFi Protected Setup (WPS): failed in enrollee mode");
        break;
    case ARDUINO_EVENT_WPS_ER_TIMEOUT:
        return ("WiFi Protected Setup (WPS): timeout in enrollee mode");
        break;
    case ARDUINO_EVENT_WPS_ER_PIN:
        return ("WiFi Protected Setup (WPS): pin code in enrollee mode");
        break;
    case ARDUINO_EVENT_WIFI_AP_START:
        return ("WiFi access point started");
        break;
    case ARDUINO_EVENT_WIFI_AP_STOP:
        return ("WiFi access point stopped");
        break;
    case ARDUINO_EVENT_WIFI_AP_STACONNECTED:
        return ("Client connected");
        break;
    case ARDUINO_EVENT_WIFI_AP_STADISCONNECTED:
        return ("Client disconnected");
        break;
    case ARDUINO_EVENT_WIFI_AP_STAIPASSIGNED:
        return ("Assigned IP address to client");
        break;
    case ARDUINO_EVENT_WIFI_AP_PROBEREQRECVED:
        return ("Received probe request");
        break;
    case ARDUINO_EVENT_WIFI_AP_GOT_IP6:
        return ("AP IPv6 is preferred");
        break;
    case ARDUINO_EVENT_WIFI_STA_GOT_IP6:
        return ("STA IPv6 is preferred");
        break;
    case ARDUINO_EVENT_ETH_GOT_IP6:
        return ("Ethernet IPv6 is preferred");
        break;
    case ARDUINO_EVENT_ETH_START:
        return ("Ethernet started");
        break;
    case ARDUINO_EVENT_ETH_STOP:
        return ("Ethernet stopped");
        break;
    case ARDUINO_EVENT_ETH_CONNECTED:
        return ("Ethernet connected");
        break;
    case ARDUINO_EVENT_ETH_DISCONNECTED:
        return ("Ethernet disconnected");
        break;
    case ARDUINO_EVENT_ETH_GOT_IP:
        return ("Obtained IP address");
        break;
    default:
        sprintf(buf, "unknown event: %d", event);
        return buf;
        break;
    }
}
