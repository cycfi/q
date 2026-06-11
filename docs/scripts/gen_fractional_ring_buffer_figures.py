#!/usr/bin/env python3
"""
Generate the fractional_ring_buffer reference figure.

Produces, in docs/modules/ROOT/images/:

   fractional_ring_buffer_read.svg -- discrete buffer samples, the
       interpolated continuum between them, and a fractional read
       (buf[2.4]) landing between samples

Style and palette follow gen_interpolation_figures.py (the canonical
PALETTE lives there).

Usage: python3 docs/scripts/gen_fractional_ring_buffer_figures.py
"""

import os
import numpy as np
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt

SKY = '#64b5f6'            # the stored samples
SITE_ACCENT = '#1565c0'    # the interpolated continuum
MAGENTA = '#d81b60'        # event marker: the fractional read

OUT_DIR = os.path.join(
   os.path.dirname(__file__), '..', 'modules', 'ROOT', 'images')


def hermite(y, i):
   k = int(np.floor(i))
   mu = i - k
   y0, y1, y2, y3 = y[k-1], y[k], y[k+1], y[k+2]
   c1 = 0.5 * (y2 - y0)
   c2 = y0 - 2.5*y1 + 2.0*y2 - 0.5*y3
   c3 = 0.5 * (y3 - y0) + 1.5 * (y1 - y2)
   return ((c3*mu + c2)*mu + c1)*mu + y1


if __name__ == '__main__':
   # Eight stored samples of a smooth signal; the fractional index
   # interpolates between them (hermite: valid range [1, size-3])
   idx = np.arange(8)
   y = np.sin(2 * np.pi * idx / 5.5 + 0.6) * np.exp(-idx / 12)

   xi = np.linspace(1, 5, 400)
   yi = [hermite(y, i) for i in xi]

   read_at = 2.4
   read_val = hermite(y, read_at)

   fig, ax = plt.subplots(figsize=(10, 6))
   ax.set_xlabel('Index  (0 = newest sample, increasing = older)')
   ax.set_ylabel('Value')
   ax.grid(True, linestyle='--', linewidth=0.5, color='#b0b0b0', alpha=0.8)

   # Stems for the discrete samples
   ax.vlines(idx, 0, y, color=SKY, linewidth=1.0, alpha=0.6)
   ax.axhline(0, color='#b0b0b0', linewidth=0.8)
   ax.plot(idx, y, 'o', color=SKY, markersize=8, label='Stored samples')

   # The interpolated continuum between samples
   ax.plot(xi, yi, color=SITE_ACCENT, linewidth=1.75,
           label='Interpolated (hermite)')

   # A fractional read
   ax.vlines(read_at, 0, read_val, color=MAGENTA, linewidth=1.2,
             linestyle='--')
   ax.plot([read_at], [read_val], 'o', color=MAGENTA, markersize=9,
           label=f'buf[{read_at}]')
   ax.legend(loc='best')

   path = os.path.join(OUT_DIR, 'fractional_ring_buffer_read.svg')
   fig.savefig(path, format='svg', bbox_inches=None)
   print('wrote', path)
