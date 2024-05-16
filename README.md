## Introduction
Home security is a big market while existing doorbell cameras have drawbacks such as large power consumption due to the always-on camera. 
We are going to create a low-power doorbell camera equipped with edge AI for detecting potential break-ins by thieves.
A XIAO ESP32C3 serves as the main controller and remains in a low-power sleep mode. When a human presence is detected by the PIR sensor, it wakes up the ESP32C3. Subsequently, the ESP32C3 activates the Grove Vision AI Module v2 for person classification. If the identified person is not the tenant, the ESP32C3 sends a WhatsApp message to the tenant.

[![cover-image](/doc/image/cover-image.png)](https://github.com/teamprof/github-we2-doorbell/blob/main/doc/image/cover-image.png)


[![License: GPL v3](https://img.shields.io/badge/License-GPL_v3-blue.svg)](https://github.com/teamprof/github-we2-doorbell/blob/main/LICENSE)

<a href="https://www.buymeacoffee.com/teamprof" target="_blank"><img src="https://cdn.buymeacoffee.com/buttons/v2/default-yellow.png" alt="Buy Me A Coffee" style="height: 28px !important;width: 108px !important;" ></a>

---
## Hardware
The following components are required for this project:
1. [Grove Vision AI Module V2](https://wiki.seeedstudio.com/grove_vision_ai_v2/)
2. [OV5647 camera module](https://www.seeedstudio.com/OV5647-69-1-FOV-Camera-module-for-Raspberry-Pi-3B-4B-p-5484.html)
3. [Seeed Studio XIAO ESP32C3](https://www.seeedstudio.com/Seeed-XIAO-ESP32C3-p-5431.html)
4. [PIR LED lamp](https://www.aliexpress.com/item/1005005973394439.html)
5. A mobile phone with Whatsapp installed

---
## Software 
1. Install [Arduino IDE 2.0+ for Arduino](https://www.arduino.cc/en/Main/Software)
2. Install [ArduProf lib 1.2.0+](https://www.arduino.cc/reference/en/libraries/arduprof/)
3. Install [Arduino DebugLog lib](https://www.arduino.cc/reference/en/libraries/debuglog/)
4. Install [Arduino UrlEncode lib](https://www.arduino.cc/reference/en/libraries/urlencode/)
5. Clone this github-we2-doorbell code by "git clone https://github.com/teamprof/github-we2-doorbell"


---
## Hardware
### Wiring between XIAO ESP32C3 and Grove Vision AI Module V2 (WE2) 
Following the pin assignment below (or [schematic](https://github.com/teamprof/github-we2-doorbell/blob/main/doc/we2-esp32c3-sch-v1.0.pdf)) for wiring between XIAO ESP32C3 and Grove Vision AI Module V2
```
  +----------------------+        +---------------------+     
  | XIAO ESP32C3         |        | WE2 XIAO Connector  |     
  +-------+--------------+        +-------+-------------+     
  | pin   | description  |        | pin   | description |     
  +-------+--------------+        +-------+-------------+     
  | GPIO5 | io reset we2 |        | RST   | RST         |     
  | GPIO6 | I2C SDA      |        | PA3   | I2CS0 SDA   |     
  | GPIO7 | I2C SCL      |        | PA2   | I2CS0 SCL   |     
  | 5V    | 5V           |        | VMAIN | VMAIN       |     
  | GND   | GND          |        | GND   | GND         |     
  +-------+--------------+        +-------+-------------+     
```
### Wiring between XIAO ESP32C3 and PIR lamp
Connect the PIR detector output by wire wrapping NSA3182FT180's pin5 to XIAO ESP32C3 GPIO4.  
[![PIR lamp photo 1](/doc/image/pir-lamp-01.jpg)](https://github.com/teamprof/github-we2-doorbell/blob/main/doc/images/pir-lamp-01.jpg)

Since NSA3182FT180's pin5 is connected to R9, it is easier to soldering a wire on R9.  
[![PIR lamp photo 2](/doc/image/pir-lamp-02.jpg)](https://github.com/teamprof/github-we2-doorbell/blob/main/doc/images/pir-lamp-02.jpg)

## Hardware photo
[![hardware-front](/doc/image/hw-front.jpg)](https://github.com/teamprof/github-we2-doorbell/blob/main/doc/images/hw-front.jpg)

[![hardware-top](/doc/image/hw-top.jpg)](https://github.com/teamprof/github-we2-doorbell/blob/main/doc/images/hw-top.jpg)

[![hardware-side](/doc/image/hw-side.jpg)](https://github.com/teamprof/github-we2-doorbell/blob/main/doc/images/hw-side.jpg)
---

## Flash firmware on the Grove Vision AI Module v2 (WE2)
To simplify the demonstration, we utilize a pre-trained pet detection model on the Grove Vision AI Module v2. 
Visit https://seeed-studio.github.io/SenseCraft-Web-Toolkit/#/setup/process, connect the ‘Grove Vision AI (V2)’, select the ‘Pet Detection’ model, and click the ‘Send’ button in your browser. This action will flash the Pet Detection model onto the Grove Vision AI Module v2.
[![SenseCraft-pet-detection](/doc/image/SenseCraft-pet-detection.png)](https://github.com/teamprof/github-we2-doorbell/blob/main/doc/images/SenseCraft-pet-detection.png)


## Build and Flash firmware on XIAO ESP32C3
1. Clone this repository by "git clone https://github.com/teamprof/github-we2-doorbell"
2. Follow the instruction on "https://www.callmebot.com/blog/free-api-whatsapp-messages/" to get the APIKEY.
3. Replace the "<MobileNumber>" and "<ApiKey>" values with your mobile number and the APIKEY got on step 2 in "secret.h".
4. Replace the "<YourWifiSsid>" and "<YourWifiPassword>" in "secret.h"
5. Build and upload the firmware in Arduino IDE.


## Run the demo
**DO NOT connect USB cable to both XIAO ESP32C3 and Grove Vision AI Module v2 (WE2) simultaneously. Otherwise, board(s) MAY be permanently damage!**
1. Connect a USB cable between XIAO ESP32C3 and PC
2. Launch a Serial terminal, set baud rate to 115200 and connect it to the USB port of the XIAO ESP32C3 Board  
3. Press the RESET button on XIAO ESP32C3 
4. If everything goes smooth, you should see the followings on the serial terminal:
[![serial terminal screen](/doc/image/serial-terminal.png)](https://github.com/teamprof/github-we2-doorbell/blob/main/doc/image/serial-terminal.png)
5. Put a "cat" in front of the camera/PIR, the PIR lamp turns on, the Grove Vision AI Module v2 starts inference. If everything works smoothly, a whatsapp message with "tenant" will be received on the mobile phone in couples of seconds.
6. Put a "dog" in front of the camera/PIR, the PIR lamp turns on, the Grove Vision AI Module v2 starts inference. If everything works smoothly, a whatsapp message with "stranger" will be received on the mobile phone in couples of seconds.

## Demo




---
### Debug
Enable or disable log be defining/undefining macro on "src/app/AppLog.h"

Debug is disabled by "#undef DEBUG_LOG_LEVEL"
Enable trace debug by "#define DEBUG_LOG_LEVEL Debug"

Example of AppLog.h
```
// enable debug log by defining the following macro
#define DEBUG_LOG_LEVEL Debug
// disable debug log by comment out the macro DEBUG_LOG_LEVEL 
// #undef DEBUG_LOG_LEVEL
```
---
### Troubleshooting
If you get compilation errors, more often than not, you may need to install a newer version of the coralmicro.

Sometimes, the project will only work if you update the board core to the latest version because I am using newly added functions.

---
### Issues
Submit issues to: [github-we2-doorbell issues](https://github.com/teamprof/github-we2-doorbell/issues) 

---
### TO DO
1. Low power sleep on ESP32C3, ESP32C3 wakeup by pirINT pin
2. Low power sleep on Grove Vision AI Module V2, Grove Vision AI Module V2 wakeup by npuWakeup pin 
3. Search for bug and improvement.
---

### Contributions and Thanks
Many thanks to the following authors who have developed great software and libraries.
1. [Seeed Studio Grove Vision AI Module V2](https://wiki.seeedstudio.com/grove_vision_ai_v2/)
2. [Seeed Studio OV5647-62 FOV Camera Module](https://www.seeedstudio.com/OV5647-69-1-FOV-Camera-module-for-Raspberry-Pi-3B-4B-p-5484.html)
3. [Seeed Studio XIAO ESP32C3](https://www.seeedstudio.com/Seeed-XIAO-ESP32C3-p-5431.html)
4. [DebugLog](https://github.com/hideakitai/DebugLog)
5. [UrlEncode](https://github.com/plageoj/urlencode)

#### Many thanks for everyone for bug reporting, new feature suggesting, testing and contributing to the development of this project.
---

### Contributing
If you want to contribute to this project:
- Report bugs and errors
- Ask for enhancements
- Create issues and pull requests
- Tell other people about this library
---

### License
- The project is licensed under GNU GENERAL PUBLIC LICENSE Version 3
---

### Copyright
- Copyright 2024 teamprof.net@gmail.com. All rights reserved.

