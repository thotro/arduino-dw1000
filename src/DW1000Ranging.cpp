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
#include "DW1000Device.h"

DW1000RangingClass DW1000Ranging;

 
//other devices we are going to communicate with which are on our network:
DW1000Device DW1000RangingClass::_networkDevices[MAX_DEVICES];
byte DW1000RangingClass::_currentAddress[8];
byte DW1000RangingClass::_currentShortAddress[2];
byte DW1000RangingClass::_lastSentToShortAddress[2];
short DW1000RangingClass::_networkDevicesNumber=0;
short DW1000RangingClass::_lastDistantDevice=0;
DW1000Mac DW1000RangingClass::_globalMac;

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
long DW1000RangingClass::timer=0;
short DW1000RangingClass::counterForBlink=0;


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
//Here our handlers
void (*DW1000RangingClass::_handleNewRange)(void) = 0;




/* ###########################################################################
 * #### Init and end #######################################################
 * ######################################################################### */

void DW1000RangingClass::initCommunication(unsigned int RST, unsigned int SS){
    // reset line to the chip
    _RST = RST;
    _SS = SS;
    _resetPeriod = DEFAULT_RESET_PERIOD;
    // reply times (same on both sides for symm. ranging)
    _replyDelayTimeUS = DEFAULT_REPLY_DELAY_TIME;
    
    DW1000.begin(0, RST);
    DW1000.select(SS);
}
 

void DW1000RangingClass::configureNetwork(unsigned int deviceAddress, unsigned int networkId, const byte mode[]){
    // general configuration
    DW1000.newConfiguration();
    DW1000.setDefaults();
    DW1000.setDeviceAddress(deviceAddress);
    DW1000.setNetworkId(networkId);
    DW1000.enableMode(mode);
    DW1000.commitConfiguration();
    
}

void DW1000RangingClass::generalStart(){
    // attach callback for (successfully) sent and received messages
    DW1000.attachSentHandler(handleSent);
    DW1000.attachReceivedHandler(handleReceived);
    // anchor starts in receiving mode, awaiting a ranging poll message
    
    
    if(DEBUG){
        // DEBUG monitoring
        Serial.println("DW1000-arduino");
        // initialize the driver
        
        
        Serial.println("configuration..");
        // DEBUG chip info and registers pretty printed
        char msg[90];
        DW1000.getPrintableDeviceIdentifier(msg);
        Serial.print("Device ID: "); Serial.println(msg);
        DW1000.getPrintableExtendedUniqueIdentifier(msg);
        Serial.print("Unique ID: "); Serial.print(msg);
        char string[6];
        sprintf(string, "%02X:%02X", _currentShortAddress[0], _currentShortAddress[1]);
        Serial.print(" short: ");Serial.println(string);
        
        DW1000.getPrintableNetworkIdAndShortAddress(msg);
        Serial.print("Network ID & Device Address: "); Serial.println(msg);
        DW1000.getPrintableDeviceMode(msg);
        Serial.print("Device mode: "); Serial.println(msg);
    }
    
    
    // anchor starts in receiving mode, awaiting a ranging poll message
    receiver();
    // for first time ranging frequency computation
    _rangingCountPeriod = millis();
}


void DW1000RangingClass::startAsAnchor(char address[],  const byte mode[]){
    //save the address
    DW1000.convertToByte(address, _currentAddress);
    //write the address on the DW1000 chip
    DW1000.setEUI(address);
    Serial.print("device address: ");
    Serial.println(address);
    //we need to define a random short address:
    randomSeed(analogRead(0));
    _currentShortAddress[0]=random(0,256);
    _currentShortAddress[1]=random(0,256);
    
    //we configur the network for mac filtering
    //(device Address, network ID, frequency)
    DW1000Ranging.configureNetwork(_currentShortAddress[0]*256+_currentShortAddress[1], 0xDECA, mode);
    
    //general start:
    generalStart();
    
    //defined type as anchor
    _type=ANCHOR;
    
    Serial.println("### ANCHOR ###");
    
}

void DW1000RangingClass::startAsTag(char address[],  const byte mode[]){
    //save the address
    DW1000.convertToByte(address, _currentAddress);
    //write the address on the DW1000 chip
    DW1000.setEUI(address);
    Serial.print("device address: ");
    Serial.println(address);
    //we need to define a random short address:
    randomSeed(analogRead(0));
    _currentShortAddress[0]=random(0,256);
    _currentShortAddress[1]=random(0,256);
    
    //we configur the network for mac filtering
    //(device Address, network ID, frequency)
    DW1000Ranging.configureNetwork(_currentShortAddress[0]*256+_currentShortAddress[1], 0xDECA, mode);
    
    generalStart();
    //defined type as anchor
    _type=TAG;
    
    Serial.println("### TAG ###");
}

