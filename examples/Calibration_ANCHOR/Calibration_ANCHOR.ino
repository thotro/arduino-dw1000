#include <SPI.h>
#include <DW1000Ranging.h>

const uint8_t PIN_RST = 9; // reset pin
const uint8_t PIN_IRQ = 3; // irq pin
const uint8_t PIN_SS = SS; // spi select pin
const uint16_t ANTENNA_DELAY = 0;
const char ADDRESS[] = "B2:00:00:00:B1:6B:00:B5";

void setup() {
  Serial.begin(115200);
  
  //init the configuration
  DW1000Ranging.initCommunication(PIN_RST, PIN_SS, PIN_IRQ);

  // General configuration
  DW1000.setAntennaDelay(ANTENNA_DELAY);

  // LED configuration
  DW1000.enableDebounceClock();
  DW1000.enableLedBlinking();
  DW1000.setGPIOMode(MSGP0, LED_MODE); // enable GPIO0/RXOKLED blinking
  DW1000.setGPIOMode(MSGP1, LED_MODE); // enable GPIO1/SFDLED blinking
  DW1000.setGPIOMode(MSGP2, LED_MODE); // enable GPIO2/RXLED blinking
  DW1000.setGPIOMode(MSGP3, LED_MODE); // enable GPIO3/TXLED blinking
  
  //we start the module as an anchor
  DW1000Ranging.startAsAnchor(ADDRESS, DW1000.MODE_LONGDATA_RANGE_ACCURACY, false);
}

void loop() {
  DW1000Ranging.loop();
}