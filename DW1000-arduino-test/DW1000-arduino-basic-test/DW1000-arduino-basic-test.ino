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
// chip driver instances with chip select and reset
DW1000 dw = DW1000(SS, RST);

void setup() {
  // DEBUG monitoring
  Serial.begin(9600);
  // initialize the driver
  dw.initialize();
  Serial.println("DW1000 initialized ...");
  // general configuration
  dw.newConfiguration(); 
  dw.setDeviceAddress(5);
  dw.setNetworkId(10);
  dw.commitConfiguration();
  Serial.println("Committed configuration ...");
}

void loop() {
    // wait a bit
    delay(1000);
    // DEBUG chip info and registers pretty printed
    Serial.print("Device ID: "); Serial.println(dw.getPrintableDeviceIdentifier());
    Serial.print("Unique ID: "); Serial.println(dw.getPrintableExtendedUniqueIdentifier());
    Serial.print("Network ID & Device Address: "); Serial.println(dw.getPrintableNetworkIdAndShortAddress());
    // DEBUG print device tuning results
    Serial.println(dw.getPrettyBytes(AGC_TUNE, AGC_TUNE1_SUB, LEN_AGC_TUNE1));
    Serial.println(dw.getPrettyBytes(AGC_TUNE, AGC_TUNE2_SUB, LEN_AGC_TUNE2));
    // wait a bit
    delay(10000);
}
