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
 * Arduino global library (source file) working with the DW1000 library 
 * for the Decawave DW1000 UWB transceiver IC.
 */
 
 
 
#include "DW1000Ranging.h"

DW1000RangingClass DW1000Ranging;

//module type (anchor or tag)
int DW1000RangingClass::_type;
// message flow state
volatile byte DW1000RangingClass::_expectedMsgId;
// message sent/received state
volatile boolean DW1000RangingClass::_sentAck=false;
volatile boolean DW1000RangingClass::_receivedAck=false;
// protocol error state
boolean DW1000RangingClass::_protocolFailed=false;
// timestamps to remember
DW1000Time DW1000RangingClass::_timePollSent;
DW1000Time DW1000RangingClass::_timePollReceived;
DW1000Time DW1000RangingClass::_timePollAckSent;
DW1000Time DW1000RangingClass::_timePollAckReceived;
DW1000Time DW1000RangingClass::_timeRangeSent;
DW1000Time DW1000RangingClass::_timeRangeReceived;
// last computed range/time
DW1000Time DW1000RangingClass::_timeComputedRange;
// data buffer
byte DW1000RangingClass::data[LEN_DATA];
// reset line to the chip
unsigned int DW1000RangingClass::_RST;
unsigned int DW1000RangingClass::_SS;
// watchdog and reset period
unsigned long DW1000RangingClass::_lastActivity;
unsigned long DW1000RangingClass::_resetPeriod;
// reply times (same on both sides for symm. ranging)
unsigned int DW1000RangingClass::_replyDelayTimeUS;
// ranging counter (per second)
unsigned int DW1000RangingClass::_successRangingCount=0;
unsigned long DW1000RangingClass::_rangingCountPeriod=0;
//ranging variables
float DW1000RangingClass::_distance;
float DW1000RangingClass::_RXPower=0;
float DW1000RangingClass::_FPPower=0;
float DW1000RangingClass::_quality=0;

//Here our handlers
void (*DW1000RangingClass::_handleNewRange)(void) = 0;



/* ###########################################################################
 * #### Init and end #######################################################
 * ######################################################################### */


void DW1000RangingClass::init(unsigned int RST, unsigned int SS){
    // reset line to the chip
    _RST = RST;
    _SS = SS;
    _resetPeriod = DEFAULT_RESET_PERIOD;
    // reply times (same on both sides for symm. ranging)
    _replyDelayTimeUS = DEFAULT_REPLY_DELAY_TIME;
    
    DW1000.begin(0, RST);
    DW1000.select(SS);
}

void DW1000RangingClass::configureMode(const byte mode[]){ 
    DW1000.enableMode(mode);
    DW1000.commitConfiguration();

    
}

void DW1000RangingClass::configureNetwork(unsigned int deviceAddress, unsigned int networkId){
    // general configuration
    DW1000.newConfiguration();
    DW1000.setDefaults();
    DW1000.setDeviceAddress(deviceAddress);
    DW1000.setNetworkId(networkId);
    
}

void DW1000RangingClass::generalStart(){
    // attach callback for (successfully) sent and received messages
    DW1000.attachSentHandler(handleSent);
    DW1000.attachReceivedHandler(handleReceived);
    // anchor starts in receiving mode, awaiting a ranging poll message
    
    
    if(DEBUG){
        // DEBUG monitoring
        Serial.println("### DW1000-arduino-ranging-library ###");
        // initialize the driver
        
        
        Serial.println("Committed configuration ...");
        // DEBUG chip info and registers pretty printed
        char msg[256];
        DW1000.getPrintableDeviceIdentifier(msg);
        Serial.print("Device ID: "); Serial.println(msg);
        DW1000.getPrintableExtendedUniqueIdentifier(msg);
        Serial.print("Unique ID: "); Serial.println(msg);
        DW1000.getPrintableNetworkIdAndShortAddress(msg);
        Serial.print("Network ID & Device Address: "); Serial.println(msg);
        DW1000.getPrintableDeviceMode(msg);
        Serial.print("Device mode: "); Serial.println(msg);
    }
    
    
    // anchor starts in receiving mode, awaiting a ranging poll message
    receiver();
    noteActivity();
    // for first time ranging frequency computation
    _rangingCountPeriod = millis();
}


