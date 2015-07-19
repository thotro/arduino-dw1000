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

DW1000Time::~DW1000Time() {}

void DW1000Time::setTime(float timeUs) {
	_timestamp = (long long int)(timeUs * TIME_RES_INV);
}

void DW1000Time::setTime(long value, float factorUs) {
	float tsValue = value * factorUs;
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

long long int DW1000Time::getTimestamp() const {
	return _timestamp;
}

void DW1000Time::getTimestamp(byte data[]) const {
	memset(data, 0, LEN_STAMP);
	for(int i = 0; i < LEN_STAMP; i++) {
	    data[i] = (byte)((_timestamp >> (i*8)) & 0xFF);
	}
}

float DW1000Time::getAsFloat() const {
	return fmod((float)_timestamp, TIME_OVERFLOW) * TIME_RES;
}

float DW1000Time::getAsMeters() const {
	return fmod((float)_timestamp, TIME_OVERFLOW) * DISTANCE_OF_RADIO;
}

DW1000Time& DW1000Time::operator=(const DW1000Time &assign) {
	if(this == &assign) {
		return *this;
	}
	_timestamp = assign.getTimestamp();
	return *this;
}

DW1000Time& DW1000Time::operator+=(const DW1000Time &add) {
	_timestamp += add.getTimestamp();
	return *this;
}

const DW1000Time DW1000Time::operator+(const DW1000Time &add) const {
    return DW1000Time(*this) += add;
}

DW1000Time& DW1000Time::operator-=(const DW1000Time &sub) {
	_timestamp -= sub.getTimestamp();
	return *this;
}

const DW1000Time DW1000Time::operator-(const DW1000Time &sub) const {
	return DW1000Time(*this) -= sub;
}

DW1000Time& DW1000Time::operator*=(float factor) {
	float tsValue = (float)_timestamp * factor;
	_timestamp = (long long int)tsValue;
	return *this;
}

const DW1000Time DW1000Time::operator*(float factor) const {
	return DW1000Time(*this) *= factor;
}

DW1000Time& DW1000Time::operator*=(const DW1000Time &factor) {
	_timestamp *= factor.getTimestamp();
	return *this;
}

const DW1000Time DW1000Time::operator*(const DW1000Time &factor) const {
	return DW1000Time(*this) *= factor;
}

DW1000Time& DW1000Time::operator/=(float factor) {
	_timestamp *= (1.0f/factor);
	return *this;
}

const DW1000Time DW1000Time::operator/(float factor) const {
	return DW1000Time(*this) /= factor;
}

DW1000Time& DW1000Time::operator/=(const DW1000Time &factor) {
	_timestamp /= factor.getTimestamp(); 
	return *this;
}

const DW1000Time DW1000Time::operator/(const DW1000Time &factor) const {
	return DW1000Time(*this) /= factor;
}

boolean DW1000Time::operator==(const DW1000Time &cmp) const {
	return _timestamp == cmp.getTimestamp();
}

boolean DW1000Time::operator!=(const DW1000Time &cmp) const {
	return !(*this == cmp);
}

