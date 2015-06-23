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
 * DW1000. Complements the "DW1000-arduino-sender-test" sketch. 
 */

#include <SPI.h>
#include <DW1000.h>

// DEBUG packet sent status and count
volatile boolean received = false;
volatile boolean error = false;
volatile int numReceived = 0;
String message;
// reset line to the chip
int RST = 9;

void setup() {
  // DEBUG monitoring
  Serial.begin(9600);
  Serial.println("### DW1000-arduino-receiver-test ###");
  // initialize the driver
  DW1000.begin(0, RST);
  DW1000.select(SS);
  Serial.println("DW1000 initialized ...");
  // general configuration
  DW1000.newConfiguration();
  DW1000.setDefaults();
  DW1000.setDeviceAddress(6);
  DW1000.setNetworkId(10);
  DW1000.setFrameFilter(false);
  DW1000.commitConfiguration();
  Serial.println("Committed configuration ...");
  // DEBUG chip info and registers pretty printed
  Serial.print("Device ID: "); Serial.println(DW1000.getPrintableDeviceIdentifier());
  Serial.print("Unique ID: "); Serial.println(DW1000.getPrintableExtendedUniqueIdentifier());
  Serial.print("Network ID & Device Address: "); Serial.println(DW1000.getPrintableNetworkIdAndShortAddress());
  // attach callback for (successfully) received messages
  DW1000.attachReceivedHandler(handleReceived);
  DW1000.attachReceiveErrorHandler(handleReceiveError);
  // start reception
  receiver();
}

void handleReceived() {
  // status change on reception success
  received = true;
}

void handleReceiveError() {
  error = true;
}

void receiver() {
  DW1000.newReceive();
  DW1000.setDefaults();
  // so we don't need to restart the receiver manually
  DW1000.receivePermanently(true);
  DW1000.startReceive();
}

void loop() {
    // enter on confirmation of ISR status change (successfully received)
    if(received) {
      uint64_t a = 0;
      numReceived++;
      // get data as string
      DW1000.getData(message);
      Serial.print("Received message ... #"); Serial.println(numReceived);
      Serial.print("Data is ... "); Serial.println(message);
      received = false;
    }
    if(error) {
      Serial.println("Error receiving a message");
      error = false;
      DW1000.getData(message);
      Serial.print("Error data is ... "); Serial.println(message);
    }
}