void DW1000RangingClass::startAsAnchor(){
    //general start:
    generalStart();
    
    //defined type as anchor
    _type=ANCHOR;
    
    if(DEBUG){
        Serial.println("### START AS ANCHOR ###");
    }
}

void DW1000RangingClass::startAsTag(){
    generalStart();
    //defined type as anchor
    _type=TAG;
    
    if(DEBUG){
        Serial.println("### START AS TAG ###");
    }
    //we can start to poll: (this is the TAG which start to poll)
    transmitPoll();
}


/* ###########################################################################
 * #### Setters and Getters ##################################################
 * ######################################################################### */

void DW1000RangingClass::setReplyTime(unsigned int replyDelayTimeUs){ _replyDelayTimeUS=replyDelayTimeUs;}
void DW1000RangingClass::setResetPeriod(unsigned long resetPeriod){ _resetPeriod=resetPeriod;}


float DW1000RangingClass::getRange(){ return _distance; };
float DW1000RangingClass::getRXPower(){ return _RXPower; };
float DW1000RangingClass::getFPPower(){ return _FPPower; };
float DW1000RangingClass::getQuality(){ return _quality; };



/* ###########################################################################
 * #### Public methods #######################################################
 * ######################################################################### */

void DW1000RangingClass::checkForReset(){
    long curMillis = millis();
    if(!_sentAck && !_receivedAck) {
        // check if inactive
        if(curMillis - _lastActivity > _resetPeriod) {
            resetInactive();
        }
        return;
    }
}



void DW1000RangingClass::loop(){
    //we check if needed to reset !
    checkForReset();
        
    
    if(_sentAck){
        _sentAck = false;
        //A msg was sent. We launch the ranging protocole when a message was sent
        if(_type==ANCHOR){
            if(data[0] == POLL_ACK) {
                DW1000.getTransmitTimestamp(_timePollAckSent);
                noteActivity();
            }
        }
        else if(_type==TAG){
            if(data[0] == POLL) {
                DW1000.getTransmitTimestamp(_timePollSent);
                //Serial.print("Sent POLL @ "); Serial.println(timePollSent.getAsFloat());
            } else if(data[0] == RANGE) {
                DW1000.getTransmitTimestamp(_timeRangeSent);
                noteActivity();
            }
        }
    }
    
    //check for new received message
    if(_receivedAck){
        _receivedAck=false;
        //we read the datas from the modules:
        // get message and parse
        DW1000.getData(data, LEN_DATA);
        
        //then we proceed to range protocole
        if(_type==ANCHOR){
            if(data[0] != _expectedMsgId) {
                // unexpected message, start over again (except if already POLL)
                _protocolFailed = true;
            }
            if(data[0] == POLL) {
                // on POLL we (re-)start, so no protocol failure
                _protocolFailed = false;
                DW1000.getReceiveTimestamp(_timePollReceived);
                _expectedMsgId = RANGE;
                transmitPollAck();
                noteActivity();
            }
            else if(data[0] == RANGE) {
                DW1000.getReceiveTimestamp(_timeRangeReceived);
                _expectedMsgId = POLL;
                if(!_protocolFailed) {
                    _timePollSent.setTimestamp(data+1);
                    _timePollAckReceived.setTimestamp(data+6);
                    _timeRangeSent.setTimestamp(data+11);
                    
                    // (re-)compute range as two-way ranging is done
                    computeRangeAsymmetric(); // CHOSEN RANGING ALGORITHM
                    
                    float distance=_timeComputedRange.getAsMeters();
                    _RXPower=DW1000.getReceivePower();
                    float rangeBias=rangeRXCorrection(_RXPower);
                    _distance=distance-rangeBias;
                    
                    //we wend the range to TAG
                    transmitRangeReport();
                    
                    _FPPower=DW1000.getFirstPathPower();
                    _quality=DW1000.getReceiveQuality();
                    
                    //we have finished our range computation. We send the corresponding handler
                    
                    if(_handleNewRange != 0) {
                        (*_handleNewRange)();
                    }
                    
                }
                else {
                    transmitRangeFailed();
                }
                
                noteActivity();
            }
        }
        else if(_type==TAG){
            // get message and parse
            if(data[0] != _expectedMsgId) {
                // unexpected message, start over again
                //Serial.print("Received wrong message # "); Serial.println(msgId);
                _expectedMsgId = POLL_ACK;
                transmitPoll();
                return;
            }
            if(data[0] == POLL_ACK) {
                DW1000.getReceiveTimestamp(_timePollAckReceived);
                _expectedMsgId = RANGE_REPORT;
                transmitRange();
                noteActivity();
            }
            else if(data[0] == RANGE_REPORT) {
                _expectedMsgId = POLL_ACK;
                float curRange;
                memcpy(&curRange, data+1, 4);
                float curRXPower;
                memcpy(&curRXPower, data+5, 4);
                //we have a new range to save !
                _distance=curRange;
                _RXPower=curRXPower;
                
                //We can call our handler ! 
                if(_handleNewRange != 0){
                    (*_handleNewRange)();
                }
                
                //we start again ranging
                transmitPoll();
                noteActivity();
            }
            else if(data[0] == RANGE_FAILED) {
                _expectedMsgId = POLL_ACK;
                transmitPoll();
                noteActivity();
            }
        }
         
    }
}








