#!/usr/bin/env python3
"""
Generate the q bitset reference figure.

Produces, in docs/modules/ROOT/images/:

   bitset_layout.svg  -- how bits pack into machine words. A small example
                         with an 8-bit word type (value_size = 8) and a
                         20-bit request: the storage rounds up to 3 words
                         (24 bits of capacity), bit i lives in word i / 8 at
                         position i % 8 (LSB first). A range set,
                         set(5, 10, true), is highlighted to show the
                         word-at-a-time write.

Mirrors q/utility/bitset.hpp: array_size = ceil(num_bits / value_size),
size() = array_size * value_size.

Style and palette follow gen_interpolation_figures.py (the canonical
PALETTE lives there).

Usage: python3 docs/scripts/gen_bitset_figures.py
"""

import os
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
from matplotlib.patches import Rectangle

SITE_ACCENT = '#1565c0'    # set bits
SKY = '#64b5f6'            # word frame accent
AMBER = '#ffb300'          # the range-set span
GRID = '#b0b0b0'
INK = '#1a1a1a'
MUTED = '#777777'

OUT_DIR = os.path.join(
   os.path.dirname(__file__), '..', 'modules', 'ROOT', 'images')

if __name__ == '__main__':
   value_size = 8           # bits per word (uint8_t example)
   num_bits = 20            # requested
   n_words = (num_bits + value_size - 1) // value_size   # 3
   capacity = n_words * value_size                       # 24

   # Highlight set(5, 10, true): bits 5..14 inclusive.
   set_lo, set_len = 5, 10
   set_hi = set_lo + set_len - 1

   fig, ax = plt.subplots(figsize=(10, 6))

   cell = 1.0
   # Draw words top (word 0) to bottom, bits LSB (left) to MSB (right).
   for w in range(n_words):
      y = (n_words - 1 - w) * (cell + 0.35)
      for p in range(value_size):
         i = w * value_size + p
         x = p * cell
         is_set = set_lo <= i <= set_hi
         requested = i < num_bits
         face = SITE_ACCENT if is_set else ('#ffffff' if requested else '#f0f0f0')
         edge = MUTED if requested else '#d5d5d5'
         ax.add_patch(Rectangle((x, y), cell, cell, facecolor=face,
            edgecolor=edge, linewidth=1.2, zorder=2))
         txt = INK if not is_set else '#ffffff'
         ax.text(x + cell / 2, y + cell / 2, str(i), ha='center',
            va='center', fontsize=10, color=txt, zorder=3)
      # Word label on the left.
      ax.text(-0.7, y + cell / 2, f'word {w}', ha='right', va='center',
         fontsize=11, color=INK)
      # Word frame.
      ax.add_patch(Rectangle((-0.06, y - 0.06), value_size * cell + 0.12,
         cell + 0.12, fill=False, edgecolor=SKY, linewidth=1.6, zorder=1))

   # Capacity vs requested note, under the bottom word.
   ax.text(value_size * cell / 2, -1.15,
      f'bitset<uint8_t>({num_bits}) rounds up to {n_words} words = '
      f'{capacity} bits of capacity  (size() == {capacity})',
      ha='center', va='center', fontsize=11, color=INK)

   # LSB / MSB direction hint on the top word.
   top_y = (n_words - 1) * (cell + 0.35) + cell + 0.45
   ax.annotate('bit position 0..7 within a word, LSB first',
      xy=(0.5, top_y - 0.2), xytext=(value_size * cell - 0.5, top_y - 0.2),
      ha='right', va='center', fontsize=10, color=MUTED,
      arrowprops=dict(arrowstyle='<-', color=MUTED, lw=1.2))

   # Range-set legend swatch.
   ax.add_patch(Rectangle((0.0, -2.1), cell, cell * 0.6,
      facecolor=SITE_ACCENT, edgecolor=MUTED, linewidth=1.0))
   ax.text(cell + 0.15, -2.1 + cell * 0.3,
      f'set({set_lo}, {set_len}, true)  ->  bits {set_lo}..{set_hi} set to 1',
      ha='left', va='center', fontsize=11, color=INK)

   ax.set_xlim(-3.0, value_size * cell + 0.4)
   ax.set_ylim(-2.5, top_y + 0.2)
   ax.set_aspect('equal')
   ax.axis('off')

   os.makedirs(OUT_DIR, exist_ok=True)
   out = os.path.join(OUT_DIR, 'bitset_layout.svg')
   fig.savefig(out, bbox_inches='tight', pad_inches=0.1)
   print('wrote', out)
