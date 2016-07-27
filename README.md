# arduino-dw1000
A library that offers functionality to use Decawave's DW1000 chips/modules with Arduino
(see http://www.decawave.com/products/dwm1000-module).

Project state
-------------

**Progress:** 90% (requires some minor optimizations and a lot more testing towards a v1.0 release)

**Current milestone:** MAC and frame filtering, error handling

**Subsequent milestone:** Sleep/power optimizations

**General notice:** Stable transmission of messages between two modules is possible. The code for device tuning is working as well, hence different modes of operation can be chosen. As frame filtering (i.e. via MAC conforming messages) is not implemented yet, internal features of the chip for node addressing and auto-acknowledgement of messages can not be used. This is part of a future milestone. For now, if acknowledgements are required, they have to be sent manually and node addresses have to be encoded in the message payload and processed by the host controller.

Installation
------------

**Requires c++11 support**, Arduino IDE >= 1.6.6 support c++11.

 1. Get a ZIP file of the master branch or the latest release and save somewhere on your machine.
 2. Open your Arduino IDE and goto _Sketch_ / _Include Library_ / _Add .ZIP Library..._
 3. Select the downloaded ZIP file of the DW1000 library
 4. You should now see the library in the list and have access to the examples in the dedicated section of the IDE

Note that in version 1.6.6 of your Arduino IDE you can get the library via the Arduino library manager.

Contents
--------

 * [Project structure](../../wiki/Project-structure)
 * [Features and design intentions](../../wiki/Features)
 * [Testbed and Adapter board](../../wiki/Testbed-and-Adapter-board)
 * [Projects, Media, Links](../../wiki/Projects)
 * [Benchmarks](../../wiki/Benchmarks)
 * API docs
   * [HTML](https://cdn.rawgit.com/thotro/arduino-dw1000/master/extras/doc/html/index.html)
   * [PDF](https://cdn.rawgit.com/thotro/arduino-dw1000/master/extras/doc/DW1000_Arduino_API_doc.pdf)

Usage
-----

General usage of the DW1000 library is depicted below. Please see the Arduino test example codes (described in the [project structure](../../wiki/Project-structure)) for more up-to-date and operational reference usage. 

At the moment the library contains two types:
 * **DW1000:** The statically accessible entity to work with your modules. Offers a variety of configuration options and manages module states and actions. 
 
 * **DW1000Time:** Container entities that handle DW1000 specific timing values. These are required to allow accurate timestamps and time based computations; they aid in avoiding potential precision and capacity problems of standard number formats in Arduino and basically are wrapper objects for 64-bit signed integer data types; most importantly they take care of all bit-to-time-and-distance (and vice versa) conversions.
 
 * **DW1000Ranging:** Contain all functions which allow to make the ranging protocole. 
 
 * **DW1000Device:** Contain all informations (long address, short ephemeral address, DW1000Time of transition)  about a distant device (anchor or tag) on the same network.
 
 * **DW1000Mac:** This class is a child of the DW1000Device class and allow to generate the MAC frame for his DW1000Device parent.
 

```Arduino
#include <DW1000.h>
...
// init with interrupt line and optionally with reset line
DW1000.begin(irq_pin[, rst_pin]);
// select a specific chip via a chip select line
DW1000.select(cs_pin);
// or use DW1000.reselect(cs_pin) to switch to previously selected chip
...
// open a device configuration sessions
DW1000.newConfiguration();
// configure specific aspects and/or choose defaults
DW1000.setDefaults();
DW1000.setDeviceAddress(5);
DW1000.setNetworkId(10);
// modes that define data rate, frequency, etc. (see API docs)
DW1000.enableMode(DW1000.MODE_LONGDATA_RANGE_LOWPOWER);
// ... and other stuff - finally upload to the module.
DW1000.commitConfiguration();
...
// set some interrupt callback routines
DW1000.attachSentHandler(some_handler_function);
DW1000.attachReceivedHandler(another_handler_function);
...
// open a new transmit session
DW1000.newTransmit();
// configure specific aspects and/or choose defaults
DW1000.setDefaults();
DW1000.setData(some_data);
DW1000Time delayTime = DW1000Time(100, DW1000Time::MILLISECONDS);
[DW1000Time futureTimestamp = ]DW1000.setDelay(delayTime);
// ... and other stuff - finally start the transmission
DW1000.startTransmit();
...
// similar for a receiving session, like so ...
DW1000.newReceive();
DW1000.setDefaults();
// so we don't need to restart the receiver manually each time
DW1000.receivePermanently(true);
// ... and other stuff - finally start awaiting messages
DW1000.startReceive();
...
```


