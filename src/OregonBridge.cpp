/**
 * OregonBridge.cpp - This file is part of OregonBridge Arduino Library.
 * 
 * @file OregonBridge.cpp
 * @brief An Arduino library to receive, decode and interpret Oregon Scientific
 * V2.1 data packets from suitable remote weather sensors.
 * @version 1.0
 * @date 2021-11-06
 * 
 * @copyright Copyright (c) 2021 - Dominique Pierre, Olivier Lebrun, 
 * Mickael Hubert <github@winlux.fr>, Davide Vertuani
 * 
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 * 
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 * 
 * This library is the result of the work of many; credit is due to every
 * contributor. The work of each is attributed where possible.
 * 
 * Revision history:
 * - Nov. 2021: Code organized into OregonBridge library. Decoupled device-specific
 *    logic from library core. Added checksum verification and loop(). Davide Vertuani.
 * - Oregon V2 decoder modified for MySensors compatibility - Mickael Hubert
 *    (<github@winlux.fr>)
 * - Oregon V2 decoder modified - Olivier Lebrun
 * - Oregon V2 decoder added - Dominique Pierre
 */

#include "OregonBridge.h"

#include "Arduino.h"

/**
 * @brief Construct a new Oregon Bridge:: Oregon Bridge object 
 */
OregonBridge::OregonBridge(void) {
  INCLUDE_ALL_DEVICES
}

/**
 * @brief Add supported devices and decoders to the devices array - via macro. * 
 * @tparam T (extends Device) the class of the device to be added.
 */
template <class T>
void OregonBridge::addDevice() {
  if (this->devicesCount >= DEVICES_NUM) return;
  devices[this->devicesCount] = new T;
  this->devicesCount++;
}

/**
 * @brief Main loop. Checks the pulse value recored by the interrupt and decodes
 * the message - if any. 
 */
void OregonBridge::loop(void) {
  // deactivate interrupts to avoid issues while handling data
  noInterrupts();

  word p = this->pulse;
  this->pulse = 0;

  if (p == 0) {
    interrupts();
    return;
  }

  // loop on every available device
  for (uint8_t kk = 0; kk < devicesCount; kk++) {
    Device* d = devices[kk];
    if (!d->nextPulse(p)) continue;

    const byte* dataDecoded = dataToDecoder(d);

    // Validate payload via checksum. If invalid, do not proceed
    if (!d->validateChecksum(dataDecoded)) continue;

    // Invoke user callback function if not nullpntr
    if (this->usrCallbackfunc) this->usrCallbackfunc(d, dataDecoded);

    // Print info to serial
    printDetails(d, dataDecoded);
  }
  interrupts();
}

/**
 * @brief Interrups function. Must be called by the main sketch when a change
 * on the RF receiver signal pin is detected.
 * The function determines the length of the pulses in the incoming message. 
 */
void OregonBridge::externalInterrupt(void) {
  static word last;
  // determine the pulse length in microseconds, for either polarity
  pulse = micros() - last;
  last += this->pulse;
}

// Decode data once
const byte* OregonBridge::dataToDecoder(Device* device) {
  DecodeOOK* decoder = device->decoder();
  byte pos;
  const byte* data = decoder->getData(pos);

#ifdef OS_DEBUG
  Serial.println("\n--- Signal received ---");
  Serial.print("Raw Hexadecimal data from sensor: ");
  for (byte i = 0; i < pos; ++i) {
    Serial.print(data[i] >> 4, HEX);
    Serial.print(data[i] & 0x0F, HEX);
  }
  Serial.println();
#endif

  decoder->resetDecoder();
  return data;
}

void OregonBridge::registerCallback(osCallbackFunc callbackFunction) {
  this->usrCallbackfunc = callbackFunction;
}

void OregonBridge::printDetails(Device* d, const byte* data) {
#ifdef OS_DEBUG
  Serial.println("\n--- Found remote - model " + String(d->getRemoteModel(data)) + " ---");
  Serial.println("Version: \tOS " + String(d->getOsVersion()));
  Serial.print("ID: \t\t" + String(d->getId(data)) + ", HEX ");
  Serial.println(d->getId(data), HEX);
  Serial.println("Channel: \t" + String(d->getChannel(data)));
  Serial.println("Battery level: \t" + (d->getBattery(data) ? String("good") : String("low")));
  Serial.println("Temperature: \t" + String(d->getTemperature(data)) + "°C");
  Serial.println("Humidity: \t" + String(d->getHumidity(data)) + "%");
#endif
}