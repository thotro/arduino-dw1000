/*
 * Copyright (c) 2015 by Thomas Trojer <thomas@trojer.net>
 * Decawave DW1000 library for arduino.
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 *
 * Use this to test two-way ranging functionality with two
 * DW1000. This is the tag component's code which polls for range computation. 
 * Addressing and frame filtering is currently done in a custom way, as no MAC 
 * features are implemented yet.
 *
 * Complements the "DW1000-arduino-ranging-anchor" sketch. 
 */

#include <SPI.h>
#include <DW1000.h>

// toggle state
#define SENDER true
#define RECEIVER false
// toggle and message RX/TX
// first node
//volatile boolean trxToggle = SENDER;
// second node
volatile boolean trxToggle = RECEIVER;
volatile boolean trxAck = false;
//volatile unsigned int msgNum = 0;
String msg;
// reset line to the chip
int RST = 9;

void setup() {
  // DEBUG monitoring
  Serial.begin(9600);
  Serial.println("### DW1000-arduino-ping-pong-test ###");
  // initialize the driver
  DW1000.begin(SS, RST, 0);
  Serial.println("DW1000 initialized ...");
  // general configuration
  DW1000.newConfiguration();
  DW1000.setDefaults();
  DW1000.setDeviceAddress(1);
  DW1000.setNetworkId(10);
  DW1000.setFrameFilter(false);
  DW1000.commitConfiguration();
  Serial.println("Committed configuration ...");
  // DEBUG chip info and registers pretty printed
  Serial.print("Device ID: "); Serial.println(DW1000.getPrintableDeviceIdentifier());
  Serial.print("Unique ID: "); Serial.println(DW1000.getPrintableExtendedUniqueIdentifier());
  Serial.print("Network ID & Device Address: "); Serial.println(DW1000.getPrintableNetworkIdAndShortAddress());
  // attach callback for (successfully) sent and received messages
  DW1000.attachSentHandler(handleSent);
  DW1000.attachReceivedHandler(handleReceived);
  // anchor starts by transmitting a POLL message
  if(trxToggle == SENDER) {
    msg = "Ping";
    receiver();
    transmit();
  } else {
    msg = "Pong";
    receiver();
  }
}

void handleSent() {
  // status change on sent success
  trxAck = true;
}

void handleReceived() {
  // status change on received success
  trxAck = true;
}

void transmit() {
  DW1000.newTransmit();
  DW1000.setDefaults();
  DW1000.setData(msg);
  DW1000.startTransmit();
}

void receiver() {
  DW1000.newReceive();
  DW1000.setDefaults();
  // so we don't need to restart the receiver manually
  DW1000.permanentReceive(true);
  DW1000.startReceive();
}

void loop() {
  if(!trxAck) {
    return;
  }
  // continue on any success confirmation
  trxAck = false; 
  trxToggle = !trxToggle;
  if(trxToggle == SENDER) {
    // fomerly in receiving mode
    String rxMsg; 
    DW1000.getData(rxMsg);
    Serial.print("Received | "); Serial.println(rxMsg);
    transmit();
  } else {
    Serial.print("Transmitted | "); Serial.println(msg);
  }
}

