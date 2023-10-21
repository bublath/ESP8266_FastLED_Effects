# ESP8266_FastLED_Effects
FastLED Effect Webserver on ESP8266 with multiple effects and configuration options

- Provide a simple webserver to switch between effects and change parameters like effect speed, fade speed, colors, direction and more
- Most effects are modification from existing demos, but improved especially for long LED Strips (tested on 300 LED strip with 5m)
- Serial interface to define WiFi SSID/PW
- WiFi AP in addition to serial to enter WiFi credentials (IP: 192.168.33.1)
- Save function to remember WiFi and Effect Settings
- Includes a DHT11 temperature/humidity sensor, but that can be excluded with #undef ENABLE_DHT11

Installation:
- Install the Arduino IDE and add the ESP8266 Board Manage (File->Preferences, under "Additional boards manager URLs" add https://arduino.esp8266.com/stable/package_esp8266com_index.json
- To use the DHT11 Sensor part, add the library "DHT sensor library for ESPx by beegee_tokyo" (tested with 1.19)
- For the LED add "FastLED by Danial Garcia" (tested with 3.6.0)

Have fun

Current web frontend:

![image](https://github.com/bublath/ESP8266_FastLED_Effects/assets/74186638/e157fc14-d29f-44ac-ab5a-73ba68a9183a)

How to connect on a breadboard:

![ESP-LED-Breadboard](https://github.com/bublath/ESP8266_FastLED_Effects/assets/74186638/d7c4b43d-d24e-4b5f-a0cd-3a004ecbfa49)

Additionally you can place a button between D4 and GND that can be used to toggle through the effects without using the webinterface.
