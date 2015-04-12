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
  dw.initialize();
  // general configuration
  dw.newConfiguration(); 
  dw.setDeviceAddress(5);
  dw.setNetworkId(10);
  dw.setFrameFilter(true);
  dw.commitConfiguration();
  // print chip info
  Serial.print("Device ID: "); Serial.println(dw.getPrintableDeviceIdentifier());
  Serial.print("Unique ID: "); Serial.println(dw.getPrintableExtendedUniqueIdentifier());
  Serial.print("Network ID & Device Address: "); Serial.println(dw.getPrintableNetworkIdAndShortAddress());
  // DEBUG 
  Serial.println(DW1000::getPrettyBytes(dw.getSystemConfiguration(), 4));
  Serial.println(DW1000::getPrettyBytes(dw.getNetworkIdAndShortAddress(), 4));
}

void loop() {
    // TODO proper sender config and receiver test
    // transmit some data
    dw.newTransmit();
    {
      dw.setDefaults();
      byte data[4] = {'t', 'e', 's', 't'};
      dw.setData(data, 4);
      dw.startTransmit();
    }
    while(!dw.isTransmitDone()) {
      Serial.println("No ...");
      delay(1000); 
    }
    Serial.println("YES ...");
    // wait a bit
    delay(2000);
}
