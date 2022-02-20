/**
 * OregonDevice_v1.h - This file is part of OregonBridge Arduino Library.
 * 
 * @file OregonDevice_v1.h
 * @author Davide Vertuani
 * @brief Decode and parse data packets from Oregon Scientific V1 devices.
 * @version 1.0
 * @date 2021-11-09
 * 
 * @copyright Copyright (c) 2021
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
 * Revision history:
 * - Nov. 2021: OregonDevice_v1 created and added to OregonBridge 
 *    library - Davide Vertuani. 
 */

#ifndef OregonDevice_v1_h
#define OregonDevice_v1_h

#include "DecodeOOK.h"
#include "Device.h"

class OregonDecoder_v1 : public DecodeOOK {
 public:
  virtual char decode(word width) {
    if (900 <= width && width <= 7000) {
      byte w = width >= 2300;

      switch (state) {
        case UNKNOWN:
          if (w == 0) {
            // Short pulse
            ++flip;
          } else if (w != 0 && 22 <= flip) {
            // Long pulse, start bit
            flip = 0;
            state = T1;
          } else {
            // Reset decoder
            return -1;
          }
          break;
        case OK:
          /** Due to Manchester encoding: short pulse maintain the same bit
           * as the previous, long pulses flip the value (1 to 0 or vice
           * versa)
           * */
          if (w == 0) {
            // Short pulse
            state = T0;
          } else {
            // Long pulse
            manchester(1);
          }
          break;
        case T0:
          if (w == 0) {
            // Second short pulse
            manchester(0);
          } else {
            // Reset decoder
            return -1;
          }
          break;
        case T1:
          // RF-on long pulse (approx 5.7 ms)
          //if (width < 4000) return -1;
          if (5550 <= width && width <= 6000)
            state = T2;
          else
            return -1;
          break;
        case T2:
          /* RF-off long period (approx 5 ms)
          If a '0' is the first bit, no signal transition occurs, but can
          be detected by measuring the pulse length.
          ~5.2ms: first bit 1
          ~6.6ms: first bit 0 */
          if (4800 <= width && width <= 5400) {
            flip = 1;
            state = T0;
          } else if (6480 <= width && width <= 6880) {
            gotBit(0);
          } else
            return -1;
          break;
      }
    } else {
      return -1;
    }
    // Done decoding if a fixed number of 32 bits have been received
    if (total_bits >= 32) return 1;
    return 0;
  }
};

class OregonDevice_v1 : public Device {
 public:
  OregonDevice_v1() {
    this->dDecoder = new OregonDecoder_v1();
  }

  virtual const char* getOsVersion(void) {
    return OS_PROTOCOL_V1;
  }

  /**
  * @brief Validate the checksum found at nibbles 6 and 7 with the value computed
  * by summing the preceding bytes.
  * Checksum for v1 devices is byte-oriented.
  * */
  virtual bool validateChecksum(const byte* data) {
    // Oregon Scientific v1 checksum is a 1 byte 'sum of bytes' checksum.

    unsigned int i = 0, checksum = data[3], sum_of_bytes = 0;
    for (i = 0; i < 3; i++) sum_of_bytes += data[i];

    // Manage overflow (checksum must be of two nibbles)
    sum_of_bytes &= 0xff;

    // Validation is successfull if the two figures match
    bool success = sum_of_bytes == checksum;

#ifdef OS_DEBUG
    Serial.print("Checksum " + (success ? String("OK") : String("error")));
    Serial.print(". Expected: ");
    Serial.print(checksum, HEX);
    Serial.print(", computed: ");
    Serial.println(sum_of_bytes, HEX);
#endif

    return success;
  }

  /**
    * Compute and return the signed temperature value.
    * For OS v1, the temperature is contained in the 3rd to 6th nibbles. 
    * The sign is in the flags field of the 5th nibble.
    * 
    * Examples:
    * 
    *    44 53 02 99
    *    xx bc sa xx
    *    Temperature (s) ab.c -> +25.3°C
    * */
  float getTemperature(const byte* data) {
    int sign = (data[2] & 0x20) < 1 ? 1 : -1;
    float temp = (data[2] & 0x0f) * 10 + ((data[1] & 0xf0) >> 4) + (float)((data[1] & 0x0f) / 10.0);
    float result = sign * temp;
    return sign * temp;
  }

  /**
    * @brief Compute and return the battery status.
    * For OS v1, the battery status flag is contained in the 4th bit of 5th nibble.
    * @return true = good, false = low.
    * */
  bool getBattery(const byte* data) {
    bool BatteryLevel = !(data[2] & 0x80);
    return BatteryLevel;
  }

  /**
  * @brief Return the sensor id.
  * For v1 sensors, the device id is the second nibble (first in order of reception)
  * */
  byte getId(const byte* data) {
    return (data[0] & 0x0f);
  }

  /**
  * @brief Compute the channel of the OS sensor.
  * For v1 sensors, the channel is reported by the 1st nibble.
  */
  byte getChannel(const byte* data) {
    byte channel = 0;
    switch ((data[0] >> 4) & 0x0f) {
      // Seems like v1 sensors have channel 1 reported as either 2 or 0
      case 0x0:
      case 0x2:
        channel = 1;
        break;
      case 0x4:
        channel = 2;
        break;
      case 0x8:
        channel = 3;
        break;
    }
    return channel;
  }

  // Cannot identify device model from data (no specific id)
  const char* getRemoteModel(const byte* data) {
    return "Generic OS v1";
  }
};

#endif