/*
 * Copyright (c) 2015 by Thomas Trojer <thomas@trojer.net>
 * Decawave DW1000 library for Arduino.
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include "pins_arduino.h"
#include "DW1000.h"

DW1000Class DW1000;

/* ###########################################################################
 * #### Static member variables ##############################################
 * ######################################################################### */

// pins
unsigned int DW1000Class::_ss;
unsigned int DW1000Class::_rst;
unsigned int DW1000Class::_irq;
// IRQ callbacks
void (*DW1000Class::_handleSent)(void) = 0;
void (*DW1000Class::_handleReceived)(void) = 0;
void (*DW1000Class::_handleReceiveError)(void) = 0;
void (*DW1000Class::_handleReceiveTimeout)(void) = 0;
void (*DW1000Class::_handleReceiveTimestampAvailable)(void) = 0;
// message printing
char DW1000Class::_msgBuf[1024];
// registers
byte DW1000Class::_syscfg[LEN_SYS_CFG];
byte DW1000Class::_sysctrl[LEN_SYS_CTRL];
byte DW1000Class::_sysstatus[LEN_SYS_STATUS];
byte DW1000Class::_txfctrl[LEN_TX_FCTRL];
byte DW1000Class::_sysmask[LEN_SYS_MASK];
byte DW1000Class::_chanctrl[LEN_CHAN_CTRL];
byte DW1000Class::_networkAndAddress[LEN_PANADR];
// driver internal state
boolean DW1000Class::_extendedFrameLength = false;
boolean DW1000Class::_permanentReceive = false;
int DW1000Class::_deviceMode = IDLE_MODE;

/* ###########################################################################
 * #### Init and end #######################################################
 * ######################################################################### */

void DW1000Class::end() {
	SPI.end();
}

void DW1000Class::select(int ss) {
	_ss = ss;
	pinMode(_ss, OUTPUT);
	digitalWrite(_ss, HIGH);
}

void DW1000Class::begin(int ss, int rst, int irq) {
	select(ss);
	begin(rst, irq);
}

void DW1000Class::begin(int rst, int irq) {
	// SPI setup
	SPI.begin();
	SPI.setBitOrder(MSBFIRST);
	SPI.setDataMode(SPI_MODE0);
	// TODO increase clock speed after chip clock lock (CPLOCK in 0x0f)
	SPI.setClockDivider(SPI_CLOCK_DIV8);
	// pin and basic member setup
	_rst = rst;
	_irq = irq;
	_deviceMode = IDLE_MODE;
	_extendedFrameLength = false;
	pinMode(_rst, OUTPUT);
	digitalWrite(_rst, HIGH);
	// reset chip
	reset();
	// default network and node id
	writeValueToBytes(_networkAndAddress, 0xFF, LEN_PANADR);
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
	writeValueToBytes(otpctrl, 0x8000, LEN_OTP_CTRL);
	writeValueToBytes(pmscctrl0, 0x0301, LEN_PMSC_CTRL0);
	writeBytes(PMSC_CTRL0, NO_SUB, pmscctrl0, LEN_PMSC_CTRL0);
	writeBytes(OTP_CTRL, OTP_CTRL_SUB, otpctrl, LEN_OTP_CTRL);
	delay(10);
	writeValueToBytes(pmscctrl0, 0x0200, LEN_PMSC_CTRL0);
	writeBytes(PMSC_CTRL0, NO_SUB, pmscctrl0, LEN_PMSC_CTRL0);
	tune();
	delay(10);
	// attach interrupt
	attachInterrupt(_irq, DW1000Class::handleInterrupt, RISING);
}

void DW1000Class::reset() {
	digitalWrite(_rst, LOW);
	delay(10);
	digitalWrite(_rst, HIGH);
	delay(10);
	// force into idle mode (although it should be already after reset)
	idle();
}

