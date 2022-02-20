# Arduino OregonBridge Library
Receive and decode data from Oregon Sensors v1 or v2.1.

## How to install:
1) Download the [source code .zip file](https://github.com/davidevertuani/OregonBridge/archive/master.zip).
2) Extract `master.zip`
3) Rename folder into `OregonBridge`
4) Cut-paste the `OregonBridge` folder in your Arduino `libraries` folder.

## Basic usage
Not much is needed to get your Oregon Scientific decoder up and running.  
Just instantiate the object, define the external interrupt and register the callback function. Below is a bare minimum working example.

```
// Include the library
#include <OregonBridge.h>

// Instantiate the object
OregonBridge orbridge;

// Define the pin where the 433Mhz receiver is attached
// Must be interrupt enabled!
#define RCVR_PIN 2

// Define te interrupt function
// add 'ICACHE_RAM_ATTR' if running on ESP!!
void mExtInterrupt() {
  orbridge.externalInterrupt();
}

void setup() {
  // Setup external interrupt on pin 'RCVR_PIN'
  pinMode(RCVR_PIN, INPUT);
  attachInterrupt(digitalPinToInterrupt(RCVR_PIN), mExtInterrupt, CHANGE);

  /* Register the library to call function 'osCallback' when a valid data
  packet is received. The data is passed as argument. */
  orbridge.registerCallback(osCallback);
}

void loop() {
  orbridge.loop();
}

// Callback function
void osCallback(Device* device, const byte* data) {
  // Process the data as you wish
}
```

## Supported devices
As of now (first release, Nov. 2021) the library only supports Oregon V1 devices (all, since they share the same protocol), and some OS v2 remote units.  
The latter are:

* THN132N
* THGR228N

If you have other devices on hand and want to extend the library support, feel free to open a pull request.

## Example output
The following is the serial output when debug logs are enabled, by defining `OS_DEBUG` in `OregonBridge.h`.

```
--- Found remote - model THGR228N ---
Version: 	    OS v2.1
ID: 		    209, HEX D1
Channel: 	    2
Battery level: 	good
Temperature: 	14.10°C
Humidity: 	    49%
```

## Available remote readings
Currently, the following parameters are available:

* All
    - Channel (1, 2, 3)
    - Battery status
    - ID
    - OS version
    - OS model
* OS v1
    - Temperature
* OS v2
    - Temperature
    - Humidity

Measurements of rain / wind / etc. are not available, as the author did not have models recording these properties to conduct tests on.  

In the callback, you can access all the measurements by calling 

```
device->getOsVersion();
device->getRemoteModel(data);
device->getId(data);
device->getChannel(data);
device->getTemperature(data);
device->getHumidity(data);
device->getBattery(data);
```

As the callback is invoked when a packet is received and correctly validated via checksum, it will only receive data from supported devices.  
To debug your code or to support new devices, display any received packet via `OS_DEBUG` in `OregonBridge.h`.

## Tested Hardware
- Arduino UNO & ESP8266 (NodeMCU v1)
- 433Mhz RXB6 receiver
- Oregon sensors THGR228N (v2.1), THR128 (v1).

## Credits
This project is licensed under the terms of the MIT license.  

Credit is due to the following projects, from which this library takes inspiration or sections of code:

* [Arduino-Oregon-Library](https://github.com/Mickaelh51/Arduino-Oregon-Library) by Mickaelh51, providing many of the core functions.
* [rtl_443](https://github.com/merbanan/rtl_433) - generic data receiver for ISM bands devices, from which data validation for v2 sensors is taken.
* The DecodeOOK class providing the base decoding capabilities for OOK signals (jcw@equi4.com).