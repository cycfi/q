#!/usr/bin/env python3
"""
Generate the delay1 / delay2 reference figure.

Produces, in docs/modules/ROOT/images/:

   delay1_delay2.svg  -- a short input and what delay1 and delay2 return
                         for it, sample by sample.

Style and palette follow gen_interpolation_figures.py.

Usage: /usr/bin/python3 docs/scripts/gen_delay_figures.py
"""

import os
import numpy as np
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt

SITE_ACCENT = '#1565c0'
SKY = '#64b5f6'
GREY = '#b0b0b0'

OUT_DIR = os.path.join(
   os.path.dirname(__file__), '..', 'modules', 'ROOT', 'images')

INPUT = [0.0, 1.0, 0.6, -0.4, 0.8, -0.2, 0.3, 0.0, 0.0, 0.0, 0.0, 0.0]


def delayed(x, n):
   # delay1 and delay2 start at 0 and return the sample pushed n calls ago
   return [0.0] * n + list(x[:len(x) - n])


def gen():
   x = np.array(INPUT)
   n = np.arange(len(x))
   rows = [
      ('input s', x),
      ('delay1: one sample later', np.array(delayed(INPUT, 1))),
      ('delay2: two samples later', np.array(delayed(INPUT, 2))),
   ]

   fig, axes = plt.subplots(3, 1, figsize=(10, 5.6), sharex=True)
   for k, (ax, (title, y)) in enumerate(zip(axes, rows)):
      markers, stems, base = ax.stem(n, y)
      plt.setp(markers, color=SKY, markersize=7, zorder=3)
      plt.setp(stems, color=SITE_ACCENT, linewidth=1.6)
      plt.setp(base, color=GREY, linewidth=0.8)
      ax.axvline(1 + k, color=GREY, linestyle=':', linewidth=1.2)
      ax.set_title(title, color='#333333', fontsize=11, loc='left')
      ax.set_ylim(-0.7, 1.3)
      for s in ('top', 'right'):
         ax.spines[s].set_visible(False)
      ax.spines['left'].set_color(GREY)
      ax.spines['bottom'].set_color(GREY)
      ax.grid(True, linestyle='--', color=GREY, alpha=0.5)
   axes[-1].set_xlabel('Time (samples)')
   axes[-1].set_xticks(n)

   fig.tight_layout()
   path = os.path.join(OUT_DIR, 'delay1_delay2.svg')
   fig.savefig(path, format='svg', bbox_inches=None)
   plt.close(fig)
   print('wrote', path)


if __name__ == '__main__':
   gen()
