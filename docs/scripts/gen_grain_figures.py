#!/usr/bin/env python3
"""
Generate the q::grain reference figures.

Produces, in docs/modules/ROOT/images/:

   grain_window.svg   -- a grain: Hann window over a slice of the buffer,
                         and the windowed output it emits
   grain_cola.svg     -- constant overlap-add: windows at half-width hops
                         sum to one

Style and palette follow gen_interpolation_figures.py (the canonical
PALETTE lives there).

Usage: python3 docs/scripts/gen_grain_figures.py
"""

import os
import numpy as np
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt

SKY = '#64b5f6'            # input signal
SITE_ACCENT = '#1565c0'    # window
AMBER = '#ffb300'          # secondary series: the grain output
GREEN = '#43a047'          # reference/ideal: the COLA sum

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


def hann(width):
   j = np.arange(width)
   return 0.5 * (1.0 - np.cos(2 * np.pi * j / width))


def window_figure():
   # A decaying tone in the buffer; one grain anchored at sample 120,
   # 160 samples wide (4 cycles of the period-40 tone)
   n = np.arange(400)
   signal = np.exp(-n / 350) * np.sin(2 * np.pi * n / 40)

   anchor, width = 120, 160
   w = hann(width)
   out = np.zeros_like(signal)
   out[anchor:anchor+width] = signal[anchor:anchor+width] * w

   fig, ax = new_axes()
   ax.plot(n, signal, color=SKY, linewidth=1.2, label='Buffer contents')
   ax.plot(np.arange(anchor, anchor+width), w, color=SITE_ACCENT,
           linewidth=1.75, linestyle='--', label='Hann window')
   ax.plot(n, out, color=AMBER, linewidth=1.75, label='Grain output')
   ax.legend(loc='best')
   save(fig, 'grain_window.svg')


def cola_figure():
   # Hann windows at half-width hops sum to one (constant overlap-add):
   # the contract overlap-add scheduling relies on
   width, hop = 160, 80
   n = np.arange(400)
   total = np.zeros(400, dtype=float)

   fig, ax = new_axes(ylabel='Window value')
   first = True
   for start in range(0, 400, hop):
      w = hann(width)
      end = min(start + width, 400)
      seg = np.arange(start, end)
      ax.plot(seg, w[:end-start], color=SITE_ACCENT, linewidth=1.2,
              alpha=0.75, label='Hann windows' if first else None)
      total[start:end] += w[:end-start]
      first = False

   # The sum is exactly one wherever coverage is complete
   ax.plot(n[hop:], total[hop:], color=GREEN, linewidth=1.75,
           label='Sum (= 1)')
   ax.legend(loc='best')
   save(fig, 'grain_cola.svg')


if __name__ == '__main__':
   window_figure()
   cola_figure()
