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
#define RXAUTR_BIT 29

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
#define TXFRB_BIT 4
#define TXPRS_BIT 5
#define TXPHS_BIT 6
#define TXFRS_BIT 7
#define LDEDONE_BIT 10
#define RXDFR_BIT 13
#define RXFCG_BIT 14
#define RXFCE_BIT 15
#define RXRFSL_BIT 16
#define LDEERR_BIT 18

// RX timestamp register
#define RX_TIME 0x15
#define LEN_RX_TIME 14
#define RX_STAMP_SUB 0
#define LEN_RX_STAMP_SUB 5

// timing register (for delayed RX/TX)
#define DX_TIME 0x0A
#define LEN_DX_TIME 5

// transmit data buffer
#define TX_BUFFER 0x09
#define LEN_TX_BUFFER 1024
#define LEN_UWB_FRAMES 127
#define LEN_EXT_UWB_FRAMES 1023

// transmit control
#define TX_FCTRL 0x08
#define LEN_TX_FCTRL 5

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef DEBUG
#include <Arduino.h>
#include "../SPI/SPI.h"
#else
#include <stdint.h>
#define boolean uint8_t
#define byte uint8_t
#define word uint16_t
#define bitSet(value, bit) ((value) |= (1UL << (bit)))
#define bitRead(value, bit) (((value) >> (bit)) & 0x01)
#define bitClear(value, bit) ((value) &= ~(1UL << (bit)))
#endif

class DW1000 {
public:
	/* TODO impl: later
	 * - TXBOFFS in TX_FCTRL for offset buffer transmit
 	 * - TR in TX_FCTRL for flagging for ranging messages
	 * - CANSFCS in SYS_CTRL to cancel frame check suppression
	 * - HSRBP in SYS_CTRL to determine in double buffered mode from which buffer to read
	 */

	// construction with chip select pin number
	DW1000(int ss);
	~DW1000();
	void initialize();

	// chip select
	int getChipSelect();

	// device id, address, etc.
	char* getDeviceIdentifier();
	char* getExtendedUniqueIdentifier();
	char* getNetworkIdAndShortAddress();

	// PAN_ID, SHORT_ADDR, device address management
	void setNetworkId(unsigned int val);
	void setDeviceAddress(unsigned int val);
	
	// SYS_CFG, general device configuration
	byte* getSystemConfiguration();
	void setFrameFilter(boolean val);
	void setDoubleBuffering(boolean val); // NOTE should be set to false
	void setReceiverAutoReenable(boolean val);

	// SYS_CTRL, TX_FCTRL, transmit and receive configuration
	void suppressFrameCheck(boolean val);
	boolean isSuppressFrameCheck();
	void delayedTransceive(unsigned int delayNanos); // TODO impl
	void transmitRate(byte rate);
	void pulseFrequency(byte freq);
	void preambleLength(byte prealen);
	void waitForResponse(boolean val);
	void setData(byte data[], unsigned int n);
	int getData(byte data[]);

	// RX/TX default settings
	void setDefaults();

	// SYS_STATUS, device status flags
	boolean isLDEDone();
	boolean isTransmitDone();
	boolean isReceiveDone();
	boolean isReceiveSuccess();

	void clearReceiveStatus();
	void clearTransmitStatus(); // TODO impl

	// RX_TIME, ..., timing, timestamps, etc.
	// TODO void readReceiveTimestamp(byte[] timestamp);

	// idle
	void idle();

	// general configuration
	void newConfiguration();
	void commitConfiguration();

	// reception
	void newReceive();
	void startReceive();
	void cancelReceive();

	// transmission
	void newTransmit();
	void startTransmit();
	void cancelTransmit();

	// transmission bit rate
	static const byte TX_RATE_110KBPS = 0x00;
	static const byte TX_RATE_850KBPS = 0x01;
	static const byte TX_RATE_6800KBPS = 0x02;

	// transmission pulse frequencey
	// 0x00 is 4MHZ, but receiver in DW1000 does not support it (!??)
	static const byte TX_PULSE_FREQ_16MHZ = 0x01; 
	static const byte TX_PULSE_FREQ_64MHZ = 0x02;

	// preamble lengthes (PE + TXPSR bits)
	static const byte TX_PREAMBLE_LEN_64 = 0x01;
	static const byte TX_PREAMBLE_LEN_128 = 0x05;
	static const byte TX_PREAMBLE_LEN_256 = 0x09;
	static const byte TX_PREAMBLE_LEN_512 = 0x0D;
	static const byte TX_PREAMBLE_LEN_1024 = 0x02;
	static const byte TX_PREAMBLE_LEN_1536 = 0x06;
	static const byte TX_PREAMBLE_LEN_2048 = 0x0A;
	static const byte TX_PREAMBLE_LEN_4096 = 0x03;

#ifdef DEBUG
	byte debugBuffer[1024];
	inline void clearDebugBuffer() {
		memset(debugBuffer, 0, 1024);
	}
#endif

private:
	/* configured chip select pin. */
	unsigned int _ss;

	/* register caches. */
	byte _syscfg[LEN_SYS_CFG];
	byte _sysctrl[LEN_SYS_CTRL];
	byte _sysstatus[LEN_SYS_STATUS];
	byte _txfctrl[LEN_TX_FCTRL];

	/* PAN and short address. */
	byte _networkAndAddress[LEN_PANADR];

	/* internal helper to determine send mode for set data. */
	boolean _extendedFrameLength;

	// whether RX or TX is active
	int _deviceMode;

	/* internal helper to read/write system registers. */
	void readSystemEventStatusRegister();
	void readSystemConfigurationRegister();
	void writeSystemConfigurationRegister();
	void readNetworkIdAndDeviceAddress();
	void writeNetworkIdAndDeviceAddress();

	/* reading and writing bytes from and to DW1000 module. */
	void readBytes(byte cmd, byte data[], int n);
	void writeBytes(byte cmd, word offset, byte data[], int n);

	/* internal helper for bit operations on multi-bytes. */
	boolean getBit(byte data[], int n, int bit);
	void setBit(byte data[], int n, int bit, boolean val);
	
	/* Register is 6 bit, 7 = write, 6 = sub-adressing, 5-0 = register value
	 * Total header with sub-adressing can be 15 bit. */
	static const byte WRITE = 0x80; // regular write
	static const byte WRITE_SUB = 0xC0; // write with sub address
	static const byte READ = 0x00; // regular read
};

#endif
