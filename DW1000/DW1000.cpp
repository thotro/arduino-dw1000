/*
 * Copyright (c) 2014 by Thomas Trojer <thomas@trojer.net>
 * Decawave DW1000 library for Arduino.
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <string.h>
#include "pins_arduino.h"
#include "DW1000.h"

/* ###################################################################################################
 * #### Construction and init ########################################################################
 * ################################################################################################ */

DW1000::DW1000(int ss) {
	_ss = ss;
	pinMode(_ss, OUTPUT);
	SPI.begin();
}

DW1000::~DW1000() {
	SPI.end();
}

void DW1000::loadSystemConfiguration() {
	readSystemConfiguration(_syscfg);
}

/* ###################################################################################################
 * #### Member access ################################################################################
 * ################################################################################################ */

byte* DW1000::getSystemConfiguration() {
	return _syscfg;
}

int DW1000::getChipSelect() {
	return _ss;
}

/* ###################################################################################################
 * #### DW1000 operation functions ###################################################################
 * ################################################################################################ */

char* DW1000::readDeviceIdentifier() {
	char infoString[128];
	byte data[LEN_DEV_ID];

	readBytes(DEV_ID, data, LEN_DEV_ID);

	sprintf(infoString, "DECA - model: %d, version: %d, revision: %d", data[1], data[0] >> 4, data[0] & 0x0F);
	return infoString;
}

void DW1000::readSystemConfiguration(byte data[]) {
	readBytes(SYS_CFG, data, LEN_SYS_CFG);
}

void DW1000::setFrameFilter(boolean val) {
	if(val) {
		_syscfg[0] |= 0x01;
	} else {
		_syscfg[0] &= ~0x01;
	}
	writeBytes(SYS_CFG, 0x00, _syscfg, LEN_SYS_CFG);
}

void DW1000::suppressFrameCheck() {
	_sysctrl[0] |= SFCST;
}

void DW1000::delayedTransmit(unsigned int time) {
	_sysctrl[0] |= TXDLYS;
	// 40 bit in DX_TIME register, 9 lower sig. bits are ignored
	// TODO implement
}

// TODO implement data(), other TX states, ...

void DW1000::beginTransmit() {
	memset(_sysctrl, 0, LEN_SYS_CTRL);
}

void DW1000::cancelTransmit() {
	beginTransmit();
}

void DW1000::endTransmit() {
	_sysctrl[0] |= TXSTRT;
}

/* ###################################################################################################
 * #### Helper functions #############################################################################
 * ################################################################################################ */

/*
 * Read bytes from the DW1000. Number of bytes depend on register length.
 * @param cmd 
 * 		The register address (see Chapter 7 in the DW1000 user manual).
 * @param data 
 *		The data array to be read into.
 * @param n
 *		The number of bytes expected to be received.
 */
void DW1000::readBytes(byte cmd, byte data[], int n) {
	int i;

	digitalWrite(_ss, LOW);
	SPI.transfer(READ | cmd);
	for(i = 0; i < n; i++) {
		data[i] = SPI.transfer(JUNK);
	}
	digitalWrite(_ss,HIGH);
}

/*
 * Write bytes to the DW1000. Single bytes can be written to registers via sub-addressing.
 * @param cmd 
 * 		The register address (see Chapter 7 in the DW1000 user manual).
 * @param offset
 *		The offset to select register sub-parts for writing, or 0x00 to disable sub-adressing.
 * @param data 
 *		The data array to be written.
 * @param n
 *		The number of bytes to be written (take care not to go out of bounds of the register).
 */
void DW1000::writeBytes(byte cmd, word offset, byte data[], int n) {
	byte header[3];
	int headerLen = 1;
	int i;

	if(offset == 0) {
		header[0] = WRITE | cmd;
	} else {
		header[0] = WRITE_SUB | cmd;
		if(offset < 128) {
			header[1] = (byte)offset;
			headerLen++;
		} else {
			header[1] = WRITE | (byte)offset;
			header[2] = (byte)(offset >> 7);
			headerLen+=2;
		}
	}
	
	digitalWrite(_ss, LOW);
	for(i = 0; i < headerLen; i++) {
		SPI.transfer(header[i]);
	}
	for(i = 0; i < n; i++) {
		SPI.transfer(data[i]);
	}
	digitalWrite(_ss,HIGH);
}
