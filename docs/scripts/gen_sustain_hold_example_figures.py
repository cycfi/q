#!/usr/bin/env python3
"""
Generate the sustain_hold example figure.

Produces, in docs/modules/ROOT/images/:

   sustain_hold_handover.svg   -- the click-free engage: the dry signal's
                                  half-Hann down-ramp is the exact
                                  complement of the grains' overlap-add
                                  ramp-in, so the sum stays at one

Style and palette follow gen_interpolation_figures.py (the canonical
PALETTE lives there).

Usage: python3 docs/scripts/gen_sustain_hold_example_figures.py
"""

import os
import numpy as np
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt

SKY = '#64b5f6'            # the dry gain
SITE_ACCENT = '#1565c0'
AMBER = '#ffb300'          # secondary series: the grain ramp-in
GREEN = '#43a047'          # reference/ideal: the constant-voltage sum
MAGENTA = '#d81b60'        # event markers: the engage instant

OUT_DIR = os.path.join(
   os.path.dirname(__file__), '..', 'modules', 'ROOT', 'images')


def new_axes(xlabel='Sample', ylabel='Value'):
   fig, ax = plt.subplots(figsize=(10, 6))
   ax.set_xlabel(xlabel)
   ax.set_ylabel(ylabel)
   ax.grid(True, linestyle='--', linewidth=0.5, color='#b0b0b0', alpha=0.8)
   return fig, ax


def save(fig, name):
   path = os.path.join(OUT_DIR, name)
   fig.savefig(path, format='svg', bbox_inches=None)
   plt.close(fig)
   print('wrote', path)


def handover_figure():
   # One hop (8 periods) on each side of the engage instant. The first
   # grain's Hann window ramps in over the hop; the dry feed leaves
   # through the complementary half-Hann down-ramp.
   hop = 200
   n = np.arange(-hop, 2 * hop)

   ramp_in = np.zeros_like(n, dtype=float)
   up = (n >= 0) & (n < hop)
   ramp_in[up] = 0.5 * (1 - np.cos(np.pi * n[up] / hop))
   ramp_in[n >= hop] = 1.0

   dry = 1.0 - ramp_in

   fig, ax = new_axes(xlabel='Samples around engage', ylabel='Gain')
   ax.plot(n, dry, color=SKY, linewidth=1.75,
           label='Dry gain (half-Hann)')
   ax.plot(n, ramp_in, color=AMBER, linewidth=1.75,
           label='Grain OLA ramp-in')
   ax.plot(n, dry + ramp_in, color=GREEN, linewidth=1.5, linestyle='--',
           label='Sum = 1 (no click)')
   ax.axvline(0, color=MAGENTA, linewidth=1.0, linestyle=':',
              label='Engage (SPACE)')
   ax.set_ylim(-0.05, 1.15)
   ax.legend(loc='center right')
   save(fig, 'sustain_hold_handover.svg')


if __name__ == '__main__':
   handover_figure()
