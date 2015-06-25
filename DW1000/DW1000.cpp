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
byte DW1000Class::_extendedFrameLength = FRAME_LENGTH_NORMAL;
byte DW1000Class::_pacSize = PAC_SIZE_8;
byte DW1000Class::_pulseFrequency = TX_PULSE_FREQ_16MHZ;
byte DW1000Class::_dataRate = TRX_RATE_6800KBPS;
byte DW1000Class::_preambleLength = TX_PREAMBLE_LEN_128;
byte DW1000Class::_preambleCode = PREAMBLE_CODE_16MHZ_4;
byte DW1000Class::_channel = CHANNEL_5;
byte DW1000Class::_frameCheck = true;
boolean DW1000Class::_permanentReceive = false;
int DW1000Class::_deviceMode = IDLE_MODE;
// modes of operation
const byte DW1000Class::MODE_LOCATION_LONGRANGE_LOWPOWER[] = {TRX_RATE_110KBPS, TX_PULSE_FREQ_16MHZ, TX_PREAMBLE_LEN_1024};
const byte DW1000Class::MODE_LOCATION_SHORTRANGE_LOWPOWER[] = {TRX_RATE_6800KBPS, TX_PULSE_FREQ_16MHZ, TX_PREAMBLE_LEN_128};
const byte DW1000Class::MODE_LONGDATA_SHORTRANGE_LOWPOWER[] = {TRX_RATE_6800KBPS, TX_PULSE_FREQ_16MHZ, TX_PREAMBLE_LEN_1024};
const byte DW1000Class::MODE_LONGDATA_LONGRANGE_LOWPOWER[] = {TRX_RATE_110KBPS, TX_PULSE_FREQ_16MHZ, TX_PREAMBLE_LEN_1024};
const byte DW1000Class::MODE_SHORTDATA_SHORTRANGE_LOWPOWER[] = {TRX_RATE_6800KBPS, TX_PULSE_FREQ_16MHZ, TX_PREAMBLE_LEN_128};
const byte DW1000Class::MODE_SHORTDATA_LONGRANGE_LOWPOWER[] = {TRX_RATE_110KBPS, TX_PULSE_FREQ_16MHZ, TX_PREAMBLE_LEN_1024};
const byte DW1000Class::MODE_LOCATION_LONGRANGE_ACCURACY[] = {TRX_RATE_110KBPS, TX_PULSE_FREQ_64MHZ, TX_PREAMBLE_LEN_1024};
const byte DW1000Class::MODE_LOCATION_SHORTRANGE_ACCURACY[] = {TRX_RATE_6800KBPS, TX_PULSE_FREQ_64MHZ, TX_PREAMBLE_LEN_128};
const byte DW1000Class::MODE_LONGDATA_SHORTRANGE_ACCURACY[] = {TRX_RATE_6800KBPS, TX_PULSE_FREQ_64MHZ, TX_PREAMBLE_LEN_1024};
const byte DW1000Class::MODE_LONGDATA_LONGRANGE_ACCURACY[] = {TRX_RATE_110KBPS, TX_PULSE_FREQ_64MHZ, TX_PREAMBLE_LEN_1024};
const byte DW1000Class::MODE_SHORTDATA_SHORTRANGE_ACCURACY[] = {TRX_RATE_6800KBPS, TX_PULSE_FREQ_64MHZ, TX_PREAMBLE_LEN_128};
const byte DW1000Class::MODE_SHORTDATA_LONGRANGE_ACCURACY[] = {TRX_RATE_110KBPS, TX_PULSE_FREQ_64MHZ, TX_PREAMBLE_LEN_1024};

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

void DW1000Class::begin(int irq) {
	begin(irq, -1);
}

void DW1000Class::begin(int irq, int rst) {
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
	pinMode(_rst, OUTPUT);
	digitalWrite(_rst, HIGH);
	// reset chip (either soft or hard)
	if(_rst <= 0) {
		softReset();
	} else {
		reset();
	}
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
	// attach interrupt
	attachInterrupt(_irq, DW1000Class::handleInterrupt, RISING);
}

void DW1000Class::loadLDE() {
	// transfer any ldo tune values
	byte ldoTune[LEN_OTP_RDAT];
	readBytesOTP(0x04, ldoTune); // TODO #define
	if(ldoTune[0] != 0) {
		// TODO tuning available, copy over to RAM: use OTP_LDO bit
	}
	// tell the chip to load the LDE microcode
	byte pmscctrl0[LEN_PMSC_CTRL0];
	byte otpctrl[LEN_OTP_CTRL];
	memset(pmscctrl0, 0, LEN_PMSC_CTRL0);
	memset(otpctrl, 0, LEN_OTP_CTRL);
	readBytes(PMSC, PMSC_CTRL0_SUB, pmscctrl0, LEN_PMSC_CTRL0);
	readBytes(OTP_IF, OTP_CTRL_SUB, otpctrl, LEN_OTP_CTRL);
	pmscctrl0[0] = 0x01;
	pmscctrl0[1] = 0x03;
	otpctrl[0] = 0x00;
	otpctrl[1] = 0x80;
	writeBytes(PMSC, PMSC_CTRL0_SUB, pmscctrl0, LEN_PMSC_CTRL0);
	writeBytes(OTP_IF, OTP_CTRL_SUB, otpctrl, LEN_OTP_CTRL);
	delay(5);
	pmscctrl0[0] = 0x00;
	pmscctrl0[1] = 0x02;
	writeBytes(PMSC, PMSC_CTRL0_SUB, pmscctrl0, LEN_PMSC_CTRL0);
}

