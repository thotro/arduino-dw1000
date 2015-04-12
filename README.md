# arduino-dw1000
A library that offers functionality to use Decawave's DW1000 chips/modules with Arduino/AVR
(see http://www.decawave.com/products/dwm1000-module).

Project structure:
 * DW1000 ... contains the Arduino library (which is to be copied to the corresponding libraries folder of your Arduino install or imported via the GUI)
 * DW1000-arduino-test ... contains Arduino test code using the DW1000 library
 * DW1000-unit-test ... contains plain C++ unit test code for the library
 * AdapterBoard ... contains PCB files for a 1/10 inch adapter board for the DW1000 module

Project status: 20%
Current milestone: RX/TX test with two chips, planned till end of April

What works so far:
 * Basic SPI read/write with the chip
 * Fetching of chip configuration and device id
 * Writing of chip configuration
 * Writing of node id
 * Writing of transmit data and transmit controls
 * Transmission and reception sessions (structure)

Next on the agenda:
 * Configuration of full transmission sessions
 * Simple IRQ handing and ISR definition
 * Basic transmission and receiving
 * Different setups and performance benchmarks
 * Ranging and simple communication examples
 * ...

Usage will be something like:
```
DW1000 dw = DW1000(cs_pin);
dw.initialize();
...
dw.newConfiguration();
// configure specific aspects or choose defaults
dw.setDefaults();
dw.commitConfiguration();
...
dw.newTransmit();
// configure specific aspects or choose defaults
dw.setDefaults();
dw.setData(some_data);
dw.startTransmit();
...
// similar for receiving plus IRQ handling
```
