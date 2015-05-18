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

DW1000::DW1000(int ss, int rst) {
	_ss = ss;
	_rst = rst;
	_deviceMode = IDLE_MODE;
	_extendedFrameLength = false;
#ifndef DEBUG
	pinMode(_ss, OUTPUT);
	digitalWrite(_ss, HIGH);
	pinMode(_rst, OUTPUT);
	digitalWrite(_rst, HIGH);
	SPI.begin();
	SPI.setBitOrder(MSBFIRST);
	SPI.setDataMode(SPI_MODE0);
	// TODO increase clock speed after chip clock lock (CPLOCK in 0x0f)
	SPI.setClockDivider(SPI_CLOCK_DIV8);
#endif
}

DW1000::~DW1000() {
#ifndef DEBUG
	SPI.end();
#endif
}

void DW1000::initialize() {
	// reset chip
	digitalWrite(_rst, LOW);
	delay(10);
	digitalWrite(_rst, HIGH);
	delay(10);
	// default network and node id
	memset(_networkAndAddress, 0xFF, LEN_PANADR);
	writeNetworkIdAndDeviceAddress();
	// default system configuration
	memset(_syscfg, 0, LEN_SYS_CFG);
	setDoubleBuffering(false);
	setInterruptPolarity(true);
	writeSystemConfigurationRegister();
	// default interrupt mask, i.e. no interrupts
	clearInterrupts();
	writeSystemEventMaskRegister();
	// tell the chip to load the LDE microcode
	byte pmscctrl0[LEN_PMSC_CTRL0];
	byte otpctrl[LEN_OTP_CTRL];
	memset(otpctrl, 0x8000, LEN_OTP_CTRL);
	memset(pmscctrl0, 0x0301, LEN_PMSC_CTRL0);
	writeBytes(PMSC_CTRL0, NO_SUB, pmscctrl0, LEN_PMSC_CTRL0);
	writeBytes(OTP_CTRL, OTP_CTRL_SUB, otpctrl, LEN_OTP_CTRL);
	delay(10);
	memset(pmscctrl0, 0x0200, LEN_PMSC_CTRL0);
	writeBytes(PMSC_CTRL0, NO_SUB, pmscctrl0, LEN_PMSC_CTRL0);
	tune();
	delay(10);
}

void DW1000::tune() {
	// re-tune chip for channel 5 (default)
	byte agctune1[LEN_AGC_TUNE1];
	byte agctune2[LEN_AGC_TUNE2];
	byte drxtune2[LEN_DRX_TUNE2];
	byte ldecfg1[LEN_LDE_CFG1];
	byte ldecfg2[LEN_LDE_CFG2];
	byte txpower[LEN_TX_POWER];
	byte rftxctrl[LEN_RF_TXCTRL];
	byte tcpgdelay[LEN_TC_PGDELAY];
	byte fsplltune[LEN_FS_PLLTUNE];
	memset(agctune1, 0x8870, LEN_AGC_TUNE1);
	memset(agctune2, 0x2502A907, LEN_AGC_TUNE2);
	memset(drxtune2, 0x311A002D, LEN_DRX_TUNE2);
	memset(ldecfg1, 0x6D, LEN_LDE_CFG1);
	memset(ldecfg2, 0x1607, LEN_LDE_CFG2);
	memset(txpower, 0x0E082848, LEN_TX_POWER);
	memset(rftxctrl, 0x001E3FE0, LEN_RF_TXCTRL);
	memset(tcpgdelay, 0xC0, LEN_TC_PGDELAY);
	memset(fsplltune, 0xA6, LEN_FS_PLLTUNE);
	writeBytes(AGC_TUNE, AGC_TUNE1_SUB, agctune1, LEN_AGC_TUNE1);
	writeBytes(AGC_TUNE, AGC_TUNE2_SUB, agctune2, LEN_AGC_TUNE2);
	writeBytes(DRX_TUNE, DRX_TUNE2_SUB, drxtune2, LEN_DRX_TUNE2);
	writeBytes(LDE_CFG, LDE_CFG1_SUB, ldecfg1, LEN_LDE_CFG1);
	writeBytes(LDE_CFG, LDE_CFG2_SUB, ldecfg2, LEN_LDE_CFG2);
	writeBytes(TX_POWER, NO_SUB, txpower, LEN_TX_POWER);
	writeBytes(RF_CONF, RF_TXCTRL_SUB, rftxctrl, LEN_RF_TXCTRL);
	writeBytes(TX_CAL, TC_PGDELAY_SUB, tcpgdelay, LEN_TC_PGDELAY);
	writeBytes(FS_CTRL, FS_PLLTUNE_SUB, fsplltune, LEN_FS_PLLTUNE);
	
	// TODO others as well, RF_TXCTRL, TX_PGDELAY, FS_PLLTUNE, LDOTUNE
	// TODO see 2.5.5, p. 21
}

