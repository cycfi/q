#!/usr/bin/env python3
"""
Generate the q::clip / q::soft_clip reference figure.

Produces, in docs/modules/ROOT/images/:

   clip_transfer.svg  -- transfer curves: hard clip vs soft clip
                         (1.5s - 0.5s^3 after a +/-1 clamp)

Style and palette follow gen_interpolation_figures.py (the canonical
PALETTE lives there).

Usage: python3 docs/scripts/gen_clip_figures.py
"""

import os
import numpy as np
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt

SITE_ACCENT = '#1565c0'    # hard clip
AMBER = '#ffb300'          # secondary series: soft clip

OUT_DIR = os.path.join(
   os.path.dirname(__file__), '..', 'modules', 'ROOT', 'images')

if __name__ == '__main__':
   x = np.linspace(-2, 2, 801)
   hard = np.clip(x, -1, 1)
   s = np.clip(x, -1, 1)
   soft = 1.5 * s - 0.5 * s**3

   fig, ax = plt.subplots(figsize=(10, 6))
   ax.set_xlabel('Input')
   ax.set_ylabel('Output')
   ax.grid(True, linestyle='--', linewidth=0.5, color='#b0b0b0', alpha=0.8)

   ax.plot(x, x, color='#b0b0b0', linewidth=1.0, linestyle=':',
           label='Identity')
   ax.plot(x, hard, color=SITE_ACCENT, linewidth=1.75, label='clip')
   ax.plot(x, soft, color=AMBER, linewidth=1.75, label='soft_clip')
   ax.set_ylim(-1.6, 1.6)
   ax.legend(loc='best')

   path = os.path.join(OUT_DIR, 'clip_transfer.svg')
   fig.savefig(path, format='svg', bbox_inches=None)
   print('wrote', path)
