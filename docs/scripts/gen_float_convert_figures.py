#!/usr/bin/env python3
"""
Generate the q float_convert reference figure.

Produces, in docs/modules/ROOT/images/:

   float_convert_map.svg  -- the integer-code to float mapping for a small
                             4-bit example: the bipolar to_float scale
                             (signed / offset-binary, spanning [-1, +1)) and
                             the unipolar to_unsigned_float scale (spanning
                             [0, 1]).

The values mirror the divisors in q/utility/float_convert.hpp: to_float uses
half = 2^(bits-1), to_unsigned_float uses max = 2^bits - 1.

Style and palette follow gen_interpolation_figures.py (the canonical
PALETTE lives there).

Usage: python3 docs/scripts/gen_float_convert_figures.py
"""

import os
import numpy as np
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt

SITE_ACCENT = '#1565c0'    # bipolar
AMBER = '#ffb300'          # unipolar
GREEN = '#43a047'          # reference
GRID = '#b0b0b0'

OUT_DIR = os.path.join(
   os.path.dirname(__file__), '..', 'modules', 'ROOT', 'images')

if __name__ == '__main__':
   bits = 4
   codes = np.arange(2 ** bits)              # 0 .. 15
   half = 2 ** (bits - 1)                    # 8
   max_u = 2 ** bits - 1                      # 15

   # Bipolar: offset-binary code -> [-1, +1). Signed two's complement gives
   # the same float set; the raw code axis differs only by an XOR of the
   # sign bit, so one curve represents both.
   bipolar = (codes - half) / half           # -1.0 .. +0.875
   unipolar = codes / max_u                   # 0.0 .. 1.0

   fig, ax = plt.subplots(figsize=(10, 6))
   ax.set_xlabel('Integer code (4-bit example)')
   ax.set_ylabel('Float value')
   ax.grid(True, linestyle='--', linewidth=0.5, color=GRID, alpha=0.8)
   ax.axhline(0.0, color='#333333', linewidth=0.8)

   ax.plot(codes, bipolar, color=SITE_ACCENT, linewidth=1.6,
           marker='o', markersize=5, label='to_float (bipolar, [-1, +1))')
   ax.plot(codes, unipolar, color=AMBER, linewidth=1.6,
           marker='s', markersize=5, label='to_unsigned_float (unipolar, [0, 1])')

   # +1.0 is not representable on the bipolar scale (max is +0.875 here).
   ax.axhline(1.0, color=GREEN, linewidth=1.0, linestyle=':')
   ax.annotate('+1.0 not representable\n(max = (2^bits-1) / 2^(bits-1))',
               xy=(15, 0.875), xytext=(7.6, 1.06),
               fontsize=9, color='#333333',
               arrowprops=dict(arrowstyle='->', color='#333333', lw=0.8))

   ax.set_xticks(codes)
   ax.set_ylim(-1.15, 1.2)
   ax.legend(loc='upper left')
   ax.set_title(
      'Sample-code to float, 4-bit example: bipolar normalizes by 2^(bits-1), '
      'unipolar by 2^bits - 1')

   path = os.path.join(OUT_DIR, 'float_convert_map.svg')
   fig.savefig(path, format='svg', bbox_inches=None)
   print('wrote', path)
