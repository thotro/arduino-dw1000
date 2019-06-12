#include <SPI.h>
#include <DW1000Ranging.h>
#include <ArduinoJson.h>

enum class Events : byte { NEW_RANGE = 0, NEW_DEVICE = 1, INACTIVE_DEVICE = 2, };

const uint8_t PIN_RST = 9; // reset pin
const uint8_t PIN_IRQ = 3; // irq pin
const uint8_t PIN_SS = SS; // spi select pin
const uint16_t ANTENNA_DELAY = 0;
const char ADDRESS[] = "B1:00:00:00:B1:6B:00:B5";

void setup() {
  Serial.begin(115200);

  //init the configuration
  DW1000Ranging.initCommunication(PIN_RST, PIN_SS, PIN_IRQ);
  DW1000Ranging.attachNewRange(newRange);
  DW1000Ranging.attachNewDevice(newDevice);
  DW1000Ranging.attachInactiveDevice(inactiveDevice);

  // General configuration
  DW1000.setAntennaDelay(ANTENNA_DELAY);

  // LED configuration
  DW1000.enableDebounceClock();
  DW1000.enableLedBlinking();
  DW1000.setGPIOMode(MSGP0, LED_MODE); // enable GPIO0/RXOKLED blinking
  DW1000.setGPIOMode(MSGP1, LED_MODE); // enable GPIO1/SFDLED blinking
  DW1000.setGPIOMode(MSGP2, LED_MODE); // enable GPIO2/RXLED blinking
  DW1000.setGPIOMode(MSGP3, LED_MODE); // enable GPIO3/TXLED blinking
  
  //we start the module as a tag
  DW1000Ranging.startAsTag(ADDRESS, DW1000.MODE_LONGDATA_RANGE_ACCURACY, false);
}

void loop() {
  DW1000Ranging.loop();
}

void newRange() {
  transmitDATA(Events::NEW_RANGE, DW1000Ranging.getDistantDevice());
}

void newDevice(DW1000Device* device) {
  transmitDATA(Events::NEW_DEVICE, device);
}

void inactiveDevice(DW1000Device* device) {
  transmitDATA(Events::INACTIVE_DEVICE, device);
}

void transmitDATA(const Events event, const DW1000Device* device) {
  StaticJsonBuffer<200> jsonBuffer;
  JsonObject& root = jsonBuffer.createObject();

  const byte* shortAddress = DW1000Ranging.getCurrentShortAddress();
  float temp, vbat;
  DW1000.getTempAndVbat(temp, vbat);

  root["type"] = static_cast<byte>(event);
  root["ta"] = shortAddress[1]*256+shortAddress[0];
  root["aa"] = device->getShortAddress();
  root["r"] = (event==Events::NEW_RANGE) ? device->getRange() : 0.0f;
  root["rxp"] = (event==Events::NEW_RANGE) ? device->getRXPower() : 0.0f;
  root["fpp"] = (event==Events::NEW_RANGE) ? device->getFPPower() : 0.0f;
  root["q"] = (event==Events::NEW_RANGE) ? device->getQuality() : 0.0f;
  root["t"] = temp;
  root["v"] = vbat;

  root.printTo(Serial);
  Serial.println();
}