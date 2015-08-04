
#include <SPI.h> 
#include "DW1000Ranging.h"
#include <DW1000.h>


void setup() {
  Serial.begin(115200);
  delay(1000);
  //init the configuration
  DW1000Ranging.init(9, 10); //Reset and CS pin
  DW1000Ranging.configureNetwork(1, 10); //device Address and network ID 
  DW1000Ranging.configureMode(DW1000.MODE_LONGDATA_RANGE_ACCURACY);
  //define the sketch as anchor. It will be great to dynamically change the type of module
  DW1000Ranging.attachNewRange(newRange);
  DW1000Ranging.startAsAnchor();
}

void loop() { 
  DW1000Ranging.loop();
  
  
}

void newRange(){
  Serial.print("Range: "); Serial.print(DW1000Ranging.getRange()); Serial.print(" m"); 
  Serial.print("\t RX power: "); Serial.print(DW1000Ranging.getFPPower()); Serial.println(" dBm");
  
}
