#!/usr/bin/env python3
"""
Generate the pictures Dexter's editor shows, in resources/:

- algorithms.png, the 32 DX7 algorithms in one image of eight columns by
  four rows, and algorithm_boxes.hpp, where each operator's box is within
  its cell;
- keys.png, the keys a break point spans, A-1 to C8, drawn under the
  keyboard scaling and stretched to its width;
- waves.png, the six LFO waves, one per row, shown on the wave buttons;
- logo.png, the header's logo: a sine whose phase another sine bends,
  frequency modulation's own picture.

Each cell is drawn as Q's reference chart draws it, in the editor's
colors: a box per operator, a modulator above what it modulates, the
carriers tinted along the bottom, tied by a bus, and the feedback loop.
The layout is the reference chart's (docs/scripts/gen_dx7_chart.py), read
from Q's own dx7_routing, so the picture and the sound agree. The editor
draws its live highlights over the boxes, from the table.

Needs rsvg-convert. Usage: python3 example/q_plug/dexter/gen_images.py
"""

import math
import os
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.normpath(os.path.join(HERE, '../../..'))
sys.path.insert(0, os.path.join(REPO, 'docs/scripts'))
from gen_dx7_chart import routings, layout, feedback_lane  # noqa: E402

PNG = os.path.join(HERE, 'resources/algorithms.png')
TABLE = os.path.join(HERE, 'algorithm_boxes.hpp')
KEYS = os.path.join(HERE, 'resources/keys.png')
WAVES = os.path.join(HERE, 'resources/waves.png')
LOGO = os.path.join(HERE, 'resources/logo.png')

COLUMNS, ROWS = 8, 4
CELL_W, CELL_H = 128, 104          # a cell, as the menu shows it
ZOOM = 4                           # pixels per unit, for the larger chart

# The editor's palette (palette.hpp) and the theme's label color
LINE = '#64b4e6'
MUTED = '#9aa4b8'
CARRIER = '#12315f'
BOX = '#1c1c1e'
LABEL = 'rgba(220,220,220,0.78)'

# The reference chart's units: a box is a column wide
UNIT, BOX_W, BOX_H, ROW_H = 34, 34, 21, 34
BUS, LOOP = 11, 9

# The worst case of all 32: six columns wide, four rows high
CHART_W = 5 * UNIT + BOX_W + 2 * (LOOP + 4)
CHART_H = 3 * ROW_H + BOX_H + LOOP + BUS + 4


