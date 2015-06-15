# arduino-dw1000
A library that offers functionality to use Decawave's DW1000 chips/modules with Arduino/AVR
(see http://www.decawave.com/products/dwm1000-module).

Project structure:
 * DW1000 ... contains the Arduino library (which is to be copied to the corresponding libraries folder of your Arduino install or imported via the GUI)
 * DW1000-arduino-test ... contains Arduino test code using the DW1000 library
   * DW1000-arduino-basic-test ... connectivity test
   * DW1000-arduino-sender-test ... sender part of the basic sender/receiver test; note that due to the manual delays and the amount of Serial prints that occur for logging/debugging, this sender is not intended for high message throughput
   * DW1000-arduino-receiver-test ... receiver part of the basic sender/receiver test
   * DW1000-ping-pong-test ... sender and receiver part of a simple ping pong messaging test between two modules
   * DW1000-arduino-ranging-anchor ... the anchor part (i.e. the range-computing reference module) of the simple ranging test application
   * DW1000-arduino-ranging-tag ... the tag part (i.e. a module that is considered for range determination to/by the anchor) of the simple ranging application
 * AdapterBoard ... contains PCB files for a 1/10 inch adapter board for the DW1000 module

Project status: 70%

Current milestone: Extensive testing of two-way ranging application; by end of June.

Following milestone: Frame filtering rules, node addressing and MAC data format implementation

Features and design intentions:
 * Fully encapsulated SPI communication with the chip
 * Fully encapsulated interrupt handling by means of custom callback handlers; available is
   * on sent (a message has been successfully sent)
   * on receive (a message has been successfully received and its transmitted data is available)
   * on receive error (something in between message detection and decoding went wrong)
   * on receive timeout (only useful if timeouts are enabled and enabling timeouts is still a TODO :-)
   * on receive timestamp available (might be useful for ranging applications)
 * Simple device status querying
 * Simple, comprehensible RX/TX session config and general config API (docs will follow shortly)
 * Automatic tuning of the chip (i.e. as indicated at the various places in the spec)

What works so far:
 * SPI communication with the chip
 * Handling of the most important chip configurations
 * Management of IRQs
 * byte[] and (Arduino-)String based data RX/TX
 * RX/TX/config sessions
 * Delayed/timed RX/TX
 * Stable transmission of messages between two chips
 * Arduino test applications (Basic, Sender/receiver, Ping-pong, Simple ranging) demonstrating library reference use

TODO:
 * Extensive testing of certain configurations, code cleanup (and bug fixing)
 * Performance benchmarks of different configurations
 * Ranging of multiple tags
 * Module calibration
 * Setting and appending of data to be sent (e.g. "appendFloat")
 * MAC message format and frame filtering/adressing
 * API docs

Usage is something like:
```
#include <DW1000.h>
...
// init with chip select, reset and interrupt pin/line
DW1000.begin(cs_pin, rst_pin, int_pin);
...
DW1000.newConfiguration();
// configure specific aspects or choose defaults
DW1000.setDefaults();
DW1000.interruptOnSent(true);
// .. and other stuff.
DW1000.commitConfiguration();
...
DW1000.attachSentHandler(some_handler_function);
...
DW1000.newTransmit();
// configure specific aspects or choose defaults
DW1000.setDefaults();
DW1000.setData(some_data);
DW1000.setDelay(100, DW1000.MILLISECONDS);
DW1000.startTransmit();
...
// similar for receiving, like so ...
DW1000.newReceive();
DW1000.setDefaults();
// so we don't need to restart the receiver manually each time
DW1000.receivePermanently(true);
DW1000.startReceive();
...
```

The testbed in use employs
 * the DWM1000 adapter board
 * an Arduino (in this case a "Pro Mini")
 * (a resistor and some wires)

and looks as follows:

![testbed](https://github.com/thotro/arduino-dw1000/blob/master/AdapterBoard/AdapterBoardTestBed.png)

The adapter board has the following pin mapping:

Pin | Function
----| ---------
1 (EXTON) | see DW1000 manual
2 (WAKEUP) | see DW1000 manual
3 (RSTn) | Resetting the chip (by pulling low)
4 (GPIO7) | see DW1000 manual
5 (VDD) | 3.3V power supply
6 (GPIO6) | see DW1000 manual
7 (GPIO5) | see DW1000 manual
8 (GPIO4) | see DW1000 manual
9 (GPIO3) | see DW1000 manual
10 (GPIO2) | see DW1000 manual
11 (GPIO1) | see DW1000 manual
12 (GPIO0) | see DW1000 manual
13 (SPICSn) | SPI chip select line (pulling low to activate chip)
14 (SPIMOSI) | SPI communication towards chip
15 (SPIMISO) | SPI communication towards controller
16 (SPICLK) | SPI clock (at most 3MHz in init phase, afterwards up to 20MHz)
17 (GPIO8) | interrupt asserted line, see DW1000 manual
18 (GND) | Ground
