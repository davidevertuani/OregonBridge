/**
 * DecodeOOK.h - This file is part of OregonBridge Arduino Library.
 * 
 * @file DecodeOOK.h
 * @brief Decode OOK signals from weather sensors and supported devices.
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
 * - Nov. 2021: DecodeOOK added to OregonBridge library - Davide Vertuani.
 * - Apr. 2010: New code to decode OOK signals from weather sensors, etc.
 *    2010-04-11 <jcw@equi4.com> http://opensource.org/licenses/mit-license.php
 *    $Id: ookDecoder.pde 5331 2010-04-17 10:45:17Z jcw $
 */

#ifndef DecodeOOK_h
#define DecodeOOK_h

class DecodeOOK {
 protected:
  byte total_bits, bits, flip, state, pos, data[25];

  virtual char decode(word width) = 0;

 public:
  enum { UNKNOWN,
         T0,
         T1,
         T2,
         T3,
         OK,
         DONE };

  DecodeOOK() { resetDecoder(); }

  bool nextPulse(word width) {
    if (state != DONE)

      switch (decode(width)) {
        case -1:
          resetDecoder();
          break;
        case 1:
          done();
          break;
      }
    return isDone();
  }

  bool isDone() const { return state == DONE; }

  const byte* getData(byte& count) const {
    count = pos;
    return data;
  }

  void resetDecoder() {
    total_bits = bits = pos = flip = 0;
    state = UNKNOWN;
  }

  // add one bit to the packet data buffer
  virtual void gotBit(char value) {
    total_bits++;
    byte* ptr = data + pos;
    *ptr = (*ptr >> 1) | (value << 7);

    if (++bits >= 8) {
      bits = 0;
      if (++pos >= sizeof data) {
        resetDecoder();
        return;
      }
    }
    state = OK;
  }

  // store a bit using Manchester encoding
  void manchester(char value) {
    flip ^= value;  // manchester code, long pulse flips the bit
    gotBit(flip);
  }

  // move bits to the front so that all the bits are aligned to the end
  void alignTail(byte max = 0) {
    // align bits
    if (bits != 0) {
      data[pos] >>= 8 - bits;
      for (byte i = 0; i < pos; ++i)
        data[i] = (data[i] >> bits) | (data[i + 1] << (8 - bits));
      bits = 0;
    }
    // optionally shift bytes down if there are too many of 'em
    if (max > 0 && pos > max) {
      byte n = pos - max;
      pos = max;
      for (byte i = 0; i < pos; ++i)
        data[i] = data[i + n];
    }
  }

  void reverseBits() {
    for (byte i = 0; i < pos; ++i) {
      byte b = data[i];
      for (byte j = 0; j < 8; ++j) {
        data[i] = (data[i] << 1) | (b & 1);
        b >>= 1;
      }
    }
  }

  void reverseNibbles() {
    for (byte i = 0; i < pos; ++i)
      data[i] = (data[i] << 4) | (data[i] >> 4);
  }

  void done() {
    while (bits)
      gotBit(0);  // padding
    state = DONE;
  }
};

#endif