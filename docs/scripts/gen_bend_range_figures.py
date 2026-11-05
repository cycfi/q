#!/usr/bin/env python3
"""
Generate the bend range figure.

Produces, in docs/modules/ROOT/images/:

   bend_range.svg   -- a bend_range of 12 semitones down, 7 up and a
                       middle of 1 semitone either way: the pitch offset
                       each MIDI 1.0 bend value plays, three straight
                       segments

The mapping is bend_range's own (note 1 in q/midi/bend_range.hpp). Style
and palette follow
gen_interpolation_figures.py (the canonical PALETTE lives there).

Usage: python3 docs/scripts/gen_bend_range_figures.py
"""

import os
import numpy as np
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt

SITE_ACCENT = '#1565c0'    # the mapping
AMBER = '#ffb300'          # the depths
MAGENTA = '#d81b60'        # the center
GREEN = '#43a047'          # the middle segment

OUT_DIR = os.path.join(
   os.path.dirname(__file__), '..', 'modules', 'ROOT', 'images')

DOWN, UP, LINEAR = 12.0, 7.0, 1.0


def semitones(v):
   # Three straight segments; see note 1 in q/midi/bend_range.hpp.
   d = v.astype(float) - 8192.0
   x = np.where(d < 0, d / 8192.0, d / 8191.0)
   k = (DOWN + UP) / 2
   m = min(max(LINEAR, 0.0), min(DOWN, UP))
   w = m / k
   a = np.abs(x)
   depth = np.where(x < 0, DOWN, UP)
   s = np.where(a <= w, k * a, m + (depth - m) * (a - w) / (1.0 - w))
   return np.sign(x) * s


def depth_arrow(ax, x, y0, y1, text, ha):
   ax.annotate('', xy=(x, y1), xytext=(x, y0),
               arrowprops=dict(arrowstyle='<->', color=AMBER, lw=2.0))
   ax.text(x + (-350 if ha == 'right' else 350), (y0 + y1) / 2, text,
           color='#333333', fontsize=14, ha=ha, va='center')


def bend_range_figure():
   v = np.arange(0, 16384)
   fig, ax = plt.subplots(figsize=(10, 6))
   ax.grid(True, linestyle='--', linewidth=0.5, color='#b0b0b0', alpha=0.8)
   ax.set_xlabel('Pitch bend (MIDI 1.0, 14 bits)', fontsize=13)
   ax.set_ylabel('Semitones', fontsize=13)

   ax.axhline(-DOWN, color=AMBER, linestyle=':', linewidth=1.2)
   ax.axhline(UP, color=AMBER, linestyle=':', linewidth=1.2)
   w = LINEAR / ((DOWN + UP) / 2)
   ax.axvspan(8192 - w * 8192, 8192 + w * 8191, color=GREEN,
              alpha=0.15, linewidth=0, label='middle: 1 semitone either way')
   ax.plot(v, semitones(v), color=SITE_ACCENT, linewidth=2.5,
           label='bend_range{12, 7, 1}')
   ax.plot([8192], [0], 'o', color=MAGENTA, markersize=9,
           label='center: 0 semitones, ratio 1')

   depth_arrow(ax, 600, 0, -DOWN, 'down: 12', 'left')
   depth_arrow(ax, 15800, 0, UP, 'up: 7', 'right')

   ax.set_xticks([0, 4096, 8192, 12288, 16383])
   ax.set_yticks([-12, -9, -6, -3, 0, 3, 7])
   ax.set_xlim(-300, 16683)
   ax.set_ylim(-14, 9)
   ax.tick_params(labelsize=12)
   ax.legend(loc='upper left', fontsize=12)

   path = os.path.join(OUT_DIR, 'bend_range.svg')
   fig.savefig(path, format='svg', bbox_inches='tight')
   plt.close(fig)
   print('wrote', path)


if __name__ == '__main__':
   bend_range_figure()
