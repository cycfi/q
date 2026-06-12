# =============================================================================
#  Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.
#
#  Distributed under the Boost Software License, Version 1.0.
#  [ https://www.boost.org/LICENSE_1_0.txt ]
# =============================================================================
# Renders the true RMS envelope follower response figure for the docs
# (true_rms_envelope_follower.adoc), in the same style as the fast RMS
# figure (python/2023/main.py recipe: linear amplitude axis with dB tick
# labels, signal #2a4d69 lw 0.5, envelope #00b159 lw 1, x 0.9-3.0 s,
# transparent, 600 dpi).
#
# Input: build/test/results/rms_envelope_follower_1a-Low-E.wav
#        (3 channels from test/rms_envelope_follower.cpp:
#         signal, fast RMS env, true RMS env)
# Output: docs/modules/ROOT/images/true-rms-envelope-follower-output.png
#
# Run from the q root, e.g. with the python/2023 venv:
#    python/2023/venv/bin/python docs/scripts/plot_true_rms_response.py

import numpy as np
import matplotlib.pyplot as plt
import matplotlib.ticker as ticker
import matplotlib.patheffects as pe
from scipy.io import wavfile

src = 'build/test/results/rms_envelope_follower_1a-Low-E.wav'
dst = 'docs/modules/ROOT/images/true-rms-envelope-follower-output.png'

sps, data = wavfile.read(src)          # float32, 3 channels
signal = data[:, 0]
true_env = data[:, 2]
time = np.arange(len(signal)) / sps

plt.rcParams['xtick.labelsize'] = 8
plt.rcParams['ytick.labelsize'] = 8
plt.rcParams['font.family'] = 'Roboto'

plt.plot(time, signal, lw=0.5, color='#2a4d69')
# Light green core with a dark green halo: pops against the dense
# signal mass, stays visible against the white page background.
plt.plot(time, true_env, lw=1.4, color='#b9f6ca',
   path_effects=[pe.withStroke(linewidth=2.8, foreground='#00753c')])
plt.xlabel('Time (s)')
plt.ylabel('Amplitude (dB)')

plt.gca().yaxis.set_major_formatter(ticker.ScalarFormatter())
plt.gca().yaxis.set_major_formatter(
   lambda x, _: f'{round(20*np.log10(abs(x)), 0) if abs(x) > 0 else "-∞"}')

plt.ylim(-0.75, 0.75)
plt.xlim(0.9, 3.0)

plt.savefig(dst, facecolor='none', transparent=True, dpi=600)
print('written:', dst)
