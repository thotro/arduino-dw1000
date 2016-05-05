/*
 * Copyright (c) 2015 by Thomas Trojer <thomas@trojer.net>
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
 * @file DW1000Time.h
 * Arduino driver library timestamp wrapper (header file) for the Decawave 
 * DW1000 UWB transceiver IC.
 */

#ifndef _DW1000Time_H_INCLUDED
#define _DW1000Time_H_INCLUDED

// Time resolution in micro-seconds of time based registers/values.
// Each bit in a timestamp counts for a period of approx. 15.65ps
#define TIME_RES 0.000015650040064103f
#define TIME_RES_INV 63897.6f

// Speed of radio waves [m/s] * timestamp resolution [~15.65ps] of DW1000
#define DISTANCE_OF_RADIO 0.0046917639786159f
#define DISTANCE_OF_RADIO_INV 213.139451293f

// time stamp byte length
#define LEN_STAMP 5


// timer/counter overflow (40 bits)
#define TIME_OVERFLOW 1099511627776

// time factors (relative to [us]) for setting delayed transceive
#define DW_SECONDS 1e6
#define DW_MILLISECONDS 1e3
#define DW_MICROSECONDS 1
#define DW_NANOSECONDS 1e-3

#include <Arduino.h>

class DW1000Time {
public:
	DW1000Time();
	DW1000Time(long long int time);
	DW1000Time(float timeUs);
	DW1000Time(byte data[]);
	DW1000Time(long value, float factorUs);
	DW1000Time(const DW1000Time& copy);
	~DW1000Time();
	
	void setTime(float timeUs);
	void setTime(long value, float factorUs);
	
	float getAsFloat() const;
	void  getAsBytes(byte data[]) const;
	float getAsMeters() const;
	
	void          getTimestamp(byte data[]) const;
	long long int getTimestamp() const;
	void          setTimestamp(byte data[]);
	void          setTimestamp(const DW1000Time& copy);
	void          setTimestamp(int value);
	
	DW1000Time& wrap();
	
	DW1000Time& operator=(const DW1000Time& assign);
	DW1000Time& operator+=(const DW1000Time& add);
	DW1000Time operator+(const DW1000Time& add) const;
	DW1000Time& operator-=(const DW1000Time& sub);
	DW1000Time operator-(const DW1000Time& sub) const;
	DW1000Time& operator*=(float factor);
	DW1000Time operator*(const DW1000Time& factor) const;
	DW1000Time& operator*=(const DW1000Time& factor);
	DW1000Time operator*(float factor) const;
	DW1000Time& operator/=(float factor);
	DW1000Time operator/(float factor) const;
	DW1000Time& operator/=(const DW1000Time& factor);
	DW1000Time operator/(const DW1000Time& factor) const;
	boolean operator==(const DW1000Time& cmp) const;
	boolean operator!=(const DW1000Time& cmp) const;
	
	void print();

private:
	long long int _timestamp;
};

#endif