boolean DW1000RangingClass::addNetworkDevices(DW1000Device *device, boolean shortAddress)
{
    boolean addDevice=true;
    //we test our network devices array to check
    //we don't already have it
    for(short i=0; i<_networkDevicesNumber; i++){
        if(_networkDevices[i].isAddressEqual(device) && !shortAddress)
        {
            //the device already exists
            addDevice=false;
            return false;
        }
        else if(_networkDevices[i].isShortAddressEqual(device) && shortAddress)
        {
            //the device already exists
            addDevice=false;
            return false;
        }
        
    }
    
    if(addDevice)
    {
        memcpy(_networkDevices+_networkDevicesNumber*sizeof(DW1000Device), device, sizeof(DW1000Device));
        _networkDevicesNumber++;
        return true;
    }
    
    return false;
}

boolean DW1000RangingClass::addNetworkDevices(DW1000Device *device)
{
    boolean addDevice=true;
    //we test our network devices array to check
    //we don't already have it
    for(short i=0; i<_networkDevicesNumber; i++){
        if(_networkDevices[i].isAddressEqual(device) && _networkDevices[i].isShortAddressEqual(device))
        {
            //the device already exists
            addDevice=false;
            return false;
        }
        
    }
    
    if(addDevice)
    {
        if(_type==ANCHOR) //for now let's start with 1 TAG
        {
            _networkDevicesNumber=0;
        }
        memcpy(_networkDevices+_networkDevicesNumber*sizeof(DW1000Device), device, sizeof(DW1000Device));
        _networkDevicesNumber++;
        return true;
    }
    
    return false;
}

/* ###########################################################################
 * #### Setters and Getters ##################################################
 * ######################################################################### */

//setters
void DW1000RangingClass::setReplyTime(unsigned int replyDelayTimeUs){ _replyDelayTimeUS=replyDelayTimeUs;}
void DW1000RangingClass::setResetPeriod(unsigned long resetPeriod){ _resetPeriod=resetPeriod;}
 


DW1000Device* DW1000RangingClass::searchDistantDevice(byte shortAddress[]){
    
    //we compare the 2 bytes address with the others
    for(int i=0; i<_networkDevicesNumber; i++)
    {
        if(memcmp(shortAddress, _networkDevices[i].getByteShortAddress(), 2)==0)
        {
            //we have found our device ! 
            return &_networkDevices[i];
        }
    }
    
    return 0;
}

DW1000Device* DW1000RangingClass::getDistantDevice(){
    //we get the device which correspond to the message which was sent (need to be filtered by MAC address)
    return &_networkDevices[_lastDistantDevice];
    
}




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

short DW1000RangingClass::detectMessageType(byte datas[]){
    if(datas[0]==0xC5)
    {
        return BLINK;
    }
    else if(datas[0]==FC_1 && datas[1]==FC_2)
    {
        //we have a long MAC frame message (ranging init)
        return datas[LONG_MAC_LEN];
    }
    else if(datas[0]==FC_1 && datas[1]==FC_2_SHORT)
    {
        //we have a short mac frame message (poll, range, range report, etc..)
        return datas[SHORT_MAC_LEN];
    }  
}

