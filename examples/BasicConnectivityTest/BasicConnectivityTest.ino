/*
 * Copyright (c) 2015 by Thomas Trojer <thomas@trojer.net>
 * Decawave DW1000 library for arduino.
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 *
 * Use this to test connectivity with your DW1000 from Arduino.
 * It performs an arbitrary setup of the chip and prints some information.
 */

#include <SPI.h>
#include <DW1000.h>

// reset line to the chip
int RST = 9;

void setup() {
  // DEBUG monitoring
  Serial.begin(9600);
  // initialize the driver
  DW1000.begin(0, RST);
  DW1000.select(SS);
  Serial.println("DW1000 initialized ...");
  // general configuration
  DW1000.newConfiguration(); 
  DW1000.setDeviceAddress(5);
  DW1000.setNetworkId(10);
  DW1000.commitConfiguration();
  Serial.println("Committed configuration ...");
}

void loop() {
    // wait a bit
    delay(1000);
    // DEBUG chip info and registers pretty printed
    char msg[1024];
    DW1000.getPrintableDeviceIdentifier(msg);
    Serial.print("Device ID: "); Serial.println(msg);
    DW1000.getPrintableExtendedUniqueIdentifier(msg);
    Serial.print("Unique ID: "); Serial.println(msg);
    DW1000.getPrintableNetworkIdAndShortAddress(msg);
    Serial.print("Network ID & Device Address: "); Serial.println(msg);
    DW1000.getPrintableDeviceMode(msg); 
    Serial.print("Device mode: "); Serial.println(msg);
    // wait a bit
    delay(10000);
}
