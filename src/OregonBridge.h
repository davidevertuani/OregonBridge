/**
 * OregonBridge.h - This file is part of OregonBridge Arduino Library.
 * 
 * @file OregonBridge.h
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
 * 
 * Revision history:
 * - Nov. 2021: Code organized into OregonBridge library - Davide Vertuani.
 * - Oregon V2 decoder modified for MySensors compatibility - Mickael Hubert
 * (<github@winlux.fr>)
 * - Oregon V2 decoder modified - Olivier Lebrun
 * - Oregon V2 decoder added - Dominique Pierre
 */

#ifndef OregonBridge_h
#define OregonBridge_H

/* Enable/disable debug logging */
// #define OS_DEBUG

#include "Arduino.h"
#include "SupportedDevices.h"

class OregonBridge {
 public:
  /**
   * @brief Construct a new Oregon Bridge object
   */
  OregonBridge(void);

  /**
   * @brief Main library function. Must be called each loop to check new data.
   */
  void loop(void);

  /* */
  void externalInterrupt(void);

  /**
   * @brief User-defined callback. Is invoked when a valid data package is received and parsed. The data is passed as argument for further processing.
   */
  using osCallbackFunc = void (*)(Device*, const byte*);

  /**
   * @brief Registers user-defined callback.
   * Callback prototype: void (*)(const byte*)
   * 
   * @param callbackFunction the callback function.
   */
  void registerCallback(osCallbackFunc callbackFunction);

 private:
  /**
   * @brief Instances of decoder classes.   
   */
  Device** devices = new Device*[DEVICES_NUM];

  /* Counter of used positions in 'devices' array */
  uint8_t devicesCount = 0;

  /**
    * @brief Pulse length 
    */
  volatile word pulse;

  /**
   * @brief Pointer to user-provided callback function   
   */
  osCallbackFunc usrCallbackfunc;

  /**
   * @brief Sends raw data to the decode class, and gets a parsed byte array.
   * 
   * @param decoder DecodeOOK instance
   * @return const byte*, decoded data
   */
  const byte* dataToDecoder(class Device* decoder);

  /**
 * @brief Utility function to log details aboout the incoming message.
 * 
 * @param device The device object generating the message
 * @param data The message data
 */
  void printDetails(Device* device, const byte* data);

  /**
   * @brief Add supported devices (and decoders) to the array.
   * 
   * @tparam T The class of the device.
   */
  template <class T>
  void addDevice();
};

#endif