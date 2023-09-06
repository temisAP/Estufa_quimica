"""

    CALIBRATION 2: DATA PROCESSING

    This code is the second part of a module aimed to calibrate correctly the
    thermocouple module of the stove.

    The data obtained from the stove and external (and calibrated) thermometer
    are compared in order to obtain the linear coefficients of the calibration.

    The variable 'segments' must be eddited in order to use only the recordings
    taken once the stove has stabilized its temperature.

"""

segments = {40: [120,200], 80: [570,600], 120: [980,1050], 160: [1570,1600], 200: [2100,2250]}



import pandas as pd
import numpy as np
from datetime import datetime
import matplotlib.pyplot as plt

stove_readings = './DATA/recordings.csv'             # Stove thermometer data
thermometer_recordings = './DATA/thermometer.csv'    # Calibrated thermometer data



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

# Plot
plt.figure('Recordings',figsize=(8,6))
plt.plot(stove['t'], stove['T'], label='Stove thermometer')
plt.plot(therm['t'], therm['T'], label='Calibrated thermometer')
plt.ylabel(r'T [C$^o$]',labelpad=20).set_rotation(0)
plt.xlabel('Elapsed time [s]')
plt.grid()
plt.legend()
plt.show()

""" Filter """

# Add a STL to filter stove temperature readings
import statsmodels.api as sm

# Filter data
stove['T_filt'] = sm.tsa.filters.hpfilter(stove['T'], lamb=25)[1]

# Plot
plt.figure('Filtered_signal',figsize=(8,6))
plt.plot(stove['t'], stove['T'], label='Stove thermometer')
plt.plot(stove['t'], stove['T_filt'], label='Filtered stove thermometer')
plt.plot(therm['t'], therm['T'], label='Calibrated thermometer')
plt.ylabel(r'T [C$^o$]',labelpad=20).set_rotation(0)
plt.xlabel('Elapsed time [s]')
plt.grid()
plt.legend()

""" Calibration """

# Interpolate
stove['T'] = np.interp(therm['t'], stove['t'], stove['T'])
stove['t'] = therm['t']

if not segments:
    # Determine number of segments and limits
    n_segments = int(input('Number of segments: '))

    segments = list()
    for i in range(n_segments):
        ele = float(input(f'Temperature of segment no{i} [Cº]: '))
        segments.append(ele)

    segments = dict.fromkeys(segments)
    for key in segments.keys():
        print(f'Segment at T = {key} Cº:')
        l1 = float(input(' start time: '))
        l2 = float(input(' end time: '))
        segments[key] = [l1,l2]

stove_Ts = list()
therm_Ts = list()
for T, t in segments.items():
    T = stove_df[stove_df['date'] >= t[0]]
    T = stove_df[stove_df['date'] <= t[1]]
    stove_Ts.append(np.mean(T['T'].to_numpy()))

    T = therm_df[therm_df['date'] >= t[0]]
    T = therm_df[therm_df['date'] <= t[1]]
    therm_Ts.append(np.mean(T['T'].to_numpy()))


def linear_regression(x,a,b):
    try:
        return a*x + b
    except:
        return a*np.array(x) + b

def accuracy(xdata, ydata):
    from scipy.optimize import curve_fit
    popt, pcov = curve_fit(linear_regression, xdata, ydata)
    residuals = ydata- linear_regression(xdata, *popt)
    ss_res = np.sum(residuals**2)
    ss_tot = np.sum((ydata-np.mean(ydata))**2)
    r_squared = 1 - (ss_res / ss_tot)
    return popt,pcov,r_squared


popt,pcov,r_squared = accuracy(therm_Ts,stove_Ts)

plt.figure('Linear_regression',figsize=(8,6))
plt.scatter(therm_Ts, stove_Ts, label='Data')
plt.plot(therm_Ts, np.array(linear_regression(therm_Ts,*popt)), label='Linear regression',color='tab:orange')
plt.ylabel('Stove\nthermometer'+'\n'+r'T [C$^o$]',labelpad=30).set_rotation(0)
plt.xlabel(r'Calibrated thermometer T [C$^o$]')
plt.grid()
plt.legend()

print(f'y = {popt[0]:.2f} x + {popt[1]:.2f} | r = {r_squared:.2f}')
stove['T'] =  (stove['T'] - popt[1]) / popt[0]

plt.figure('Once_calibrated',figsize=(8,6))
plt.plot(stove['t'], stove['T'], label='Stove calibrated thermometer')
plt.plot(therm['t'], therm['T'], label='Calibrated thermometer')
plt.ylabel(r'T [C$^o$]',labelpad=20).set_rotation(0)
plt.xlabel('Elapsed time [s]')
plt.grid()
plt.legend()

plt.show()

# Save stove recordings once calibrated
stove_caibrated_df = pd.DataFrame(data={'T':stove['T'], 't':stove['t']})
stove_caibrated_df.to_csv('./stove_calibrated.csv', index=False)