/* ###########################################################################
 * #### Private methods and Handlers for transmit & Receive reply ############
 * ######################################################################### */


void DW1000RangingClass::handleSent() {
    // status change on sent success
    _sentAck = true;
    
}

void DW1000RangingClass::handleReceived() {
    // status change on received success
    _receivedAck = true;
}


void DW1000RangingClass::noteActivity() {
    // update activity timestamp, so that we do not reach "resetPeriod"
    _lastActivity = millis();
}

void DW1000RangingClass::resetInactive() {
    //if inactive
    if(_type==ANCHOR){
        _expectedMsgId = POLL;
        receiver();
    }
    else if(_type==TAG){
        _expectedMsgId = POLL_ACK;
        transmitPoll();
    }
    noteActivity();
}



/* ###########################################################################
 * #### Methods for ranging protocole (anchor) ###############################
 * ######################################################################### */

void DW1000RangingClass::transmitPollAck() {
    DW1000.newTransmit();
    DW1000.setDefaults();
    data[0] = POLL_ACK;
    // delay the same amount as ranging tag
    DW1000Time deltaTime = DW1000Time(_replyDelayTimeUS, DW1000Time::MICROSECONDS);
    DW1000.setDelay(deltaTime);
    DW1000.setData(data, LEN_DATA);
    DW1000.startTransmit();
}

void DW1000RangingClass::transmitRangeReport() {
    DW1000.newTransmit();
    DW1000.setDefaults();
    data[0] = RANGE_REPORT;
    // write final ranging result
    float curRange=_distance;
    float curRXPower=_RXPower;
    //We add the Range and then the RXPower
    memcpy(data+1, &curRange, 4);
    memcpy(data+5, &curRXPower, 4);
    
    DW1000.setData(data, LEN_DATA);
    DW1000.startTransmit();
}

void DW1000RangingClass::transmitRangeFailed() {
    DW1000.newTransmit();
    DW1000.setDefaults();
    data[0] = RANGE_FAILED;
    DW1000.setData(data, LEN_DATA);
    DW1000.startTransmit();
}

void DW1000RangingClass::receiver() {
    DW1000.newReceive();
    DW1000.setDefaults();
    // so we don't need to restart the receiver manually
    DW1000.receivePermanently(true);
    DW1000.startReceive();
}





/* ###########################################################################
 * #### Methods for ranging protocole (TAG) #################################
 * ######################################################################### */


void DW1000RangingClass::transmitPoll() {
    DW1000.newTransmit();
    DW1000.setDefaults();
    data[0] = POLL;
    DW1000.setData(data, LEN_DATA);
    DW1000.startTransmit();
}


