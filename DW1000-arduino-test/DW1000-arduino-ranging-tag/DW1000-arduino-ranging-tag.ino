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

// messages used in the ranging protocol
#define POLL 0
#define POLL_ACK 1
#define RANGE 2
#define RANGE_REPORT 3
#define RANGE_FAILED 255
volatile byte expectedMsgId = POLL_ACK;
volatile boolean sentAck = false;
volatile boolean receivedAck = false;
// timestamps to remember
float timePollSent;
float timePollAckReceived;
float timeRangeSent;
// data buffer
#define LEN_DATA 16
byte data[LEN_DATA];
// reset line to the chip
int RST = 9;

void setup() {
  // DEBUG monitoring
  Serial.begin(9600);
  Serial.println("### DW1000-arduino-ranging-tag ###");
  // initialize the driver
  DW1000.begin(SS, RST, 0);
  Serial.println("DW1000 initialized ...");
  // general configuration
  DW1000.newConfiguration();
  DW1000.setDefaults();
  DW1000.setDeviceAddress(2);
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
  receiver();
  transmitPoll();
}

void handleSent() {
  // status change on sent success
  sentAck = true;
}

void handleReceived() {
  // status change on received success
  receivedAck = true;
}

void transmitPoll() {
  DW1000.newTransmit();
  DW1000.setDefaults();
  data[0] = POLL;
  DW1000.setData(data, LEN_DATA);
  DW1000.startTransmit();
}

void transmitRange() {
  DW1000.newTransmit();
  DW1000.setDefaults();
  data[0] = RANGE;
  // delay sending the message and remember expected future sent timestamp
  timeRangeSent = DW1000.setDelay(100, DW1000.MILLISECONDS);
  DW1000.writeFloatUsToTimestamp(timePollSent, data+1);
  DW1000.writeFloatUsToTimestamp(timePollAckReceived, data+6);
  DW1000.writeFloatUsToTimestamp(timeRangeSent, data+11);
  DW1000.setData(data, LEN_DATA);
  DW1000.startTransmit();
}

void receiver() {
  DW1000.newReceive();
  DW1000.setDefaults();
  // so we don't need to restart the receiver manually
  DW1000.receivePermanently(true);
  DW1000.startReceive();
}

void loop() {
  if(!sentAck && !receivedAck) {
    return;
  }
  // continue on any success confirmation
  if(sentAck) {
    sentAck = false; 
    // get timestamp
    float txTime = DW1000.getTransmitTimestamp();
    byte msgId = data[0];
    if(msgId == POLL) {
      timePollSent = txTime;
      Serial.print("Sent POLL @ "); Serial.println(timePollSent);
    } else if(msgId == RANGE) {
      timeRangeSent = txTime;
      Serial.print("Sent RANGE @ "); Serial.println(timeRangeSent);
    }
  } else if(receivedAck) {
    receivedAck = false;
    // get timestamp
    float rxTime = DW1000.getReceiveTimestamp();
    // get message and parse
    DW1000.getData(data, LEN_DATA);
    byte msgId = data[0];
    if(msgId != expectedMsgId) {
      // unexpected message, start over again
      Serial.print("Received wrong message # "); Serial.println(msgId);
      delay(2000);
      expectedMsgId = POLL_ACK;
      transmitPoll();
      return;
    }
    if(msgId == POLL_ACK) {
      timePollAckReceived = rxTime;
      expectedMsgId = RANGE_REPORT;
      Serial.print("Received POLL ACK @ "); Serial.println(timePollAckReceived);
      transmitRange();
    } else if(msgId == RANGE_REPORT) {
      expectedMsgId = POLL_ACK;
      float curRange = DW1000.readTimestampAsFloatUs(data+1);
      Serial.print("Received RANGE REPORT = "); Serial.println(curRange);
      delay(2000);
      transmitPoll();
    } else if(msgId == RANGE_FAILED) {
      expectedMsgId = POLL_ACK;
      Serial.println("Received RANGE FAILED");
      delay(2000);
      transmitPoll();
    }
  }
}


