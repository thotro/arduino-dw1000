# arduino-dw1000
A library that offers functionality to use Decawave's DW1000 chips/modules with Arduino/AVR
(see http://www.decawave.com/products/dwm1000-module).

Project structure:
 * DW1000 ... contains the Arduino library (which is to be copied to the corresponding libraries folder of your Arduino install or imported via the GUI)
 * DW1000-arduino-test ... contains Arduino test code using the DW1000 library
   * DW1000-arduino-basic-test ... connectivity test
   * DW1000-arduino-sender-test ... sender part of the basic sender/receiver test
   * DW1000-arduino-receiver-test ... receiver part of the basic sender/receiver test
 * DW1000-unit-test ... contains plain C++ unit test code for the library
 * AdapterBoard ... contains PCB files for a 1/10 inch adapter board for the DW1000 module

Project status: 45%

Current milestone: Extensive RX/TX config testing with two chips; planned till beginning of June

Features and design intentions:
 * Fully encapsulated SPI communication with the chip
 * Simple device status querying
 * Simple and readable RX/TX/config API (docs will follow shortly)

What works so far:
 * SPI communication with the chip
 * Handling of the most important chip configurations
 * Management of IRQs
 * byte[] and (Arduino-)String based data RX/TX
 * RX/TX/config sessions
 * Stable transmission of packets between two chips

Next on the agenda:
 * Extensive testing of certain configurations, code cleanup (and bug fixing)
 * Performance benchmarks of different configurations
 * Real-time location sensing specific code
 * Ranging and other simple communication examples (as Arduino code files)
 * ...

Misc todos:
 * API docs to follow shortly
 * Wiring instruction and some pictures of the testbed (an Arduino Pro Mini, a )

Usage is something like:
```
DW1000 dw = DW1000(cs_pin, rst_pin);
dw.initialize();
attachInterrupt(...);
...
dw.newConfiguration();
// configure specific aspects or choose defaults
dw.setDefaults();
dw.interruptOnSent(true);
// .. and other stuff.
dw.commitConfiguration();
...
dw.newTransmit();
// configure specific aspects or choose defaults
dw.setDefaults();
dw.setData(some_data);
dw.startTransmit();
...
// similar for receiving
```

The testbed in use employs
 * the DWM1000 adapter board
 * an Arduino (in this case a "Pro Mini")
 * (a resistor and some wires)

and looks as follows:

![testbed](https://github.com/thotro/arduino-dw1000/blob/master/AdapterBoardTestBed.png

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
