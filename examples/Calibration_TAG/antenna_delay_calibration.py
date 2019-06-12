"""
    Implementation of the APS014 - DW1000 Antenna Delay Calibration.
"""
#!/usr/bin/env python3


import random
import operator
import numpy as np


SPEED_OF_LIGHT = 299792458                       # [m/s]
UNIT_OF_LOW_ORDER_BIT = 1/(128*499.2*10**6)      # [s]

INITAL_DELAY = 513*10**-9                        # [s]
INITAL_RANGE = 6*10**-9                          # [s]
INITAL_QUALITY = 0
pertubation_limit = 0.2*10**-9                   # [s]

ITERATIONS = 100
CANDIDATE_COUNT = 1000

# All EDM_Actual ranges should be the same. (Ground Truth)
EDM_ACTUAL = np.array([
    [0, 2.0, 2.0],
    [2.0, 0, 2.0],
    [2.0, 2.0, 0]])

# Use TWR to measure the EDM_Measured with all antenna delays set to zero.
# This will require nChips*(nChips-1) measurements. Ideally each measurement should be
# an average of several range estimates, we use 200 range estimates for each measurement.
EDM_MEASUREMENT = np.array([
    [0, 156.310368098160, 156.399548133595],
    [156.359591002045, 0, 156.401709233791],
    [156.366707566462, 156.503889980354, 0]])

# Convert measured EDM_Measured and EDM_Actual to times (ToF_measured and ToF_actual)
TOF_ACTUAL = EDM_ACTUAL / SPEED_OF_LIGHT
TOF_MEASUREMENT = EDM_MEASUREMENT / SPEED_OF_LIGHT


def main():
    """ Main function """
    candidate_set = []

    for i in range(0, ITERATIONS):
        populate(candidate_set, i)
        evaluate(candidate_set)

        print(
            "Iter: {0:03d}, Norm: {1:2.6e}, Delays: {2:2.6e} {3:2.6e} {4:2.6e} {5:2.6e} {6:2.6e} {7:2.6e}".format(
                i+1,
                candidate_set[0][0],
                candidate_set[0][1], candidate_set[0][2], candidate_set[0][3],
                candidate_set[0][4], candidate_set[0][5], candidate_set[0][6]))

    # Print Best Candidate
    # The units of the low order bit are approximately 15.65 picoseconds.
    # The actual unit may be calculated as 1 / (128*499.2*10^6) seconds.
    print(
        "Antenna Delay: 1={0:05d} 2={1:05d} 3={2:05d} 4={3:05d} 5={4:05d} 6={5:05d}".format(
            round(candidate_set[0][1]/(2*UNIT_OF_LOW_ORDER_BIT)),
            round(candidate_set[0][2]/(2*UNIT_OF_LOW_ORDER_BIT)),
            round(candidate_set[0][3]/(2*UNIT_OF_LOW_ORDER_BIT)),
            round(candidate_set[0][4]/(2*UNIT_OF_LOW_ORDER_BIT)),
            round(candidate_set[0][5]/(2*UNIT_OF_LOW_ORDER_BIT)),
            round(candidate_set[0][6]/(2*UNIT_OF_LOW_ORDER_BIT))))


def populate(candidate_set, iteration):
    """ Populate the set of candidate delay estimates """
    global pertubation_limit

    # First iteration?
    if not candidate_set:
        # Generate a set of random delays uniformly distributed round initial delay ±6ns.
        a = INITAL_DELAY-INITAL_RANGE
        b = INITAL_DELAY+INITAL_RANGE

        for i in range(0, CANDIDATE_COUNT):
            candidate_set.append((
                INITAL_QUALITY,
                random.uniform(a, b),
                random.uniform(a, b),
                random.uniform(a, b),
                random.uniform(a, b),
                random.uniform(a, b),
                random.uniform(a, b)))
    else:
        # Select the best 25% of the set and include them in the new set.
        best25_count = round(len(candidate_set) * 0.25)
        best25 = candidate_set[0:best25_count]
        candidate_set.clear()
        for candidate in best25:
            candidate_set.append((
                INITAL_QUALITY,
                candidate[1], candidate[2], candidate[3],
                candidate[4], candidate[5], candidate[6]))

        # Perform 3 times
        for i in range(0, 3):
            # Randomly perturb the initial 25% within the perturbation limits
            # and add them to the set.
            for candidate in best25:
                candidate_set.append((
                    INITAL_QUALITY,
                    random.uniform(candidate[1]-pertubation_limit, candidate[1]+pertubation_limit),
                    random.uniform(candidate[2]-pertubation_limit, candidate[2]+pertubation_limit),
                    random.uniform(candidate[3]-pertubation_limit, candidate[3]+pertubation_limit),
                    random.uniform(candidate[4]-pertubation_limit, candidate[4]+pertubation_limit),
                    random.uniform(candidate[5]-pertubation_limit, candidate[5]+pertubation_limit),
                    random.uniform(candidate[6]-pertubation_limit, candidate[6]+pertubation_limit)))

    # Every 20 iterations halve the perturbation limits
    if iteration != 0 and iteration % 20 == 0:
        pertubation_limit = pertubation_limit / 2


def evaluate(candidate_set):
    """ Evaluate the quality of the candidates """

    # Foreach candidate
    for i in range(0, len(candidate_set)):
        candidate = candidate_set[i]

        delay_chip1 = np.zeros((3, 3))
        delay_chip2 = np.zeros((3, 3))

        create_chip_delay(candidate, delay_chip1, delay_chip2)

        # Compute the time of flight matrix given the candidate delays.
        tof_candidate = 0.5 * delay_chip1 + 0.5 * delay_chip2 + TOF_MEASUREMENT

        # Compute the (euclidean) norm of the difference between the
        # tof_acutal matrix and the tof_candidate matrix.
        quality = np.linalg.norm(TOF_ACTUAL - tof_candidate)

        candidate_set[i] = (
            quality,
            candidate[1], candidate[2], candidate[3],
            candidate[4], candidate[5], candidate[6])

    # Sort the candidates. Lowest error first.
    candidate_set.sort(key=operator.itemgetter(0), reverse=False)

#
# Are the delay_chip matrix are correct?
#
def create_chip_delay(candidate, delay_chip1, delay_chip2):
    """ Creates the chip delay matrix """

    delay_chip1[0, 1] = delay_chip1[0, 2] = candidate[1]
    delay_chip1[1, 0] = delay_chip1[1, 2] = candidate[2]
    delay_chip1[2, 0] = delay_chip1[2, 1] = candidate[3]

    delay_chip2[1, 0] = delay_chip2[2, 0] = candidate[4]
    delay_chip2[0, 1] = delay_chip2[2, 1] = candidate[5]
    delay_chip2[0, 2] = delay_chip2[1, 2] = candidate[6]

'''
def create_chip_delay(candidate, delay_chip1, delay_chip2):
    """ Creates the chip delay matrix """

    delay_chip1[0, 1] = delay_chip1[0, 2] = candidate[1]
    delay_chip1[1, 0] = delay_chip1[1, 2] = candidate[2]
    delay_chip1[2, 0] = delay_chip1[2, 1] = candidate[3]

    delay_chip2[1, 0] = delay_chip2[2, 0] = candidate[1]
    delay_chip2[0, 1] = delay_chip2[2, 1] = candidate[2]
    delay_chip2[0, 2] = delay_chip2[1, 2] = candidate[3]
'''

if __name__ == '__main__':
    main()
