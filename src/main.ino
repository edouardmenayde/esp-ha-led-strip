#include <Arduino.h>
#include <ArduinoOTA.h>
#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <WS2812FX.h>
#include <EasyButton.h>
#include <arduino_homekit_server.h>

#include "secrets.h"

//access the config defined in C code

extern "C" homekit_server_config_t accessory_config;
extern "C" homekit_characteristic_t cha_on;
extern "C" homekit_characteristic_t cha_bright;
extern "C" homekit_characteristic_t cha_sat;
extern "C" homekit_characteristic_t cha_hue;

char _id[12];
char _hostname[18] = HOSTNAME_PREFIX;

WS2812FX ws2812fx = WS2812FX(LED_COUNT, LED_PIN, NEO_RGB + NEO_KHZ800);
WiFiClient wifi;

EasyButton reset_homekit(BUTTON_PIN);

// Maintained state
bool received_sat = false;
bool received_hue = false;

bool is_on = false;
float current_brightness = 100.0;
float current_sat = 0.0;
float current_hue = 0.0;
int rgb_colors[3] = {0, 0, 0};

void set_on(const homekit_value_t v)
{
  bool on = v.bool_value;
  cha_on.value.bool_value = on; //sync the value

  if (on) {
    is_on = true;
    Serial.println("On");
  }
  else {
    is_on = false;
    Serial.println("Off");
  }

  updateColor();
}

void set_hue(const homekit_value_t v)
{
  Serial.println("set_hue");
  float hue = v.float_value;
  cha_hue.value.float_value = hue; //sync the value

  current_hue = hue;
  received_hue = true;

  updateColor();
}

void set_sat(const homekit_value_t v)
{
  Serial.println("set_sat");
  float sat = v.float_value;
  cha_sat.value.float_value = sat; //sync the value

  current_sat = sat;
  received_sat = true;

  updateColor();
}

void set_bright(const homekit_value_t v)
{
  Serial.println("set_bright");
  int bright = v.int_value;
  cha_bright.value.int_value = bright; //sync the value

  current_brightness = bright;

  updateColor();
}

void updateColor()
{
  if (is_on)
  {
    if (received_hue && received_sat) {
      HSV2RGB(current_hue, current_sat, current_brightness);
      received_hue = false;
      received_sat = false;
    }

    int b = map(current_brightness, 0, 100, 75, 255);
    Serial.println(b);

    ws2812fx.setBrightness(b);
    ws2812fx.setColor(rgb_colors[0], rgb_colors[1], rgb_colors[2]);
    ws2812fx.start();
  }
  else {
    ws2812fx.stop();
  }
}

// void mqttProcessJson(char *message)
// {
//     StaticJsonDocument<_mqtt_buffer_size> jsonDoc;
//     auto error = deserializeJson(jsonDoc, message);

//     if (error)
//     {
//         return;
//     }

//     if (jsonDoc.containsKey("state"))
//     {
//         if (strcmp(jsonDoc["state"], "ON") == 0)
//         {
//             ws2812fx.start();
//         }
//         else if (strcmp(jsonDoc["state"], "OFF") == 0)
//         {
//             ws2812fx.stop();
//         }
//     }

//     if (jsonDoc.containsKey("effect"))
//     {
//         ws2812fx.setMode(effectNumber(jsonDoc["effect"]));
//     }

//     if (jsonDoc.containsKey("color"))
//     {
//         red = jsonDoc["color"]["r"];
//         green = jsonDoc["color"]["g"];
//         blue = jsonDoc["color"]["b"];
//         ws2812fx.setColor(getColorInt());
//     }

//     if (jsonDoc.containsKey("white_value"))
//     {
//         white = jsonDoc["white_value"];
//         ws2812fx.setColor(getColorInt());
//     }

//     if (jsonDoc.containsKey("brightness"))
//     {
//         ws2812fx.setBrightness(jsonDoc["brightness"]);
//     }

//     if (jsonDoc.containsKey("speed"))
//     {
//         ws2812fx.setSpeed(jsonDoc["speed"]);
//     }
// }

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
    ws2812fx.setColor(rgb_colors[0], rgb_colors[1], rgb_colors[2]);
    ws2812fx.setMode(FX_MODE_STATIC);
    ws2812fx.start();
    Serial.println("FX: ready");
}

void homekit_setup()
{
  cha_on.setter = set_on;
  cha_bright.setter = set_bright;
  cha_sat.setter = set_sat;
  cha_hue.setter = set_hue;

  arduino_homekit_setup(&accessory_config);
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
    homekit_setup();

    reset_homekit.onPressed(homekit_storage_reset);

    Serial.println();
}

void loop()
{
    ArduinoOTA.handle();
    ws2812fx.service();
    arduino_homekit_loop();
    reset_homekit.read();
}

void HSV2RGB(float h, float s, float v)
{

  int i;
  float m, n, f;

  s /= 100;
  v /= 100;

  if (s == 0)
  {
    rgb_colors[0] = rgb_colors[1] = rgb_colors[2] = round(v * 255);
    return;
  }

  h /= 60;
  i = floor(h);
  f = h - i;

  if (!(i & 1))
  {
    f = 1 - f;
  }

  m = v * (1 - s);
  n = v * (1 - s * f);

  switch (i)
  {

  case 0:
  case 6:
    rgb_colors[0] = round(v * 255);
    rgb_colors[1] = round(n * 255);
    rgb_colors[2] = round(m * 255);
    break;

  case 1:
    rgb_colors[0] = round(n * 255);
    rgb_colors[1] = round(v * 255);
    rgb_colors[2] = round(m * 255);
    break;

  case 2:
    rgb_colors[0] = round(m * 255);
    rgb_colors[1] = round(v * 255);
    rgb_colors[2] = round(n * 255);
    break;

  case 3:
    rgb_colors[0] = round(m * 255);
    rgb_colors[1] = round(n * 255);
    rgb_colors[2] = round(v * 255);
    break;

  case 4:
    rgb_colors[0] = round(n * 255);
    rgb_colors[1] = round(m * 255);
    rgb_colors[2] = round(v * 255);
    break;

  case 5:
    rgb_colors[0] = round(v * 255);
    rgb_colors[1] = round(m * 255);
    rgb_colors[2] = round(n * 255);
    break;
  }
}