def cell(alg, left, top):
   """One algorithm's chart in the cell at (left, top), the worst case
   fitted, this one centered. The menu labels the cells with their
   numbers. Returns the SVG and each operator's box, in the cell's unit
   square.
   """
   col, row = layout(alg)
   carriers = set(alg['carriers'])

   area = (left + 7, top + 7, left + CELL_W - 7, top + CELL_H - 7)
   s = min((area[2] - area[0]) / CHART_W, (area[3] - area[1]) / CHART_H)
   content = max(col.values()) * UNIT + BOX_W
   x0 = (area[0] + area[2]) / 2 - content * s / 2
   y_top = (area[1] + area[3]) / 2 - CHART_H * s / 2
   y0 = y_top + (LOOP + 3 * ROW_H + BOX_H) * s
   lw = 0.75

   def center(n):
      return (x0 + (col[n] * UNIT + BOX_W / 2) * s,
              y0 - (row[n] * ROW_H + BOX_H / 2) * s)

   out = []

   for src, dst in alg['edges']:                    # who modulates whom
      (mx, my), (dx, dy) = center(src), center(dst)
      out.append(f'<line x1="{mx:.2f}" y1="{my + BOX_H / 2 * s:.2f}" '
                 f'x2="{dx:.2f}" y2="{dy - BOX_H / 2 * s:.2f}" '
                 f'stroke="{MUTED}" stroke-width="{lw}"/>')

   # The feedback loop, laid out in chart units, then placed
   def unit_center(n):
      x, y = center(n)
      return (x - x0) / s, (y0 - y) / s

   src, dst = alg['fb_src'], alg['fb_dst']
   lane = x0 + feedback_lane(alg, row, unit_center, BOX_W, LOOP) * s
   (sx, sy), (dx, dy) = center(src), center(dst)
   leave = sx + BOX_W / 2 * s if lane > sx else sx - BOX_W / 2 * s
   over = dy - (BOX_H / 2 + LOOP) * s
   out.append(f'<path d="M {leave:.2f} {sy:.2f} H {lane:.2f} V {over:.2f} '
              f'H {dx:.2f} V {dy - BOX_H / 2 * s:.2f}" fill="none" '
              f'stroke="{MUTED}" stroke-width="{lw}"/>')

   low = y0 + BUS * s                               # the bus
   ends = sorted(center(n)[0] for n in carriers)
   for x in ends:
      out.append(f'<line x1="{x:.2f}" y1="{y0:.2f}" x2="{x:.2f}" '
                 f'y2="{low:.2f}" stroke="{LINE}" stroke-width="{lw}"/>')
   out.append(f'<line x1="{ends[0]:.2f}" y1="{low:.2f}" x2="{ends[-1]:.2f}" '
              f'y2="{low:.2f}" stroke="{LINE}" stroke-width="{lw}"/>')

   boxes = []
   for n in range(1, 7):                            # the boxes, numbered
      cx, cy = center(n)
      x, y = cx - BOX_W / 2 * s, cy - BOX_H / 2 * s
      w, h = BOX_W * s, BOX_H * s
      carrier = n in carriers
      out.append(f'<rect x="{x:.2f}" y="{y:.2f}" width="{w:.2f}" '
                 f'height="{h:.2f}" rx="{3 * s:.2f}" '
                 f'fill="{CARRIER if carrier else BOX}" '
                 f'stroke="{LINE if carrier else MUTED}" '
                 f'stroke-width="{lw}"/>')
      out.append(f'<text x="{cx:.2f}" y="{cy:.2f}" font-size="{16.8 * s:.2f}" '
                 f'font-weight="bold" text-anchor="middle" '
                 f'dominant-baseline="central" fill="{LABEL}">{n}</text>')
      boxes.append(((x - left) / CELL_W, (y - top) / CELL_H,
                    (x + w - left) / CELL_W, (y + h - top) / CELL_H))
   return out, boxes


def table(all_boxes):
   lines = [
      '/*' + '=' * 77,
      '   Copyright (c) 2019-2026 Joel de Guzman',
      '',
      '   Distributed under the MIT License '
      '[ https://opensource.org/licenses/MIT ]',
      '=' * 77 + '*/',
      '#if !defined(Q_PLUG_DEXTER_ALGORITHM_BOXES_OCTOBER_1_2026)',
      '#define Q_PLUG_DEXTER_ALGORITHM_BOXES_OCTOBER_1_2026',
      '',
      '/' * 79,
      '// Generated by gen_images.py with resources/algorithms.png: where',
      '// each operator\'s box is in its algorithm\'s cell, left, top, right,',
      '// bottom, in the cell\'s unit square, y downward. Do not edit.',
      '/' * 79,
      'constexpr float algorithm_boxes[32][6][4] =',
      '{',
   ]
   for a, boxes in enumerate(all_boxes):
      lines.append(f'   // {a + 1}')
      lines.append('   {')
      for b in boxes:
         lines.append('      {' + ', '.join(f'{v:.4f}f' for v in b) + '},')
      lines.append('   },')
   lines += ['};', '', '#endif', '']
   return '\n'.join(lines)


def render(svg, out, zoom):
   with tempfile.TemporaryDirectory() as tmp:
      src = os.path.join(tmp, 'image.svg')
      with open(src, 'w') as f:
         f.write(svg)
      subprocess.run(['rsvg-convert', '--zoom', str(zoom), '-o', out, src],
                     check=True)


