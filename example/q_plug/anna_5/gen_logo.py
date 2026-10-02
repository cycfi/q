#!/usr/bin/env python3
"""
Generate Anna's logo for the header, resources/logo.png: a sawtooth, the
wave the synth starts from, in the editor's blue.

Needs rsvg-convert. Usage: python3 example/q_plug/anna_5/gen_logo.py
"""

import os
import subprocess
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
PNG = os.path.join(HERE, 'resources/logo.png')

W, H, PAD, ZOOM = 44, 22, 3, 4          # points, and pixels per point
LINE = '#64b4e6'                        # the editor's line color
CYCLES = 3


def main():
   w, h = W - 2 * PAD, H - 2 * PAD
   pts = [(PAD, PAD + h)]
   for c in range(CYCLES):               # up the ramp, then straight down
      x1 = PAD + w * (c + 1) / CYCLES
      pts += [(x1, PAD), (x1, PAD + h)]
   path = ' '.join(f'{x:.2f},{y:.2f}' for x, y in pts)
   svg = (f'<svg xmlns="http://www.w3.org/2000/svg" width="{W}" '
          f'height="{H}">\n<polyline points="{path}" fill="none" '
          f'stroke="{LINE}" stroke-width="2" stroke-linejoin="round" '
          f'stroke-linecap="round"/>\n</svg>\n')
   with tempfile.TemporaryDirectory() as tmp:
      src = os.path.join(tmp, 'logo.svg')
      with open(src, 'w') as f:
         f.write(svg)
      subprocess.run(['rsvg-convert', '--zoom', str(ZOOM), '-o', PNG, src],
                     check=True)
   print('wrote', PNG)


if __name__ == '__main__':
   main()