void DW1000Class::enableClock(byte clock) {
	byte pmscctrl0[LEN_PMSC_CTRL0];
	memset(pmscctrl0, 0, LEN_PMSC_CTRL0);
	readBytes(PMSC, PMSC_CTRL0_SUB, pmscctrl0, LEN_PMSC_CTRL0);
	if(clock == AUTO_CLOCK) {
		pmscctrl0[0] = AUTO_CLOCK;
		pmscctrl0[1] &= 0xFE;
	} else if(clock == XTI_CLOCK) {
		pmscctrl0[0] &= 0xFC;
		pmscctrl0[0] |= XTI_CLOCK;
	} else if(clock == PLL_CLOCK) {
		pmscctrl0[0] &= 0xFC;
		pmscctrl0[0] |= PLL_CLOCK;
	} else {
		// TODO deliver proper warning
	}
	writeBytes(PMSC, PMSC_CTRL0_SUB, pmscctrl0, 1);
    	writeBytes(PMSC, PMSC_CTRL0_SUB, pmscctrl0, LEN_PMSC_CTRL0);
}

void DW1000Class::reset() {
	digitalWrite(_rst, LOW);
	delay(10);
	digitalWrite(_rst, HIGH);
	delay(10);
	// force into idle mode (although it should be already after reset)
	idle();
}

void DW1000Class::softReset() {
	byte pmscctrl0[LEN_PMSC_CTRL0];
	readBytes(PMSC, PMSC_CTRL0_SUB, pmscctrl0, LEN_PMSC_CTRL0);
	pmscctrl0[0] = 0x01;
	writeBytes(PMSC, PMSC_CTRL0_SUB, pmscctrl0, LEN_PMSC_CTRL0);
	pmscctrl0[3] = 0x00;
	writeBytes(PMSC, PMSC_CTRL0_SUB, pmscctrl0, LEN_PMSC_CTRL0);
	delay(10);
	pmscctrl0[0] = 0x00;
	pmscctrl0[3] = 0xF0;
	writeBytes(PMSC, PMSC_CTRL0_SUB, pmscctrl0, LEN_PMSC_CTRL0);
	// force into idle mode
	idle();
}

void DW1000Class::enableMode(const byte mode[]) {
	setDataRate(mode[0]);
	setPulseFrequency(mode[1]);
	setPreambleLength(mode[2]);
	// TODO add channel and code to mode tuples
	setChannel(CHANNEL_5);
	setPreambleCode(PREAMBLE_CODE_16MHZ_4);
}

