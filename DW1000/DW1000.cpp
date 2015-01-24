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

/* ###########################################################################
 * #### Construction and init ################################################
 * ######################################################################### */

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

/* ###########################################################################
 * #### Member access ########################################################
 * ######################################################################### */

byte* DW1000::getSystemConfiguration() {
	return _syscfg;
}

int DW1000::getChipSelect() {
	return _ss;
}

/* ###########################################################################
 * #### DW1000 operation functions ###########################################
 * ######################################################################### */

char* DW1000::readDeviceIdentifier() {
	char infoString[128];
	byte data[LEN_DEV_ID];

	readBytes(DEV_ID, data, LEN_DEV_ID);

	sprintf(infoString, "DECA - model: %d, version: %d, revision: %d", 
		data[1], data[0] >> 4, data[0] & 0x0F);
	return infoString;
}

void DW1000::readSystemConfiguration(byte data[]) {
	readBytes(SYS_CFG, data, LEN_SYS_CFG);
}

void DW1000::setFrameFilter(boolean val) {
	if(val) {
		bitSet(_syscfg[0], FFEN_BIT);
	} else {
		bitClear(_syscfg[0], FFEN_BIT);
	}
	writeBytes(SYS_CFG, NO_SUB, _syscfg, LEN_SYS_CFG);
}

void DW1000::suppressFrameCheck() {
	bitSet(_sysctrl[0], SFCST_BIT);
}

void DW1000::delayedTransmit(unsigned int delayNanos) {
	bitSet(_sysctrl[0], TXDLYS_BIT);
	// 40 bit in DX_TIME register, 9 lower sig. bits are ignored
	// TODO implement
}

// TODO implement data(), other TX states, ...

void DW1000::beginTransmit() {
	// clear out SYS_CTRL for a new transmit operation
	memset(_sysctrl, 0, LEN_SYS_CTRL);
}

void DW1000::cancelTransmit() {
	// same as beginTransmit
	beginTransmit();
}

void DW1000::endTransmit() {
	// set transmit flag
	bitSet(_sysctrl[0], TXSTRT_BIT);
}

// system event register
bool DW1000::readAndClearLDEDone() {
	byte data[LEN_SYS_STATUS];
	bool ldeDone;
	
	// read whole register and check bit
	readBytes(SYS_STATUS, data, LEN_SYS_STATUS);
	ldeDone = getBit(data, LEN_SYS_STATUS, LDEDONE_BIT);
	// clear latched (i.e. write 1 to clear)
	setBit(data, LEN_SYS_STATUS, LDEDONE_BIT, true);
	writeBytes(SYS_STATUS, NO_SUB, data, LEN_SYS_STATUS);
}

/* ###########################################################################
 * #### Helper functions #####################################################
 * ######################################################################### */

/*
 * Set the value of a bit in an array of bytes that are considered
 * consecutive and stored from MSB to LSB.
 * @param data
 * 		The number as byte array.
 * @param n
 * 		The number of bytes in the array.
 * @param bit
 * 		The position of the bit to be set.
 * @param val
 *		The boolean value to be set to the given bit position.
 */
void DW1000::setBit(byte data[], int n, int bit, boolean val) {
	int idx;
	int shift;
	byte mask;

	idx = bit / 8;
	if(idx >= n) {
		return;
	}
	byte targetByte = data[idx];
	if(val) {
		bitSet(targetByte, shift);
	} else {
		bitClear(targetByte, shift);
	}
}

/*
 * Check the value of a bit in an array of bytes that are considered
 * consecutive and stored from MSB to LSB.
 * @param data
 * 		The number as byte array.
 * @param n
 * 		The number of bytes in the array.
 * @param bit
 * 		The position of the bit to be checked.
 */
boolean DW1000::getBit(byte data[], int n, int bit) {
	int idx;
	int shift;

	idx = bit / 8;
	if(idx >= n) {
		return false;
	}
	byte targetByte = data[idx];
	shift = bit % 8;
	
	return bitRead(targetByte, shift);
}

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
 *		The offset to select register sub-parts for writing, or 0x00 to disable 
 * 		sub-adressing.
 * @param data 
 *		The data array to be written.
 * @param n
 *		The number of bytes to be written (take care not to go out of bounds of 
 * 		the register).
 */
void DW1000::writeBytes(byte cmd, word offset, byte data[], int n) {
	byte header[3];
	int headerLen = 1;
	int i;

	if(offset == NO_SUB) {
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
