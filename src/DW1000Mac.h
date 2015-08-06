/*
 * Copyright (c) 2015 by Leopold Sayous <leosayous@gmail.com> and Thomas Trojer <thomas@trojer.net>
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
 * @file DW1000Mac.h
 * Arduino global library (header file) working with the DW1000 library
 * for the Decawave DW1000 UWB transceiver IC. This class has the purpose
 * to generate the mac layer
 */

#define FC_1 0x41
#define FC_2 0x8C
#define FC_2_SHORT 0x88

#define PAN_ID_1 0xCA
#define PAN_ID_2 0xDE

#include <Arduino.h>



#ifndef _DW1000Mac_H_INCLUDED
#define _DW1000Mac_H_INCLUDED

class DW1000Mac {
    public:
        //Constructor and destructor
        DW1000Mac();
        ~DW1000Mac();
    
    
        //setters
        void setDestinationAddress(byte* destinationAddress);
        void setDestinationAddressShort(byte* shortDestinationAddress);
        void setSourceAddress(byte* sourceAddress);
        void setSourceAddressShort(byte* shortSourceAddress);
    
    
        //for poll message we use just 2 bytes address
        void generatePollMessage(byte data[]);
    
        void incrementSeqNumber();
    
    
    private:
        byte* _destinationAddress;
        byte* _destinationAddressShort;
        byte* _sourceAddress;
        byte* _sourceAddressShort;
        byte* _seqNumber;
    
 
};

#endif