void DW1000Class::tune() {
	// these registers are going to be tuned/configured
	byte agctune1[LEN_AGC_TUNE1];
	byte agctune2[LEN_AGC_TUNE2];
	byte agctune3[LEN_AGC_TUNE3];
	byte drxtune0b[LEN_DRX_TUNE0b];
	byte drxtune1a[LEN_DRX_TUNE1a];
	byte drxtune1b[LEN_DRX_TUNE1b];
	byte drxtune2[LEN_DRX_TUNE2];
	byte drxtune4H[LEN_DRX_TUNE4H];
	byte ldecfg1[LEN_LDE_CFG1];
	byte ldecfg2[LEN_LDE_CFG2];
	byte lderepc[LEN_LDE_REPC];
	byte txpower[LEN_TX_POWER];
	byte rfrxctrlh[LEN_RF_RXCTRLH];
	byte rftxctrl[LEN_RF_TXCTRL];
	byte tcpgdelay[LEN_TC_PGDELAY];
	byte fspllcfg[LEN_FS_PLLCFG];
	byte fsplltune[LEN_FS_PLLTUNE];
	// AGC_TUNE1
	if(_pulseFrequency == TX_PULSE_FREQ_16MHZ) {
		writeValueToBytes(agctune1, 0x8870, LEN_AGC_TUNE1);
	} else if(_pulseFrequency == TX_PULSE_FREQ_64MHZ) {
		writeValueToBytes(agctune1, 0x889B, LEN_AGC_TUNE1);
	} else {
		// TODO proper error/warning handling
	}
	// AGC_TUNE2
	writeValueToBytes(agctune2, 0x2502A907L, LEN_AGC_TUNE2);
	// AGC_TUNE3
	writeValueToBytes(agctune3, 0x0035, LEN_AGC_TUNE3);
	// DRX_TUNE0b
	if(_dataRate == TRX_RATE_110KBPS) {
		writeValueToBytes(drxtune0b, 0x0016, LEN_DRX_TUNE0b);
	} else if(_dataRate == TRX_RATE_850KBPS) {
		writeValueToBytes(drxtune0b, 0x0006, LEN_DRX_TUNE0b);
	} else if(_dataRate == TRX_RATE_6800KBPS) {
		writeValueToBytes(drxtune0b, 0x0001, LEN_DRX_TUNE0b);
	} else {
		// TODO proper error/warning handling
	}
	// DRX_TUNE1a
	if(_pulseFrequency == TX_PULSE_FREQ_16MHZ) {
		writeValueToBytes(drxtune1a, 0x0087, LEN_DRX_TUNE1a);
	} else if(_pulseFrequency == TX_PULSE_FREQ_64MHZ) {
		writeValueToBytes(drxtune1a, 0x008D, LEN_DRX_TUNE1a);
	} else {
		// TODO proper error/warning handling
	}
	// DRX_TUNE1b
	if(_preambleLength ==  TX_PREAMBLE_LEN_1536 || _preambleLength ==  TX_PREAMBLE_LEN_2048 ||  
			_preambleLength ==  TX_PREAMBLE_LEN_4096) {
		if(_dataRate == TRX_RATE_110KBPS) {
			writeValueToBytes(drxtune1b, 0x0064, LEN_DRX_TUNE1b);
		} else {
			// TODO proper error/warning handling
		}
	} else if(_preambleLength != TX_PREAMBLE_LEN_64) {
		if(_dataRate == TRX_RATE_850KBPS || _dataRate == TRX_RATE_6800KBPS) {
			writeValueToBytes(drxtune1b, 0x0020, LEN_DRX_TUNE1b);
		} else {
			// TODO proper error/warning handling
		}
	} else {
		if(_dataRate == TRX_RATE_6800KBPS) {
			writeValueToBytes(drxtune1b, 0x0010, LEN_DRX_TUNE1b);
		} else {
			// TODO proper error/warning handling
		}
	}
	// DRX_TUNE2
	if(_pacSize == PAC_SIZE_8) {
		if(_pulseFrequency == TX_PULSE_FREQ_16MHZ) {
			writeValueToBytes(drxtune2, 0x311A002DL, LEN_DRX_TUNE2);
		} else if(_pulseFrequency == TX_PULSE_FREQ_64MHZ) {
			writeValueToBytes(drxtune2, 0x313B006BL, LEN_DRX_TUNE2);
		} else {
			// TODO proper error/warning handling
		}
	} else if(_pacSize == PAC_SIZE_16) {
		if(_pulseFrequency == TX_PULSE_FREQ_16MHZ) {
			writeValueToBytes(drxtune2, 0x331A0052L, LEN_DRX_TUNE2);
		} else if(_pulseFrequency == TX_PULSE_FREQ_64MHZ) {
			writeValueToBytes(drxtune2, 0x333B00BEL, LEN_DRX_TUNE2);
		} else {
			// TODO proper error/warning handling
		}
	} else if(_pacSize == PAC_SIZE_32) {
		if(_pulseFrequency == TX_PULSE_FREQ_16MHZ) {
			writeValueToBytes(drxtune2, 0x351A009AL, LEN_DRX_TUNE2);
		} else if(_pulseFrequency == TX_PULSE_FREQ_64MHZ) {
			writeValueToBytes(drxtune2, 0x353B015EL, LEN_DRX_TUNE2);
		} else {
			// TODO proper error/warning handling
		}
	} else if(_pacSize == PAC_SIZE_64) {
		if(_pulseFrequency == TX_PULSE_FREQ_16MHZ) {
			writeValueToBytes(drxtune2, 0x371A011DL, LEN_DRX_TUNE2);
		} else if(_pulseFrequency == TX_PULSE_FREQ_64MHZ) {
			writeValueToBytes(drxtune2, 0x373B0296L, LEN_DRX_TUNE2);
		} else {
			// TODO proper error/warning handling
		}
	} else {
		// TODO proper error/warning handling
	}
	// DRX_TUNE4H
	if(_preambleLength == TX_PREAMBLE_LEN_64) {
		writeValueToBytes(drxtune4H, 0x0010, LEN_DRX_TUNE4H);
	} else {
		writeValueToBytes(drxtune4H, 0x0028, LEN_DRX_TUNE4H);
	}
	// RF_RXCTRLH
	if(_channel != CHANNEL_4 && _channel != CHANNEL_7) {
		writeValueToBytes(rfrxctrlh, 0xD8, LEN_RF_RXCTRLH);
	} else {
		writeValueToBytes(rfrxctrlh, 0xBC, LEN_RF_RXCTRLH);
	}
	// RX_TXCTRL
	if(_channel == CHANNEL_1) {
		writeValueToBytes(rftxctrl, 0x00005C40L, LEN_RF_TXCTRL);
	} else if(_channel == CHANNEL_2) {
		writeValueToBytes(rftxctrl, 0x00045CA0L, LEN_RF_TXCTRL);
	} else if(_channel == CHANNEL_3) {
		writeValueToBytes(rftxctrl, 0x00086CC0L, LEN_RF_TXCTRL);
	} else if(_channel == CHANNEL_4) {
		writeValueToBytes(rftxctrl, 0x00045C80L, LEN_RF_TXCTRL);
	} else if(_channel == CHANNEL_5) {
		writeValueToBytes(rftxctrl, 0x001E3FE0L, LEN_RF_TXCTRL);
	} else if(_channel == CHANNEL_7) {
		writeValueToBytes(rftxctrl, 0x001E7DE0L, LEN_RF_TXCTRL);
	} else {
		// TODO proper error/warning handling
	}
	// TC_PGDELAY
	if(_channel == CHANNEL_1) {
		writeValueToBytes(tcpgdelay, 0xC9, LEN_TC_PGDELAY);
	} else if(_channel == CHANNEL_2) {
		writeValueToBytes(tcpgdelay, 0xC2, LEN_TC_PGDELAY);
	} else if(_channel == CHANNEL_3) {
		writeValueToBytes(tcpgdelay, 0xC5, LEN_TC_PGDELAY);
	} else if(_channel == CHANNEL_4) {
		writeValueToBytes(tcpgdelay, 0x95, LEN_TC_PGDELAY);
	} else if(_channel == CHANNEL_5) {
		writeValueToBytes(tcpgdelay, 0xC0, LEN_TC_PGDELAY);
	} else if(_channel == CHANNEL_7) {
		writeValueToBytes(tcpgdelay, 0x93, LEN_TC_PGDELAY);
	} else {
		// TODO proper error/warning handling
	}
	// FS_PLLCFG and FS_PLLTUNE
	if(_channel == CHANNEL_1) {
		writeValueToBytes(fspllcfg, 0x09000407L, LEN_FS_PLLCFG);
		writeValueToBytes(fsplltune, 0x1E, LEN_FS_PLLTUNE);
	} else if(_channel == CHANNEL_2 || _channel == CHANNEL_4) {
		writeValueToBytes(fspllcfg, 0x08400508L, LEN_FS_PLLCFG);
		writeValueToBytes(fsplltune, 0x26, LEN_FS_PLLTUNE);
	} else if(_channel == CHANNEL_3) {
		writeValueToBytes(fspllcfg, 0x08401009L, LEN_FS_PLLCFG);
		writeValueToBytes(fsplltune, 0x5E, LEN_FS_PLLTUNE);
	} else if(_channel == CHANNEL_5 || _channel == CHANNEL_7) {
		writeValueToBytes(fspllcfg, 0x0800041DL, LEN_FS_PLLCFG);
		writeValueToBytes(fsplltune, 0xA6, LEN_FS_PLLTUNE);
	} else {
		// TODO proper error/warning handling
	}
	// LDE_CFG1
	writeValueToBytes(ldecfg1, 0xD, LEN_LDE_CFG1);
	// LDE_CFG2
	if(_pulseFrequency == TX_PULSE_FREQ_16MHZ) {
		writeValueToBytes(ldecfg2, 0x1607, LEN_LDE_CFG2);
	} else if(_pulseFrequency == TX_PULSE_FREQ_64MHZ) {
		writeValueToBytes(ldecfg2, 0x0607, LEN_LDE_CFG2);
	} else {
		// TODO proper error/warning handling
	}
	// LDE_REPC
	if(_preambleCode == PREAMBLE_CODE_16MHZ_1 || _preambleCode == PREAMBLE_CODE_16MHZ_2) {
		if(_dataRate == TRX_RATE_110KBPS) {
			writeValueToBytes(lderepc, ((0x5998 >> 3) & 0xFFFF), LEN_LDE_REPC);
		} else {
			writeValueToBytes(lderepc, 0x5998, LEN_LDE_REPC);
		}
	} else if(_preambleCode == PREAMBLE_CODE_16MHZ_3 || _preambleCode == PREAMBLE_CODE_16MHZ_8) {
		if(_dataRate == TRX_RATE_110KBPS) {
			writeValueToBytes(lderepc, ((0x51EA >> 3) & 0xFFFF), LEN_LDE_REPC);
		} else {
			writeValueToBytes(lderepc, 0x51EA, LEN_LDE_REPC);
		}
	} else if(_preambleCode == PREAMBLE_CODE_16MHZ_4) {
		if(_dataRate == TRX_RATE_110KBPS) {
			writeValueToBytes(lderepc, ((0x428E >> 3) & 0xFFFF), LEN_LDE_REPC);
		} else {
			writeValueToBytes(lderepc, 0x428E, LEN_LDE_REPC);
		}
	} else if(_preambleCode == PREAMBLE_CODE_16MHZ_5) {
		if(_dataRate == TRX_RATE_110KBPS) {
			writeValueToBytes(lderepc, ((0x451E >> 3) & 0xFFFF), LEN_LDE_REPC);
		} else {
			writeValueToBytes(lderepc, 0x451E, LEN_LDE_REPC);
		}
	} else if(_preambleCode == PREAMBLE_CODE_16MHZ_6) {
		if(_dataRate == TRX_RATE_110KBPS) {
			writeValueToBytes(lderepc, ((0x2E14 >> 3) & 0xFFFF), LEN_LDE_REPC);
		} else {
			writeValueToBytes(lderepc, 0x2E14, LEN_LDE_REPC);
		}
	} else if(_preambleCode == PREAMBLE_CODE_16MHZ_7) {
		if(_dataRate == TRX_RATE_110KBPS) {
			writeValueToBytes(lderepc, ((0x8000 >> 3) & 0xFFFF), LEN_LDE_REPC);
		} else {
			writeValueToBytes(lderepc, 0x8000, LEN_LDE_REPC);
		}
	} else if(_preambleCode == PREAMBLE_CODE_64MHZ_9) {
		if(_dataRate == TRX_RATE_110KBPS) {
			writeValueToBytes(lderepc, ((0x28F4 >> 3) & 0xFFFF), LEN_LDE_REPC);
		} else {
			writeValueToBytes(lderepc, 0x28F4, LEN_LDE_REPC);
		}
	} else if(_preambleCode == PREAMBLE_CODE_64MHZ_10 || _preambleCode == PREAMBLE_CODE_64MHZ_17) {
		if(_dataRate == TRX_RATE_110KBPS) {
			writeValueToBytes(lderepc, ((0x3332 >> 3) & 0xFFFF), LEN_LDE_REPC);
		} else {
			writeValueToBytes(lderepc, 0x3332, LEN_LDE_REPC);
		}
	} else if(_preambleCode == PREAMBLE_CODE_64MHZ_11) {
		if(_dataRate == TRX_RATE_110KBPS) {
			writeValueToBytes(lderepc, ((0x3AE0 >> 3) & 0xFFFF), LEN_LDE_REPC);
		} else {
			writeValueToBytes(lderepc, 0x3AE0, LEN_LDE_REPC);
		}
	} else if(_preambleCode == PREAMBLE_CODE_64MHZ_12) {
		if(_dataRate == TRX_RATE_110KBPS) {
			writeValueToBytes(lderepc, ((0x3D70 >> 3) & 0xFFFF), LEN_LDE_REPC);
		} else {
			writeValueToBytes(lderepc, 0x3D70, LEN_LDE_REPC);
		}
	} else if(_preambleCode == PREAMBLE_CODE_64MHZ_18 || _preambleCode == PREAMBLE_CODE_64MHZ_19) {
		if(_dataRate == TRX_RATE_110KBPS) {
			writeValueToBytes(lderepc, ((0x35C2 >> 3) & 0xFFFF), LEN_LDE_REPC);
		} else {
			writeValueToBytes(lderepc, 0x35C2, LEN_LDE_REPC);
		}
	} else if(_preambleCode == PREAMBLE_CODE_64MHZ_20) {
		if(_dataRate == TRX_RATE_110KBPS) {
			writeValueToBytes(lderepc, ((0x47AE >> 3) & 0xFFFF), LEN_LDE_REPC);
		} else {
			writeValueToBytes(lderepc, 0x47AE, LEN_LDE_REPC);
		}
	} else {
		// TODO proper error/warning handling
	}
	// TX_POWER (enabled smart transmit power control)
	if(_channel == CHANNEL_1 || _channel == CHANNEL_2) {
		if(_pulseFrequency == TX_PULSE_FREQ_16MHZ) {
			writeValueToBytes(txpower, 0x15355575L, LEN_TX_POWER);
		} else if(_pulseFrequency == TX_PULSE_FREQ_64MHZ) {
			writeValueToBytes(txpower, 0x07274767L, LEN_TX_POWER);
		} else {
			// TODO proper error/warning handling
		}
	} else if(_channel == CHANNEL_3) {
		if(_pulseFrequency == TX_PULSE_FREQ_16MHZ) {
			writeValueToBytes(txpower, 0x0F2F4F6FL, LEN_TX_POWER);
		} else if(_pulseFrequency == TX_PULSE_FREQ_64MHZ) {
			writeValueToBytes(txpower, 0x2B4B6B8BL, LEN_TX_POWER);
		} else {
			// TODO proper error/warning handling
		}
	} else if(_channel == CHANNEL_4) {
		if(_pulseFrequency == TX_PULSE_FREQ_16MHZ) {
			writeValueToBytes(txpower, 0x1F1F3F5FL, LEN_TX_POWER);
		} else if(_pulseFrequency == TX_PULSE_FREQ_64MHZ) {
			writeValueToBytes(txpower, 0x3A5A7A9AL, LEN_TX_POWER);
		} else {
			// TODO proper error/warning handling
		}
	} else if(_channel == CHANNEL_5) {
		if(_pulseFrequency == TX_PULSE_FREQ_16MHZ) {
			writeValueToBytes(txpower, 0x0E082848L, LEN_TX_POWER);
		} else if(_pulseFrequency == TX_PULSE_FREQ_64MHZ) {
			writeValueToBytes(txpower, 0x25456585L, LEN_TX_POWER);
		} else {
			// TODO proper error/warning handling
		}
	} else if(_channel == CHANNEL_7) {
		if(_pulseFrequency == TX_PULSE_FREQ_16MHZ) {
			writeValueToBytes(txpower, 0x32527292L, LEN_TX_POWER);
		} else if(_pulseFrequency == TX_PULSE_FREQ_64MHZ) {
			writeValueToBytes(txpower, 0x5171B1D1L, LEN_TX_POWER);
		} else {
			// TODO proper error/warning handling
		}
	} else {
		// TODO proper error/warning handling
	}
	// write configuration back to chip
	writeBytes(AGC_TUNE, AGC_TUNE1_SUB, agctune1, LEN_AGC_TUNE1);
	writeBytes(AGC_TUNE, AGC_TUNE2_SUB, agctune2, LEN_AGC_TUNE2);
	writeBytes(AGC_TUNE, AGC_TUNE3_SUB, agctune3, LEN_AGC_TUNE3);
	writeBytes(DRX_TUNE, DRX_TUNE0b_SUB, drxtune0b, LEN_DRX_TUNE0b);
	writeBytes(DRX_TUNE, DRX_TUNE1a_SUB, drxtune1a, LEN_DRX_TUNE1a);
	writeBytes(DRX_TUNE, DRX_TUNE1b_SUB, drxtune1b, LEN_DRX_TUNE1b);
	writeBytes(DRX_TUNE, DRX_TUNE2_SUB, drxtune2, LEN_DRX_TUNE2);
	writeBytes(DRX_TUNE, DRX_TUNE4H_SUB, drxtune4H, LEN_DRX_TUNE4H);
	writeBytes(LDE_CFG, LDE_CFG1_SUB, ldecfg1, LEN_LDE_CFG1);
	writeBytes(LDE_CFG, LDE_CFG2_SUB, ldecfg2, LEN_LDE_CFG2);
	writeBytes(LDE_CFG, LDE_REPC_SUB, lderepc, LEN_LDE_REPC);
	writeBytes(TX_POWER, NO_SUB, txpower, LEN_TX_POWER);
	writeBytes(RF_CONF, RF_RXCTRLH_SUB, rfrxctrlh, LEN_RF_RXCTRLH);
	writeBytes(RF_CONF, RF_TXCTRL_SUB, rftxctrl, LEN_RF_TXCTRL);
	writeBytes(TX_CAL, TC_PGDELAY_SUB, tcpgdelay, LEN_TC_PGDELAY);
	writeBytes(FS_CTRL, FS_PLLTUNE_SUB, fsplltune, LEN_FS_PLLTUNE);
	writeBytes(FS_CTRL, FS_PLLCFG_SUB, fspllcfg, LEN_FS_PLLCFG);
	// TODO LDOTUNE, see 2.5.5, p. 21
}

