# ESP8266_FastLED_Effects
FastLED Effect Webserver on ESP8266 with multiple effects and configuration options

- Provide a simply webserver to switch between effects and change parameters like effect speed, fade speed, colors, direction and more
- Most effects are modification from existing demos, but improved especially for long LED Strips (tested on 300 LED strip with 5m)
- Serial interface to define WiFi SSID/PW
- WiFi AP in addition to serial to enter WiFi credentials (IP: 192.168.33.1)
- Save function to remember WiFi and Effect Settings
- Includes a DHT11 temperature/humidity sensor, but that can be excluded with #undef ENABLE_DHT11

Have fun

Current web frontend:

![image](https://github.com/bublath/ESP8266_FastLED_Effects/assets/74186638/fa47a364-715c-48be-a678-ba3bef8a3cf5)

How to connect on a breadboard:

![ESP-LED-Breadboard](https://github.com/bublath/ESP8266_FastLED_Effects/assets/74186638/d7c4b43d-d24e-4b5f-a0cd-3a004ecbfa49)

Additionally you can place a button between D4 and GND that can be used to toggle through the effects without using the webinterface.
