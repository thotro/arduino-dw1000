"""
    Reads the json data from the DWM1000 Controller Board and write it to a csv file.
"""
#!/usr/bin/env python3


import json
import csv
import serial


SERIAL_PORT = "/dev/ttyUSB0"
CSV_FILE_NAME = "B1.csv"
GROUND_TRUTH = 2.0
MEASUREMENTS = 1000


with serial.Serial(SERIAL_PORT, 115200, timeout=2) as ser:
    with open(CSV_FILE_NAME, "a") as csv_file:
        csv_writer = csv.writer(csv_file, delimiter=';', lineterminator='\n')

        for i in range(0, MEASUREMENTS):
            print("{0:03d} of {1:03d}".format(i+1, MEASUREMENTS))
            while True:
                line = ser.readline()
                line = line.decode("utf-8")
                try:
                    print(line)
                    obj = json.loads(line)
                except ValueError:
                    continue
                break

            # CSV Columns:
            # type; tag_address; anchor_address; ground_truth; range;
            # receive_power; first_path_power; receive_quality;
            # temperature; voltage
            csv_writer.writerow([
                obj["type"], obj["ta"], obj["aa"], GROUND_TRUTH, obj["r"],
                obj["rxp"], obj["fpp"], obj["q"], obj["t"], obj["v"]])
