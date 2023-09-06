import pandas as pd
import numpy as np
from datetime import datetime
import matplotlib.pyplot as plt

noPIDfile = './noPID.csv'             # PID thermometer data
PIDfile = './PID.csv'

""" Open recordings """


# Open recordings as pandas df
PIDdf   = pd.read_csv(PIDfile)
noPIDdf = pd.read_csv(noPIDfile)

# PID and thermometer to numpy arrays
PID  = {'T':PIDdf['T'].to_numpy(), 't': PIDdf['t'].to_numpy()}
noPID  = {'T':noPIDdf['T'].to_numpy(), 't': noPIDdf['t'].to_numpy()}

# Crop files to start at same temperature
initial_temp = max([PID['T'][0],noPID['T'][0]])

max_length = min([len(PID['T']),len(noPID['T'])])

idx = PID['T']>=initial_temp

PID['T'] = PID['T'][idx]
PID['t'] = PID['t'][idx]

idx = noPID['T']>=initial_temp

noPID['T'] = noPID['T'][idx]
noPID['t'] = noPID['t'][idx]

PID['T'] = PID['T'][:max_length]
PID['t'] = PID['t'][:max_length] - PID['t'][0]

noPID['T'] = noPID['T'][:max_length]
noPID['t'] = noPID['t'][:max_length] - noPID['t'][0]



# Plot
plt.figure('Recordings')
plt.plot(PID['t'], PID['T'],label='PID')
plt.plot(noPID['t'], noPID['T'],label='Bang bang')
plt.axhline(y=75.3571, color='r', linestyle='-',label='Target temperature')
plt.ylabel(r'$T$'+'\n'+r'[C$^o$]',labelpad=20).set_rotation(0)
plt.xlabel('Elapsed time [s]')
plt.grid()
plt.legend()
plt.show()
