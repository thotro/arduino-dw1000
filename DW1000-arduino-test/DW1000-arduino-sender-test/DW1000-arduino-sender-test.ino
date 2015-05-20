/*
 * Copyright (c) 2015 by Thomas Trojer <thomas@trojer.net>
 * Decawave DW1000 library for arduino.
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 *
 * Use this to test simple sender/receiver functionality with two
 * DW1000. Complements the "DW1000-arduino-receiver-test" sketch. 
 */

#include <SPI.h>
#include <DW1000.h>

// DEBUG packet sent status and count
volatile boolean sent = false;
volatile int numSent = 0;
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
  dw.setDefaults();
  dw.setDeviceAddress(5);
  dw.setNetworkId(10);
  dw.setFrameFilter(false);
  dw.interruptOnSent(true);
  dw.commitConfiguration();
  Serial.println("Committed configuration ...");
  // DEBUG chip info and registers pretty printed
  Serial.print("Device ID: "); Serial.println(dw.getPrintableDeviceIdentifier());
  Serial.print("Unique ID: "); Serial.println(dw.getPrintableExtendedUniqueIdentifier());
  Serial.print("Network ID & Device Address: "); Serial.println(dw.getPrintableNetworkIdAndShortAddress());
  Serial.println(dw.getPrettyBytes(SYS_CFG, NO_SUB, LEN_SYS_CFG));
  Serial.println(dw.getPrettyBytes(PANADR, NO_SUB, LEN_PANADR));
  Serial.println(dw.getPrettyBytes(SYS_MASK, NO_SUB, LEN_SYS_MASK));
  // attach interrupt and ISR
  pinMode(INT0, INPUT);
  attachInterrupt(0, serviceIRQ, FALLING);
  Serial.println("Interrupt attached ...");
}

void serviceIRQ() {
  if(sent) {
    return;
  }
  // "NOP" ISR
  sent = true;
  numSent++;
}

void loop() {
  // transmit some data
  Serial.print("Transmitting packet ... #"); Serial.println(numSent+1);
  dw.newTransmit();
  dw.setDefaults();
  String msg = "Hello DW1000";
  dw.setData(msg);
  dw.startTransmit();
  // enter on confirmation of ISR status change (successfully sent)
  if(sent) {
    Serial.print("Processed packet ... #"); Serial.println(numSent);
    sent = false;
  }
  // wait a bit
  delay(500);
  //Serial.println(dw.getPrettyBytes(SYS_STATUS, NO_SUB, LEN_SYS_STATUS));
}
