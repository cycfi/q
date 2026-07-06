#!/usr/bin/env python3
"""
Generate the q::window_comparator reference figure.

Produces, in docs/modules/ROOT/images/:

   window_comparator.svg  -- a wandering signal crossing a low/high band,
                             with the boolean output that latches high above
                             `high`, low below `low`, and holds inside the
                             band (noise within the band causes no chatter).

Style and palette follow gen_interpolation_figures.py.

Usage: /usr/bin/python3 docs/scripts/gen_window_comparator_figures.py
"""

import os
import numpy as np
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt

SITE_ACCENT = '#1565c0'    # the input signal
AMBER = '#ffb300'          # the hysteresis band
MAGENTA = '#d81b60'        # the boolean output (an event/state marker)
GREY = '#5d5d5d'
INK = '#1a1a1a'

OUT_DIR = os.path.join(
   os.path.dirname(__file__), '..', 'modules', 'ROOT', 'images')


def gen():
   n = 300
   t = np.arange(n)
   low, high = 0.35, 0.65
   rng = np.random.default_rng(5)

   base = 0.5 + 0.40 * np.sin(2 * np.pi * t / 175) \
      + 0.13 * np.sin(2 * np.pi * t / 33 + 1.0)
   s = base + 0.025 * rng.standard_normal(n)

   y = np.zeros(n)
   state = 0
   for i in range(n):
      if s[i] < low:
         state = 0
      elif s[i] > high:
         state = 1
      y[i] = state

   fig, ax = plt.subplots(figsize=(10, 4.5))

   ax.axhspan(low, high, color=AMBER, alpha=0.18, zorder=1)
   ax.axhline(high, color=GREY, lw=1.0, ls='--', zorder=1)
   ax.axhline(low, color=GREY, lw=1.0, ls='--', zorder=1)
   ax.text(n - 2, high + 0.01, 'high', ha='right', va='bottom',
           color=GREY, fontsize=9)
   ax.text(n - 2, low - 0.01, 'low', ha='right', va='top',
           color=GREY, fontsize=9)

   ax.plot(t, s, color=SITE_ACCENT, linewidth=1.3, label='input s', zorder=2)

   # boolean output as a step band along the bottom
   ybase, yamp = -0.42, 0.24
   out = ybase + y * yamp
   ax.fill_between(t, ybase, out, step='post', color=MAGENTA, alpha=0.28,
                   zorder=2)
   ax.step(t, out, where='post', color=MAGENTA, linewidth=1.5,
           label='output (1 = high, 0 = low)', zorder=3)

   ax.set_xlabel('Time (samples)')
   ax.set_yticks([])
   ax.set_xlim(0, n - 1)
   ax.set_ylim(-0.52, 1.12)
   for sp in ('top', 'right', 'left'):
      ax.spines[sp].set_visible(False)
   ax.spines['bottom'].set_color('#b0b0b0')
   ax.legend(loc='upper left')

   path = os.path.join(OUT_DIR, 'window_comparator.svg')
   fig.savefig(path, format='svg', bbox_inches=None)
   plt.close(fig)
   print('wrote', path)


if __name__ == '__main__':
   gen()
