#include <Arduino.h>
#include <ArduinoOTA.h>
#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <WiFiClient.h>
#include <WS2812FX.h>

#include "secrets.h"

char _id[12];
char _hostname[18] = HOSTNAME_PREFIX;

WS2812FX ws2812fx = WS2812FX(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);
ESP8266WebServer server(80);
WiFiClient client;

IPAddress staticIP(192, 168, 1, 110);
IPAddress gateway(192, 168, 1, 254); 
IPAddress subnet(255, 255, 255, 0);
IPAddress dns(8, 8, 8, 8);

bool is_on = false;
int red = 255;
int green = 0;
int blue = 0;
String hexColor = "#FF0000";

void start()
{
  Serial.println("start");
  ws2812fx.setBrightness(computeBrightness());
  ws2812fx.show();
  is_on = true;
  server.send(200, "text/plain", "Start");
}

void stop()
{
  Serial.println("stop");
  ws2812fx.setBrightness(0);
  ws2812fx.show();
  is_on = false;
  server.send(200, "text/plain", "Stop");
}

void setColor()
{
  hexColor = server.arg("color");
  long number = (long)strtol(&hexColor[0], NULL, 16);
  red = number >> 16;
  green = number >> 8 & 0xFF;
  blue = number & 0xFF;
  ws2812fx.setColor(red, green, blue);
  server.send(200, "text/plain", "SetColor");
}

void getStatus()
{
  server.send(200, "text/plain", is_on ? "1" : "0");
}

void getColor()
{
  server.send(200, "text/plain", hexColor);
}

void getBrightness()
{
  server.send(200, "text/plain", String(computeBrightness()));
}

void handleNotFound()
{
  server.send(404, "text/plain", "Not found");
}

float computeBrightness()
{
  float r = roundf(red / 2.55);
  float g = roundf(green / 2.55);
  float b = roundf(blue / 2.55);
  float brightness = _max(r, g);
  brightness = _max(brightness, b);

  return brightness;
}

void setupOTA()
{
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

void setupWifi()
{
  WiFi.hostname(_hostname);
  WiFi.config(staticIP, subnet, gateway, dns);
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

void setup_server()
{
  server.on("/start", start);
  server.on("/stop", stop);
  server.on("/set-color", setColor);
  server.on("/status", getStatus);
  server.on("/color", getColor);
  server.on("/bright", getBrightness);
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
  setup_server();
  server.begin();

  Serial.println("Server started");
}

void loop()
{
  ArduinoOTA.handle();
  ws2812fx.service();
  server.handleClient();
}

float max3(float a, float b, float c)
{
  float m = _max(a, b);
  m = _max(m, c);
  return m;
}
