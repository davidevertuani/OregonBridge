/**
 * Device.h - This file is part of OregonBridge Arduino Library.
 * 
 * @file Device.h
 * @brief 
 * @date 2021-11-06
 * @version 1.0
 * 
 * @copyright Copyright (c) 2010 <jcw@equi4.com>
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
 * - Nov. 2021: Device.h created - Davide Vertuani.
 */

#ifndef Device_h
#define Device_h

#include "DecodeOOK.h"

#define OS_PROTOCOL_V1 "v1"
#define OS_PROTOCOL_V2 "v2.1"

class Device {
 protected:
  DecodeOOK* dDecoder;

 public:
  Device() {}

  /**
   * @brief Perform validation of the incoming data via checksum
   * 
   * @param data const byte* received via callback or dataToDecoder
   * @return true if the data packet is valid
   * @return false if the data packet is invalid
   */
  virtual bool validateChecksum(const byte* data) {
    return false;
  }

  /**
   * @brief Get float temperature value from the raw data array.
   * 
   * @param data const byte* received via callback or dataToDecoder
   * @return float, the computed temperature value [degrees]
   */
  virtual float getTemperature(const byte* data) {
    return 0.0;
  }

  /**
   * @brief Get byte humidity percentage value from the raw data array.
   * 
   * @param data const byte* received via callback or dataToDecoder
   * @return byte, the computed humidity value [percentage]
   */
  virtual byte getHumidity(const byte* data) {
    return 0;
  }

  /**
   * @brief Get boolean flag for the remote battery status. True means good
   * battery level, false low battery level.
   * 
   * @param data const byte* received via callback or dataToDecoder
   * @return true, when the battery level is good
   * @return false, with low battery level
   */
  virtual bool getBattery(const byte* data) {
    return false;
  }

  /**
   * @brief Get the numeric ID of the remote sensor.
   * 
   * @param data const byte* received via callback or dataToDecoder
   * @return byte, the device ID
   */
  virtual byte getId(const byte* data) {
    return 0;
  }

  /**
   * @brief Get the channel on which the remote is operating.
   * 
   * @param data const byte* received via callback or dataToDecoder
   * @return byte, the device channel
   */
  virtual byte getChannel(const byte* data) {
    return 0;
  }

  /**
   * @brief Get a string with the model name of the remote, if supported, or
   * 'UNKNOWN' otherwise.
   * 
   * @param data const byte* received via callback or dataToDecoder
   * @return const char*, remote model name
   */
  virtual const char* getRemoteModel(const byte* data) {
    return "UNKNOWN";
  }

  bool nextPulse(word width) {
    return this->dDecoder->nextPulse(width);
  }

  DecodeOOK* decoder() {
    return dDecoder;
  }

  virtual const char* getOsVersion(void) {
    return "undefined";
  }
};

#endif