/* ###########################################################################
 * #### Member access ########################################################
 * ######################################################################### */

// ...

/* ###########################################################################
 * #### DW1000 operation functions ###########################################
 * ######################################################################### */

char* DW1000::getPrintableDeviceIdentifier() {
	byte data[LEN_DEV_ID];
	readBytes(DEV_ID, NO_SUB, data, LEN_DEV_ID);
	sprintf(_msgBuf, "DECA - model: %d, version: %d, revision: %d", 
		data[1], (data[0] >> 4) & 0x0F, data[0] & 0x0F);
	return _msgBuf;
}

char* DW1000::getPrintableExtendedUniqueIdentifier() {
	byte data[LEN_EUI];
	readBytes(EUI, NO_SUB, data, LEN_EUI);
	sprintf(_msgBuf, "EUI: %d:%d:%d:%d:%d, OUI: %d:%d:%d",
		data[0], data[1], data[2], data[3], data[4], data[5], data[6], data[7]);
	return _msgBuf;
}

char* DW1000::getPrintableNetworkIdAndShortAddress() {
	byte data[LEN_PANADR];
	readBytes(PANADR, NO_SUB, data, LEN_PANADR);
	sprintf(_msgBuf, "PAN: %u, Short Address: %u",
		(unsigned int)((data[3] << 8) | data[2]), (unsigned int)((data[1] << 8) | data[0]));
	return _msgBuf;
}

void DW1000::readSystemConfigurationRegister() {
	readBytes(SYS_CFG, NO_SUB, _syscfg, LEN_SYS_CFG);
}

void DW1000::writeSystemConfigurationRegister() {
	writeBytes(SYS_CFG, NO_SUB, _syscfg, LEN_SYS_CFG);
}

void DW1000::readSystemEventStatusRegister() {
	readBytes(SYS_STATUS, NO_SUB, _sysstatus, LEN_SYS_STATUS);
}

void DW1000::readNetworkIdAndDeviceAddress() {
	readBytes(PANADR, NO_SUB, _networkAndAddress, LEN_PANADR);
}

void DW1000::writeNetworkIdAndDeviceAddress() {
	writeBytes(PANADR, NO_SUB, _networkAndAddress, LEN_PANADR);
}

void DW1000::readSystemEventMaskRegister() {
	readBytes(SYS_MASK, NO_SUB, _sysmask, LEN_SYS_MASK);
}

void DW1000::writeSystemEventMaskRegister() {
	writeBytes(SYS_MASK, NO_SUB, _sysmask, LEN_SYS_MASK);
}

void DW1000::readChannelControlRegister() {
	readBytes(CHAN_CTRL, NO_SUB, _chanctrl, LEN_CHAN_CTRL);
}

void DW1000::writeChannelControlRegister() {
	writeBytes(CHAN_CTRL, NO_SUB, _chanctrl, LEN_CHAN_CTRL);
}

void DW1000::readTransmitFrameControlRegister() {
	readBytes(TX_FCTRL, NO_SUB, _txfctrl, LEN_TX_FCTRL);
}

void DW1000::writeTransmitFrameControlRegister() {
	writeBytes(TX_FCTRL, NO_SUB, _txfctrl, LEN_TX_FCTRL);
}

void DW1000::setNetworkId(unsigned int val) {
	_networkAndAddress[2] = (byte)(val & 0xFF);
	_networkAndAddress[3] = (byte)((val >> 8) & 0xFF);
}

void DW1000::setDeviceAddress(unsigned int val) {
	_networkAndAddress[0] = (byte)(val & 0xFF);
	_networkAndAddress[1] = (byte)((val >> 8) & 0xFF);
}

void DW1000::setFrameFilter(boolean val) {
	setBit(_syscfg, LEN_SYS_CFG, FFEN_BIT, val);
}

