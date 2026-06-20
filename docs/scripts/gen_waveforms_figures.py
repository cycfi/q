#!/usr/bin/env python3
"""
Generate the Waveforms tutorial figure.

Produces, in docs/modules/ROOT/images/:

   waveform_shapes.svg

The four oscillator shapes Q provides band-limited (sawtooth, square, pulse at
30% duty, triangle), drawn over two cycles. The ideal shapes are shown for
clarity; in Q each transition is corrected with poly_blep / poly_blamp so the
harmonic series never aliases (see the Square Synth tutorial's spectrum figure).

Style matches gen_interpolation_figures.py (blue palette, dashed grid).

Usage: /usr/bin/python3 docs/scripts/gen_waveforms_figures.py
"""

import os
import numpy as np
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt

CURVE_COLOR = '#1565c0'   # site_accent, matches the docs UI accent
TEXT_COLOR = '#1a1a1a'
GRID_COLOR = '#b0b0b0'

OUT_DIR = os.path.join(
   os.path.dirname(__file__), '..', 'modules', 'ROOT', 'images')

PULSE_WIDTH = 0.3   # duty cycle, matches the example


def shapes():
   cycles = 2
   t = np.linspace(0, cycles, 2000, endpoint=False)
   ph = t % 1.0                                   # phase, 0..1 each cycle

   saw = 2.0 * ph - 1.0
   square = np.where(ph < 0.5, 1.0, -1.0)
   pulse = np.where(ph < PULSE_WIDTH, 1.0, -1.0)
   # Triangle: -1 at the cycle start, +1 at the half cycle.
   triangle = 2.0 * np.abs(2.0 * (ph - np.floor(ph + 0.5))) - 1.0

   return t, [
      ('Sawtooth', saw),
      ('Square', square),
      ('Pulse (30%)', pulse),
      ('Triangle', triangle),
   ]


def main():
   t, waves = shapes()
   fig, axs = plt.subplots(2, 2, figsize=(10, 6))
   for ax, (name, y) in zip(axs.flat, waves):
      ax.plot(t, y, color=CURVE_COLOR, linewidth=1.75)
      ax.set_title(name, color=TEXT_COLOR)
      ax.set_ylim(-1.25, 1.25)
      ax.set_xlim(0, t[-1])
      ax.set_yticks([-1, 0, 1])
      ax.set_xlabel('Cycles', color=TEXT_COLOR)
      ax.grid(True, linestyle='--', linewidth=0.5, color=GRID_COLOR, alpha=0.8)

   fig.tight_layout()
   path = os.path.join(OUT_DIR, 'waveform_shapes.svg')
   fig.savefig(path, format='svg', bbox_inches=None)
   plt.close(fig)
   print('wrote', path)


if __name__ == '__main__':
   main()
