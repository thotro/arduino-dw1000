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
#define TIME_RES 0.000015650040064103
#define TIME_RES_INV 63897.6

// time stamp byte length
#define LEN_STAMP 5

#include <Arduino.h>

class DW1000Time {
public:
	DW1000Time();
	DW1000Time(byte data[]);
	DW1000Time(unsigned long value, float factorUs);
	DW1000Time(const DW1000Time& copy);
	~DW1000Time();

	float getAsFloat() const;
	void getAsBytes(byte data[]) const;
	uint64_t getAsInt() const;

	DW1000Time& operator=(const DW1000Time &assign);
	DW1000Time& operator+=(const DW1000Time &add);
	const DW1000Time operator+(const DW1000Time &add) const;
	DW1000Time& operator-=(const DW1000Time &sub);
	const DW1000Time operator-(const DW1000Time &sub) const;
	boolean operator==(const DW1000Time &cmp) const;
	boolean operator!=(const DW1000Time &cmp) const;

	// time factors (relative to [us]) for setting delayed transceive
	static const float SECONDS = 1e6;
	static const float MILLISECONDS = 1e3;
	static const float MICROSECONDS = 1;
	static const float NANOSECONDS = 1e-3;

	// timer/counter overflow (40 bits)
	static const float TIME_OVERFLOW = 1099511627776.0f;

private:
	byte _timestamp[LEN_STAMP];

	static void addTimestampBytes(byte r[], byte a[], const byte b[]);
	static void subtractTimestampBytes(byte r[], byte a[], const byte b[]);

	void setFromFloat(float time);
};

#endif

