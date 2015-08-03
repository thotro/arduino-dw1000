/*
 * Copyright (c) 2015 by Thomas Trojer <thomas@trojer.net> and Leopold Sayous <leosayous@gmail.com>
 * Decawave DW1000 library for arduino.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * @file DW1000Ranging.h
 * Arduino global library (header file) working with the DW1000 library
 * for the Decawave DW1000 UWB transceiver IC.
 */

#include "DW1000.h"
#include "DW1000Time.h"

// messages used in the ranging protocol
#define POLL 0
#define POLL_ACK 1
#define RANGE 2
#define RANGE_REPORT 3
#define RANGE_FAILED 255

#define LEN_DATA 16

//Default Pin for module:
#define DEFAULT_RST_PIN 9
#define DEFAULT_SPI_SS_PIN 10

//Default value
//in ms
#define DEFAULT_RESET_PERIOD 250
//in us
#define DEFAULT_REPLY_DELAY_TIME 3000

//sketch type (anchor or tag)
#define TAG 0
#define ANCHOR 1

//debug mode
#ifndef DEBUG
#define DEBUG true
#endif


class DW1000RangingClass {
  public:
    //variables 
    // data buffer
    static byte data[LEN_DATA];
    
    
    //initialisation
    static void init(unsigned int RST=DEFAULT_RST_PIN, unsigned int SS=DEFAULT_SPI_SS_PIN);
    static void configureNetwork(unsigned int deviceAddress, unsigned int networkId);
    static void configureMode(const byte mode[]);
    static void generalStart();
    static void startAsAnchor();
    static void startAsTag();
    
    //setters
    static void setReplyTime(unsigned int replyDelayTimeUs);
    static void setResetPeriod(unsigned long resetPeriod);
    
    //getters
    static float getRange();
    static float getRXPower();
    static float getFPPower();
    static float getQuality(); 
    
    
    //ranging functions
    static void loop();
    
    
    //Handlers:
    static void attachNewRange(void (*handleNewRange)(void)) {_handleNewRange = handleNewRange; }
    
    
    //if new receiver is available
    //static boolean available();
    //transmit a message
    //static void write();
    
    
        
  private:
    //Handlers:
    static void (*_handleNewRange)(void);
    
    //sketch type (tag or anchor)
    static int _type; //0 for tag and 1 for anchor
    // message flow state
    static volatile byte _expectedMsgId;
    // message sent/received state
    static volatile boolean _sentAck;
    static volatile boolean _receivedAck;
    // protocol error state
    static boolean _protocolFailed;
    // timestamps to remember
    static DW1000Time _timePollSent;
    static DW1000Time _timePollReceived;
    static DW1000Time _timePollAckSent;
    static DW1000Time _timePollAckReceived;
    static DW1000Time _timeRangeSent;
    static DW1000Time _timeRangeReceived;
    // last computed range/time
    static DW1000Time _timeComputedRange;
    // reset line to the chip
    static unsigned int _RST;
    static unsigned int _SS;
    // watchdog and reset period
    static unsigned long _lastActivity;
    static unsigned long _resetPeriod;
    // reply times (same on both sides for symm. ranging)
    static unsigned int _replyDelayTimeUS;
    // ranging counter (per second)
    static unsigned int _successRangingCount;
    static unsigned long _rangingCountPeriod;
    static float _bias[17][3];

    //ranging variables
    static float _distance;
    static float _RXPower;
    static float _FPPower;
    static float _quality;
    
    
    //methods
    static void handleSent();
    static void handleReceived();
    static void noteActivity();
    static void resetInactive();
    
    //global functions:
    static void checkForReset();
    
    //for ranging protocole (ANCHOR)
    static void transmitPollAck();
    static void transmitRangeReport();
    static void transmitRangeFailed();
    static void receiver();
    
    //for ranging protocole (TAG)
    static void transmitPoll();
    static void transmitRange();
    
    //methods for range computation
    static void computeRangeAsymmetric();
    static void computeRangeSymmetric();
    static float rangeRXCorrection(float RXPower);
    static float computeRangeBias(float RXPower, int prf);
    
 

};

extern DW1000RangingClass DW1000Ranging;
