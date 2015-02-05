/*
 * Copyright (c) 2015 by Thomas Trojer <thomas@trojer.net>
 * Decawave DW1000 library for Arduino.
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef DEBUG
#include "pins_arduino.h"
#endif
#include "DW1000.h"

/* ###########################################################################
 * #### Construction and init ################################################
 * ######################################################################### */

DW1000::DW1000(int ss) {
	_ss = ss;
	_deviceMode = IDLE_MODE;

	_frameCheckSuppressed = false;
	_extendedFrameLength = false;

#ifndef DEBUG
	pinMode(_ss, OUTPUT);
	SPI.begin();
#endif
}

DW1000::~DW1000() {
#ifndef DEBUG
	SPI.end();
#endif
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
	char* infoString = (char*)malloc(128);
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
	setBit(_syscfg, LEN_SYS_CFG, FFEN_BIT, val);
	writeBytes(SYS_CFG, NO_SUB, _syscfg, LEN_SYS_CFG);
}

void DW1000::setDoubleBuffering(boolean val) {
	setBit(_syscfg, LEN_SYS_CFG, RXAUTR_BIT, !val);
	writeBytes(SYS_CFG, NO_SUB, _syscfg, LEN_SYS_CFG);
}

void DW1000::setReceiverAutoReenable(boolean val) {
	setBit(_syscfg, LEN_SYS_CFG, DIS_DRXB_BIT, val);
	writeBytes(SYS_CFG, NO_SUB, _syscfg, LEN_SYS_CFG);
}

void DW1000::idle() {
	memset(_sysctrl, 0, LEN_SYS_CTRL);
	bitSet(_sysctrl[0], TRXOFF_BIT);
	_deviceMode = IDLE_MODE;
	writeBytes(SYS_CTRL, NO_SUB, _sysctrl, LEN_SYS_CTRL);
}

void DW1000::waitForResponse(boolean val) {
	setBit(_sysctrl, LEN_SYS_CTRL, WAIT4RESP_BIT, val);
}

void DW1000::suppressFrameCheck() {
	bitSet(_sysctrl[0], SFCST_BIT);
	_frameCheckSuppressed = true;
}

void DW1000::delayedTransceive(unsigned int delayNanos) {
	if(_deviceMode == TX_MODE) {
		setBit(_sysctrl, LEN_SYS_CTRL, TXDLYS_BIT, true);
	} else if(_deviceMode == RX_MODE) {
		setBit(_sysctrl, LEN_SYS_CTRL, RXDLYS_BIT, true);
	} else {
		// in idle, ignore
		return;
	}
	// TODO impl 40 bit in DX_TIME register, 9 lower sig. bits are ignored
}

void DW1000::transmitRate(byte rate) {
	rate &= 0x03;
	if(rate >= 0x03) {
		rate = TX_RATE_6800KBPS;
	}
	_txfctrl[1] |= (byte)((rate << 5) & 0xFF);
}

void DW1000::pulseFrequency(byte freq) {
	freq &= 0x03;
	if(freq == 0x00 || freq >= 0x03) {
		freq = TX_PULSE_FREQ_64MHZ;
	}
	_txfctrl[2] |= (byte)(freq & 0xFF);
}

void DW1000::preambleLength(byte prealen) {
	prealen &= 0x0F;
	_txfctrl[2] |= (byte)((prealen << 2) & 0xFF);
	// TODO set PAC size accordingly for RX (see table 6, page 31)
}

void DW1000::newReceive() {
	memset(_sysctrl, 0, LEN_SYS_CTRL);
	_deviceMode = RX_MODE;
	_frameCheckSuppressed = false;
}

void DW1000::startReceive() {
	setBit(_sysctrl, LEN_SYS_CTRL, RXENAB_BIT, true);
	writeBytes(SYS_CTRL, NO_SUB, _sysctrl, LEN_SYS_CTRL);
}

void DW1000::cancelReceive() {
	newReceive();
	idle();
}

// TODO implement data(), other TX states, ...

void DW1000::newTransmit() {
	// clear out SYS_CTRL for a new transmit operation
	memset(_sysctrl, 0, LEN_SYS_CTRL);
	memset(_txfctrl, 0, LEN_TX_FCTRL);
	_deviceMode = TX_MODE;
	_frameCheckSuppressed = false;
}

void DW1000::setDefaults() {
	if(_deviceMode == TX_MODE) {
		transmitRate(TX_RATE_6800KBPS);
		pulseFrequency(TX_PULSE_FREQ_64MHZ);
		preambleLength(TX_PREAMBLE_LEN_1024);
	} else if(_deviceMode == RX_MODE) {
		// TODO impl
	}
}

void DW1000::cancelTransmit() {
	newTransmit();
	idle();
}

void DW1000::startTransmit() {
	// set transmit flag
	bitSet(_sysctrl[0], TXSTRT_BIT);
	// TODO ... write to device (_sysctrl, _txfctrl)
	writeBytes(TX_FCTRL, NO_SUB, _txfctrl, LEN_TX_FCTRL);
	writeBytes(SYS_CTRL, NO_SUB, _sysctrl, LEN_SYS_CTRL);
	
	// reset to idel
	_deviceMode = IDLE_MODE;
}

