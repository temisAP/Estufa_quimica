import pandas as pd
import numpy as np
from datetime import datetime
import matplotlib.pyplot as plt

stove_readings = './stove_calibrated.csv'             # Stove thermometer data
limits = None
limits = [589,910]

""" Open recordings """


# Open recordings as pandas df
stove_df = pd.read_csv(stove_readings)

# Stove and thermometer to numpy arrays
stove  = {'T':stove_df['T'].to_numpy(), 't': stove_df['t'].to_numpy(),'relay': stove_df['relay'].to_numpy()}


# Plot
plt.figure('Recordings')
plt.plot(stove['t'], stove['T'])
plt.plot(stove['t'], stove['relay']*100)
plt.ylabel(r'T [C$^o$]').set_rotation(0)
plt.xlabel('Elapsed time [s]')
plt.grid()
plt.show()


""" Calibration """

if limits:

    def find_index(array,value):
        idx = (np.abs(np.array(array) - value)).argmin()
        return idx

    idx_ini = find_index(stove['t'],limits[0])
    idx_fin = find_index(stove['t'],limits[1])

    # Crop
    stove['t'] = stove['t'][idx_ini:idx_fin]
    stove['T'] = stove['T'][idx_ini:idx_fin]
    stove['relay'] = stove['relay'][idx_ini:idx_fin]

    # Zero as origin
    stove['t'] = stove['t'] - stove['t'][0]
    stove['T'] = stove['T'] - stove['T'][0]

    # Plot
    plt.figure('Region to consider')
    plt.plot(stove['t'], stove['T'])
    plt.ylabel(r'$\Delta$T [C$^o$]').set_rotation(0)
    plt.xlabel('Elapsed time [s]')
    plt.grid()
    plt.show()

# Save region
stove_caibrated_df = pd.DataFrame(data={'time':stove['t'],'input':stove['relay'],'output':stove['T']})
stove_caibrated_df.to_csv('./region.csv', index=False)


def func(x, a, b):
    return a*(x-b+b**(-x/b))

def accuracy(func,xdata, ydata):
    from scipy.optimize import curve_fit
    popt, pcov = curve_fit(func, xdata, ydata,bounds=([0,0], [10,100]))
    residuals  = ydata - func(xdata, *popt)
    ss_res = np.sum(residuals**2)
    ss_tot = np.sum((ydata-np.mean(ydata))**2)
    r_squared = 1 - (ss_res / ss_tot)
    return popt,pcov,r_squared

popt, pcov, r_squared = accuracy(func, stove['t'], stove['T'])

plt.figure('Curve fitted')
plt.plot(stove['t'], stove['T'], label='Sensor measure')
plt.plot(stove['t'], np.array(func(stove['t'],*popt)), label='Model')
plt.ylabel(r'$\Delta$T [C$^o$]').set_rotation(0)
plt.xlabel('Elapsed time [s]')
plt.grid()
plt.legend()
plt.show()

print(*popt)



plt.show()