void DW1000Class::tune() {
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
	writeValueToBytes(agctune1, 0x8870, LEN_AGC_TUNE1);
	writeValueToBytes(agctune2, 0x2502A907, LEN_AGC_TUNE2);
	writeValueToBytes(drxtune2, 0x311A002D, LEN_DRX_TUNE2);
	writeValueToBytes(ldecfg1, 0x6D, LEN_LDE_CFG1);
	writeValueToBytes(ldecfg2, 0x1607, LEN_LDE_CFG2);
	writeValueToBytes(txpower, 0x0E082848, LEN_TX_POWER);
	writeValueToBytes(rftxctrl, 0x001E3FE0, LEN_RF_TXCTRL);
	writeValueToBytes(tcpgdelay, 0xC0, LEN_TC_PGDELAY);
	writeValueToBytes(fsplltune, 0xA6, LEN_FS_PLLTUNE);
	writeBytes(AGC_TUNE, AGC_TUNE1_SUB, agctune1, LEN_AGC_TUNE1);
	writeBytes(AGC_TUNE, AGC_TUNE2_SUB, agctune2, LEN_AGC_TUNE2);
	writeBytes(DRX_TUNE, DRX_TUNE2_SUB, drxtune2, LEN_DRX_TUNE2);
	writeBytes(LDE_CFG, LDE_CFG1_SUB, ldecfg1, LEN_LDE_CFG1);
	writeBytes(LDE_CFG, LDE_CFG2_SUB, ldecfg2, LEN_LDE_CFG2);
	writeBytes(TX_POWER, NO_SUB, txpower, LEN_TX_POWER);
	writeBytes(RF_CONF, RF_TXCTRL_SUB, rftxctrl, LEN_RF_TXCTRL);
	writeBytes(TX_CAL, TC_PGDELAY_SUB, tcpgdelay, LEN_TC_PGDELAY);
	writeBytes(FS_CTRL, FS_PLLTUNE_SUB, fsplltune, LEN_FS_PLLTUNE);
	// TODO LDOTUNE, see 2.5.5, p. 21
}

/* ###########################################################################
 * #### Interrupt handling ###################################################
 * ######################################################################### */

void DW1000Class::handleInterrupt() {
	// read current status and handle via callbacks
	readSystemEventStatusRegister();
	if(isTransmitDone() && _handleSent != 0) {
		(*_handleSent)();
		clearTransmitStatus();
	}
	if(isReceiveDone() && _handleReceived != 0) {
		(*_handleReceived)();
		clearReceiveStatus();
		if(_permanentReceive) {
			startReceive();
		}
	} else if(isReceiveError() && _handleReceiveError != 0) {
		(*_handleReceiveError)();
		clearReceiveStatus();
		if(_permanentReceive) {
			startReceive();
		}
	} else if(isReceiveTimeout() && _handleReceiveTimeout != 0) {
		(*_handleReceiveTimeout)();
		clearReceiveStatus();
		if(_permanentReceive) {
			startReceive();
		}
	}
	if(isReceiveTimestampAvailable() && _handleReceiveTimestampAvailable != 0) {
		(*_handleReceiveTimestampAvailable)();
		clearReceiveTimestampAvailableStatus();
	}
	// TODO impl other callbacks
}

/* ###########################################################################
 * #### Pretty printed device information ####################################
 * ######################################################################### */

char* DW1000Class::getPrintableDeviceIdentifier() {
	byte data[LEN_DEV_ID];
	readBytes(DEV_ID, NO_SUB, data, LEN_DEV_ID);
	sprintf(_msgBuf, "DECA - model: %d, version: %d, revision: %d", 
		data[1], (data[0] >> 4) & 0x0F, data[0] & 0x0F);
	return _msgBuf;
}

char* DW1000Class::getPrintableExtendedUniqueIdentifier() {
	byte data[LEN_EUI];
	readBytes(EUI, NO_SUB, data, LEN_EUI);
	sprintf(_msgBuf, "EUI: %d:%d:%d:%d:%d, OUI: %d:%d:%d",
		data[0], data[1], data[2], data[3], data[4], data[5], data[6], data[7]);
	return _msgBuf;
}

char* DW1000Class::getPrintableNetworkIdAndShortAddress() {
	byte data[LEN_PANADR];
	readBytes(PANADR, NO_SUB, data, LEN_PANADR);
	sprintf(_msgBuf, "PAN: %u, Short Address: %u",
		(unsigned int)((data[3] << 8) | data[2]), (unsigned int)((data[1] << 8) | data[0]));
	return _msgBuf;
}

/* ###########################################################################
 * #### DW1000 register read/write ###########################################
 * ######################################################################### */

void DW1000Class::readSystemConfigurationRegister() {
	readBytes(SYS_CFG, NO_SUB, _syscfg, LEN_SYS_CFG);
}

void DW1000Class::writeSystemConfigurationRegister() {
	writeBytes(SYS_CFG, NO_SUB, _syscfg, LEN_SYS_CFG);
}

