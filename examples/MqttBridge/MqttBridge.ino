/**
 * @file MqttBridge.ino
 * @author Davide Vertuani
 * @brief Send OS messages over MQTT.
 * @version 1.0
 * @date 2021-11-06
 * 
 * @copyright Copyright (c) 2021 - MIT Licence
 * 
 * This sketch provides an example of how to bridge OS messages over MQTT.
 * The receiver must be hooked up to GPIO D2 (or any other interrupt-enabled).
 * 
 * Currently only a few v2 devices are supported, as the author did not have the
 * ability to test the library on other models. The source code can be easily edited
 * to include other remote devices as well.
 * V1 sensors are instead all supported, due to their unique message structure.
 * 
 * To debug or test for unsupported devices, define 'OS_DEBUG' in the library.
 * 
 * Tested on ESP8266 (NodeMCU) with 433MHz receiver RXB6.
 * 
 */

#include <ESP8266WiFi.h>
#include <OregonBridge.h>
#include <PubSubClient.h>

#define WIFI_SSID "<your-ssid-here>"
#define WIFI_PASS "<your-wifi-password-here>"
#define MQTT_SERVER "<host_addr>"
#define MQTT_RECONNECT_INTERVAL 4000

WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

char msg[50];
uint32_t lastReconnectAttempt = 0;

// Instantiate the library
OregonBridge orbridge;

// 'ICACHE_RAM_ATTR' strictly required for ESPs
void ICACHE_RAM_ATTR mExtInterrupt() {
  orbridge.externalInterrupt();
}

// Define the pin where the 433Mhz receiver is attached
// Must be interrupt enabled!
#define RCVR_PIN D2

void setup() {
  Serial.begin(115200);

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  Serial.println();
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi connected; parameters:");
  Serial.print(WiFi.localIP());
  Serial.print(" - ");
  Serial.println(WiFi.macAddress());

  MQTT_setup();

  Serial.println(F("MqttBridge.ino - Example script for library OregonBridge"));
  Serial.println(F("Decode and validate messages from OSv2.1 devices with Manchester encoding."));
  Serial.print(F("Waiting for signals to be received on GPIO "));
  Serial.println(RCVR_PIN);

  if (digitalPinToInterrupt(RCVR_PIN) < 0)
    Serial.println(F("\n--- INVALID PIN - INTERRUPT NOT AVAILABLE ---"));

  // Setup external interrupt on pin 'RCVR_PIN'
  pinMode(RCVR_PIN, INPUT);
  attachInterrupt(digitalPinToInterrupt(RCVR_PIN), mExtInterrupt, CHANGE);

  // Register the library to call function 'osCallback' when a valid data
  // packet is received. The data is passed as argument.
  orbridge.registerCallback(osCallback);
}

void loop() {
  MQTT_loop();
  orbridge.loop();
}

/**
 * A valid data packet has been received.
 * Data can be accessed via library functions (temp, humidity, ...)
 * You can filter by ID, type, etc. and react accordingly.
 */
void osCallback(Device* device, const byte* data) {
  const char* _p = device->getOsVersion();
  const char* _m = device->getRemoteModel(data);
  byte _i = device->getId(data);
  byte _c = device->getChannel(data);
  float _t = device->getTemperature(data);
  byte _h = device->getHumidity(data);
  bool _b = device->getBattery(data);

  Serial.println("\n--- Found remote - model " + String(_m) + " ---");
  Serial.println("Version: \tOS " + String(_p));
  Serial.print("ID: \t\t" + String(_i) + ", HEX ");
  Serial.println(_i, HEX);
  Serial.println("Channel: \t" + String(_c));
  Serial.println("Battery level: \t" + (_b ? String("good") : String("low")));
  Serial.println("Temperature: \t" + String(_t) + "°C");
  Serial.println("Humidity: \t" + String(_h) + "%");

  char buf[8];
  sprintf(buf, "%.1f", _t);
  const char* _temp = buf;
  mqttClient.publish("topic/temperature", _temp);

  sprintf(buf, "%s", String(_h));
  const char* _hum = buf;
  mqttClient.publish("topic/humidity", _hum);

  Serial.print("\nSent over MQTT: T. ");
  Serial.print(_t);
  Serial.print("°C - H. %");
  Serial.println(_h);
}