void DW1000RangingClass::transmitRange() {
    DW1000.newTransmit();
    DW1000.setDefaults();
    data[0] = RANGE;
    // delay sending the message and remember expected future sent timestamp
    DW1000Time deltaTime = DW1000Time(_replyDelayTimeUS, DW1000Time::MICROSECONDS);
    _timeRangeSent = DW1000.setDelay(deltaTime);
    _timePollSent.getTimestamp(data+1);
    _timePollAckReceived.getTimestamp(data+6);
    _timeRangeSent.getTimestamp(data+11);
    DW1000.setData(data, LEN_DATA);
    DW1000.startTransmit();
    //Serial.print("Expect RANGE to be sent @ "); Serial.println(timeRangeSent.getAsFloat());
}








/* ###########################################################################
 * #### Methods for range computation and corrections  #######################
 * ######################################################################### */


void DW1000RangingClass::computeRangeAsymmetric() {
    // asymmetric two-way ranging (more computation intense, less error prone)
    DW1000Time round1 = (_timePollAckReceived-_timePollSent).wrap();
    DW1000Time reply1 = (_timePollAckSent-_timePollReceived).wrap();
    DW1000Time round2 = (_timeRangeReceived-_timePollAckSent).wrap();
    DW1000Time reply2 = (_timeRangeSent-_timePollAckReceived).wrap();
    DW1000Time tof = (round1 * round2 - reply1 * reply2) / (round1 + round2 + reply1 + reply2);
    // set tof timestamp
    _timeComputedRange.setTimestamp(tof);
}

void DW1000RangingClass::computeRangeSymmetric() {
    // symmetric two-way ranging (less computation intense, more error prone on clock drift)
    DW1000Time tof = ((_timePollAckReceived-_timePollSent)-(_timePollAckSent-_timePollReceived) +
                      (_timeRangeReceived-_timePollAckSent)-(_timeRangeSent-_timePollAckReceived)) * 0.25f;
    // set tof timestamp
    _timeComputedRange.setTimestamp(tof);
}


// ----  Range RX correction ----



// {RSL, PRF 16MHz, PRF 64 MHz}
float DW1000RangingClass::_bias[17][3]={
    {-61, -19.8, -11.0},
    {-63, -18.7, -10.5},
    {-65, -17.9, -10.0},
    {-67, -16.3, -9.3},
    {-69, -14.3, -8.2},
    {-71, -12.7, -6.9},
    {-73, -10.9, -5.1},
    {-75, -8.4,  -2.7},
    {-77, -5.9,   0.0},
    {-79, -3.1,   2.1},
    {-81,  0.0,   3.5},
    {-83,  3.6,   4.2},
    {-85,  6.5,   4.9},
    {-87,  8.4,   6.2},
    {-89,  9.7,   7.1},
    {-91,  10.6,  7.6},
    {-93,  11.0,  8.1},
};

float DW1000RangingClass::rangeRXCorrection(float RXPower){
    byte PRF=DW1000.getPulseFrequency();
    float rangeBias=0;
    if(PRF==DW1000.TX_PULSE_FREQ_16MHZ)
    {
        Serial.println("16Mhz");
        rangeBias=computeRangeBias(RXPower, 1);
    }
    else if(PRF==DW1000.TX_PULSE_FREQ_64MHZ)
    {
        rangeBias=computeRangeBias(RXPower, 2);
    }
    
    return rangeBias;
}


float DW1000RangingClass::computeRangeBias(float RXPower, int prf)
{
    //We test first boundary
    if(RXPower>=_bias[0][0])
        return _bias[0][prf];
    
    //we test last boundary
    if(RXPower<=_bias[16][0])
        return _bias[16][prf];
    
    for(int i=1; i<17; i++){
        //we search for the position we are. All is in negative !
        if(RXPower<_bias[i-1][0] && RXPower>_bias[i][0]){
            //we have our position i. We now need to calculate the line
            float a=(_bias[i-1][prf]-_bias[i][prf])/(_bias[i-1][0]-_bias[i][0]);
            float b=_bias[i-1][prf] - a * _bias[i-1][0];
            //return our bias
            return (a*RXPower + b)/100;
        }
    }
    
}



