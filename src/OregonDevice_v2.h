/**
 * OregonDevice_v2.h - This file is part of OregonBridge Arduino Library.
 * 
 * @file OregonDevice_v2.h
 * @author Davide Vertuani
 * @brief Decode messages from Oregon Scientific v2 devices (Manchester encoding)
 * @version 1.0
 * @date 2021-11-06
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
 * - Nov. 2021: OregonDevice_v2 created and added to OregonBridge 
 *    library - Davide Vertuani. 
 */

#ifndef OregonDevice_v2_h
#define OregonDevice_v2_h

#include "DecodeOOK.h"
#include "Device.h"

class OregonDecoder_v2 : public DecodeOOK {
 public:
  // add one bit to the packet data buffer
  virtual void gotBit(char value) {
    // Add one bit only if the count is even as v2.1 messages are doubled
    if (!(total_bits & 0x01)) {
      data[pos] = (data[pos] >> 1) | (value ? 0x80 : 00);
    }
    total_bits++;
    pos = total_bits >> 4;
    if (pos >= sizeof data) {
      resetDecoder();
      return;
    }
    state = OK;
  }

  virtual char decode(word width) {
    if (200 <= width && width < 1200) {
      // Pulse length: w=1 -> 'long' pulse, w=0 -> 'short' pulse
      byte w = width >= 700;

      switch (state) {
        case UNKNOWN:

          /* For v2.1 or v3 sensors, the preamble consists of “1” bits, 24 bits (6 nibbles)
            or v3.0 sensors and 16 bits (4 nibbles) for v2.1 sensors (since a v2.1 sensor bit
            stream contains an inverted and interleaved copy of the data bits, there is in fact
            a 32 bit sequence of alternating “0” and “1” bits in the preamble). 
            Here, if more then 24 '01' (or '10') are detected by detecting a flip. i.e. a long
            pulse, the preamble is considered finished, and we wait for the sync nibble - which
            is '1010', or hex 'A'. Long bits are used for the count. */

          if (w != 0) {
            // Long pulse
            ++flip;
          } else if (w == 0 && 24 <= flip) {
            // Short pulse, start bit
            flip = 0;
            state = T0;
          } else {
            // Reset decoder
            return -1;
          }
          break;
        case OK:
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
      }
    } else if (width >= 2500 && pos >= 8) {
      /* If at least 8 bits have been received, and a long duration signal
      ('trailing off sync') is detected, the decoder is done and return successfully. */
      return 1;
    } else {
      return -1;
    }
    return 0;
  }
};

class OregonDevice_v2 : public Device {
 public:
  OregonDevice_v2() {
    this->dDecoder = new OregonDecoder_v2();
  }

  virtual const char* getOsVersion(void) {
    return OS_PROTOCOL_V2;
  }

  /**
 * Validate the checksum found at 'checksum_nibble_idx' with the value computed
 * by summing the nibbles. No inversion in the nibbles themselves is required
 * (these nibbles are flipped comparing to rtl_433 approach).
 * 
 * Credits for this function to: github.com/merbanan/rtl_433
 * */
  virtual bool validateChecksum(const byte* data) {
    // Oregon Scientific v2.1 and v3 checksum is a 1 byte 'sum of nibbles' checksum.

    // Only proceed validating if a valid position is returned (>0)
    // Otherwise, the model is not supported.
    uint8_t checksum_nibble_idx = getChecksumPos(data);
    if (!checksum_nibble_idx) return false;

    unsigned int checksum, sum_of_nibbles = 0;
    // Sum nibble by nibble, incrementing i by 2 (one byte = 2 nibbles)
    for (int i = 0; i < checksum_nibble_idx - 1; i += 2) {
      unsigned char val = data[i >> 1];
      sum_of_nibbles += ((val >> 4) + (val & 0x0f));
    }
    if (checksum_nibble_idx & 1) {
      // If the position is odd, also take the first nibble in the following pair
      sum_of_nibbles += (data[checksum_nibble_idx >> 1] >> 4);
      checksum = ((data[(checksum_nibble_idx + 1) >> 1] & 0xf0) >> 4) | ((data[checksum_nibble_idx >> 1] & 0x0f) << 4);
    } else {
      checksum = data[checksum_nibble_idx >> 1];
    }

    // Remove 0x0A from the sum. 'A' is the first char (hex notation) of the data
    // array, but must not be included in the checksum
    sum_of_nibbles -= 0x0a;

    // Manage overflow (checksum must be of two nibbles)
    sum_of_nibbles &= 0xff;

    // Validation is successfull if the two figures match
    bool success = sum_of_nibbles == checksum;

#ifdef OS_DEBUG
    Serial.print("Checksum " + (success ? String("OK") : String("error")));
    Serial.print(". Expected: ");
    Serial.print(checksum, HEX);
    Serial.print(", computed: ");
    Serial.println(sum_of_nibbles, HEX);
#endif

    return success;
  }