void DW1000::setDoubleBuffering(boolean val) {
	setBit(_syscfg, LEN_SYS_CFG, DIS_DRXB_BIT, !val);
}

void DW1000::setInterruptPolarity(boolean val) {
	setBit(_syscfg, LEN_SYS_CFG, HIRQ_POL_BIT, val);
}

void DW1000::setReceiverAutoReenable(boolean val) {
	setBit(_syscfg, LEN_SYS_CFG, RXAUTR_BIT, val);
}

void DW1000::interruptOnSent(boolean val) {
	setBit(_sysmask, LEN_SYS_MASK, TXFRS_BIT, val);
}

void DW1000::interruptOnReceived(boolean val) {
	setBit(_sysmask, LEN_SYS_MASK, RXDFR_BIT, val);
}

void DW1000::interruptOnAutomaticAcknowledgeTrigger(boolean val) {
	setBit(_sysmask, LEN_SYS_MASK, AAT_BIT, val);
}

void DW1000::clearInterrupts() {
	memset(_sysmask, 0, LEN_SYS_MASK);
}

void DW1000::idle() {
	memset(_sysctrl, 0, LEN_SYS_CTRL);
	setBit(_sysctrl, LEN_SYS_CTRL, TRXOFF_BIT, true);
	_deviceMode = IDLE_MODE;
	writeBytes(SYS_CTRL, NO_SUB, _sysctrl, LEN_SYS_CTRL);
}

void DW1000::newConfiguration() {
	idle();
	readNetworkIdAndDeviceAddress();
	readSystemConfigurationRegister();
	readSystemEventMaskRegister();
	readChannelControlRegister();
	readTransmitFrameControlRegister();
}

void DW1000::commitConfiguration() {
	writeNetworkIdAndDeviceAddress();
	writeSystemConfigurationRegister();
	writeSystemEventMaskRegister();
	writeChannelControlRegister();
	writeTransmitFrameControlRegister();
}

void DW1000::waitForResponse(boolean val) {
	setBit(_sysctrl, LEN_SYS_CTRL, WAIT4RESP_BIT, val);
}

void DW1000::suppressFrameCheck(boolean val) {
	setBit(_sysctrl, LEN_SYS_CTRL, SFCST_BIT, val);
}