void DW1000RangingClass::loop(){
    //we check if needed to reset !
    checkForReset();
    long time=millis(); 
    if(time-timer>DEFAULT_TIMER_DELAY){
        timer=time;
        timerTick();
    }
    
    if(_sentAck){
        _sentAck = false;
        
        
        int messageType=detectMessageType(data);
        
        if(messageType!=POLL_ACK && messageType!= POLL && messageType!=RANGE)
            return;
        
        
        DW1000Device *myDistantDevice=searchDistantDevice(_lastSentToShortAddress);
        
        if(myDistantDevice==0)
        {
            //we don't have the short address of the device in memory
            /*
            Serial.print("unknown sent: ");
            Serial.print(_lastSentToShortAddress[0], HEX);
            Serial.print(":");
            Serial.println(_lastSentToShortAddress[1], HEX);
             */
        }
        
        
        //A msg was sent. We launch the ranging protocole when a message was sent
        if(_type==ANCHOR){
            if(messageType == POLL_ACK) {
                DW1000.getTransmitTimestamp(myDistantDevice->timePollAckSent);
            }
        }
        else if(_type==TAG){
            if(messageType == POLL) {
                DW1000.getTransmitTimestamp(myDistantDevice->timePollSent);
                //Serial.print("Sent POLL @ "); Serial.println(timePollSent.getAsFloat());
            } else if(messageType == RANGE) { 
                
                DW1000.getTransmitTimestamp(myDistantDevice->timeRangeSent);
            }
        }
        
    }
    
    //check for new received message
    if(_receivedAck){
        _receivedAck=false;
        
        //we read the datas from the modules:
        // get message and parse
        DW1000.getData(data, LEN_DATA);
        
        
        int messageType=detectMessageType(data);
        
        //we have just received a BLINK message from tag
        if(messageType==BLINK && _type==ANCHOR){ 
            byte address[8];
            byte shortAddress[2];
            _globalMac.decodeBlinkFrame(data, address, shortAddress);
            //we crate a new device with th tag
            DW1000Device myTag(address, shortAddress);
            Serial.print(shortAddress[1], HEX);
            Serial.println(shortAddress[0], HEX);
            if(addNetworkDevices(&myTag))
            {
                Serial.print("blink; 1 device added ! -> "); 
                Serial.print(" short:");
                Serial.println(myTag.getShortAddress(), HEX);
                
                //we relpy by the transmit ranging init message
                transmitRangingInit(&myTag);
                noteActivity();
            }
            _expectedMsgId=POLL;
        }
        else if(messageType==RANGING_INIT && _type==TAG){
            
            byte address[2];
            _globalMac.decodeLongMACFrame(data, address);
            //we crate a new device with the anchor
            DW1000Device myAnchor(address, true);
            
            if(addNetworkDevices(&myAnchor, true))
            {
                Serial.print("ranging init; 1 device added ! -> ");
                Serial.print(" short:");
                Serial.println(myAnchor.getShortAddress(), HEX);
            }
            //we relpy by the transmit ranging init message
            //transmitPoll(&myAnchor);
        
            noteActivity();
            
        }
        else
        {
            //we have a short mac layer frame !
            byte address[2];
            _globalMac.decodeShortMACFrame(data, address);
            
             
            
            //we get the device which correspond to the message which was sent (need to be filtered by MAC address)
            DW1000Device *myDistantDevice=searchDistantDevice(address);
            
            if(myDistantDevice==0)
            {
                //we don't have the short address of the device in memory
                /*
                Serial.print("unknown: ");
                Serial.print(address[0], HEX);
                Serial.print(":");
                Serial.println(address[1], HEX);
                */
                return;
            }
            
            
            
        
            //then we proceed to range protocole
            if(_type==ANCHOR){
                if(messageType != _expectedMsgId) {
                    // unexpected message, start over again (except if already POLL)
                    _protocolFailed = true;
                }
                if(messageType == POLL) {
                    //Serial.println("receive poll");
                    // on POLL we (re-)start, so no protocol failure
                    _protocolFailed = false;
                    
                    DW1000.getReceiveTimestamp(myDistantDevice->timePollReceived);
                    _expectedMsgId = RANGE;
                    transmitPollAck(myDistantDevice);
                    noteActivity();
                }
                else if(messageType == RANGE) {
                    DW1000.getReceiveTimestamp(myDistantDevice->timeRangeReceived);
                    noteActivity();
                    _expectedMsgId = POLL;
                    
                    if(!_protocolFailed) {
                        
                        myDistantDevice->timePollSent.setTimestamp(data+1+SHORT_MAC_LEN);
                        myDistantDevice->timePollAckReceived.setTimestamp(data+6+SHORT_MAC_LEN);
                        myDistantDevice->timeRangeSent.setTimestamp(data+11+SHORT_MAC_LEN);
                        
                        // (re-)compute range as two-way ranging is done
                        DW1000Time myTOF;
                        computeRangeAsymmetric(myDistantDevice, &myTOF); // CHOSEN RANGING ALGORITHM
                        
                        float distance=myTOF.getAsMeters();
                        
                        myDistantDevice->setRXPower(DW1000.getReceivePower());
                        myDistantDevice->setRange(distance);
                         
                        myDistantDevice->setFPPower(DW1000.getFirstPathPower());
                        myDistantDevice->setQuality(DW1000.getReceiveQuality());
                        
                        //we send the range to TAG
                        transmitRangeReport(myDistantDevice);
                        
                        //we have finished our range computation. We send the corresponding handler
                        
                        if(_handleNewRange != 0) {
                            (*_handleNewRange)();
                        }
                        
                    }
                    else {
                        transmitRangeFailed(myDistantDevice);
                    }
                    
                    noteActivity();
                }
            }
            else if(_type==TAG){
                // get message and parse
                if(messageType != _expectedMsgId) {
                    // unexpected message, start over again
                    _expectedMsgId = POLL_ACK;
                    //transmitPoll(myDistantDevice);
                    return;
                }
                if(messageType == POLL_ACK) {
                    DW1000.getReceiveTimestamp(myDistantDevice->timePollAckReceived);
                    _expectedMsgId = RANGE_REPORT;
                    //we note activity for our device:
                    myDistantDevice->noteActivity();
                    //and transmit the next message (range) of the ranging protocole
                    transmitRange(myDistantDevice);
                }
                else if(messageType == RANGE_REPORT) {
                    _expectedMsgId = POLL_ACK;
                    float curRange;
                    memcpy(&curRange, data+1+SHORT_MAC_LEN, 4);
                    float curRXPower;
                    memcpy(&curRXPower, data+5+SHORT_MAC_LEN, 4);
                    //we have a new range to save !
                    myDistantDevice->setRange(curRange);
                    myDistantDevice->setRXPower(curRXPower);
                    
                    
                    //We can call our handler !
                    if(_handleNewRange != 0){
                        (*_handleNewRange)();
                    }
                }
                else if(messageType == RANGE_FAILED) {
                    _expectedMsgId = POLL_ACK;
                }
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
        Serial.println("---- RESET INACTIVE ---");
        _expectedMsgId = POLL;
        receiver();
    }
    noteActivity();
}

void DW1000RangingClass::timerTick(){
    if(_type==TAG){
        if(_networkDevicesNumber>0 && counterForBlink!=0)
        {
            _expectedMsgId = POLL_ACK;
            //need to do a broadcast poll !
            transmitPoll(&_networkDevices[0]);
        }
        else if(counterForBlink==0)
        {
            transmitBlink();
        }
        counterForBlink++;
        if(counterForBlink>50){
            counterForBlink=0;
            
            for(int i=0; i<_networkDevicesNumber; i++){
                if(_networkDevices[i].isInactive()){
                    //we need to delete the device from the array:
                    
                }
            }
        }
    }
    
}


void DW1000RangingClass::copyShortAddress(byte address1[],byte address2[]){
    *address1=*address2;
    *(address1+1)=*(address2+1);
}

/* ###########################################################################
 * #### Methods for ranging protocole   ######################################
 * ######################################################################### */

void DW1000RangingClass::transmitInit(){
    DW1000.newTransmit();
    DW1000.setDefaults();
}


void DW1000RangingClass::transmit(byte datas[]){
    DW1000.setData(datas, LEN_DATA);
    DW1000.startTransmit();
}


void DW1000RangingClass::transmit(byte datas[], DW1000Time time){
    DW1000.setDelay(time);
    DW1000.setData(data, LEN_DATA);
    DW1000.startTransmit();
}

void DW1000RangingClass::transmitBlink(){
    transmitInit();
    _globalMac.generateBlinkFrame(data, _currentAddress, _currentShortAddress);
    transmit(data);
}

void DW1000RangingClass::transmitRangingInit(DW1000Device *myDistantDevice){
    transmitInit();
    //we generate the mac frame for a ranging init message
    _globalMac.generateLongMACFrame(data, _currentShortAddress, myDistantDevice->getByteAddress());
    //we define the function code
    data[LONG_MAC_LEN]=RANGING_INIT;
    
    copyShortAddress(_lastSentToShortAddress,myDistantDevice->getByteShortAddress());
    
    transmit(data);
}

void DW1000RangingClass::transmitPoll(DW1000Device *myDistantDevice) {
    transmitInit();
    //we generate the mac frame for a Poll message
    _globalMac.generateShortMACFrame(data, _currentShortAddress, myDistantDevice->getByteShortAddress());
    data[SHORT_MAC_LEN] = POLL;
    copyShortAddress(_lastSentToShortAddress,myDistantDevice->getByteShortAddress());
    transmit(data);
}


void DW1000RangingClass::transmitPollAck(DW1000Device *myDistantDevice) {
    transmitInit();
    _globalMac.generateShortMACFrame(data, _currentShortAddress, myDistantDevice->getByteShortAddress());
    data[SHORT_MAC_LEN] = POLL_ACK;
    // delay the same amount as ranging tag
    DW1000Time deltaTime = DW1000Time(_replyDelayTimeUS, DW_MICROSECONDS);
    copyShortAddress(_lastSentToShortAddress,myDistantDevice->getByteShortAddress());
    transmit(data, deltaTime);
}

void DW1000RangingClass::transmitRange(DW1000Device *myDistantDevice) {
    transmitInit();
    _globalMac.generateShortMACFrame(data, _currentShortAddress, myDistantDevice->getByteShortAddress());
    data[SHORT_MAC_LEN] = RANGE;
    // delay sending the message and remember expected future sent timestamp
    DW1000Time deltaTime = DW1000Time(_replyDelayTimeUS, DW_MICROSECONDS);
    //we get the device which correspond to the message which was sent (need to be filtered by MAC address)
    myDistantDevice->timeRangeSent = DW1000.setDelay(deltaTime);
    myDistantDevice->timePollSent.getTimestamp(data+1+SHORT_MAC_LEN);
    myDistantDevice->timePollAckReceived.getTimestamp(data+6+SHORT_MAC_LEN);
    myDistantDevice->timeRangeSent.getTimestamp(data+11+SHORT_MAC_LEN);
    copyShortAddress(_lastSentToShortAddress,myDistantDevice->getByteShortAddress());
    transmit(data);
    //Serial.print("Expect RANGE to be sent @ "); Serial.println(timeRangeSent.getAsFloat());
}


void DW1000RangingClass::transmitRangeReport(DW1000Device *myDistantDevice) {
    transmitInit();
    _globalMac.generateShortMACFrame(data, _currentShortAddress, myDistantDevice->getByteShortAddress());
    data[SHORT_MAC_LEN] = RANGE_REPORT;
    // write final ranging result
    float curRange=myDistantDevice->getRange();
    float curRXPower=myDistantDevice->getRXPower();
    //We add the Range and then the RXPower
    memcpy(data+1+SHORT_MAC_LEN, &curRange, 4);
    memcpy(data+5+SHORT_MAC_LEN, &curRXPower, 4);
    copyShortAddress(_lastSentToShortAddress,myDistantDevice->getByteShortAddress());
    transmit(data);
}

void DW1000RangingClass::transmitRangeFailed(DW1000Device *myDistantDevice) {
    transmitInit();
    _globalMac.generateShortMACFrame(data, _currentShortAddress, myDistantDevice->getByteShortAddress());
    data[SHORT_MAC_LEN] = RANGE_FAILED;
    
    copyShortAddress(_lastSentToShortAddress,myDistantDevice->getByteShortAddress());
    transmit(data);
}

void DW1000RangingClass::receiver() {
    DW1000.newReceive();
    DW1000.setDefaults();
    // so we don't need to restart the receiver manually
    DW1000.receivePermanently(true);
    DW1000.startReceive();
}









/* ###########################################################################
 * #### Methods for range computation and corrections  #######################
 * ######################################################################### */


void DW1000RangingClass::computeRangeAsymmetric(DW1000Device *myDistantDevice, DW1000Time *myTOF) {
    // asymmetric two-way ranging (more computation intense, less error prone)
    DW1000Time round1 = (myDistantDevice->timePollAckReceived-myDistantDevice->timePollSent).wrap();
    DW1000Time reply1 = (myDistantDevice->timePollAckSent-myDistantDevice->timePollReceived).wrap();
    DW1000Time round2 = (myDistantDevice->timeRangeReceived-myDistantDevice->timePollAckSent).wrap();
    DW1000Time reply2 = (myDistantDevice->timeRangeSent-myDistantDevice->timePollAckReceived).wrap();
    
    myTOF->setTimestamp((round1 * round2 - reply1 * reply2) / (round1 + round2 + reply1 + reply2));
}


/* FOR DEBUGGING*/
void DW1000RangingClass::visualizeDatas(byte datas[]){
    char string[60];
    sprintf(string, "%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X",
            datas[0], datas[1], datas[2], datas[3], datas[4], datas[5], datas[6], datas[7],datas[8],datas[9],datas[10],datas[11],datas[12],datas[13],datas[14],datas[15]);
    Serial.println(string);
}





