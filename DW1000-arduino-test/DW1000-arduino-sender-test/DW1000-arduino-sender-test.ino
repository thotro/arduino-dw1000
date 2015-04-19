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
  dw.setDeviceAddress(5);
  dw.setNetworkId(10);
  dw.setFrameFilter(false);
  //dw.interruptOnSent(true);
  dw.commitConfiguration();
  Serial.println("Committed configuration ...");
  // DEBUG chip info and registers pretty printed
  Serial.print("Device ID: "); Serial.println(dw.getPrintableDeviceIdentifier());
  Serial.print("Unique ID: "); Serial.println(dw.getPrintableExtendedUniqueIdentifier());
  Serial.print("Network ID & Device Address: "); Serial.println(dw.getPrintableNetworkIdAndShortAddress());
  Serial.println(dw.getPrettyBytes(SYS_CFG, LEN_SYS_CFG));
  Serial.println(dw.getPrettyBytes(PANADR, LEN_PANADR));
  Serial.println(dw.getPrettyBytes(SYS_MASK, LEN_SYS_MASK));
  // attach interrupt and ISR
  pinMode(INT0, INPUT);
  digitalWrite(INT0, HIGH);
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
  // TODO proper sender config and receiver test
  // transmit some data
  Serial.print("Transmitting packet ... #"); Serial.println(numSent+1);
  dw.newTransmit();
  {
    dw.setDefaults();
    byte data[4] = {'t', 'e', 's', 't'};
    dw.setData(data, 4);
    dw.startTransmit();
  }
  // Interrupt version of transmit: Confirmation of ISR status change
  if(sent) {
    Serial.print("Processed packet ... #"); Serial.println(numSent);
    sent = false;
  }
  // Polling version of transmit (probably not really useful anymore)
  /* while(!dw.isTransmitDone()) {
    delay(10); 
  } 
  numSent++;
  Serial.print("Handeling packet ... #"); Serial.println(numSent);
  Serial.print("Processed packet ... #"); Serial.println(numSent);
  */
  // wait a bit
  delay(1000);
}