def keys():
   """MIDI keys 21 to 120, a column each: the black keys short, a line
   between B and C and between E and F."""
   first, count, w, h = 21, 100, 16, 64
   out = [f'<rect width="{count * w}" height="{h}" fill="#e8e8ea"/>']
   for i in range(count):
      key, x = first + i, i * w
      if key % 12 in (1, 3, 6, 8, 10):
         out.append(f'<rect x="{x}" width="{w}" height="{h * 0.6}" '
                    f'fill="#1e1e20"/>')
      elif key % 12 in (0, 5):
         out.append(f'<line x1="{x}" y1="0" x2="{x}" y2="{h}" '
                    f'stroke="#78787e" stroke-width="2"/>')
   return (f'<svg xmlns="http://www.w3.org/2000/svg" width="{count * w}" '
           f'height="{h}">\n' + '\n'.join(out) + '\n</svg>\n')


def waves():
   """The LFO waves in the DX7's order, two cycles each, a row each:
   triangle, saw down, saw up, square, sine, sample and hold."""
   w, h, pad = 50, 12, 1.5

   def at(wave, t):
      f = t - math.floor(t)
      if wave == 0:
         return 1 - 4 * abs(f - 0.5)
      if wave == 1:
         return 1 - 2 * f
      if wave == 2:
         return 2 * f - 1
      if wave == 3:
         return 1 if f < 0.5 else -1
      if wave == 4:
         return math.sin(2 * math.pi * t)
      return (0.6, -0.4, 0.9, -0.8)[int(t * 2) % 4]

   out = []
   for wave in range(6):
      top, pts = wave * h, []
      for i in range(129):
         t = 2 * i / 128
         x = pad + (w - 2 * pad) * i / 128
         y = top + pad + (h - 2 * pad) * (1 - at(wave, t)) / 2
         pts.append(f'{x:.2f},{y:.2f}')
      out.append(f'<polyline points="{" ".join(pts)}" fill="none" '
                 f'stroke="{LABEL}" stroke-width="1.5" '
                 f'stroke-linejoin="round"/>')
   return (f'<svg xmlns="http://www.w3.org/2000/svg" width="{w}" '
           f'height="{6 * h}">\n' + '\n'.join(out) + '\n</svg>\n')


def logo():
   """sin(2 pi (3t) + 2.4 sin(2 pi t)): three cycles of a carrier, its
   phase pushed and pulled by one cycle of a modulator, so the wave
   crowds and spreads as an FM tone does."""
   w, h, pad = 44, 22, 3
   pts = []
   for i in range(257):
      t = i / 256
      y = math.sin(2 * math.pi * 3 * t + 2.4 * math.sin(2 * math.pi * t))
      pts.append(f'{pad + (w - 2 * pad) * t:.2f},'
                 f'{pad + (h - 2 * pad) * (1 - y) / 2:.2f}')
   return (f'<svg xmlns="http://www.w3.org/2000/svg" width="{w}" '
           f'height="{h}">\n<polyline points="{" ".join(pts)}" '
           f'fill="none" stroke="{LINE}" stroke-width="2" '
           f'stroke-linejoin="round" stroke-linecap="round"/>\n</svg>\n')


def main():
   algs = routings()
   body, all_boxes = [], []
   for i, a in enumerate(algs):
      out, boxes = cell(a, (i % COLUMNS) * CELL_W, (i // COLUMNS) * CELL_H)
      body += out
      all_boxes.append(boxes)
      for b in boxes:                               # every box in its cell
         assert 0 <= b[0] < b[2] <= 1 and 0 <= b[1] < b[3] <= 1, (a['n'], b)

   w, h = COLUMNS * CELL_W, ROWS * CELL_H
   svg = (f'<svg xmlns="http://www.w3.org/2000/svg" width="{w}" '
          f'height="{h}" viewBox="0 0 {w} {h}" '
          f'font-family="Open Sans, Helvetica, Arial, sans-serif">\n'
          + '\n'.join(body) + '\n</svg>\n')

   render(svg, PNG, ZOOM)
   with open(TABLE, 'w') as f:
      f.write(table(all_boxes))
   render(keys(), KEYS, 1)
   render(waves(), WAVES, ZOOM)
   render(logo(), LOGO, ZOOM)
   for path in (PNG, TABLE, KEYS, WAVES, LOGO):
      print('wrote', path)


if __name__ == '__main__':
   main()