boolean DW1000::isSuppressFrameCheck() {
	return getBit(_sysctrl, LEN_SYS_CTRL, SFCST_BIT);
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

void DW1000::dataRate(byte rate) {
	rate &= 0x03;
	if(rate >= 0x03) {
		rate = TRX_RATE_850KBPS;
	}
	_txfctrl[1] |= (byte)((rate << 5) & 0xFF);
	if(rate == TRX_RATE_110KBPS) {
		setBit(_syscfg, LEN_SYS_CFG, RXM110K_BIT, true);
	} else {
		setBit(_syscfg, LEN_SYS_CFG, RXM110K_BIT, false);
	}
}

void DW1000::pulseFrequency(byte freq) {
	freq &= 0x03;
	if(freq == 0x00 || freq >= 0x03) {
		freq = TX_PULSE_FREQ_16MHZ;
	}
	_txfctrl[2] |= (byte)(freq & 0xFF);
	_chanctrl[2] |= (byte)((freq << 2) & 0xFF);
	// tuning
	byte agctune1[LEN_AGC_TUNE1];
	if(freq == TX_PULSE_FREQ_16MHZ) {
		memset(agctune1, 0x8870, LEN_AGC_TUNE1);
	} else if(freq == TX_PULSE_FREQ_64MHZ) {
		memset(agctune1, 0x889B, LEN_AGC_TUNE1);
	} else {
		return;
	}
	writeBytes(AGC_TUNE, AGC_TUNE1_SUB, agctune1, LEN_AGC_TUNE1);
}

void DW1000::preambleLength(byte prealen) {
	prealen &= 0x0F;
	_txfctrl[2] |= (byte)((prealen << 2) & 0xFF);
	// TODO set PAC size accordingly for RX (see table 6, page 31)
}

void DW1000::extendedFrameLength(boolean val) {
	byte extLen = 0x00;
	if(val) {
		extLen = 0x03;
	}
	_sysctrl[3] |= extLen;
}

void DW1000::newReceive() {
	memset(_sysctrl, 0, LEN_SYS_CTRL);
	clearReceiveStatus();
	_deviceMode = RX_MODE;
}

void DW1000::startReceive() {
	setBit(_sysctrl, LEN_SYS_CTRL, RXENAB_BIT, true);
	writeBytes(SYS_CTRL, NO_SUB, _sysctrl, LEN_SYS_CTRL);
}

void DW1000::newTransmit() {
	memset(_sysctrl, 0, LEN_SYS_CTRL);
	clearTransmitStatus();
	_deviceMode = TX_MODE;
}

void DW1000::setDefaults() {
	if(_deviceMode == TX_MODE) {
		suppressFrameCheck(false);
		extendedFrameLength(false);
	} else if(_deviceMode == RX_MODE) {
		suppressFrameCheck(false);
		extendedFrameLength(false);
	} else if(_deviceMode == IDLE_MODE) {
		/*dataRate(TRX_RATE_6800KBPS);
		pulseFrequency(TX_PULSE_FREQ_16MHZ);
		preambleLength(TX_PREAMBLE_LEN_1024);
		setReceiverAutoReenable(true);*/
		interruptOnAutomaticAcknowledgeTrigger(true);
	}
}

void DW1000::startTransmit() {
	// set transmit flag
	setBit(_sysctrl, LEN_SYS_CTRL, TXSTRT_BIT, true);
	writeBytes(SYS_CTRL, NO_SUB, _sysctrl, LEN_SYS_CTRL);
	// reset to idle
	_deviceMode = IDLE_MODE;
}

void DW1000::setData(byte data[], unsigned int n) {
	if(!isSuppressFrameCheck()) {
		n+=2; // two bytes CRC-16
	}
	if(n > LEN_TX_BUFFER) {
		return; // TODO proper error handling: frame/buffer size
	}
	if(n > LEN_EXT_UWB_FRAMES) {
		return; // TODO proper error handling: frame/buffer size
	}
	if(n > LEN_UWB_FRAMES) {
		_extendedFrameLength = true;
	} else {
		_extendedFrameLength = false;
	}
	// transmit data and length
	writeBytes(TX_BUFFER, NO_SUB, data, n);
	_txfctrl[0] = (byte)(n & 0xFF); // 1 byte (regular length + 1 bit)
	_txfctrl[1] |= (byte)((n >> 8) & 0x03);	// 2 added bits if extended length
	writeTransmitFrameControlRegister();
}

int DW1000::getDataLength() {
	if(_deviceMode == TX_MODE) {
		// 10 bits of TX frame control register
		return (((_txfctrl[1] << 8) | _txfctrl[0]) & 0x03FF);
	} else if(_deviceMode == RX_MODE) {
		// 10 bits of RX frame control register
		byte rxFrameInfo[LEN_RX_FINFO];
		readBytes(RX_FINFO, NO_SUB, rxFrameInfo, LEN_RX_FINFO);
		// TODO if other frame info bits are used somewhere else, store/cache bytes
		return ((((rxFrameInfo[1] << 8) | rxFrameInfo[0]) & 0x03FF) - 2); // w/o FCS 
	} else {
		return -1; // ignore in idle state
	}
}

int DW1000::getData(byte data[]) {
	int n = getDataLength(); // number of bytes w/o the two FCS ones
	if(n < 0) {
		return n;
	}
	readBytes(RX_BUFFER, NO_SUB, data, n);
	return n;
}

// system event register
boolean DW1000::isTransmitDone() {
	// read whole register and check bit
	readBytes(SYS_STATUS, NO_SUB, _sysstatus, LEN_SYS_STATUS);
	return getBit(_sysstatus, LEN_SYS_STATUS, TXFRS_BIT);
}

boolean DW1000::isLDEDone() {
	// read whole register and check bit
	readBytes(SYS_STATUS,NO_SUB,  _sysstatus, LEN_SYS_STATUS);
	return getBit(_sysstatus, LEN_SYS_STATUS, LDEDONE_BIT);
}

boolean DW1000::isReceiveDone() {
	// read whole register and check bit
	readBytes(SYS_STATUS, NO_SUB, _sysstatus, LEN_SYS_STATUS);
	return getBit(_sysstatus, LEN_SYS_STATUS, RXDFR_BIT);
}

boolean DW1000::isReceiveSuccess() {
	boolean ldeDone, ldeErr, rxGood, rxErr, rxDecodeErr;
	
	// read whole register and check bits
	readBytes(SYS_STATUS, NO_SUB, _sysstatus, LEN_SYS_STATUS);
	// first check for errors
	ldeErr = getBit(_sysstatus, LEN_SYS_STATUS, LDEERR_BIT);
	rxErr = getBit(_sysstatus, LEN_SYS_STATUS, RXFCE_BIT);
	rxDecodeErr = getBit(_sysstatus, LEN_SYS_STATUS, RXRFSL_BIT);
	if(ldeErr || rxErr || rxDecodeErr) {
		return false; 
	}
	// no errors, check for success indications
	rxGood = getBit(_sysstatus, LEN_SYS_STATUS, RXFCG_BIT);
	ldeDone = getBit(_sysstatus, LEN_SYS_STATUS, LDEDONE_BIT);
	if(rxGood && ldeDone) {
		return true;
	}
	// TODO proper 'undecided' handling
	return false;
}

void DW1000::clearReceiveStatus() {
	// clear latched RX bits (i.e. write 1 to clear)
	setBit(_sysstatus, LEN_SYS_STATUS, RXDFR_BIT, true);
	setBit(_sysstatus, LEN_SYS_STATUS, LDEDONE_BIT, true);
	setBit(_sysstatus, LEN_SYS_STATUS, LDEERR_BIT, true);
	setBit(_sysstatus, LEN_SYS_STATUS, RXFCE_BIT, true);
	setBit(_sysstatus, LEN_SYS_STATUS, RXFCG_BIT, true);
	setBit(_sysstatus, LEN_SYS_STATUS, RXRFSL_BIT, true);
	writeBytes(SYS_STATUS, NO_SUB, _sysstatus, LEN_SYS_STATUS);
}

void DW1000::clearTransmitStatus() {
	// clear latched TX bits
	setBit(_sysstatus, LEN_SYS_STATUS, TXFRB_BIT, true);
	setBit(_sysstatus, LEN_SYS_STATUS, TXPRS_BIT, true);
	setBit(_sysstatus, LEN_SYS_STATUS, TXPHS_BIT, true);
	setBit(_sysstatus, LEN_SYS_STATUS, TXFRS_BIT, true);
	writeBytes(SYS_STATUS, NO_SUB, _sysstatus, LEN_SYS_STATUS);
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

	idx = bit / 8;
	if(idx >= n) {
		return; // TODO proper error handling: out of bounds
	}
	byte* targetByte = &data[idx];
	shift = bit % 8;
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
void DW1000::readBytes(byte cmd, word offset, byte data[], int n) {
	byte header[3];
	int headerLen = 1;
	int i;
	if(offset == NO_SUB) {
		header[0] = READ | cmd;
	} else {
		header[0] = READ_SUB | cmd;
		if(offset < 128) {
			header[1] = (byte)offset;
			headerLen++;
		} else {
			header[1] = READ | (byte)offset;
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
	delay(1);
	digitalWrite(_ss,HIGH);
	delay(1);
#endif
}

char* DW1000::getPrettyBytes(unsigned int reg, unsigned int n) {
	unsigned int i, j, b;
	byte* readBuf = (byte*)malloc(n);
	readBytes(reg, NO_SUB, readBuf, n);
	b = sprintf(_msgBuf, "Reg: 0x%02x, bytes: %d\nB: 7 6 5 4 3 2 1 0\n", reg, n);
	for(i = 0; i < n; i++) {
		byte curByte = readBuf[i];
		snprintf(&_msgBuf[b++], 2, "%d", (i + 1));
		_msgBuf[b++] = (char)((i + 1) & 0xFF); _msgBuf[b++] = ':'; _msgBuf[b++] = ' ';
		for(j = 0; j < 8; j++) {
			_msgBuf[b++] = ((curByte >> (7 - j)) & 0x01) ? '1' : '0';
			if(j < 7) {
				_msgBuf[b++] = ' '; 
			} else if(i < n-1) {
				_msgBuf[b++] = '\n';
			} else {
				_msgBuf[b++] = '\0';
			}
		}
		
	}
	_msgBuf[b++] = '\0';
	delete[] readBuf;
	return _msgBuf;
}
