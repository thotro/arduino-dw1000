
#include <SPI.h> 
#include "DW1000Ranging.h"
#include "DW1000Device.h"  
  

void setup() {
  Serial.begin(115200);
  delay(1000);
  //init the configuration
  DW1000Ranging.initCommunication(9, 10); //Reset and CS pin 
  //network
  DW1000Ranging.configureNetwork(1, 10, DW1000.MODE_LONGDATA_RANGE_ACCURACY); //device Address, network ID and frequency
  //define the sketch as anchor. It will be great to dynamically change the type of module
  DW1000Ranging.attachNewRange(newRange);
  //we start the module as an anchor
  DW1000Ranging.startAsAnchor("82:17:5B:D5:A9:9A:E2:9C");
}

void loop() { 
  DW1000Ranging.loop(); 
}

void newRange(){  
  Serial.print("from: "); Serial.print(DW1000Ranging.getDistantDevice()->getShortAddress(), HEX);
  Serial.print("\t Range: "); Serial.print(DW1000Ranging.getDistantDevice()->getRange()); Serial.print(" m"); 
  Serial.print("\t RX power: "); Serial.print(DW1000Ranging.getDistantDevice()->getRXPower()); Serial.println(" dBm");
  
}