  /**
 * Compute and return the signed temperature value.
 * For OS v2.1, the temperature is contained in the 5th, 6th and 7th nibbles 
 * (assuming 'leading A notation').
 * 
 * Examples:
 * 
 *    1A 2D 20 8B 58 21 40 C7 4C 8C
 *    xx xx xx xx cx ab xs xx xx xx
 *    Temperature (s) ab.c -> +21.5°C
 * 
 *    1A 2D 40 58 4C 08 88 82 53
 *    xx xx xx xx cx ab xs xx xx
 *    Temperature (s) ab.c -> -08.4°C
 * */
  float getTemperature(const byte* data) {
    int sign = (data[6] & 0x8) ? -1 : 1;
    float temp = ((data[5] & 0xF0) >> 4) * 10 + (data[5] & 0xF) + (float)(((data[4] & 0xF0) >> 4) / 10.0);
    float result = sign * temp;
    return sign * temp;
  }

  /**
 * Compute and return the percentage humidity value.
 * For OS v2.1, the humidity is contained in the 7th and 8th nibbles 
 * (assuming 'leading A notation').
 * 
 * Examples:
 * 
 *    1A 2D 20 8B 58 21 40 C7 4C 8C
 *    xx xx xx xx xx xx bx xa xx xx
 *    Temperature ab -> 74%
 * 
 *    1A 2D 40 58 4C 08 88 82 53
 *    xx xx xx xx xx xx bx xa xx xx
 *    Temperature ab -> 28%
 * */
  byte getHumidity(const byte* data) {
    byte humidity = (data[7] & 0xF) * 10 + ((data[6] & 0xF0) >> 4);
    return (humidity);
  }

  /**
 * Compute and return the battery status.
 * For OS v2.1, the battery status flag is contained in the 5th nibble 
 * (assuming 'leading A notation').
 * */
  bool getBattery(const byte* data) {
    bool BatteryLevel = !(data[4] & 0x4);
    return BatteryLevel;
  }

  /**
 * Return the sensor id.
 * */
  byte getId(const byte* data) {
    return (data[3]);
  }

  /**
 * @brief Compute the channel of the OS sensor.
 * 
 * @param data 
 * @return byte 
 */
  byte getChannel(const byte* data) {
    byte channel;
    return (1 << (((data[2] & 0xf0) >> 4) - 1));
  }

  /**
 * @brief Returns the position in the data array in which we expect to find the first
 * of the two checksum nibbles.
 * Differently from rtl_433, the array begins with 'A', which is not stripped, 
 * so starting values are greater by one here.
* 
* @param data const byte* received via callback or dataToDecoder
* @return uint8_t the position of the first checksum nibble
 */
  uint8_t getChecksumPos(const byte* data) {
    switch ((data[0] << 8) | data[1]) {
      case 0xea4c:  // THN132N
      case 0x1a2d:  // THGR228N
        return 16;
      default:
#ifdef OS_DEBUG
        Serial.println("Known remote identifier not found - unable to validate checksum.");
#endif
        return 0;
    }

    /*
    uint8_t checksum_idx = 0;

    // THN132N
    if (data[0] == 0xEA && data[1] == 0x4C)
      checksum_idx = 16;

    // THGR228N
    else if (data[0] == 0x1A && data[1] == 0x2D)
      checksum_idx = 16;

#ifdef OS_DEBUG
    if (checksum_idx > 0)
      Serial.println("Valid remote identifier found - the model is known");
#endif

    return checksum_idx;
    */
  }

  // Detect type of sensor module
  const char* getRemoteModel(const byte* data) {
    switch ((data[0] << 8) | data[1]) {
      case 0xea4c:
        return "THN132N";
      case 0x1a2d:
        return "THGR228N";
      default:
        return "UNKNOWN";
    }
  }
};

#endif