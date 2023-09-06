"""

    CALIBRATION 1: DATA ADQUISITION

    This code is the first part of a module aimed to calibrate correctly the
    thermocouple module of the stove.

    Before running this code, "calibracion.ino" must be uploaded into the arduino
    and an external thermometer, with serial comunication must be plugged into
    the USB port specified

    Also, it records the data given by the stove and the thermometer in a .csv
    which can be plotted with "plot_evol.py" script from the FOS/OBR module

"""

import os
import pandas as pd
import serial
import time
import re

""" *** PARAMETERS *** """

# CSV directory
csv_path = './DATA/recordings.csv'

# Set serial port of the stove
stove = serial.Serial('COM3', 9600)


""" *** FUNCTIONS *** """

def read_from_arduino(verbosity = False):

    try:
        # read line
        line = stove.readline()
        # decode bits to string
        line = line.decode('unicode_escape')
        # remove newline
        line = line.rstrip()
        # Split in time and temperature parts
        line1, line2 = line.split('//')
        # Extract float from line
        if 'Temp' in line1:
            T = re.findall("\d+\.\d+", line1)[0]
        if 'Elapsed time' in line2:
            t = re.findall("\d+\.\d+", line2)[0]

        print(f'{line} \n ---> Temperature: {T} °C // Elapsed time: {t} s') if verbosity else False

        return T,t
    except Exception as e:
        print(line)
        return None,None



""" *** MAIN LOOP *** """

# Create directory if it doesn't exist'
if not os.path.exists(os.path.dirname(csv_path)):
    os.makedirs(os.path.dirname(csv_path))

# Start recording
input('Press enter to start recording')

# Start the loop
while True:

    # Read from arduino
    T_stove, t = read_from_arduino(verbosity=True)

    if T_stove and t:
        # Store data in df
        df = pd.DataFrame(columns=['T_stove','t','Global time'])

        df = df.append({'T_stove':T_stove,
                        't':t,
                        'Global time': time.strftime("%Y,%m,%d,%H:%M:%S")},
                        ignore_index=True)

        # Append new line to data in csv
        df.to_csv(csv_path, mode='a', header=False)
