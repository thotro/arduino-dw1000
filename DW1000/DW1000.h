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

// used for SPI ready w/o actual writes
#define JUNK 0x00

// no sub-address for register write
#define NO_SUB 0x00

// device id register
#define DEV_ID 0x00
#define LEN_DEV_ID 4

// device configuration register
#define SYS_CFG 0x04
#define LEN_SYS_CFG 4
#define FFEN_BIT 0

// device control register
#define SYS_CTRL 0x0D
#define LEN_SYS_CTRL 4
#define SFCST_BIT 0
#define TXSTRT_BIT 1
#define TXDLYS_BIT 2

// system event status register
#define SYS_STATUS 0x0F
#define LEN_SYS_STATUS 5
#define LDEDONE_BIT 10

// RX timestamp register
#define RX_TIME 0x15
#define LEN_RX_TIME 14
#define RX_STAMP_SUB 0x00
#define LEN_RX_STAMP_SUB 5

// timing register (for delayed RX/TX)
#define DX_TIME 0x0A
#define LEN_DX_TIME 5

#include <stdio.h>
#include <Arduino.h>
#include "../SPI/SPI.h"

class DW1000 {
public:
	DW1000(int ss);
	~DW1000();

	byte* getSystemConfiguration();
	int getChipSelect();

	// DEV_ID
	char* readDeviceIdentifier();
	
	// SYS_CFG
	void loadSystemConfiguration();
	void readSystemConfiguration(byte syscfg[]);
	void setFrameFilter(boolean val);

	// SYS_CTRL
	void suppressFrameCheck();
	void delayedTransmit(unsigned int delayNanos); // TODO impl

	// SYS_STATUS
	bool readAndClearLDEDone();

	// TODO data and other state set functions for TX

	// RX_TIME
	// TODO void readReceiveTimestamp(byte[] timestamp);

	// transmission
	void beginTransmit();
	void endTransmit();
	void cancelTransmit();

private:
	unsigned int _ss;
	byte _syscfg[LEN_SYS_CFG];
	byte _sysctrl[LEN_SYS_CTRL];

	void readBytes(byte cmd, byte data[], int n);
	void writeBytes(byte cmd, word offset, byte data[], int n);

	boolean getBit(byte data[], int n, int bit);
	void setBit(byte data[], int n, int bit, boolean val);
	
	/* Register is 6 bit, 7 = write, 6 = sub-adressing, 5-0 = register value
	 * Total header with sub-adressing can be 15 bit
	 */
	static const byte WRITE = 0x80; // regular write
	static const byte WRITE_SUB = 0xC0; // write with sub address
	static const byte READ = 0x00; // regular read
};

#endif
