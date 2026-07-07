#!/usr/bin/env python3
"""
Generate the q::integrator reference figure.

Produces, in docs/modules/ROOT/images/:

   integrator.svg  -- two panels showing the running accumulator: a constant
                      input integrating to a linear ramp, and a square wave
                      integrating to a triangle wave.

Style and palette follow gen_interpolation_figures.py.

Usage: /usr/bin/python3 docs/scripts/gen_integrator_figures.py
"""

import os
import numpy as np
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt

SITE_ACCENT = '#1565c0'    # the input signal
AMBER = '#ffb300'          # the integrated output
GREY = '#5d5d5d'

OUT_DIR = os.path.join(
   os.path.dirname(__file__), '..', 'modules', 'ROOT', 'images')


def integrate(x, gain):
   y = np.empty_like(x)
   acc = 0.0
   for i, s in enumerate(x):
      acc += gain * s
      y[i] = acc
   return y


def gen():
   n = 240
   t = np.arange(n)
   gain = 0.1

   # Constant input -> linear ramp
   const = np.ones(n)
   ramp = integrate(const, gain)

   # Zero-mean square wave -> triangle wave
   period = 60
   square = np.where((t % period) < (period / 2), 1.0, -1.0)
   triangle = integrate(square, gain)

   fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(10, 4.2))

   ax1.plot(t, const, color=SITE_ACCENT, linewidth=1.6,
            label='input (constant)', zorder=2)
   ax1.plot(t, ramp, color=AMBER, linewidth=2.0,
            label='integrator output', zorder=3)
   ax1.set_title('Constant in, ramp out', color='#333333', fontsize=11)

   ax2.plot(t, square, color=SITE_ACCENT, linewidth=1.6,
            label='input (square)', zorder=2)
   ax2.plot(t, triangle, color=AMBER, linewidth=2.0,
            label='integrator output', zorder=3)
   ax2.set_title('Square in, triangle out', color='#333333', fontsize=11)

   for ax in (ax1, ax2):
      ax.set_xlabel('Time (samples)')
      ax.set_xlim(0, n - 1)
      ax.axhline(0, color='#b0b0b0', linewidth=0.8, zorder=1)
      for s in ('top', 'right'):
         ax.spines[s].set_visible(False)
      ax.spines['left'].set_color('#b0b0b0')
      ax.spines['bottom'].set_color('#b0b0b0')
      ax.grid(True, linestyle='--', color='#b0b0b0', alpha=0.5)
      ax.legend(loc='upper left', fontsize=9)

   fig.tight_layout()
   path = os.path.join(OUT_DIR, 'integrator.svg')
   fig.savefig(path, format='svg', bbox_inches=None)
   plt.close(fig)
   print('wrote', path)


if __name__ == '__main__':
   gen()
