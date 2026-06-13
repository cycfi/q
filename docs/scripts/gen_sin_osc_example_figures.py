#!/usr/bin/env python3
"""
Generate the sin_osc example figure.

Produces, in docs/modules/ROOT/images/:

   sin_osc_phase.svg   -- the phase accumulator ramp (wrapping once per
                          cycle) and the sine it is mapped to, for a few
                          cycles of the example's 440 Hz at 44.1 kHz

Style and palette follow gen_interpolation_figures.py (the canonical
PALETTE lives there).

Usage: python3 docs/scripts/gen_sin_osc_example_figures.py
"""

import os
import numpy as np
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt

SKY = '#64b5f6'            # samples
SITE_ACCENT = '#1565c0'    # curves
AMBER = '#ffb300'          # secondary series: the phase ramp
MAGENTA = '#d81b60'        # event markers: phase wraps

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


def phase_figure():
   freq, sps = 440.0, 44100.0
   n = np.arange(int(3 * sps / freq) + 1)     # three cycles
   phase = (n * freq / sps) % 1.0             # accumulator, wraps at 1
   out = np.sin(2 * np.pi * phase)

   fig, ax = new_axes(ylabel='Value')
   ax.plot(n, phase, color=AMBER, linewidth=1.5,
           label='Phase accumulator (normalized)')
   ax.plot(n, out, color=SITE_ACCENT, linewidth=1.75,
           label='q::sin(phase++)')
   wraps = np.nonzero(np.diff(phase) < 0)[0] + 1
   for i, w in enumerate(wraps):
      ax.axvline(w, color=MAGENTA, linewidth=1.0, linestyle=':',
                 label='Phase wrap (new cycle)' if i == 0 else None)
   ax.legend(loc='best')
   save(fig, 'sin_osc_phase.svg')


if __name__ == '__main__':
   phase_figure()
