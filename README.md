# arduino-dw1000
A library that offers functionality to use Decawave's DW1000 chips/modules with Arduino/AVR
(see http://www.decawave.com/products/dwm1000-module).

Project structure:
 * DW1000 ... contains the Arduino library (which is to be copied to the corresponding libraries folder of your Arduino install or imported via the GUI)
 * DW1000-arduino-test ... contains Arduino test code using the DW1000 library
 * DW1000-unit-test ... contains plain C++ unit test code for the library

Project status: 15%
Current milestone: RX/TX test with two chips, planned till latest March 1

What works so far:
 * Basic SPI read/write with the chip
 * Fetching of chip configuration and device id
 * Writing of chip configuration
 * Writing of transmit data and transmit controls
 * Transmission and reception sessions (structure)

Next on the agenda:
 * Configuration of full transmission sessions
 * Basic transmission and receiving
 * Different setups and performance benchmarks
 * Ranging and simple communication examples
 * ...
