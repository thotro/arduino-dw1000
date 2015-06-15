/*
 * Copyright (c) 2015 by Thomas Trojer <thomas@trojer.net>
 * Decawave DW1000 library for arduino.
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef _DW1000_H_INCLUDED
#define _DW1000_H_INCLUDED

// Time resolution in micro-seconds of time based registers/values.
// Each bit in a timestamp counts for a period of approx. 15.65ps
#define TIME_RES 0.000015650040064103
#define TIME_RES_INV 63897.6

// time stamp byte length
#define LEN_STAMP 5

// enum to determine RX or TX mode of device
#define IDLE_MODE 0x00
#define RX_MODE 0x01
#define TX_MODE 0x02

// used for SPI ready w/o actual writes
#define JUNK 0x00

// no sub-address for register write
#define NO_SUB 0x00

// device id register
#define DEV_ID 0x00
#define LEN_DEV_ID 4

// extended unique identifier register
#define EUI 0x01
#define LEN_EUI 8

// PAN identifier, short address register
#define PANADR 0x03
#define LEN_PANADR 4

// device configuration register
#define SYS_CFG 0x04
#define LEN_SYS_CFG 4
#define FFEN_BIT 0
#define DIS_DRXB_BIT 12
#define HIRQ_POL_BIT 9
#define RXAUTR_BIT 29
#define PHR_MODE_SUB 16
#define LEN_PHR_MODE_SUB 2
#define RXM110K_BIT 22

// device control register
#define SYS_CTRL 0x0D
#define LEN_SYS_CTRL 4
#define SFCST_BIT 0
#define TXSTRT_BIT 1
#define TXDLYS_BIT 2
#define TRXOFF_BIT 6
#define WAIT4RESP_BIT 7
#define RXENAB_BIT 8
#define RXDLYS_BIT 9

// system event status register
#define SYS_STATUS 0x0F
#define LEN_SYS_STATUS 5
#define AAT_BIT 3
#define TXFRB_BIT 4
#define TXPRS_BIT 5
#define TXPHS_BIT 6
#define TXFRS_BIT 7
#define LDEDONE_BIT 10
#define RXPHE_BIT 12
#define RXDFR_BIT 13
#define RXFCG_BIT 14
#define RXFCE_BIT 15
#define RXRFSL_BIT 16
#define RXRFTO_BIT 17
#define LDEERR_BIT 18

// system event mask register
// NOTE: uses the bit definitions of SYS_STATUS (below 32)
#define SYS_MASK 0x0E
#define LEN_SYS_MASK 4

// system time counter
#define SYS_TIME 0x06
#define LEN_SYS_TIME LEN_STAMP

// RX timestamp register
#define RX_TIME 0x15
#define LEN_RX_TIME 14
#define RX_STAMP_SUB 0
#define LEN_RX_STAMP LEN_STAMP

// TX timestamp register
#define TX_TIME 0x17
#define LEN_TX_TIME 10
#define TX_STAMP_SUB 0
#define LEN_TX_STAMP LEN_STAMP

// timing register (for delayed RX/TX)
#define DX_TIME 0x0A
#define LEN_DX_TIME LEN_STAMP

// transmit data buffer
#define TX_BUFFER 0x09
#define LEN_TX_BUFFER 1024
#define LEN_UWB_FRAMES 127
#define LEN_EXT_UWB_FRAMES 1023

// RX frame info
#define RX_FINFO 0x10
#define LEN_RX_FINFO 4

// receive data buffer
#define RX_BUFFER 0x11
#define LEN_RX_BUFFER 1024

// transmit control
#define TX_FCTRL 0x08
#define LEN_TX_FCTRL 5

// channel control
#define CHAN_CTRL 0x1F
#define LEN_CHAN_CTRL 4

// OTP control (for LDE micro code loading only)
#define OTP_CTRL 0x2D
#define OTP_CTRL_SUB 0x06
#define LEN_OTP_CTRL 2

// PMSC_CTRL0 (for LDE micro code loading only)
#define PMSC_CTRL0 0x36
#define LEN_PMSC_CTRL0 2

// AGC_TUNE1/2 (for re-tuning only)
#define AGC_TUNE 0x23
#define AGC_TUNE1_SUB 0x04
#define AGC_TUNE2_SUB 0x0C
#define AGC_TUNE3_SUB 0x12
#define LEN_AGC_TUNE1 2
#define LEN_AGC_TUNE2 4
#define LEN_AGC_TUNE3 2

// DRX_TUNE2 (for re-tuning only)
#define DRX_TUNE 0x27
#define DRX_TUNE0b_SUB 0x02
#define DRX_TUNE1a_SUB 0x04
#define DRX_TUNE1b_SUB 0x06
#define DRX_TUNE2_SUB 0x08
#define DRX_TUNE4H_SUB 0x26
#define LEN_DRX_TUNE0b 2
#define LEN_DRX_TUNE1a 2
#define LEN_DRX_TUNE1b 2
#define LEN_DRX_TUNE2 4
#define LEN_DRX_TUNE4H 2

// LDE_CFG1 (for re-tuning only)
#define LDE_CFG 0x2E
#define LDE_CFG1_SUB 0x0806
#define LDE_CFG2_SUB 0x1806
#define LDE_REPC_SUB 0x2804
#define LEN_LDE_CFG1 1
#define LEN_LDE_CFG2 2
#define LEN_LDE_REPC 2

// TX_POWER (for re-tuning only)
#define TX_POWER 0x1E
#define LEN_TX_POWER 4

// RF_CONF (for re-tuning only)
#define RF_CONF 0x28
#define RF_RXCTRLH_SUB 0x0B
#define RF_TXCTRL_SUB 0x0C
#define LEN_RF_RXCTRLH 1
#define LEN_RF_TXCTRL 4

// TX_CAL (for re-tuning only)
#define TX_CAL 0x2A
#define TC_PGDELAY_SUB 0x0B
#define LEN_TC_PGDELAY 1

// FS_CTRL (for re-tuning only)
#define FS_CTRL 0x2B
#define FS_PLLCFG_SUB 0x07
#define FS_PLLTUNE_SUB 0x0B
#define LEN_FS_PLLCFG 4
#define LEN_FS_PLLTUNE 1

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <Arduino.h>
#include "../SPI/SPI.h"

class DW1000Class {
public:
	/* TODO impl: later
	 * - TXBOFFS in TX_FCTRL for offset buffer transmit
 	 * - TR in TX_FCTRL for flagging for ranging messages
	 * - CANSFCS in SYS_CTRL to cancel frame check suppression
	 * - HSRBP in SYS_CTRL to determine in double buffered mode from which buffer to read
	 *
	 * - replace all |= with bitChange (bitClear + bitSet)
	 */

	// construction with chip select, reset and irq pin number
	static void begin(int ss, int rst, int irq);
	static void begin(int rst, int irq);
	static void select(int ss);
	static void end();
	static void reset();

	// print device id, address, etc.
	static char* getPrintableDeviceIdentifier();
	static char* getPrintableExtendedUniqueIdentifier();
	static char* getPrintableNetworkIdAndShortAddress();

	// device address management
	static void setNetworkId(unsigned int val);
	static void setDeviceAddress(unsigned int val);
	
	// general device configuration
	static void setFrameFilter(boolean val);
	static void setDoubleBuffering(boolean val); // NOTE should be set to false
	static void setReceiverAutoReenable(boolean val);
	static void setInterruptPolarity(boolean val);
	static void suppressFrameCheck(boolean val);
	static void setDataRate(byte rate);
	static void setPulseFrequency(byte freq);
	static void setPreambleLength(byte prealen);
	static void setChannel(byte channel);
	static void setPreambleCode(byte preacode);
	static void useExtendedFrameLength(boolean val);

	// transmit and receive configuration
	static float setDelay(unsigned int value, unsigned long factorUs);
	static void receivePermanently(boolean val);
	static void waitForResponse(boolean val);
	static void setData(byte data[], int n);
	static void setData(const String& data);
	static void getData(byte data[], int n);
	static void getData(String& data);
	static int getDataLength();
	static float getTransmitTimestamp();
	static float getReceiveTimestamp();
	static float getSystemTimestamp();

	// device status flags
	static boolean isReceiveTimestampAvailable();
	static boolean isTransmitDone();
	static boolean isReceiveDone();
	static boolean isReceiveError();
	static boolean isReceiveTimeout();

	// interrupt handling
	static void interruptOnSent(boolean val);
	static void interruptOnReceived(boolean val);
	static void interruptOnReceiveError(boolean val);
	static void interruptOnReceiveTimeout(boolean val);
	static void interruptOnReceiveTimestampAvailable(boolean val);
	static void interruptOnAutomaticAcknowledgeTrigger(boolean val);
	static void clearInterrupts();
	static void clearAllStatus();
	static void clearReceiveStatus();
	static void clearReceiveTimestampAvailableStatus();
	static void clearTransmitStatus();

	// callback handler management
	static void attachSentHandler(void (*handleSent)(void)) {
		_handleSent = handleSent;
	}
	static void attachReceivedHandler(void (*handleReceived)(void)) {
		_handleReceived = handleReceived;
	}
	static void attachReceiveErrorHandler(void (*handleReceiveError)(void)) {
		_handleReceiveError = handleReceiveError;
	}
	static void attachReceiveTimeoutHandler(void (*handleReceiveTimeout)(void)) {
		_handleReceiveTimeout = handleReceiveTimeout;
	}
	static void attachReceiveTimestampAvailableHandler(void (*handleReceiveTimestampAvailable)(void)) {
		_handleReceiveTimestampAvailable = handleReceiveTimestampAvailable;
	}

	// idle state
	static void idle();

	// general configuration state
	static void newConfiguration();
	static void commitConfiguration();

	// reception state
	static void newReceive();
	static void startReceive();

	// transmission state
	static void newTransmit();
	static void startTransmit();

	// chip tuning
	static void tune();
	// TODO impl modes

	// use RX/TX specific and general default settings
	void setDefaults();

	// helpers, converting DW1000 timestamp values to and from float
	static float readTimestampAsFloatUs(byte ts[]);
	static void writeFloatUsToTimestamp(float tsValue, byte ts[]);

	// debug pretty print registers
	static char* getPrettyBytes(byte cmd, word offset, int n);
	static char* getPrettyBytes(byte data[], int n);

	// time factors (relative to [us]) for setting delayed transceive
	static const unsigned long SECONDS = 1e6;
	static const unsigned long MILLISECONDS = 1e3;
	static const unsigned long MICROSECONDS = 1;
	static const unsigned long NANOSECONDS = 1e-3;

	// timer/counter overflow (40 bits)
	static const float TIME_OVERFLOW = 1099511627776.0f;

	// transmission/reception bit rate
	static const byte TRX_RATE_110KBPS = 0x00;
	static const byte TRX_RATE_850KBPS = 0x01;
	static const byte TRX_RATE_6800KBPS = 0x02;

	// transmission pulse frequency
	// 0x00 is 4MHZ, but receiver in DW1000 does not support it (!??)
	static const byte TX_PULSE_FREQ_16MHZ = 0x01; 
	static const byte TX_PULSE_FREQ_64MHZ = 0x02;

	// preamble length (PE + TXPSR bits)
	static const byte TX_PREAMBLE_LEN_64 = 0x01;
	static const byte TX_PREAMBLE_LEN_128 = 0x05;
	static const byte TX_PREAMBLE_LEN_256 = 0x09;
	static const byte TX_PREAMBLE_LEN_512 = 0x0D;
	static const byte TX_PREAMBLE_LEN_1024 = 0x02;
	static const byte TX_PREAMBLE_LEN_1536 = 0x06;
	static const byte TX_PREAMBLE_LEN_2048 = 0x0A;
	static const byte TX_PREAMBLE_LEN_4096 = 0x03;

	// PAC size. */
	static const byte PAC_SIZE_8 = 8;
	static const byte PAC_SIZE_16 = 16;
	static const byte PAC_SIZE_32 = 32;
	static const byte PAC_SIZE_64 = 64;

	/* channel of operation. */
	static const byte CHANNEL_1 = 1;
	static const byte CHANNEL_2 = 2;
	static const byte CHANNEL_3 = 3;
	static const byte CHANNEL_4 = 4;
	static const byte CHANNEL_5 = 5;
	static const byte CHANNEL_7 = 7;

	/* preamble codes. */
	static const byte PREAMBLE_CODE_16MHZ_1 = 1;
	static const byte PREAMBLE_CODE_16MHZ_2 = 2;
	static const byte PREAMBLE_CODE_16MHZ_3 = 3;
	static const byte PREAMBLE_CODE_16MHZ_4 = 4;
	static const byte PREAMBLE_CODE_16MHZ_5 = 5;
	static const byte PREAMBLE_CODE_16MHZ_6 = 6;
	static const byte PREAMBLE_CODE_16MHZ_7 = 7;
	static const byte PREAMBLE_CODE_16MHZ_8 = 8;
	static const byte PREAMBLE_CODE_64MHZ_9 = 9;
	static const byte PREAMBLE_CODE_64MHZ_10 = 10;
	static const byte PREAMBLE_CODE_64MHZ_11 = 11;
	static const byte PREAMBLE_CODE_64MHZ_12 = 12;
	static const byte PREAMBLE_CODE_64MHZ_17 = 17;
	static const byte PREAMBLE_CODE_64MHZ_18 = 18;
	static const byte PREAMBLE_CODE_64MHZ_19 = 19;
	static const byte PREAMBLE_CODE_64MHZ_20 = 20;

	/* frame length settings. */
	static const byte FRAME_LENGTH_NORMAL = 0x00;
	static const byte FRAME_LENGTH_EXTENDED = 0x03;

