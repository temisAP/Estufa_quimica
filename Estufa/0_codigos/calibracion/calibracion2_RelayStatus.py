import pandas as pd
import numpy as np
from datetime import datetime
import matplotlib.pyplot as plt

stove_readings = './recordings.csv'             # Stove thermometer data
thermometer_recordings = './thermometer.csv'    # Calibrated thermometer data
segments = {40: [100,200], 80: [550,600], 120: [975,1050], 160: [1520,1600], 200: [2100,2250]}

""" Open recordings """


# Open recordings as pandas df
stove_df = pd.read_csv(stove_readings)
therm_df = pd.read_csv(thermometer_recordings)

# Convert date strings to datetime objects
stove_df['date'] = stove_df['date'].apply(lambda x: datetime.strptime(str(x), '%Y,%m,%d,%H:%M:%S').time())
therm_df['date'] = therm_df['date'].apply(lambda x: datetime.strptime(str(x), '%I:%M:%S %p').time())

stove_df['date'] = stove_df['date'].apply(lambda x: datetime.strptime(str(x), '%H:%M:%S'))
therm_df['date'] = therm_df['date'].apply(lambda x: datetime.strptime(str(x), '%H:%M:%S'))

# Sort by date
stove_df.sort_values(by='date', inplace=True)
therm_df.sort_values(by='date', inplace=True)

common_times = True
if common_times:

    # Find common start time
    start_time = max(stove_df['date'].iloc[0], therm_df['date'].iloc[0])

    # Find common end time
    end_time = min(stove_df['date'].iloc[-1], therm_df['date'].iloc[-1])

    # Filter dataframes to common time range
    stove_df = stove_df[stove_df['date'] >= start_time]
    stove_df = stove_df[stove_df['date'] <= end_time]
    therm_df = therm_df[therm_df['date'] >= start_time]
    therm_df = therm_df[therm_df['date'] <= end_time]

# Change date to time increments in seconds "since start"
stove_df['date'] = stove_df['date'].apply(lambda x: (x - stove_df['date'].iloc[0]).total_seconds())
therm_df['date'] = therm_df['date'].apply(lambda x: (x - therm_df['date'].iloc[0]).total_seconds())

# Stove and thermometer to numpy arrays
stove  = {'T':stove_df['T'].to_numpy(), 't': stove_df['date'].to_numpy()}
therm  = {'T':therm_df['T'].to_numpy(), 't': therm_df['date'].to_numpy()}


def relay_activation(currentTemp,targetTemp):
     if currentTemp <= targetTemp:
         print(currentTemp,targetTemp,1)
         return 1
     elif currentTemp >= targetTemp:
         print(currentTemp,targetTemp,0)
         return 0

relay_status = np.zeros_like(stove['T'])

idx = 0; idx_max = len(stove['T'])-1

currentTemp = stove['T'][idx]

for targetTemp in segments.keys():
    print("Ramp from ",currentTemp," to ",targetTemp)


    while currentTemp <= targetTemp:
        currentTemp = stove['T'][idx]
        relay_status[idx] = relay_activation(currentTemp,targetTemp)
        idx += 1;
        if idx>=idx_max:
            loop = False
            break

    if idx>=idx_max:
      loop = False
      break

    if currentTemp >= targetTemp:
        segmentZeroTime = stove['t'][idx]
        print("Segment on",targetTemp);
        while stove['t'][idx]-segmentZeroTime < 3*60:
            currentTemp = stove['T'][idx]
            relay_status[idx] = relay_activation(currentTemp,targetTemp)
            idx += 1;
            if idx>=idx_max:
              loop = False
              break


plt.figure('Once_calibrated')
plt.plot(stove['t'], stove['T'], label='Stove temp')
plt.plot(stove['t'],relay_status, label='Relay status')
plt.ylabel(r'T [C$^o$]').set_rotation(0)
plt.xlabel('Elapsed time [s]')
plt.grid()
plt.legend()

plt.show()


# Save stove recordings once calibrated
stove_caibrated_df = pd.DataFrame(data={'T':stove['T'], 't':stove['t'],'relay':relay_status})
stove_caibrated_df.to_csv('./recordings_relay.csv', index=False)