void DW1000Class::readSystemEventStatusRegister() {
	readBytes(SYS_STATUS, NO_SUB, _sysstatus, LEN_SYS_STATUS);
}

void DW1000Class::readNetworkIdAndDeviceAddress() {
	readBytes(PANADR, NO_SUB, _networkAndAddress, LEN_PANADR);
}

void DW1000Class::writeNetworkIdAndDeviceAddress() {
	writeBytes(PANADR, NO_SUB, _networkAndAddress, LEN_PANADR);
}

void DW1000Class::readSystemEventMaskRegister() {
	readBytes(SYS_MASK, NO_SUB, _sysmask, LEN_SYS_MASK);
}

void DW1000Class::writeSystemEventMaskRegister() {
	writeBytes(SYS_MASK, NO_SUB, _sysmask, LEN_SYS_MASK);
}

void DW1000Class::readChannelControlRegister() {
	readBytes(CHAN_CTRL, NO_SUB, _chanctrl, LEN_CHAN_CTRL);
}

void DW1000Class::writeChannelControlRegister() {
	writeBytes(CHAN_CTRL, NO_SUB, _chanctrl, LEN_CHAN_CTRL);
}

void DW1000Class::readTransmitFrameControlRegister() {
	readBytes(TX_FCTRL, NO_SUB, _txfctrl, LEN_TX_FCTRL);
}

void DW1000Class::writeTransmitFrameControlRegister() {
	writeBytes(TX_FCTRL, NO_SUB, _txfctrl, LEN_TX_FCTRL);
}

/* ###########################################################################
 * #### DW1000 operation functions ###########################################
 * ######################################################################### */

void DW1000Class::setNetworkId(unsigned int val) {
	_networkAndAddress[2] = (byte)(val & 0xFF);
	_networkAndAddress[3] = (byte)((val >> 8) & 0xFF);
}

void DW1000Class::setDeviceAddress(unsigned int val) {
	_networkAndAddress[0] = (byte)(val & 0xFF);
	_networkAndAddress[1] = (byte)((val >> 8) & 0xFF);
}

void DW1000Class::setFrameFilter(boolean val) {
	setBit(_syscfg, LEN_SYS_CFG, FFEN_BIT, val);
}

void DW1000Class::setDoubleBuffering(boolean val) {
	setBit(_syscfg, LEN_SYS_CFG, DIS_DRXB_BIT, !val);
}

void DW1000Class::setInterruptPolarity(boolean val) {
	setBit(_syscfg, LEN_SYS_CFG, HIRQ_POL_BIT, val);
}

void DW1000Class::setReceiverAutoReenable(boolean val) {
	setBit(_syscfg, LEN_SYS_CFG, RXAUTR_BIT, val);
}

void DW1000Class::interruptOnSent(boolean val) {
	setBit(_sysmask, LEN_SYS_MASK, TXFRS_BIT, val);
}

void DW1000Class::interruptOnReceived(boolean val) {
	setBit(_sysmask, LEN_SYS_MASK, RXDFR_BIT, val);
}

void DW1000Class::interruptOnReceiveError(boolean val) {
	setBit(_sysmask, LEN_SYS_STATUS, LDEERR_BIT, val);
	setBit(_sysmask, LEN_SYS_STATUS, RXFCE_BIT, val);
	setBit(_sysmask, LEN_SYS_STATUS, RXPHE_BIT, val);
	setBit(_sysmask, LEN_SYS_STATUS, RXRFSL_BIT, val);
}

void DW1000Class::interruptOnReceiveTimeout(boolean val) {
	setBit(_sysmask, LEN_SYS_MASK, RXRFTO_BIT, val);
}

void DW1000Class::interruptOnReceiveTimestampAvailable(boolean val) {
	setBit(_sysmask, LEN_SYS_MASK, LDEDONE_BIT, val);
}

void DW1000Class::interruptOnAutomaticAcknowledgeTrigger(boolean val) {
	setBit(_sysmask, LEN_SYS_MASK, AAT_BIT, val);
}

void DW1000Class::clearInterrupts() {
	memset(_sysmask, 0, LEN_SYS_MASK);
}