private:
	/* chip select, reset and interrupt pins. */
	static unsigned int _ss;
	static unsigned int _rst;
	static unsigned int _irq;

	/* callbacks. */
	static void (*_handleSent)(void);
	static void (*_handleReceived)(void);
	static void (*_handleReceiveError)(void);
	static void (*_handleReceiveTimeout)(void);
	static void (*_handleReceiveTimestampAvailable)(void);

	/* fixed buffer for printed messages. */
	static char _msgBuf[1024];

	/* register caches. */
	static byte _syscfg[LEN_SYS_CFG];
	static byte _sysctrl[LEN_SYS_CTRL];
	static byte _sysstatus[LEN_SYS_STATUS];
	static byte _txfctrl[LEN_TX_FCTRL];
	static byte _sysmask[LEN_SYS_MASK];
	static byte _chanctrl[LEN_CHAN_CTRL];

	/* PAN and short address. */
	static byte _networkAndAddress[LEN_PANADR];

	/* internal helper that guide tuning the chip. */
	static byte _extendedFrameLength;
	static byte _preambleCode;
	static byte _channel;
	static byte _preambleLength;
	static byte _pulseFrequency;
	static byte _dataRate;
	static byte _pacSize;

	/* internal helper to remember how to properly act. */
	static boolean _permanentReceive;
	static boolean _frameCheck;

	// whether RX or TX is active
	static int _deviceMode;

	/* ISR. */
	static void handleInterrupt();

	/* internal helper to read/write system registers. */
	static void readSystemEventStatusRegister();
	static void readSystemConfigurationRegister();
	static void writeSystemConfigurationRegister();
	static void readNetworkIdAndDeviceAddress();
	static void writeNetworkIdAndDeviceAddress();
	static void readSystemEventMaskRegister();
	static void writeSystemEventMaskRegister();
	static void readChannelControlRegister();
	static void writeChannelControlRegister();
	static void readTransmitFrameControlRegister();
	static void writeTransmitFrameControlRegister();

	/* reading and writing bytes from and to DW1000 module. */
	static void readBytes(byte cmd, word offset, byte data[], int n);
	static void writeBytes(byte cmd, word offset, byte data[], int n);

	/* writing numeric values to bytes. */
	static void writeValueToBytes(byte data[], int val, int n); 

	/* internal helper for bit operations on multi-bytes. */
	static boolean getBit(byte data[], int n, int bit);
	static void setBit(byte data[], int n, int bit, boolean val);
	
	/* Register is 6 bit, 7 = write, 6 = sub-adressing, 5-0 = register value
	 * Total header with sub-adressing can be 15 bit. */
	static const byte WRITE = 0x80; // regular write
	static const byte WRITE_SUB = 0xC0; // write with sub address
	static const byte READ = 0x00; // regular read
	static const byte READ_SUB = 0x40; // read with sub address
};

extern DW1000Class DW1000;

#endif
