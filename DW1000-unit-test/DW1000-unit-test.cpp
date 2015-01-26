/*
 * Copyright (c) 2015 by Thomas Trojer <thomas@trojer.net>
 * Decawave DW1000 library for Arduino.
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Some unit test cases for the DW1000 module library.
 */

#include "QUnit.hpp"
#include <iostream>
#include "DW1000.h"

class DW1000Test {
private:
	QUnit::UnitTest qunit;

	void test() {
		// TODO proper tests, this is a stub
		QUNIT_IS_EQUAL(42, 42);
		QUNIT_IS_EQUAL("42", 42);
		QUNIT_IS_TRUE(42 == 42);
		QUNIT_IS_FALSE(43 == 42);
		QUNIT_IS_NOT_EQUAL(43, 42);
		std::string str = "The Answer Is 42";
		QUNIT_IS_EQUAL("The Answer Is 42", str);
		QUNIT_IS_FALSE(std::string::npos == str.find("The Answer"));
	}

public:
	DW1000Test(std::ostream &out, int verboseLevel = QUnit::verbose) : 
		qunit(out, verboseLevel) {}

	int run() {
		DW1000 dw = DW1000(1);
		std::cout << dw.readDeviceIdentifier();
		test();
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
