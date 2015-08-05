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
 * @file DW1000Device.h
 * Arduino global library (header file) working with the DW1000 library
 * for the Decawave DW1000 UWB transceiver IC.
 */

#include <DW1000Time.h>


#ifndef _DW1000Device_H_INCLUDED
#define _DW1000Device_H_INCLUDED

class DW1000Device {
    public:
        //Constructor and destructor
        DW1000Device();
        DW1000Device(char address[]);
        ~DW1000Device();
    
        //setters:
        void setReplyTime(unsigned int replyDelayTimeUs);
        void setAddress(char address[]);
    
        void setRange(float range);
        void setRXPower(float power);
        void setFPPower(float power);
        void setQuality(float quality);
        //getters
        unsigned int getReplyTime();
        void getAddress(byte address[]);
        String getAddress();
        float getRange();
        float getRXPower();
        float getFPPower();
        float getQuality();
    
        //functions which contains the date: (easier to put as public)
        // timestamps to remember
        DW1000Time timePollSent;
        DW1000Time timePollReceived;
        DW1000Time timePollAckSent;
        DW1000Time timePollAckReceived;
        DW1000Time timeRangeSent;
        DW1000Time timeRangeReceived;
        // last computed range/time
        DW1000Time timeComputedRange;
    
    
    private:
        //device ID
        byte _ownAddress[8];
    
        unsigned int _replyDelayTimeUS;
    
        float _range;
        float _RXPower;
        float _FPPower;
        float _quality;
 
};

#endif

