#!/usr/bin/env python3
"""
Generate the q ring_allocator reference figure.

Produces, in docs/modules/ROOT/images/:

   ring_allocator_layout.svg  -- one 256 byte buffer partway through use.
                                 Blocks A, B and C (40, 24 and 56 bytes)
                                 were allocated, published and consumed.
                                 Then D (24) fits before the end, E (40)
                                 does not and goes to the start behind a
                                 skip header, both are published, and F
                                 (8) is allocated but not yet published.

Mirrors q/utility/ring_allocator.hpp: a header of two sizes (16 bytes on a
64 bit target) ahead of each block, every record a whole number of
headers, and a skip header over the gap when a record does not fit before
the end. The layout was checked against the real class on 2026-10-12:
the blocks' offsets in the buffer were D 208, E 16, F 80, and consume
passed D, then E.

Style and palette follow gen_bitset_figures.py.

Usage: python3 docs/scripts/gen_ring_allocator_figures.py
"""

import os
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
from matplotlib.patches import Rectangle

SITE_ACCENT = '#1565c0'    # published blocks
SKY = '#64b5f6'            # allocated, not yet published
HEADER = '#0d3c78'         # headers
GRID = '#b0b0b0'
INK = '#1a1a1a'
MUTED = '#777777'

OUT_DIR = os.path.join(
   os.path.dirname(__file__), '..', 'modules', 'ROOT', 'images')

SIZE = 256
UNIT = 16


def layout():
   """Run the scenario with the allocator's arithmetic. Returns the
   records as (start, record, block size, name, state) and the read,
   write and allocation positions, all as offsets in the buffer."""
   records = []
   read = write = alloc = 0

   def allocate(size, name):
      nonlocal alloc
      record = UNIT + (size + UNIT - 1) // UNIT * UNIT
      to_end = SIZE - alloc % SIZE
      gap = to_end if to_end < record else 0
      if SIZE - (alloc - read) < gap + record:
         return None
      if gap:
         records.append([alloc, gap, None, 'skip', 'skip'])
         alloc += gap
      records.append([alloc, record, size, name, 'allocated'])
      alloc += record
      return alloc - record + UNIT

   def publish():
      nonlocal write
      write = alloc
      for r in records:
         if r[0] < write and r[4] == 'allocated':
            r[4] = 'published'

   def consume():
      nonlocal read
      read = write
      for r in records:
         if r[0] < read:
            r[4] = 'freed'

   for size, name in ((40, 'A'), (24, 'B'), (56, 'C')):
      allocate(size, name)
   publish()
   consume()
   d = allocate(24, 'D')
   e = allocate(40, 'E')
   publish()
   f = allocate(8, 'F')
   assert allocate(150, 'G') is None

   # The offsets the real class gave for the same scenario.
   assert (d % SIZE, e % SIZE, f % SIZE) == (208, 16, 80)

   live = [r for r in records if r[4] != 'freed']
   return live, read % SIZE, write % SIZE, alloc % SIZE


def draw_record(ax, start, record, size, name, state, y, h):
   start %= SIZE
   if state == 'skip':
      ax.add_patch(Rectangle((start, y), record, h, facecolor='#f0f0f0',
         edgecolor=MUTED, hatch='///', linewidth=1.2, zorder=2))
      ax.text(start + record / 2, y - 0.28, 'skip', ha='center',
         va='center', fontsize=12, color=INK)
      return

   face = SITE_ACCENT if state == 'published' else SKY
   ax.add_patch(Rectangle((start, y), UNIT, h, facecolor=HEADER,
      edgecolor='#ffffff', linewidth=1.2, zorder=2))
   ax.add_patch(Rectangle((start + UNIT, y), record - UNIT, h,
      facecolor=face, edgecolor='#ffffff', linewidth=1.2, zorder=2))
   ax.text(start + UNIT + (record - UNIT) / 2, y + h / 2, name,
      ha='center', va='center', fontsize=14, color='#ffffff', zorder=3)


def marker(ax, x, label, y, above):
   dy = 0.5 if above else -0.5
   ax.annotate(label, xy=(x, y), xytext=(x, y + dy),
      ha='center', va='bottom' if above else 'top', fontsize=12,
      color=INK, arrowprops=dict(arrowstyle='->', color=INK, lw=1.2))


if __name__ == '__main__':
   live, read, write, alloc = layout()

   fig, ax = plt.subplots(figsize=(10, 3.0))
   y, h = 0.0, 1.0

   # The buffer, then what is in it.
   ax.add_patch(Rectangle((0, y), SIZE, h, facecolor='#ffffff',
      edgecolor=GRID, linewidth=1.2, zorder=1))
   for start, record, size, name, state in live:
      draw_record(ax, start, record, size, name, state, y, h)

   free_mid = (alloc + read) / 2
   ax.text(free_mid, y + h / 2, 'free', ha='center', va='center',
      fontsize=12, color=MUTED)

   marker(ax, read, 'read', y, above=False)
   marker(ax, write, 'write', y + h, above=True)
   marker(ax, alloc, 'allocate', y + h, above=True)

   for x in range(0, SIZE + 1, 64):
      ax.text(x, y - 1.25, str(x), ha='center', va='center',
         fontsize=10, color=MUTED)

   # Legend.
   lx, ly = 0, -1.95
   for face, text in ((HEADER, 'header'), (SITE_ACCENT, 'published'),
                      (SKY, 'allocated, not yet published')):
      ax.add_patch(Rectangle((lx, ly), 10, 0.35, facecolor=face,
         edgecolor='none'))
      ax.text(lx + 14, ly + 0.17, text, ha='left', va='center',
         fontsize=11, color=INK)
      lx += 14 + 7 * len(text) + 14

   ax.set_xlim(-8, SIZE + 8)
   ax.set_ylim(-2.2, 2.1)
   ax.axis('off')

   os.makedirs(OUT_DIR, exist_ok=True)
   out = os.path.join(OUT_DIR, 'ring_allocator_layout.svg')
   fig.savefig(out, bbox_inches='tight', pad_inches=0.1)
   print('wrote', out)
