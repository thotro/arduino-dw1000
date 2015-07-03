/*
 * Copyright (c) 2015 by Thomas Trojer <thomas@trojer.net>
 * Decawave DW1000 library for arduino.
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef _DW1000Time_H_INCLUDED
#define _DW1000Time_H_INCLUDED

// Time resolution in micro-seconds of time based registers/values.
// Each bit in a timestamp counts for a period of approx. 15.65ps
#define TIME_RES 0.000015650040064103f
#define TIME_RES_INV 63897.6f

// Speed of radio waves [m/s] * timestamp resolution [~15.65ps] of DW1000
#define DISTANCE_OF_RADIO 0.0046917639786159f

// time stamp byte length
#define LEN_STAMP 5

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
	void getAsBytes(byte data[]) const;
	float getAsMeters() const;

	void getTimestamp(byte data[]) const;
	long long int getTimestamp() const;
	void setTimestamp(byte data[]);
	void setTimestamp(const DW1000Time& copy);

	DW1000Time& operator=(const DW1000Time &assign);
	DW1000Time& operator+=(const DW1000Time &add);
	const DW1000Time operator+(const DW1000Time &add) const;
	DW1000Time& operator-=(const DW1000Time &sub);
	const DW1000Time operator-(const DW1000Time &sub) const;
	DW1000Time& operator*=(float factor);
	const DW1000Time operator*(float factor) const;
	DW1000Time& operator/=(float factor);
	const DW1000Time operator/(float factor) const;
	boolean operator==(const DW1000Time &cmp) const;
	boolean operator!=(const DW1000Time &cmp) const;

	// time factors (relative to [us]) for setting delayed transceive
	static const float SECONDS = 1e6;
	static const float MILLISECONDS = 1e3;
	static const float MICROSECONDS = 1;
	static const float NANOSECONDS = 1e-3;

	// timer/counter overflow (40 bits)
	static const long long unsigned int TIME_OVERFLOW = 1099511627776;

private:
	long long int _timestamp;
};

#endif

