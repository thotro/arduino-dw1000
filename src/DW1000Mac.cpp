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
 * @file DW1000Mac.cpp
 * Arduino global library (header file) working with the DW1000 library
 * for the Decawave DW1000 UWB transceiver IC. This class has the purpose
 * to generate the mac layer
 */

#include "DW1000Mac.h" 
#include "DW1000Ranging.h"

//Constructor and destructor
DW1000Mac::DW1000Mac(DW1000Device *parent){
    _parentDevice=parent;
    _seqNumber=0;
}

DW1000Mac::DW1000Mac(){
    _seqNumber=0;
}


DW1000Mac::~DW1000Mac(){
    
}

//for poll message we use just 2 bytes address
//total=12 bytes
void DW1000Mac::generateBlinkFrame(byte data[]){
    //Frame Control
    *data=FC_1_BLINK;
    //sequence number
    *(data+1)=_seqNumber;
    //tag 64 bit ID
    byte sourceAddress[8];
    DW1000Ranging.getCurrentAddress(sourceAddress);
    
    memcpy(data+2, sourceAddress, sizeof(*sourceAddress));
}

//the short fram usually for Resp, Final, or Report
//2 bytes for Desination Address and 2 bytes for Source Address
//total=9 bytes
void DW1000Mac::generateShortMACFrame(byte data[]){
    //Frame controle
    *data=FC_1;
    *(data+1)=FC_2_SHORT;
    //sequence number
    *(data+2)=_seqNumber;
    //PAN ID
    *(data+3)=0xCA;
    *(data+4)=0xDE;
    
    //destination address (2 bytes)
    byte destinationAddress[2];
    _parentDevice->getShortAddress(destinationAddress);
    memcpy(data+5, destinationAddress, sizeof(*destinationAddress));
    //source address (2 bytes)
    byte sourceAddress[2];
    DW1000Ranging.getCurrentShortAddress(sourceAddress);
    memcpy(data+7, sourceAddress, sizeof(*sourceAddress));
}

//the long frame for Ranging init
//8 bytes for Destination Address and 2 bytes for Source Address
//total=15
void DW1000Mac::generateLongMACFrame(byte data[]){
    //Frame controle
    *data=FC_1;
    *(data+1)=FC_2;
    //sequence number
    *(data+2)=_seqNumber;
    //PAN ID
    *(data+3)=0xCA;
    *(data+4)=0xDE;
    
    //destination address (8 bytes)
    byte destinationAddress[8];
    _parentDevice->getAddress(destinationAddress);
    memcpy(data+5, destinationAddress, sizeof(*destinationAddress));
    //source address (2 bytes)
    byte sourceAddress[2];
    DW1000Ranging.getCurrentShortAddress(sourceAddress);
    memcpy(data+13, sourceAddress, sizeof(*sourceAddress));
}



void DW1000Mac::incrementSeqNumber()
{
    _seqNumber++;
    if(_seqNumber>255)
        _seqNumber=0;
}