void DW1000::setData(byte data[], int n) {
	if(!_frameCheckSuppressed) {
		n+=2; // two bytes CRC-16
	}
	if(n > LEN_TX_BUFFER) {
		return; // TODO proper error handling: frame/buffer size
	}
	if(!_extendedFrameLength && n > LEN_UWB_FRAMES ||
		_extendedFrameLength && n > LEN_EXT_UWB_FRAMES) {
		return; // TODO proper error handling: frame/buffer size
	}
	// transmit data and length
	writeBytes(TX_BUFFER, NO_SUB, data, n);
	_txfctrl[0] = (byte)(n & 0xFF); // 1 byte regular length
	_txfctrl[1] |= (byte)((n >> 8) & 0x07);	// 3 added bits if extended length
}

// system event register
boolean DW1000::isTransmitDone() {
	byte data[LEN_SYS_STATUS];
	// read whole register and check bit
	readBytes(SYS_STATUS, data, LEN_SYS_STATUS);
	return getBit(data, LEN_SYS_STATUS, TXFRS_BIT);
}

boolean DW1000::isLDEDone() {
	byte data[LEN_SYS_STATUS];
	// read whole register and check bit
	readBytes(SYS_STATUS, data, LEN_SYS_STATUS);
	return getBit(data, LEN_SYS_STATUS, LDEDONE_BIT);
}

boolean DW1000::isReceiveDone() {
	byte data[LEN_SYS_STATUS];
	// read whole register and check bit
	readBytes(SYS_STATUS, data, LEN_SYS_STATUS);
	return getBit(data, LEN_SYS_STATUS, RXDFR_BIT);
}

boolean DW1000::isReceiveSuccess() {
	byte data[LEN_SYS_STATUS];
	boolean ldeDone, ldeErr, rxGood, rxErr, rxDecodeErr;
	
	// read whole register and check bits
	readBytes(SYS_STATUS, data, LEN_SYS_STATUS);
	// first check for errors
	ldeErr = getBit(data, LEN_SYS_STATUS, LDEERR_BIT);
	rxErr = getBit(data, LEN_SYS_STATUS, RXFCE_BIT);
	rxDecodeErr = getBit(data, LEN_SYS_STATUS, RXRFSL_BIT);
	if(ldeErr || rxErr || rxDecodeErr) {
		return false; 
	}
	// no errors, check for success indications
	rxGood = getBit(data, LEN_SYS_STATUS, RXFCG_BIT);
	ldeDone = getBit(data, LEN_SYS_STATUS, LDEDONE_BIT);
	if(rxGood && ldeDone) {
		return true;
	}
	// TODO proper 'undecided' handling
	return false;
}

void DW1000::clearReceiveStatus() {
	byte data[LEN_SYS_STATUS];
	
	// read whole register
	readBytes(SYS_STATUS, data, LEN_SYS_STATUS);
	// clear latched RX bits (i.e. write 1 to clear)
	setBit(data, LEN_SYS_STATUS, RXDFR_BIT, true);
	setBit(data, LEN_SYS_STATUS, LDEDONE_BIT, true);
	setBit(data, LEN_SYS_STATUS, LDEERR_BIT, true);
	setBit(data, LEN_SYS_STATUS, RXFCE_BIT, true);
	setBit(data, LEN_SYS_STATUS, RXFCG_BIT, true);
	setBit(data, LEN_SYS_STATUS, RXRFSL_BIT, true);
	writeBytes(SYS_STATUS, NO_SUB, data, LEN_SYS_STATUS);
}

void DW1000::clearTransmitStatus() {
	// FIXME impl
	// setBit(data, LEN_SYS_STATUS, TXFRS_BIT, true);
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
		return; // TODO proper error handling: out of bounds
	}
	byte* targetByte = &data[idx];
	if(val) {
		bitSet(*targetByte, shift);
	} else {
		bitClear(*targetByte, shift);
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
		return false; // TODO proper error handling: out of bounds
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

#ifndef DEBUG
	digitalWrite(_ss, LOW);
	SPI.transfer(READ | cmd);
#endif
	for(i = 0; i < n; i++) {
#ifndef DEBUG
		data[i] = SPI.transfer(JUNK);
#else
		data[i] = debugBuffer[i];
#endif
	}
#ifndef DEBUG
	digitalWrite(_ss,HIGH);
#endif
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
	// TODO proper error handling: address out of bounds

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
	
#ifndef DEBUG
	digitalWrite(_ss, LOW);
#endif
	for(i = 0; i < headerLen; i++) {
#ifndef DEBUG		
		SPI.transfer(header[i]);
#endif
	}
	for(i = 0; i < n; i++) {
#ifndef DEBUG
		SPI.transfer(data[i]);
#else
		debugBuffer[i] = data[i];
#endif
	}
#ifndef DEBUG
	digitalWrite(_ss,HIGH);
#endif
}