void DW1000Class::idle() {
	memset(_sysctrl, 0, LEN_SYS_CTRL);
	setBit(_sysctrl, LEN_SYS_CTRL, TRXOFF_BIT, true);
	_deviceMode = IDLE_MODE;
	writeBytes(SYS_CTRL, NO_SUB, _sysctrl, LEN_SYS_CTRL);
}

void DW1000Class::newReceive() {
	idle();
	memset(_sysctrl, 0, LEN_SYS_CTRL);
	clearReceiveStatus();
	_deviceMode = RX_MODE;
}

void DW1000Class::startReceive() {
	setBit(_sysctrl, LEN_SYS_CTRL, RXENAB_BIT, true);
	writeBytes(SYS_CTRL, NO_SUB, _sysctrl, LEN_SYS_CTRL);
}

void DW1000Class::newTransmit() {
	idle();
	memset(_sysctrl, 0, LEN_SYS_CTRL);
	clearTransmitStatus();
	_deviceMode = TX_MODE;
}

void DW1000Class::startTransmit() {
	setBit(_sysctrl, LEN_SYS_CTRL, TXSTRT_BIT, true);
	writeBytes(SYS_CTRL, NO_SUB, _sysctrl, LEN_SYS_CTRL);
	if(_permanentReceive) {
		memset(_sysctrl, 0, LEN_SYS_CTRL);
		startReceive();
	} else {
		_deviceMode = IDLE_MODE;
	}
}

void DW1000Class::newConfiguration() {
	idle();
	readNetworkIdAndDeviceAddress();
	readSystemConfigurationRegister();
	readChannelControlRegister();
	readTransmitFrameControlRegister();
	readSystemEventMaskRegister();
}

void DW1000Class::commitConfiguration() {
	writeNetworkIdAndDeviceAddress();
	writeSystemConfigurationRegister();
	writeChannelControlRegister();
	writeTransmitFrameControlRegister();
	writeSystemEventMaskRegister();
}

void DW1000Class::waitForResponse(boolean val) {
	setBit(_sysctrl, LEN_SYS_CTRL, WAIT4RESP_BIT, val);
}

void DW1000Class::suppressFrameCheck(boolean val) {
	setBit(_sysctrl, LEN_SYS_CTRL, SFCST_BIT, val);
}

boolean DW1000Class::isSuppressFrameCheck() {
	return getBit(_sysctrl, LEN_SYS_CTRL, SFCST_BIT);
}

unsigned long DW1000Class::delayedTransceive(unsigned int value, unsigned long factorNs) {
	if(_deviceMode == TX_MODE) {
		setBit(_sysctrl, LEN_SYS_CTRL, TXDLYS_BIT, true);
	} else if(_deviceMode == RX_MODE) {
		setBit(_sysctrl, LEN_SYS_CTRL, RXDLYS_BIT, true);
	} else {
		// in idle, ignore
		return -1;
	}
	byte delayBytes[5];
	// note: counter wrap-around is considered by unsigned long overflow modulo behavior
	unsigned long tsValue = getSystemTimestamp() + (value * factorNs) / TIME_RES;
	writeLongToTimestamp(tsValue, delayBytes);
	writeBytes(DX_TIME, NO_SUB, delayBytes, LEN_DX_TIME);
	return tsValue;
}

