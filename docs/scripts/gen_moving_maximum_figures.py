#!/usr/bin/env python3
"""
Generate the q::moving_maximum reference figure.

Produces, in docs/modules/ROOT/images/:

   moving_maximum.svg  -- a bursty rectified signal and its sliding maximum,
                          the staircase that holds each peak for the window
                          width then steps down.

Style and palette follow gen_interpolation_figures.py.

Usage: /usr/bin/python3 docs/scripts/gen_moving_maximum_figures.py
"""

import os
import numpy as np
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt

SITE_ACCENT = '#1565c0'    # the input signal
AMBER = '#ffb300'          # the running maximum
GREY = '#5d5d5d'

OUT_DIR = os.path.join(
   os.path.dirname(__file__), '..', 'modules', 'ROOT', 'images')


def gen():
   w, n = 35, 260
   t = np.arange(n)
   rng = np.random.default_rng(3)

   # A few decaying plucks at different amplitudes, rectified, plus a little
   # noise: peaks the running maximum can hold and then release.
   x = np.zeros(n)
   for t0, amp, tau in ((15, 0.85, 16), (80, 0.6, 20),
                        (140, 1.0, 14), (200, 0.72, 22)):
      m = t >= t0
      x[m] += amp * np.exp(-(t[m] - t0) / tau)
   x = np.abs(x + 0.03 * rng.standard_normal(n))

   mm = np.array([x[max(0, i - w + 1):i + 1].max() for i in range(n)])

   fig, ax = plt.subplots(figsize=(10, 4.5))
   ax.plot(t, x, color=SITE_ACCENT, linewidth=1.2, label='input |s|', zorder=2)
   ax.plot(t, mm, color=AMBER, linewidth=2.0,
           label=f'moving_maximum (window = {w})', zorder=3)

   # window-width indicator under the tallest peak's plateau
   i_peak = int(np.argmax(x))
   ytop = mm.max()
   y_ind = -0.08 * ytop
   ax.annotate('', xy=(i_peak + w, y_ind), xytext=(i_peak, y_ind),
               arrowprops=dict(arrowstyle='<|-|>', color=GREY, lw=1.2))
   ax.text(i_peak + w / 2, y_ind - 0.02 * ytop, 'window', ha='center',
           va='top', color=GREY, fontsize=9)

   ax.set_xlabel('Time (samples)')
   ax.set_yticks([])
   ax.set_xlim(0, n - 1)
   ax.set_ylim(-0.16 * ytop, ytop * 1.08)
   for s in ('top', 'right', 'left'):
      ax.spines[s].set_visible(False)
   ax.spines['bottom'].set_color('#b0b0b0')
   ax.legend(loc='upper right')

   path = os.path.join(OUT_DIR, 'moving_maximum.svg')
   fig.savefig(path, format='svg', bbox_inches=None)
   plt.close(fig)
   print('wrote', path)


if __name__ == '__main__':
   gen()
