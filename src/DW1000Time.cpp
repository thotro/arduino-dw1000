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
 * @file DW1000Time.cpp
 * Arduino driver library timestamp wrapper (source file) for the Decawave 
 * DW1000 UWB transceiver IC.
 */

#include "DW1000Time.h"

DW1000Time::DW1000Time() {
	_timestamp = 0;
}

DW1000Time::DW1000Time(byte data[]) {
	setTimestamp(data);
}

DW1000Time::DW1000Time(float timeUs) {
	setTime(timeUs);
}

DW1000Time::DW1000Time(const DW1000Time& copy) {
	setTimestamp(copy);
}

DW1000Time::DW1000Time(long value, float factorUs) {
	setTime(value, factorUs);
}

DW1000Time::~DW1000Time() { }

void DW1000Time::setTime(float timeUs) {
	_timestamp = (long long int)(timeUs*TIME_RES_INV);
}

void DW1000Time::setTime(long value, float factorUs) {
	float tsValue = value*factorUs;
	tsValue = fmod(tsValue, TIME_OVERFLOW);
	setTime(tsValue);
}

void DW1000Time::setTimestamp(byte data[]) {
	_timestamp = 0;
	for(int i = 0; i < LEN_STAMP; i++) {
		_timestamp |= ((long long int)data[i] << (i*8));
	}
}

void DW1000Time::setTimestamp(const DW1000Time& copy) {
	_timestamp = copy.getTimestamp();
}

void DW1000Time::setTimestamp(int value) {
	_timestamp = value;
}

long long int DW1000Time::getTimestamp() const {
	return _timestamp;
}

DW1000Time& DW1000Time::wrap() {
	if(_timestamp < 0) {
		_timestamp += TIME_OVERFLOW;
	}
	return *this;
}

void DW1000Time::getTimestamp(byte data[]) const {
	memset(data, 0, LEN_STAMP);
	for(int i = 0; i < LEN_STAMP; i++) {
		data[i] = (byte)((_timestamp >> (i*8)) & 0xFF);
	}
}

float DW1000Time::getAsFloat() const {
	return fmod((float)_timestamp, TIME_OVERFLOW)*TIME_RES;
}

float DW1000Time::getAsMeters() const {
	return fmod((float)_timestamp, TIME_OVERFLOW)*DISTANCE_OF_RADIO;
}

DW1000Time& DW1000Time::operator=(const DW1000Time& assign) {
	if(this == &assign) {
		return *this;
	}
	_timestamp = assign.getTimestamp();
	return *this;
}

DW1000Time& DW1000Time::operator+=(const DW1000Time& add) {
	_timestamp += add.getTimestamp();
	return *this;
}

DW1000Time DW1000Time::operator+(const DW1000Time& add) const {
	return DW1000Time(*this) += add;
}

DW1000Time& DW1000Time::operator-=(const DW1000Time& sub) {
	_timestamp -= sub.getTimestamp();
	return *this;
}

DW1000Time DW1000Time::operator-(const DW1000Time& sub) const {
	return DW1000Time(*this) -= sub;
}

DW1000Time& DW1000Time::operator*=(float factor) {
	float tsValue = (float)_timestamp*factor;
	_timestamp = (long long int)tsValue;
	return *this;
}

DW1000Time DW1000Time::operator*(float factor) const {
	return DW1000Time(*this) *= factor;
}

DW1000Time& DW1000Time::operator*=(const DW1000Time& factor) {
	_timestamp *= factor.getTimestamp();
	return *this;
}

DW1000Time DW1000Time::operator*(const DW1000Time& factor) const {
	return DW1000Time(*this) *= factor;
}

DW1000Time& DW1000Time::operator/=(float factor) {
	_timestamp *= (1.0f/factor);
	return *this;
}

DW1000Time DW1000Time::operator/(float factor) const {
	return DW1000Time(*this) /= factor;
}

DW1000Time& DW1000Time::operator/=(const DW1000Time& factor) {
	_timestamp /= factor.getTimestamp();
	return *this;
}

DW1000Time DW1000Time::operator/(const DW1000Time& factor) const {
	return DW1000Time(*this) /= factor;
}

boolean DW1000Time::operator==(const DW1000Time& cmp) const {
	return _timestamp == cmp.getTimestamp();
}

boolean DW1000Time::operator!=(const DW1000Time& cmp) const {
	return !(*this == cmp);
}

void DW1000Time::print() {
	long long int number = _timestamp;
	unsigned char buf[64];
	uint8_t       i      = 0;
	
	if(number == 0) {
		Serial.print((char)'0');
		return;
	}
	
	
	while(number > 0) {
		uint64_t q = number/10;
		buf[i++] = number-q*10;
		number = q;
	}
	for(; i > 0; i--)
		Serial.print((char)(buf[i-1] < 10 ? '0'+buf[i-1] : 'A'+buf[i-1]-10));
	
	Serial.println();
	
}