/* ###########################################################################
 * #### Interrupt handling ###################################################
 * ######################################################################### */

void DW1000Class::handleInterrupt() {
	// read current status and handle via callbacks
	readSystemEventStatusRegister();
	if(isTransmitDone() &&_handleSent != 0) {
		(*_handleSent)();
		clearTransmitStatus();
	}
	if(isReceiveTimestampAvailable() && _handleReceiveTimestampAvailable != 0) {
		(*_handleReceiveTimestampAvailable)();
		clearReceiveTimestampAvailableStatus();
	}
	if(isReceiveError() && _handleReceiveError != 0) {
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
	} else if(isReceiveDone() && _handleReceived != 0) {
		(*_handleReceived)();
		clearReceiveStatus();
		if(_permanentReceive) {
			startReceive();
		}
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
	setBit(_sysmask, LEN_SYS_MASK, RXFCG_BIT, val);
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
	setBit(_sysctrl, LEN_SYS_CTRL, SFCST_BIT, !_frameCheck);
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
	setBit(_sysctrl, LEN_SYS_CTRL, SFCST_BIT, !_frameCheck);
	setBit(_sysctrl, LEN_SYS_CTRL, TXSTRT_BIT, true);
	writeBytes(SYS_CTRL, NO_SUB, _sysctrl, LEN_SYS_CTRL);
	if(_permanentReceive) {
		memset(_sysctrl, 0, LEN_SYS_CTRL);
		_deviceMode = RX_MODE;
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
	enableClock(XTI_CLOCK);
	// TODO make configurable (if false disable LDE use)
	// load LDE micro-code
	loadLDE();
	delay(10);
	enableClock(AUTO_CLOCK);
	// write all configurations back to device
	writeNetworkIdAndDeviceAddress();
	writeSystemConfigurationRegister();
	writeChannelControlRegister();
	writeTransmitFrameControlRegister();
	writeSystemEventMaskRegister();
	// tune according to configuration
	tune();
	delay(10);
}

void DW1000Class::waitForResponse(boolean val) {
	setBit(_sysctrl, LEN_SYS_CTRL, WAIT4RESP_BIT, val);
}

void DW1000Class::suppressFrameCheck(boolean val) {
	_frameCheck = !val;
}

DW1000Time DW1000Class::setDelay(const DW1000Time& delay) {
	if(_deviceMode == TX_MODE) {
		setBit(_sysctrl, LEN_SYS_CTRL, TXDLYS_BIT, true);
	} else if(_deviceMode == RX_MODE) {
		setBit(_sysctrl, LEN_SYS_CTRL, RXDLYS_BIT, true);
	} else {
		// in idle, ignore
		return DW1000Time();
	}
	byte delayBytes[5];
	DW1000Time futureTime;
	getSystemTimestamp(futureTime);
	futureTime += delay;
	futureTime.getAsBytes(delayBytes);
	delayBytes[0] = 0;
	delayBytes[1] &= 0xFE;
	writeBytes(DX_TIME, NO_SUB, delayBytes, LEN_DX_TIME);
	return futureTime;
}

void DW1000Class::setDataRate(byte rate) {
	rate &= 0x03;
	_txfctrl[1] &= 0x9F;
	_txfctrl[1] |= (byte)((rate << 5) & 0xFF);
	if(rate == TRX_RATE_110KBPS) {
		setBit(_syscfg, LEN_SYS_CFG, RXM110K_BIT, true);
	} else {
		setBit(_syscfg, LEN_SYS_CFG, RXM110K_BIT, false);
	}
	_dataRate = rate;
}

void DW1000Class::setPulseFrequency(byte freq) {
	freq &= 0x03;
	_txfctrl[2] &= 0xFC;
	_txfctrl[2] |= (byte)(freq & 0xFF);
	_chanctrl[2] &= 0xF3;
	_chanctrl[2] |= (byte)((freq << 2) & 0xFF);
	_pulseFrequency = freq;
}

void DW1000Class::setPreambleLength(byte prealen) {
	prealen &= 0x0F;
	_txfctrl[2] &= 0xC3;
	_txfctrl[2] |= (byte)((prealen << 2) & 0xFF);
	if(prealen == TX_PREAMBLE_LEN_64 || prealen == TX_PREAMBLE_LEN_128) {
		_pacSize = PAC_SIZE_8;
	} else if(prealen == TX_PREAMBLE_LEN_256 || prealen == TX_PREAMBLE_LEN_512) {
		_pacSize = PAC_SIZE_16;
	} else if(prealen == TX_PREAMBLE_LEN_1024) {
		_pacSize = PAC_SIZE_32;
	} else {
		_pacSize = PAC_SIZE_64;
	}
	_preambleLength = prealen;
}

void DW1000Class::useExtendedFrameLength(boolean val) {
	_extendedFrameLength = (val ? FRAME_LENGTH_EXTENDED : FRAME_LENGTH_NORMAL);
	_syscfg[3] &= 0xFC;
	_syscfg[3] |= _extendedFrameLength;
}

void DW1000Class::receivePermanently(boolean val) {
	_permanentReceive = val;
	if(val) {
		// in case permanent, also reenable receiver once failed
		setReceiverAutoReenable(true);
		writeSystemConfigurationRegister();
	}
}

void DW1000Class::setChannel(byte channel) {
	channel &= 0xF;
	_chanctrl[0] = ((channel | (channel << 4)) & 0xFF);
	_channel = channel;
}

void DW1000Class::setPreambleCode(byte preacode) {
	preacode &= 0x1F;
	byte rxtxpcode2 = (((preacode << 3) | (preacode >> 2)) & 0xFF);
	byte rxtxpcode1 = _chanctrl[2] | ((preacode << 6) & 0xFF);
	_preambleCode = preacode;
}

void DW1000Class::setDefaults() {
	if(_deviceMode == TX_MODE) {

	} else if(_deviceMode == RX_MODE) {

	} else if(_deviceMode == IDLE_MODE) {
		useExtendedFrameLength(false);
		suppressFrameCheck(false);
		interruptOnSent(true);
		interruptOnReceived(true);
		interruptOnReceiveError(true);
		interruptOnAutomaticAcknowledgeTrigger(true);
		setReceiverAutoReenable(true);
		// default mode when powering up the chip
		// still explicitly selected for later tuning
		//enableMode(MODE_LOCATION_LONGRANGE_LOWPOWER);
		enableMode(MODE_LOCATION_SHORTRANGE_LOWPOWER);
	}
}

void DW1000Class::setData(byte data[], unsigned int n) {
	if(_frameCheck) {
		n+=2; // two bytes CRC-16
	}
	if(n > LEN_EXT_UWB_FRAMES) {
		return; // TODO proper error handling: frame/buffer size
	}
	if(n > LEN_UWB_FRAMES && !_extendedFrameLength) {
		return; // TODO proper error handling: frame/buffer size
	}
	// transmit data and length
	writeBytes(TX_BUFFER, NO_SUB, data, n);
	_txfctrl[0] = (byte)(n & 0xFF); // 1 byte (regular length + 1 bit)
	_txfctrl[1] &= 0xE0;
	_txfctrl[1] |= (byte)((n >> 8) & 0x03);	// 2 added bits if extended length
	writeTransmitFrameControlRegister();
}

void DW1000Class::setData(const String& data) {
	unsigned int n = data.length()+1;
	byte* dataBytes = (byte*)malloc(n);
	data.getBytes(dataBytes, n);
	setData(dataBytes, n);
	free(dataBytes);
	
}

unsigned int DW1000Class::getDataLength() {
	unsigned int len = 0;
	if(_deviceMode == TX_MODE) {
		// 10 bits of TX frame control register
		len = ((((unsigned int)_txfctrl[1] << 8) | (unsigned int)_txfctrl[0]) & 0x03FF);
	} else if(_deviceMode == RX_MODE) {
		// 10 bits of RX frame control register
		byte rxFrameInfo[LEN_RX_FINFO];
		readBytes(RX_FINFO, NO_SUB, rxFrameInfo, LEN_RX_FINFO);
		len = ((((unsigned int)rxFrameInfo[1] << 8) | (unsigned int)rxFrameInfo[0]) & 0x03FF);
	}
	if(_frameCheck && len > 2) {
		return len-2;
	}
	return len;
}

void DW1000Class::getData(byte data[], unsigned int n) {
	if(n <= 0) {
		return;
	}
	readBytes(RX_BUFFER, NO_SUB, data, n);
}

void DW1000Class::getData(String& data) {
	unsigned int i;
	unsigned int n = getDataLength(); // number of bytes w/o the two FCS ones
	if(n <= 0) {
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

void DW1000Class::getTransmitTimestamp(DW1000Time& time) {
	byte txTimeBytes[LEN_TX_STAMP];
	readBytes(TX_TIME, TX_STAMP_SUB, txTimeBytes, LEN_TX_STAMP);
	time.setFromBytes(txTimeBytes);
}

void DW1000Class::getReceiveTimestamp(DW1000Time& time) {
	byte rxTimeBytes[LEN_RX_STAMP];
	readBytes(RX_TIME, RX_STAMP_SUB, rxTimeBytes, LEN_RX_STAMP);
	time.setFromBytes(rxTimeBytes);
}

void DW1000Class::getSystemTimestamp(DW1000Time& time) {
	byte sysTimeBytes[LEN_SYS_TIME];
	readBytes(SYS_TIME, NO_SUB, sysTimeBytes, LEN_SYS_TIME);
	time.setFromBytes(sysTimeBytes);
}

void DW1000Class::getTransmitTimestamp(byte data[]) {
	readBytes(TX_TIME, TX_STAMP_SUB, data, LEN_TX_STAMP);
}

void DW1000Class::getReceiveTimestamp(byte data[]) {
	readBytes(RX_TIME, RX_STAMP_SUB, data, LEN_RX_STAMP);
}

void DW1000Class::getSystemTimestamp(byte data[]) {
	readBytes(SYS_TIME, NO_SUB, data, LEN_SYS_TIME);
}

boolean DW1000Class::isTransmitDone() {
	return getBit(_sysstatus, LEN_SYS_STATUS, TXFRS_BIT);
}

boolean DW1000Class::isReceiveTimestampAvailable() {
	return getBit(_sysstatus, LEN_SYS_STATUS, LDEDONE_BIT);
}

boolean DW1000Class::isLDEDone() {
	return getBit(_sysstatus, LEN_SYS_STATUS, LDEDONE_BIT);
}

boolean DW1000Class::isReceiveDone() {
	if(_frameCheck) {
		return getBit(_sysstatus, LEN_SYS_STATUS, RXFCG_BIT);
	}
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
void DW1000Class::setBit(byte data[], unsigned int n, unsigned int bit, boolean val) {
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
boolean DW1000Class::getBit(byte data[], unsigned int n, unsigned int bit) {
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

void DW1000Class::writeValueToBytes(byte data[], long val, unsigned int n) {
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
void DW1000Class::readBytes(byte cmd, word offset, byte data[], unsigned int n) {
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

// always 4 bytes
void DW1000Class::readBytesOTP(word address, byte data[]) {
	byte addressBytes[LEN_OTP_ADDR];
	byte otpctrl[LEN_OTP_CTRL];
	readBytes(OTP_IF, OTP_CTRL_SUB, otpctrl, LEN_OTP_CTRL);
	// bytes of address
	addressBytes[0] = (address & 0xFF);
	addressBytes[1] = ((address >> 8) & 0xFF);
    	// set address
    	writeBytes(OTP_IF, OTP_ADDR_SUB, addressBytes, LEN_OTP_ADDR);
	otpctrl[0] = 0x03;
	writeBytes(OTP_IF, OTP_CTRL_SUB, otpctrl, LEN_OTP_CTRL);
	otpctrl[0] = 0x01;
	writeBytes(OTP_IF, OTP_CTRL_SUB, otpctrl, LEN_OTP_CTRL);
	// read value
	readBytes(OTP_IF, OTP_RDAT_SUB, data, LEN_OTP_RDAT);
	otpctrl[0] = 0x00;
	writeBytes(OTP_IF, OTP_CTRL_SUB, otpctrl, LEN_OTP_CTRL);
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
void DW1000Class::writeBytes(byte cmd, word offset, byte data[], unsigned int n) {
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

char* DW1000Class::getPrettyBytes(byte data[], unsigned int n) {
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

char* DW1000Class::getPrettyBytes(byte cmd, word offset, unsigned int n) {
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
