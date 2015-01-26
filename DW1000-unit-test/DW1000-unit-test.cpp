/*
 * Copyright (c) 2015 by Thomas Trojer <thomas@trojer.net>
 * Decawave DW1000 library for Arduino.
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Some unit test cases for the DW1000 module library. Compile DW1000 
 * with DEBUG preprocessor flag.
 */

#include "QUnit.hpp"
#include <iostream>
#include "DW1000.h"

// std::cout << (static_cast<unsigned int>(dw->debugBuffer[1]) & 0xFF) << std::endl;

class DW1000Test {
private:
	QUnit::UnitTest qunit;
	DW1000 *dw;

	void testSetFrameFilter() {
		dw->clearDebugBuffer();
		dw->setFrameFilter(true);
		QUNIT_IS_EQUAL(0x01, dw->debugBuffer[0] & 0xFF);
	}

	void testSetTransmitRate() {
		dw->clearDebugBuffer();
		dw->newTransmit();
		dw->transmitRate(DW1000::TX_RATE_850KBPS);
		dw->startTransmit();
		QUNIT_IS_EQUAL(0x01 << 5, dw->debugBuffer[1] & 0xFF);

		dw->clearDebugBuffer();
		dw->newTransmit();
		dw->transmitRate(0x03);
		dw->startTransmit();
		QUNIT_IS_EQUAL(DW1000::TX_RATE_6800KBPS << 5, dw->debugBuffer[1] & 0xFF);
	}

public:
	DW1000Test(std::ostream &out, int verboseLevel = QUnit::verbose) : 
		qunit(out, verboseLevel) {}

	int run() {
		dw = new DW1000(1);
		std::cout << dw->readDeviceIdentifier() << std::endl;
		// test methods
		testSetFrameFilter();
		testSetTransmitRate();
		// cleanup and summary
		delete dw;
		return qunit.errors();
	}
};

int main() {
	return DW1000Test(std::cerr).run();
} 

/*
 * Using something like
 *

g++ -g -Os -DDEBUG -I../DW1000 -I. ../DW1000/DW1000.cpp DW1000-unit-test.cpp -o /tmp/DW1000-unit.o; chmod +x /tmp/DW1000-unit.o; /tmp/DW1000-unit.o
 
 *
 * to compile and run it. DEBUG flag fakes some Arduino datatypes and excludes SPI usage.
 */
