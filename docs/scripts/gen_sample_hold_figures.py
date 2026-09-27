#!/usr/bin/env python3
"""
Generate the sample_hold reference figure.

Produces, in docs/modules/ROOT/images/:

   sample_hold.svg  -- a slow signal, the samples sample_hold takes of it,
                       and the reference it returns, d to 1.5 d behind.

sample_hold is replayed as q/fx/sample_hold.hpp runs it: a point sample
every n = d * sps / 2 samples, three holds deep, returning the oldest as it
was before the current sample joined.

Style and palette follow gen_interpolation_figures.py.

Usage: /usr/bin/python3 docs/scripts/gen_sample_hold_figures.py
"""

import os
import numpy as np
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt

SITE_ACCENT = '#1565c0'
SKY = '#64b5f6'
AMBER = '#ffb300'
GREY = '#b0b0b0'

OUT_DIR = os.path.join(
   os.path.dirname(__file__), '..', 'modules', 'ROOT', 'images')

D = 32                                     # the lookback, in samples
LENGTH = 240


def signal():
   t = np.arange(LENGTH)
   return 0.5 + 0.35 * np.sin(2 * np.pi * t / 150) \
      + 0.1 * np.sin(2 * np.pi * t / 47)


def sample_hold(x, n):
   then = last = hold = 0.0
   tick = 0
   out, taken = [], []
   for i, s in enumerate(x):
      ref = then
      tick += 1
      if tick >= n:
         tick = 0
         then, last, hold = last, hold, s
         taken.append(i)
      out.append(ref)
   return np.array(out), taken


def gen():
   x = signal()
   ref, taken = sample_hold(x, D // 2)
   t = np.arange(LENGTH)

   fig, ax = plt.subplots(figsize=(10, 4.8))
   ax.plot(t, x, color=SITE_ACCENT, linewidth=1.8, label='input s')
   ax.plot(taken, x[taken], 'o', color=SKY, markersize=6,
           label='held samples, every d / 2')
   ax.step(t, ref, where='post', color=AMBER, linewidth=2.0,
           label='sh(s): the reference')

   t0 = 170
   ax.axvspan(t0 - 1.5 * D, t0 - D, color=GREY, alpha=0.25)
   ax.axvline(t0, color=GREY, linestyle=':', linewidth=1.2)
   ax.text(t0 - 1.25 * D, 1.02, 'd to 1.5 d before', ha='center',
           va='top', color='#333333', fontsize=9)
   ax.text(t0 + 2, 1.02, 'now', va='top', color='#333333', fontsize=9)

   ax.set_xlim(0, LENGTH - 1)
   ax.set_ylim(0, 1.05)
   ax.set_xlabel(f'Time (samples), d = {D}')
   ax.set_ylabel('Value')
   for s in ('top', 'right'):
      ax.spines[s].set_visible(False)
   ax.spines['left'].set_color(GREY)
   ax.spines['bottom'].set_color(GREY)
   ax.grid(True, linestyle='--', color=GREY, alpha=0.5)
   ax.legend(loc='lower right', fontsize=9)

   fig.tight_layout()
   path = os.path.join(OUT_DIR, 'sample_hold.svg')
   fig.savefig(path, format='svg', bbox_inches=None)
   plt.close(fig)
   print('wrote', path)


if __name__ == '__main__':
   gen()
