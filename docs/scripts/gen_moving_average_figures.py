#!/usr/bin/env python3
"""
Generate the moving average reference figures.

Produces, in docs/modules/ROOT/images/:

   exp_moving_average.svg  -- moving_average and exp_moving_average of the
                              same span, n = 16: their weights (impulse
                              response) and their step response.
   moving_average2.svg     -- the magnitude response of moving_average2.

The filters are replayed with the arithmetic of q/fx/moving_average.hpp:
the moving average of the last n samples, and y = b s + (1 - b) y with
b = 2 / (n + 1).

Style and palette follow gen_interpolation_figures.py.

Usage: /usr/bin/python3 docs/scripts/gen_moving_average_figures.py
"""

import os
import numpy as np
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt

SITE_ACCENT = '#1565c0'
AMBER = '#ffb300'
GREY = '#b0b0b0'

OUT_DIR = os.path.join(
   os.path.dirname(__file__), '..', 'modules', 'ROOT', 'images')

N = 16


def sma(x, n=N):
   hist = [0.0] * n
   out = []
   for s in x:
      hist = [s] + hist[:-1]
      out.append(sum(hist) / n)
   return np.array(out)


def ema(x, n=N):
   b = 2.0 / (n + 1)
   y, out = 0.0, []
   for s in x:
      y = b * s + (1 - b) * y
      out.append(y)
   return np.array(out)


def style(ax):
   for s in ('top', 'right'):
      ax.spines[s].set_visible(False)
   ax.spines['left'].set_color(GREY)
   ax.spines['bottom'].set_color(GREY)
   ax.grid(True, linestyle='--', color=GREY, alpha=0.5)


def save(fig, name):
   fig.tight_layout()
   path = os.path.join(OUT_DIR, name)
   fig.savefig(path, format='svg', bbox_inches=None)
   plt.close(fig)
   print('wrote', path)


def ema_figure():
   length = 48
   k = np.arange(length)
   impulse = np.zeros(length)
   impulse[0] = 1.0
   step = np.ones(length)

   fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(10, 4.2))

   ax1.step(k, sma(impulse), where='post', color=SITE_ACCENT,
            linewidth=1.8, label=f'moving_average, n = {N}')
   ax1.plot(k, ema(impulse), 'o-', color=AMBER, markersize=3,
            linewidth=1.4, label=f'exp_moving_average<{N}>')
   mean_age = (N - 1) / 2
   ax1.axvline(mean_age, color=GREY, linestyle=':', linewidth=1.4)
   ax1.text(mean_age + 0.8, 0.105, f'mean age\n(n - 1) / 2 = {mean_age:g}',
            color='#333333', fontsize=9, va='top')
   ax1.set_title('Weight given to a sample, by age', color='#333333',
                 fontsize=11)
   ax1.set_xlabel('Age (samples)')
   ax1.set_ylim(0, 0.13)

   ax2.plot(k, sma(step), color=SITE_ACCENT, linewidth=1.8,
            label=f'moving_average, n = {N}')
   ax2.plot(k, ema(step), color=AMBER, linewidth=1.8,
            label=f'exp_moving_average<{N}>')
   ax2.set_title('Step response', color='#333333', fontsize=11)
   ax2.set_xlabel('Time (samples)')
   ax2.set_ylim(0, 1.05)

   for ax in (ax1, ax2):
      ax.set_xlim(0, length - 1)
      style(ax)
      ax.legend(loc='center right', fontsize=9)
   save(fig, 'exp_moving_average.svg')


def ma2_figure():
   sps = 48000.0
   f = np.linspace(0, sps / 2, 1000)
   # (s + prev) / 2 has H = cos(pi f / sps) in magnitude
   mag = np.abs(np.cos(np.pi * f / sps))
   db = 20 * np.log10(np.maximum(mag, 1e-6))

   fig, ax = plt.subplots(figsize=(10, 4.6))
   ax.plot(f / 1000, db, color=SITE_ACCENT, linewidth=1.8)
   ax.axhline(-3, color=GREY, linewidth=0.8)
   ax.axvline(sps / 4000, color=GREY, linestyle=':', linewidth=1.2)
   ax.text(sps / 4000 + 0.2, -2.4, '-3 dB at a quarter of the\nsample rate',
           color='#333333', fontsize=9, va='bottom')
   ax.text(sps / 2000 - 0.2, -38, 'zero at\nNyquist', color='#333333',
           fontsize=9, ha='right', va='bottom')
   ax.set_xlim(0, sps / 2000)
   ax.set_ylim(-40, 2)
   ax.set_xlabel('Frequency (kHz), 48 kHz sampling')
   ax.set_ylabel('Gain (dB)')
   style(ax)
   save(fig, 'moving_average2.svg')


if __name__ == '__main__':
   ema_figure()
   ma2_figure()
