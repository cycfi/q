#!/usr/bin/env python3
"""
Generate the q clip-family reference figure.

Produces, in docs/modules/ROOT/images/:

   clip_transfer.svg  -- transfer curves: hard_clip vs cubic_clip (cubic,
                         1.5s - 0.5s^3, after a +/-1 clamp) vs tanh_clip (tanh)

Style and palette follow gen_interpolation_figures.py (the canonical
PALETTE lives there).

Usage: python3 docs/scripts/gen_clip_figures.py
"""

import os
import numpy as np
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt

SITE_ACCENT = '#1565c0'    # hard_clip
AMBER = '#ffb300'          # cubic_clip
GREEN = '#2e7d32'          # tanh_clip

OUT_DIR = os.path.join(
   os.path.dirname(__file__), '..', 'modules', 'ROOT', 'images')

if __name__ == '__main__':
   x = np.linspace(-3, 3, 1201)
   s = np.clip(x, -1, 1)
   hard = s
   cubic = 1.5 * s - 0.5 * s**3
   tanhc = np.tanh(x)

   fig, ax = plt.subplots(figsize=(10, 6))
   ax.set_xlabel('Input')
   ax.set_ylabel('Output')
   ax.grid(True, linestyle='--', linewidth=0.5, color='#b0b0b0', alpha=0.8)

   ax.plot(x, x, color='#b0b0b0', linewidth=1.0, linestyle=':',
           label='Identity')
   ax.plot(x, hard, color=SITE_ACCENT, linewidth=1.75, label='hard_clip')
   ax.plot(x, cubic, color=AMBER, linewidth=1.75, label='cubic_clip')
   ax.plot(x, tanhc, color=GREEN, linewidth=1.75, label='tanh_clip')
   ax.set_ylim(-1.6, 1.6)
   ax.legend(loc='best')

   path = os.path.join(OUT_DIR, 'clip_transfer.svg')
   fig.savefig(path, format='svg', bbox_inches=None)
   print('wrote', path)
