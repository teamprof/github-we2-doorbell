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

///////////////////////////////////////////////////////////////////////////////
// ESP32 (ESP32-WROOM-32E-N4)
#if CONFIG_IDF_TARGET_ESP32
#error "NOT support ESP32"

///////////////////////////////////////////////////////////////////////////////
// ESP32S3 (ESP32S3-WROOM-N4R2)
#elif CONFIG_IDF_TARGET_ESP32S3
#error "NOT support ESP32S3"

///////////////////////////////////////////////////////////////////////////////
// ESP32C3 (ESP32-C3-WROOM-02-N4)
#elif CONFIG_IDF_TARGET_ESP32C3
#define PIN_NPU_INT GPIO_NUM_3 // npuINT
#define PIN_PIR_INT GPIO_NUM_4 // pirINT
#define PIN_NPU_RST GPIO_NUM_5 // npuRST
#define PIN_BOOT GPIO_NUM_9    // swBoot (strapping, pull-up)

#endif
