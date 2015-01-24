/*
 * Copyright (c) 2014 by Thomas Trojer <thomas@trojer.net>
 * Decawave DW1000 library for arduino.
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 *
 * Use this to test connectivity with your DW1000 from Arduino.
 *
 * At the moment it performs a really stupid test of toggling a
 * configuration bit on the chip.
 */

#include <SPI.h>
#include <DW1000.h>

DW1000 dw = DW1000(SS);
boolean toggle = true;

void setup() {
  // for debugging
  Serial.begin(9600);
  // print chip info
  Serial.print("Device ID: "); Serial.println(dw.readDeviceIdentifier());
  Serial.print("Chip Select: "); Serial.println(dw.getChipSelect());
  // load the current chip config
  dw.loadSystemConfiguration();
}

void loop() {  
    byte* cursyscfg = dw.getSystemConfiguration();
    Serial.print(cursyscfg[0]); Serial.print(" "); Serial.print(cursyscfg[1]); Serial.print(" "); Serial.print(cursyscfg[2]); Serial.print(" "); Serial.println(cursyscfg[3]);
    
    dw.setFrameFilter(toggle);
    toggle = !toggle;
    
    // wait a bit
    delay(2000);
}
