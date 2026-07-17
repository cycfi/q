#!/usr/bin/env python3
"""
Generate the q::rising_edge / q::falling_edge reference figure.

Produces, in docs/modules/ROOT/images/:

   edge.svg  -- a boolean input with rising_edge and falling_edge firing a
                single-sample event at each 0->1 and 1->0 transition.

Style and palette follow gen_interpolation_figures.py.

Usage: /usr/bin/python3 docs/scripts/gen_edge_figures.py
"""

import os
import numpy as np
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt

SITE_ACCENT = '#1565c0'    # the boolean input
MAGENTA = '#d81b60'        # edge events (markers only)
GREY = '#5d5d5d'

OUT_DIR = os.path.join(
   os.path.dirname(__file__), '..', 'modules', 'ROOT', 'images')


def gen():
   n = 280
   t = np.arange(n)

   inp = np.zeros(n)
   pulses = [(20, 60), (90, 105), (150, 220), (245, 258)]
   for a, b in pulses:
      inp[a:b] = 1.0

   # rising_edge: 1 when input transitions 0 -> 1; falling_edge: 1 on 1 -> 0
   rise = np.zeros(n, dtype=bool)
   fall = np.zeros(n, dtype=bool)
   prev = 0.0
   for i in range(n):
      v = inp[i]
      rise[i] = bool(v) and (prev != v)
      fall[i] = (not bool(v)) and (prev != v)
      prev = v

   ri = np.where(rise)[0]
   fi = np.where(fall)[0]

   fig, ax = plt.subplots(figsize=(10, 4))

   # boolean input as a filled step
   ax.fill_between(t, 0, inp, step='post', color=SITE_ACCENT, alpha=0.16,
                   zorder=1)
   ax.step(t, inp, where='post', color=SITE_ACCENT, linewidth=1.5,
           label='input (bool)', zorder=2)

   # edge events as single-sample stems above / below the input
   ax.vlines(ri, 1.0, 1.28, color=MAGENTA, linewidth=1.6, zorder=3)
   ax.plot(ri, np.full_like(ri, 1.28, dtype=float), '^', color=MAGENTA,
           markersize=7, markeredgecolor='white', markeredgewidth=0.7,
           label='rising_edge fires', zorder=4)
   ax.vlines(fi, -0.28, 0.0, color=MAGENTA, linewidth=1.6, zorder=3)
   ax.plot(fi, np.full_like(fi, -0.28, dtype=float), 'v', color=MAGENTA,
           markersize=7, markeredgecolor='white', markeredgewidth=0.7,
           label='falling_edge fires', zorder=4)

   ax.axhline(0, color=GREY, lw=0.8, zorder=1)
   ax.set_xlabel('Time (samples)')
   ax.set_yticks([0, 1])
   ax.set_yticklabels(['0', '1'])
   ax.set_xlim(0, n - 1)
   ax.set_ylim(-0.5, 1.5)
   for sp in ('top', 'right', 'left'):
      ax.spines[sp].set_visible(False)
   ax.spines['bottom'].set_color('#b0b0b0')
   ax.legend(loc='upper right', fontsize=9, ncol=3)

   fig.tight_layout()
   path = os.path.join(OUT_DIR, 'edge.svg')
   fig.savefig(path, format='svg', bbox_inches=None)
   plt.close(fig)
   print('wrote', path)


if __name__ == '__main__':
   gen()
