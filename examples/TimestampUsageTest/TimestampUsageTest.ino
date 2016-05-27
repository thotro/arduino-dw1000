/*
 * Copyright (c) 2015 by Thomas Trojer <thomas@trojer.net>
 * Decawave DW1000 library for arduino.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * @file TimestampUsageTest.ino
 * This is a simple unit test for the DW1000Time class. This test
 * has no actual use to the operation of the DW1000.
 */

#include <SPI.h>
#include <DW1000.h>

void setup() {
  Serial.begin(9600);
  Serial.println(F("### DW1000Time-arduino-test ###"));
}

void loop() {
  // variables for the test
  DW1000Time time;
  DW1000Time time2;
  DW1000Time time3;
  byte stamp[LEN_STAMP];
  // unit test
  Serial.print("Time is [us] ... "); Serial.println(time.getAsFloat(), 4);
  time += DW1000Time(10, DW_MICROSECONDS);
  Serial.print("Time is [us] ... "); Serial.println(time.getAsFloat(), 4);
  time -= DW1000Time(500, DW_NANOSECONDS);
  Serial.print("Time is [us] ... "); Serial.println(time.getAsFloat(), 4);
  time2 = time;
  time2 += DW1000Time(10.0f);
  Serial.print("Time2 == Time1 ... "); Serial.println(time == time2 ? "YES" : "NO");
  time += DW1000Time(10000, DW_NANOSECONDS);
  Serial.print("Time2 == Time1 ... "); Serial.println(time == time2 ? "YES" : "NO");
  memset(stamp, 0, LEN_STAMP);
  stamp[1] = 0x02; // = 512
  time2 = DW1000Time(stamp);
  Serial.print("Time2 is [us] ... "); Serial.println(time2.getAsFloat(), 4);
  Serial.print("Time2 range is [m] ... "); Serial.println(time2.getAsMeters(), 4);
  time3 = DW1000Time(10, DW_SECONDS);
  time3.getTimestamp(stamp);
  time3.setTimestamp(stamp);
  Serial.print("Time3 is [s] ... "); Serial.println(time3.getAsFloat() * 1.0e-6, 4);
  // keep calm
  delay(10000);
}

