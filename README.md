## ESP8266 LED-Strip Firmware for Home-Assistant 

It is a fork of [https://github.com/schmic/esp-ha-led-strip](https://github.com/schmic/esp-ha-led-strip) updated and modified to suit my needs.

Attention, don't break the single-double-quotes for the defines in the platformio.ini!

### What you need
- ESP8266 (nodemcuv2 or similar)
- WS2812b based LED stripe
    - sufficient power supply, every led draws up to 60mA

### Home-Assistant Integration
Home-Assistants [mqtt_light-component](https://www.home-assistant.io/components/light.mqtt/) is used to control everything. Check the example-directory.

The MQTT-topic and hostname is based on the mac of the Wemos.

![Screeshot](https://github.com/schmic/esp-ha-led-strip/raw/master/example/ha.png)
