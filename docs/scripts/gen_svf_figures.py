#!/usr/bin/env python3
"""
Generate the svf (TPT state-variable filter) reference figure.

Produces, in docs/modules/ROOT/images/:

   svf_response.svg

Both panels are the magnitude response of the ACTUAL filter: the TPT recurrence
(Simper/Cytomic) is run in Python exactly as in q/fx/svf.hpp, its impulse
response is taken per mode, and the FFT magnitude is plotted. Left: the five
responses available from one filter at Q = 2. Right: the lowpass at rising Q,
showing the peak height equal to Q (the exact resonance parametrization).

Style matches gen_interpolation_figures.py. Run with /usr/bin/python3.
"""

import os
import numpy as np
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt

PALETTE = {
   'site_accent': '#1565c0', 'orange': '#fb8c00', 'green': '#43a047',
   'purple': '#8e24aa', 'amber': '#ffb300',
   'powder': '#bbdefb', 'sky': '#64b5f6', 'strong_blue': '#1e88e5',
   'royal': '#0d47a1',
}
GRID_COLOR = '#b0b0b0'
TEXT_COLOR = '#1a1a1a'

OUT_DIR = os.path.join(
   os.path.dirname(__file__), '..', 'modules', 'ROOT', 'images')

SPS = 48000.0
FC = 1000.0
N = 1 << 15      # impulse-response length / FFT size


def svf_impulse(fc, Q, mode, n=N, sps=SPS):
   g = np.tan(np.pi * fc / sps)
   k = 1.0 / Q
   a1 = 1.0 / (1.0 + g * (g + k))
   a2 = g * a1
   a3 = g * a2
   ic1 = ic2 = 0.0
   h = np.empty(n)
   for i in range(n):
      x = 1.0 if i == 0 else 0.0
      v3 = x - ic2
      v1 = a1 * ic1 + a2 * v3
      v2 = ic2 + a2 * ic1 + a3 * v3
      ic1 = 2.0 * v1 - ic1
      ic2 = 2.0 * v2 - ic2
      if mode == 'lp':      y = v2
      elif mode == 'bp':    y = v1
      elif mode == 'hp':    y = x - k * v1 - v2
      elif mode == 'notch': y = x - k * v1
      elif mode == 'peak':  y = 2.0 * v2 - x + k * v1
      h[i] = y
   return h


def magnitude_db(h, sps=SPS):
   H = np.fft.rfft(h)
   f = np.fft.rfftfreq(len(h), 1.0 / sps)
   mag = 20.0 * np.log10(np.maximum(np.abs(H), 1e-9))
   return f, mag


def style(ax):
   ax.set_xscale('log')
   ax.set_xlim(20, 20000)
   ax.set_xlabel('Frequency (Hz)', color=TEXT_COLOR)
   ax.set_ylabel('Magnitude (dB)', color=TEXT_COLOR)
   ax.grid(True, which='both', linestyle='--', linewidth=0.5,
           color=GRID_COLOR, alpha=0.7)


def main():
   fig, (axL, axR) = plt.subplots(1, 2, figsize=(12, 5))

   # Left: the modes at Q = 2.
   modes = [
      ('lp', 'Lowpass', PALETTE['site_accent']),
      ('hp', 'Highpass', PALETTE['orange']),
      ('bp', 'Bandpass', PALETTE['green']),
      ('notch', 'Notch', PALETTE['purple']),
      ('peak', 'Peak', PALETTE['amber']),
   ]
   for key, label, color in modes:
      f, mag = magnitude_db(svf_impulse(FC, 2.0, key))
      axL.plot(f, mag, color=color, linewidth=1.6, label=label)
   axL.set_ylim(-40, 12)
   axL.set_title('One filter, every response  (fc = 1 kHz, Q = 2)',
                 color=TEXT_COLOR)
   axL.legend(loc='lower center', ncol=3, fontsize=8)
   style(axL)

   # Right: lowpass at rising Q -> peak height equals Q.
   qs = [(0.707, PALETTE['powder']), (2.0, PALETTE['sky']),
         (5.0, PALETTE['strong_blue']), (10.0, PALETTE['royal'])]
   for Q, color in qs:
      f, mag = magnitude_db(svf_impulse(FC, Q, 'lp'))
      axR.plot(f, mag, color=color, linewidth=1.6, label=f'Q = {Q:g}')
   axR.set_ylim(-40, 24)
   axR.set_title('Lowpass: peak height = Q', color=TEXT_COLOR)
   axR.legend(loc='lower left', fontsize=8)
   style(axR)

   fig.tight_layout()
   path = os.path.join(OUT_DIR, 'svf_response.svg')
   fig.savefig(path, format='svg', bbox_inches=None)
   plt.close(fig)
   print('wrote', path)


if __name__ == '__main__':
   main()
