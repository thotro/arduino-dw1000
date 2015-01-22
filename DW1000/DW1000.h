/*
 * Copyright (c) 2014 by Thomas Trojer <thomas@trojer.net>
 * Decawave DW1000 library for arduino.
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef _DW1000_H_INCLUDED
#define _DW1000_H_INCLUDED

#define JUNK 0x00

#define DEV_ID 0x00
#define LEN_DEV_ID 4

#define SYS_CFG 0x04
#define LEN_SYS_CFG 4

#define SYS_CTRL 0x0D
#define LEN_SYS_CTRL 4
#define SFCST 0x01
#define TXSTRT 0x02
#define TXDLYS 0x04

#define DX_TIME 0x0A
#define LEN_DX_TIME 5

#include <stdio.h>
#include <Arduino.h>
#include "../SPI/SPI.h"

class DW1000 {
public:
	DW1000(int ss);
	~DW1000();

	void loadSystemConfiguration();

	byte* getSystemConfiguration();
	int getChipSelect();

	// DEV_ID
	char* readDeviceIdentifier();
	
	// SYS_CFG
	void readSystemConfiguration(byte syscfg[]);
	void setFrameFilter(boolean val);

	// SYS_CTRL
	void suppressFrameCheck();
	void delayedTransmit(unsigned int time);

	// TODO data and other state set functions for TX

	void beginTransmit();
	void endTransmit();
	void cancelTransmit();

private:
	unsigned int _ss;
	byte _syscfg[LEN_SYS_CFG];
	byte _sysctrl[LEN_SYS_CTRL];

	void readBytes(byte cmd, byte data[], int n);
	void writeBytes(byte cmd, word offset, byte data[], int n);
	
	/* Register is 6 bit, 7 = write, 6 = sub-adressing, 5-0 = register value
	 * Total header with sub-adressing can be 15 bit
	 */
	static const byte WRITE = 0x80; // regular write
	static const byte WRITE_SUB = 0xC0; // write with sub address
	static const byte READ = 0x00; // regular read
};

#endif
