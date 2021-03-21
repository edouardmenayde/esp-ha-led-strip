#include <Arduino.h>
#include <ArduinoOTA.h>
#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <WS2812FX.h>

#include "secrets.h"

char _id[12];
char _hostname[18] = HOSTNAME_PREFIX;

WS2812FX ws2812fx = WS2812FX(LED_COUNT, LED_PIN, NEO_RGB + NEO_KHZ800);
WiFiServer server(80);
WiFiClient client;
bool is_on = false;
int red, blue, green = 0;
String hexColor;

void start() {
  ws2812fx.start();
  is_on = true;
}

void stop() {
  ws2812fx.stop();
  is_on = false;
}

void setColor(String str) {
  String hexString = str.substring(9, 15);
  long number = (long)strtol(&hexString[0], NULL, 16);
  red = number >> 16;
  green = number >> 8 & 0xFF;
  blue = number & 0xFF;
  ws2812fx.setColor(red, blue, green);
}

float computeBrightness() {
  float r = roundf(red / 2.55);
  float g = roundf(green / 2.55);
  float b = roundf(blue / 2.55);
  float brightness = _max(r, g);
  brightness = _max(brightness, b);

  return brightness;
}

void setupOTA() {
    ArduinoOTA.onStart([]() {
        ws2812fx.stop();
        String type;
        if (ArduinoOTA.getCommand() == U_FLASH)
            type = "sketch";
        else // U_SPIFFS
            type = "filesystem";
        Serial.println("Start updating " + type);
    });
    ArduinoOTA.onEnd([]() {
        Serial.println("\nEnd");
    });
    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
        Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
    });
    ArduinoOTA.onError([](ota_error_t error) {
        Serial.printf("Error[%u]: ", error);
        if (error == OTA_AUTH_ERROR)
            Serial.println("Auth Failed");
        else if (error == OTA_BEGIN_ERROR)
            Serial.println("Begin Failed");
        else if (error == OTA_CONNECT_ERROR)
            Serial.println("Connect Failed");
        else if (error == OTA_RECEIVE_ERROR)
            Serial.println("Receive Failed");
        else if (error == OTA_END_ERROR)
            Serial.println("End Failed");
    });
    ArduinoOTA.begin();
    Serial.println("OTA: ready");
}

void setupWifi() {
    WiFi.hostname(_hostname);
    WiFi.begin(WIFI_SSID, WIFI_PW);
    Serial.printf("Connecting to %s (%s) ", WIFI_SSID, _id);
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(100);
        Serial.print(".");
    }
    Serial.print(" OK");
    Serial.println();
    Serial.print("IP address: ");
    Serial.print(WiFi.localIP());
    Serial.printf(" (%s)", _hostname);
    Serial.println();
}

void setupFX()
{
    ws2812fx.init();
    ws2812fx.setBrightness(100);
    ws2812fx.setSpeed(1024);
    ws2812fx.setColor(red, green, blue);
    ws2812fx.setMode(FX_MODE_STATIC);
    ws2812fx.start();
    Serial.println("FX: ready");
}

void server_loop() {
  WiFiClient client = server.available();

  if (!client) {
    return;
  }

  while (client.connected() && !client.available()) {
    delay(1);
  }

  String request = "";

  if (client) {
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();

        if (request.length() < 100) {
          request += c;
        }

        if (c == '\n') {
          Serial.print("Request: ");
          Serial.println(request);

          client.println("HTTP/1.1 200 OK");
          client.println("Content-Type: text/html");
          client.println();

          if (request.indexOf("on") > 0) {
            start();
          }

          if (request.indexOf("off") > 0) {
            stop();
          }

          if (request.indexOf("set") > 0) {
            setColor(request);
          }

          if (request.indexOf("status") > 0) {
            client.println(is_on);
          }

          if (request.indexOf("color") > 0) {
            client.println(hexColor);
          }

          if (request.indexOf("bright") > 0) {
            client.println(computeBrightness());
          }

          delay(1);
          client.stop();
        }
      }
    }
  }
}

void setup()
{
    Serial.begin(115200);
    Serial.println();

    String mac = WiFi.macAddress();
    mac.replace(":", "");
    mac.toLowerCase();
    strncpy(_id, mac.c_str(), sizeof(_id));
    strncat(_hostname, _id, sizeof(_id));

    setupWifi();
    setupOTA();
    setupFX();

    Serial.println();
}

void loop()
{
    ArduinoOTA.handle();
    ws2812fx.service();
    server_loop();
}

float max3(float a, float b, float c) {
  float m = _max(a, b);
  m = _max(m, c);
  return m;
}
