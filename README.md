# arduino-dw1000
A library that offers functionality to use Decawave's DW1000 chips/modules with Arduino/AVR
(see http://www.decawave.com/products/dwm1000-module).

Project state
-------------

**Progress:** 80% (of a first fully configurable, tested and stable release)

**Current milestone:** Tuning and testing (sender/receiver, ping-pong and two-way ranging)

**Subsequent milestone:** Frame filtering rules, nodes addressing and MAC data format

**General notice:** Stable transmission of messages between two modules is possible. The code for device tuning is working as well, hence different modes of operation can be chosen. As frame filtering (i.e. via MAC conforming messages) is not implemented yet, internal features of the chip for node addressing and auto-acknowledgement of messages can not be used. This is part of a future milestone. For now, if acknowledgements are required, they have to be sent manually and node addresses have to be encoded in the message payload and processed by the host controller.

Contents
--------

 * [Project structure](../../wiki/Project-structure)
 * [Features and design intentions](../../wiki/Features)
 * [Testbed and Adapter board](../../wiki/Testbed-and-Adapter-board)
 * [Benchmarks](../../wiki/Benchmarks)
 * [API docs](https://cdn.rawgit.com/thotro/arduino-dw1000/master/DW1000/doc/html/index.html)

Usage
-----

General usage of the DW1000 library is depicted below. Please see the Arduino test example codes for more up-to-date and operational reference usage. 

At the moment the library contains two types:
 * **DW1000:** The statically accessible entity to work with your modules. Offers a variety of configuration options and manages module states and actions. 
 
 * **DW1000Time:** Container entities that handle DW1000 specific timing values. These are required to allow accurate timestamps and time based computations; they aid in avoiding potential precision and capacity problems of standard number formats in Arduino and basically are wrapper objects for 64-bit unsigned integer data types; most importantly they take care of all bit-to-time-and-distance (and vice versa) conversions.

API docs for the library will shortly follow. 

```Arduino
#include <DW1000.h>
...
// init with interrupt line and optionally with reset line
DW1000.begin(irq_pin[, rst_pin]);
// select a specific chip via a chip select line
DW1000.select(cs_pin);
...
// open a device configuration sessions
DW1000.newConfiguration();
// configure specific aspects and/or choose defaults
DW1000.setDefaults();
DW1000.setDeviceAddress(5);
DW1000.setNetworkId(10);
// modes that define data rate, frequency, etc.
DW1000.enableMode(..);
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


