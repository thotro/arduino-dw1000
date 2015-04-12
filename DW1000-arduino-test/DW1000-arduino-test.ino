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
  // initialize the driver
  delay(100);
  Serial.print("Device ID: "); Serial.println(dw.getDeviceIdentifier());
  // general configuration
  dw.newConfiguration(); 
  dw.setDeviceAddress(0x0001);
  dw.setNetworkId(0x0000);
  //dw.setFrameFilter(false);
  dw.commitConfiguration();
  // print chip info
  Serial.print("Unique ID: "); Serial.println(dw.getExtendedUniqueIdentifier());
  Serial.print("Network ID & Device Address: "); Serial.println(dw.getNetworkIdAndShortAddress());
}

void loop() {
    // transmit some data
    /*dw.newTransmit();
    {
      dw.setDefaults();
      byte data[6] = {'l', 'a', 'l', 'e', 'l', 'u'};
      dw.setData(data, 6);
      dw.startTransmit();
    }
    while(!dw.isTransmitDone()) {
      //Serial.println("No ...");
      delay(250); 
    }
    Serial.println("YES ...");*/
    
    // wait a bit
    delay(2000);
}
