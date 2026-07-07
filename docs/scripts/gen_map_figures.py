#!/usr/bin/env python3
"""
Generate the q::map reference figure.

Produces, in docs/modules/ROOT/images/:

   map.svg  -- the same 0 to 1 input mapped onto three output ranges by linear
              interpolation, including an inverted range where y1 > y2.

Style and palette follow gen_interpolation_figures.py.

Usage: /usr/bin/python3 docs/scripts/gen_map_figures.py
"""

import os
import numpy as np
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt

SITE_ACCENT = '#1565c0'
BRIGHT_BLUE = '#2196f3'
AMBER = '#ffb300'

OUT_DIR = os.path.join(
   os.path.dirname(__file__), '..', 'modules', 'ROOT', 'images')


def gen():
   s = np.linspace(0.0, 1.0, 200)

   ranges = [
      ((0.5, 0.8), SITE_ACCENT),
      ((0.0, 1.0), BRIGHT_BLUE),
      ((1.0, 0.0), AMBER),        # inverted: y1 > y2
   ]

   fig, ax = plt.subplots(figsize=(8, 5))

   for (y1, y2), color in ranges:
      y = y1 + s * (y2 - y1)
      ax.plot(s, y, color=color, linewidth=2.0,
              label=f'map({y1:.1f}, {y2:.1f})', zorder=3)
      # endpoint markers at input 0 (y1) and input 1 (y2)
      ax.plot([0, 1], [y1, y2], 'o', color=color, markersize=6,
              markeredgecolor='white', markeredgewidth=0.8, zorder=4)

   ax.set_xlabel('Input s (0 to 1)')
   ax.set_ylabel('Output')
   ax.set_xlim(0, 1)
   ax.set_ylim(-0.05, 1.05)
   for sp in ('top', 'right'):
      ax.spines[sp].set_visible(False)
   ax.spines['left'].set_color('#b0b0b0')
   ax.spines['bottom'].set_color('#b0b0b0')
   ax.grid(True, linestyle='--', color='#b0b0b0', alpha=0.5)
   ax.legend(loc='center right', fontsize=10)

   fig.tight_layout()
   path = os.path.join(OUT_DIR, 'map.svg')
   fig.savefig(path, format='svg', bbox_inches=None)
   plt.close(fig)
   print('wrote', path)


if __name__ == '__main__':
   gen()