void DW1000Class::dataRate(byte rate) {
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

void DW1000Class::pulseFrequency(byte freq) {
	freq &= 0x03;
	if(freq == 0x00 || freq >= 0x03) {
		freq = TX_PULSE_FREQ_16MHZ;
	}
	_txfctrl[2] |= (byte)(freq & 0xFF);
	_chanctrl[2] |= (byte)((freq << 2) & 0xFF);
	// tuning
	byte agctune1[LEN_AGC_TUNE1];
	if(freq == TX_PULSE_FREQ_16MHZ) {
		writeValueToBytes(agctune1, 0x8870, LEN_AGC_TUNE1);
	} else if(freq == TX_PULSE_FREQ_64MHZ) {
		writeValueToBytes(agctune1, 0x889B, LEN_AGC_TUNE1);
	} else {
		return;
	}
	writeBytes(AGC_TUNE, AGC_TUNE1_SUB, agctune1, LEN_AGC_TUNE1);
}

void DW1000Class::preambleLength(byte prealen) {
	prealen &= 0x0F;
	_txfctrl[2] |= (byte)((prealen << 2) & 0xFF);
	// TODO set PAC size accordingly for RX (see table 6, page 31)
}

void DW1000Class::extendedFrameLength(boolean val) {
	byte extLen = 0x00;
	if(val) {
		extLen = 0x03;
	}
	_sysctrl[3] |= extLen;
}

void DW1000Class::permanentReceive(boolean val) {
	_permanentReceive = val;
	if(val) {
		// in case permanent, also reenable receiver once failed
		setReceiverAutoReenable(true);
		writeSystemConfigurationRegister();
	}
}

void DW1000Class::setDefaults() {
	if(_deviceMode == TX_MODE) {
		interruptOnSent(true);
		extendedFrameLength(false);
		suppressFrameCheck(false);
	} else if(_deviceMode == RX_MODE) {
		extendedFrameLength(false);
		suppressFrameCheck(false);
		//permanentReceive(true); // includes RX auto reenable
	} else if(_deviceMode == IDLE_MODE) {
		/*dataRate(TRX_RATE_6800KBPS);
		pulseFrequency(TX_PULSE_FREQ_16MHZ);
		preambleLength(TX_PREAMBLE_LEN_1024);*/
		interruptOnSent(true);
		interruptOnReceived(true);
		writeSystemEventMaskRegister();
		interruptOnAutomaticAcknowledgeTrigger(true);
		setReceiverAutoReenable(true);
	}
}

void DW1000Class::setData(byte data[], int n) {
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

void DW1000Class::setData(const String& data) {
	int n = data.length()+1;
	byte* dataBytes = (byte*)malloc(n);
	data.getBytes(dataBytes, n);
	setData(dataBytes, n);
	free(dataBytes);
	
}

int DW1000Class::getDataLength() {
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

void DW1000Class::getData(byte data[], int n) {
	if(n < 0) {
		return;
	}
	readBytes(RX_BUFFER, NO_SUB, data, n);
}

void DW1000Class::getData(String& data) {
	int i;
	int n = getDataLength(); // number of bytes w/o the two FCS ones
	if(n < 0) {
		return;
	}
	byte* dataBytes = (byte*)malloc(n);
	getData(dataBytes, n);
	// clear string
	data.remove(0);
	data = "";
	// append to string
	for(i = 0; i < n; i++) {
		data += (char)dataBytes[i];
	}
	free(dataBytes);
}

unsigned long DW1000Class::getTransmitTimestamp() {
	byte txTimeBytes[LEN_TX_STAMP];
	readBytes(TX_TIME, TX_STAMP_SUB, txTimeBytes, LEN_TX_STAMP);
	return getTimestampAsLong(txTimeBytes+1);
}

unsigned long DW1000Class::getReceiveTimestamp() {
	byte rxTimeBytes[LEN_RX_STAMP];
	readBytes(RX_TIME, RX_STAMP_SUB, rxTimeBytes, LEN_RX_STAMP);
	return getTimestampAsLong(rxTimeBytes+1);
}

unsigned long DW1000Class::getSystemTimestamp() {
	byte sysTimeBytes[LEN_SYS_TIME];
	readBytes(SYS_TIME, NO_SUB, sysTimeBytes, LEN_SYS_TIME);
	return getTimestampAsLong(sysTimeBytes+1);
}

unsigned long DW1000Class::getTimestampAsLong(byte ts[]) {
	unsigned long tsValue = ((unsigned long)ts[0] >> 1);
	tsValue |= ((unsigned long)ts[1] << 7);
	tsValue |= ((unsigned long)ts[2] << 15);
	tsValue |= ((unsigned long)ts[3] << 23);
	return tsValue;
}

void DW1000Class::writeLongToTimestamp(unsigned long tsValue, byte ts[]) {
	ts[0] = (byte)0;
	ts[1] = (byte)((tsValue << 1) & 0xFF);
	ts[2] = (byte)((tsValue >> 7) & 0xFF);
	ts[3] = (byte)((tsValue >> 15) & 0xFF);
	ts[4] = (byte)((tsValue >> 23) & 0xFF);
}

boolean DW1000Class::isTransmitDone() {
	return getBit(_sysstatus, LEN_SYS_STATUS, TXFRS_BIT);
}

boolean DW1000Class::isReceiveTimestampAvailable() {
	return getBit(_sysstatus, LEN_SYS_STATUS, LDEDONE_BIT);
}

boolean DW1000Class::isReceiveDone() {
	return getBit(_sysstatus, LEN_SYS_STATUS, RXDFR_BIT);
}

boolean DW1000Class::isReceiveError() {
	boolean ldeErr, rxCRCErr, rxHeaderErr, rxDecodeErr;
	ldeErr = getBit(_sysstatus, LEN_SYS_STATUS, LDEERR_BIT);
	rxCRCErr = getBit(_sysstatus, LEN_SYS_STATUS, RXFCE_BIT);
	rxHeaderErr = getBit(_sysstatus, LEN_SYS_STATUS, RXPHE_BIT);
	rxDecodeErr = getBit(_sysstatus, LEN_SYS_STATUS, RXRFSL_BIT);
	if(ldeErr || rxCRCErr || rxHeaderErr || rxDecodeErr) {
		return true; 
	}
	return false;
}

boolean DW1000Class::isReceiveTimeout() {
	return getBit(_sysstatus, LEN_SYS_STATUS, RXRFTO_BIT);
}

void DW1000Class::clearAllStatus() {
	memset(_sysstatus, 0, LEN_SYS_STATUS);
	writeBytes(SYS_STATUS, NO_SUB, _sysstatus, LEN_SYS_STATUS);
}

void DW1000Class::clearReceiveTimestampAvailableStatus() {
	setBit(_sysstatus, LEN_SYS_STATUS, LDEDONE_BIT, true);
	writeBytes(SYS_STATUS, NO_SUB, _sysstatus, LEN_SYS_STATUS);
}

void DW1000Class::clearReceiveStatus() {
	// clear latched RX bits (i.e. write 1 to clear)
	setBit(_sysstatus, LEN_SYS_STATUS, RXDFR_BIT, true);
	setBit(_sysstatus, LEN_SYS_STATUS, LDEDONE_BIT, true);
	setBit(_sysstatus, LEN_SYS_STATUS, LDEERR_BIT, true);
	setBit(_sysstatus, LEN_SYS_STATUS, RXPHE_BIT, true);
	setBit(_sysstatus, LEN_SYS_STATUS, RXFCE_BIT, true);
	setBit(_sysstatus, LEN_SYS_STATUS, RXFCG_BIT, true);
	setBit(_sysstatus, LEN_SYS_STATUS, RXRFSL_BIT, true);
	writeBytes(SYS_STATUS, NO_SUB, _sysstatus, LEN_SYS_STATUS);
}

void DW1000Class::clearTransmitStatus() {
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
void DW1000Class::setBit(byte data[], int n, int bit, boolean val) {
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
boolean DW1000Class::getBit(byte data[], int n, int bit) {
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

void DW1000Class::writeValueToBytes(byte data[], int val, int n) {
	int i;	
	for(i = 0; i < n; i++) {
		data[i] = ((val >> (i * 8)) & 0xFF);
	}
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
void DW1000Class::readBytes(byte cmd, word offset, byte data[], int n) {
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
	noInterrupts();
	digitalWrite(_ss, LOW);
	for(i = 0; i < headerLen; i++) {
		SPI.transfer(header[i]);
	}
	for(i = 0; i < n; i++) {
		data[i] = SPI.transfer(JUNK);
	}
	digitalWrite(_ss,HIGH);
	interrupts();
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
void DW1000Class::writeBytes(byte cmd, word offset, byte data[], int n) {
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
	noInterrupts();
	digitalWrite(_ss, LOW);
	for(i = 0; i < headerLen; i++) {
		SPI.transfer(header[i]);
	}
	for(i = 0; i < n; i++) {
		SPI.transfer(data[i]);
	}
	delay(1);
	digitalWrite(_ss,HIGH);
	interrupts();
	delay(1);
}

char* DW1000Class::getPrettyBytes(byte data[], int n) {
	unsigned int i, j, b;
	b = sprintf(_msgBuf, "Data, bytes: %d\nB: 7 6 5 4 3 2 1 0\n", n);
	for(i = 0; i < n; i++) {
		byte curByte = data[i];
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
	return _msgBuf;
}

char* DW1000Class::getPrettyBytes(byte cmd, word offset, int n) {
	unsigned int i, j, b;
	byte* readBuf = (byte*)malloc(n);
	readBytes(cmd, offset, readBuf, n);
	b = sprintf(_msgBuf, "Reg: 0x%02x, bytes: %d\nB: 7 6 5 4 3 2 1 0\n", cmd, n);
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
	free(readBuf);
	return _msgBuf;
}
