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
#include "./EspUtil.h"

const char *getEspReturnString(esp_err_t value)
{
    switch (value)
    {
    case ESP_OK:
        return "ESP_OK";
    case ESP_FAIL:
        return "ESP_FAIL";
    case ESP_ERR_NO_MEM:
        return "ESP_ERR_NO_MEM";
    case ESP_ERR_INVALID_ARG:
        return "ESP_ERR_INVALID_ARG";
    case ESP_ERR_INVALID_STATE:
        return "ESP_ERR_INVALID_STATE";
    case ESP_ERR_INVALID_SIZE:
        return "ESP_ERR_INVALID_SIZE";
    case ESP_ERR_NOT_FOUND:
        return "ESP_ERR_NOT_FOUND";
    case ESP_ERR_NOT_SUPPORTED:
        return "ESP_ERR_NOT_SUPPORTED";
    case ESP_ERR_TIMEOUT:
        return "ESP_ERR_TIMEOUT";
    case ESP_ERR_INVALID_RESPONSE:
        return "ESP_ERR_INVALID_RESPONSE";
    case ESP_ERR_INVALID_CRC:
        return "ESP_ERR_INVALID_CRC";
    case ESP_ERR_INVALID_VERSION:
        return "ESP_ERR_INVALID_VERSION";
    case ESP_ERR_INVALID_MAC:
        return "ESP_ERR_INVALID_MAC";
    case ESP_ERR_NOT_FINISHED:
        return "ESP_ERR_NOT_FINISHED";
    case ESP_ERR_WIFI_BASE:
        return "ESP_ERR_WIFI_BASE";
    case ESP_ERR_MESH_BASE:
        return "ESP_ERR_MESH_BASE";
    case ESP_ERR_FLASH_BASE:
        return "ESP_ERR_FLASH_BASE";
    case ESP_ERR_HW_CRYPTO_BASE:
        return "ESP_ERR_HW_CRYPTO_BASE";
    case ESP_ERR_MEMPROT_BASE:
        return "ESP_ERR_MEMPROT_BASE";
    default:
        return "Unsupported esp_err_t";
    }
}

const char *getEspResetReasonString(void)
{
    esp_reset_reason_t resetReason = esp_reset_reason();
    switch (resetReason)
    {
    case ESP_RST_POWERON:
        return "ESP_RST_POWERON";
    case ESP_RST_EXT:
        return "ESP_RST_EXT";
    case ESP_RST_SW:
        return "ESP_RST_SW";
    case ESP_RST_PANIC:
        return "ESP_RST_PANIC";
    case ESP_RST_INT_WDT:
        return "ESP_RST_INT_WDT";
    case ESP_RST_TASK_WDT:
        return "ESP_RST_TASK_WDT";
    case ESP_RST_WDT:
        return "ESP_RST_WDT";
    case ESP_RST_DEEPSLEEP:
        return "ESP_RST_DEEPSLEEP";
    case ESP_RST_BROWNOUT:
        return "ESP_RST_BROWNOUT";
    case ESP_RST_SDIO:
        return "ESP_RST_SDIO";
        // case ESP_RST_USB:
        //     return "ESP_RST_USB";
        // case ESP_RST_JTAG:
        //     return "ESP_RST_JTAG";

    case ESP_RST_UNKNOWN:
    default:
        return "ESP_RST_UNKNOWN";
    }
}