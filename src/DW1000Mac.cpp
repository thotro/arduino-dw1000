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

//Constructor and destructor
DW1000Mac::DW1000Mac(){
    _seqNumber=0;
}


DW1000Mac::~DW1000Mac(){
    
}


void DW1000Mac::generatePollMessage(byte data[]){
    
}

void DW1000Mac::incrementSeqNumber()
{
    *_seqNumber++;
    if(*_seqNumber>255)
        *_seqNumber=0;
}