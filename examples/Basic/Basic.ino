/**
 * @file Basic.ino
 * @author Davide Vertuani
 * @brief Basic example for library OregonBridge.
 * @version 1.0
 * @date 2021-11-06
 * 
 * @copyright Copyright (c) 2021 - MIT Licence
 * 
 * This sketch provides the bare minimum to work with OregonBridge.
 * The receiver must be hooked up to GPIO 2 (or any other interrupt-enabled).
 * 
 * Currently only a few v2 devices are supported, as the author did not have the
 * ability to test the library on other models. The source code can be easily edited
 * to include other remote devices as well.
 * V1 sensors are instead all supported, due to their unique message structure.
 * 
 * To debug or test for unsupported devices, define 'OS_DEBUG' in the library.
 * 
 * Tested on Arduino UNO with 433MHz receiver RXB6.
 * 
 */

#include <OregonBridge.h>

// Define the pin where the 433Mhz receiver is attached
// Must be interrupt enabled!
#define RCVR_PIN 2

// Instantiate the library
OregonBridge orbridge;

// add 'ICACHE_RAM_ATTR' if running on ESP
void mExtInterrupt() {
  orbridge.externalInterrupt();
}

void setup() {
  Serial.begin(115200);
  delay(500);

  Serial.println(F("Basic.ino - Example script for library OregonBridge"));
  Serial.println(F("Decode and validate messages from Oregon Scientific devices with Manchester encoding."));
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
}
