#!/usr/bin/env python3
"""
Generate the q::level_crossfade reference figure.

Produces, in docs/modules/ROOT/images/:

   level_crossfade.svg  -- the gain applied to signal `a` and signal `b` as a
                          function of the control level `ctrl`, for a pivot of
                          -10 dB. Above the pivot the output is all `a`; below
                          it, `a` fades out and `b` fades in by (ctrl - pivot)
                          decibels.

Style and palette follow gen_interpolation_figures.py.

Usage: /usr/bin/python3 docs/scripts/gen_level_crossfade_figures.py
"""

import os
import numpy as np
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt

SITE_ACCENT = '#1565c0'    # gain of a
AMBER = '#ffb300'          # gain of b
GREY = '#5d5d5d'

OUT_DIR = os.path.join(
   os.path.dirname(__file__), '..', 'modules', 'ROOT', 'images')


def gen():
   pivot = -10.0
   ctrl = np.linspace(-30.0, 0.0, 400)

   # gain of a: 1 above pivot, 10^((ctrl - pivot)/20) below it
   xfade = np.power(10.0, (ctrl - pivot) / 20.0)
   gain_a = np.where(ctrl < pivot, xfade, 1.0)
   gain_b = np.where(ctrl < pivot, 1.0 - xfade, 0.0)

   fig, ax = plt.subplots(figsize=(9, 5))

   ax.axvline(pivot, color=GREY, lw=1.0, ls='--', zorder=1)
   ax.text(pivot - 0.4, 1.02, 'pivot (-10 dB)', ha='right', va='bottom',
           color=GREY, fontsize=9)

   ax.plot(ctrl, gain_a, color=SITE_ACCENT, linewidth=2.0,
           label='gain of a', zorder=3)
   ax.plot(ctrl, gain_b, color=AMBER, linewidth=2.0,
           label='gain of b', zorder=3)

   # worked example from the header: ctrl = -13 dB -> gain_a = 0.708
   ax.plot([-13.0], [np.power(10.0, (-13.0 - pivot) / 20.0)], 'o',
           color=SITE_ACCENT, markersize=6, markeredgecolor='white',
           markeredgewidth=0.8, zorder=4)

   ax.set_xlabel('Control level ctrl (dB)')
   ax.set_ylabel('Gain')
   ax.set_xlim(-30, 0)
   ax.set_ylim(-0.03, 1.08)
   for sp in ('top', 'right'):
      ax.spines[sp].set_visible(False)
   ax.spines['left'].set_color('#b0b0b0')
   ax.spines['bottom'].set_color('#b0b0b0')
   ax.grid(True, linestyle='--', color='#b0b0b0', alpha=0.5)
   ax.legend(loc='center left', fontsize=10)

   fig.tight_layout()
   path = os.path.join(OUT_DIR, 'level_crossfade.svg')
   fig.savefig(path, format='svg', bbox_inches=None)
   plt.close(fig)
   print('wrote', path)


if __name__ == '__main__':
   gen()
