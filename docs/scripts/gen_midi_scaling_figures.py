#!/usr/bin/env python3
"""
Generate the MIDI Translation reference figure.

Produces, in docs/modules/ROOT/images/:

   midi_scaling.svg  -- a 7 bit value scaled up to 16 bits two ways: by a
                        plain shift, which stops short of the maximum, and
                        by Min-Center-Max (M2-115-U section 3), which keeps
                        zero, the centre and the maximum exact.

scale_up below is the algorithm of q/midi/scaling.hpp, line for line.

Style and palette follow gen_interpolation_figures.py.

Usage: /usr/bin/python3 docs/scripts/gen_midi_scaling_figures.py
"""

import os
import numpy as np
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt

SITE_ACCENT = '#1565c0'    # Min-Center-Max
AMBER = '#ffb300'          # the plain shift
GREY = '#5d5d5d'
GRID = '#b0b0b0'

OUT_DIR = os.path.join(
   os.path.dirname(__file__), '..', 'modules', 'ROOT', 'images')


def scale_up(value, from_bits, to_bits):
   if to_bits <= from_bits:
      return value
   scale_bits = to_bits - from_bits
   centre = 1 << (from_bits - 1)
   shifted = value << scale_bits
   if value <= centre:
      return shifted
   repeat_bits = from_bits - 1
   repeat = value & ((1 << repeat_bits) - 1)
   if scale_bits > repeat_bits:
      repeat <<= scale_bits - repeat_bits
   else:
      repeat >>= repeat_bits - scale_bits
   result = shifted
   while repeat != 0:
      result |= repeat
      repeat >>= repeat_bits
   return result


def gen():
   x = np.arange(128)
   mcm = np.array([scale_up(int(v), 7, 16) for v in x])
   shift = x << 9

   assert mcm[0] == 0 and mcm[64] == 0x8000 and mcm[127] == 0xFFFF

   plt.rcParams.update({'font.size': 13})
   fig, (full, top) = plt.subplots(
      1, 2, figsize=(11, 4.6), gridspec_kw={'width_ratios': [1.15, 1]})

   for ax in (full, top):
      ax.step(x, shift, where='post', color=AMBER, linewidth=2.0,
              label='shift', zorder=2)
      ax.step(x, mcm, where='post', color=SITE_ACCENT, linewidth=2.0,
              label='Min-Center-Max', zorder=3)
      ax.grid(True, linestyle='--', color=GRID, linewidth=0.7)
      ax.set_xlabel('7 bit value')
      for s in ('top', 'right'):
         ax.spines[s].set_visible(False)

   full.set_ylabel('16 bit value')
   full.set_xlim(0, 127)
   full.set_ylim(0, 68000)
   full.set_xticks([0, 64, 127])
   full.set_yticks([0, 0x8000, 0xFFFF])
   full.set_yticklabels(['0', '32768', '65535'])
   full.plot([0, 64, 127], [0, 0x8000, 0xFFFF], 'o', color=SITE_ACCENT,
             markersize=7, zorder=4)
   full.legend(loc='upper left', frameon=False)
   full.set_title('The whole range', fontsize=13, color=GREY)

   top.set_xlim(112, 127.9)
   top.set_ylim(57000, 66200)
   top.set_xticks([112, 116, 120, 124, 127])
   top.set_yticks([57344, 61440, 65024, 65535])
   top.set_yticklabels(['57344', '61440', '65024', '65535'])
   top.plot([127], [0xFFFF], 'o', color=SITE_ACCENT, markersize=7, zorder=4)
   top.plot([127], [65024], 'o', color=AMBER, markersize=7, zorder=4)
   top.set_title('The top of the range', fontsize=13, color=GREY)

   fig.tight_layout()
   path = os.path.join(OUT_DIR, 'midi_scaling.svg')
   fig.savefig(path, format='svg', bbox_inches=None)
   plt.close(fig)
   print('wrote', path)


if __name__ == '__main__':
   gen()
