#!/usr/bin/env python3
"""
Generate the q::median3 reference figure.

Produces, in docs/modules/ROOT/images/:

   median3.svg  -- a smooth signal peppered with single-sample impulsive
                   spikes, cleaned by median3, next to a 3-tap moving average
                   that only smears each spike across its neighbours.

Style and palette follow gen_interpolation_figures.py.

Usage: /usr/bin/python3 docs/scripts/gen_median3_figures.py
"""

import os
import numpy as np
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt

SITE_ACCENT = '#1565c0'    # the median3 output (the feature)
AMBER = '#ffb300'          # the 3-tap moving average (comparison)
GREY = '#8a8a8a'           # the raw input
SIGNAL_RED = '#e53935'     # the impulsive spikes

OUT_DIR = os.path.join(
   os.path.dirname(__file__), '..', 'modules', 'ROOT', 'images')


def median3_stream(x):
   out = np.empty_like(x)
   b = c = x[0]
   for i, a in enumerate(x):
      out[i] = np.median([a, b, c])
      c = b
      b = a
   return out


def gen():
   n = 90
   t = np.arange(n)
   rng = np.random.default_rng(7)

   # A smooth underlying signal with a touch of low-level noise: the kind of
   # slowly varying signal a median filter should preserve untouched.
   base = 0.55 * np.sin(2 * np.pi * t / 55) + 0.12 * np.sin(2 * np.pi * t / 13)
   x = base + 0.02 * rng.standard_normal(n)

   # Drop in a handful of single-sample impulsive outliers.
   spikes = [12, 31, 32, 50, 68]
   for i, s in zip(spikes, (1.1, -0.95, 0.9, 1.05, -1.0)):
      x[i] += s

   med = median3_stream(x)
   ma = np.convolve(x, np.ones(3) / 3, mode='full')[:n]  # streaming 3-tap mean

   fig, ax = plt.subplots(figsize=(10, 4.5))
   ax.plot(t, x, color=GREY, linewidth=1.0, label='input', zorder=2)
   ax.plot(t, ma, color=AMBER, linewidth=1.6,
           label='3-tap moving average', zorder=3)
   ax.plot(t, med, color=SITE_ACCENT, linewidth=2.0, label='median3', zorder=4)
   ax.scatter(spikes, x[spikes], s=22, color=SIGNAL_RED, zorder=5,
              label='impulsive spike')

   ax.set_xlabel('Time (samples)')
   ax.set_yticks([])
   ax.set_xlim(0, n - 1)
   for s in ('top', 'right', 'left'):
      ax.spines[s].set_visible(False)
   ax.spines['bottom'].set_color('#b0b0b0')
   ax.legend(loc='upper right', ncol=2)

   path = os.path.join(OUT_DIR, 'median3.svg')
   fig.savefig(path, format='svg', bbox_inches=None)
   plt.close(fig)
   print('wrote', path)


if __name__ == '__main__':
   gen()
