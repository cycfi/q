#!/usr/bin/env python3
"""
Generate the one_shot_phase_iterator reference figure.

Produces, in docs/modules/ROOT/images/:

   one_shot_phase_iterator.svg  -- the phase and the sine read from it,
                                   for phase_iterator and
                                   one_shot_phase_iterator.

The phase is replayed as the C++ holds it, a 32-bit unsigned fixed point
count: phase_iterator adds its step modulo 2^32, one_shot_phase_iterator
saturates at 2^32 - 1. The step is sps / 16, exactly 2^28.

Style and palette follow gen_interpolation_figures.py.

Usage: /usr/bin/python3 docs/scripts/gen_phase_iterator_figures.py
"""

import os
import numpy as np
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt

SITE_ACCENT = '#1565c0'
AMBER = '#ffb300'
GREY = '#b0b0b0'

OUT_DIR = os.path.join(
   os.path.dirname(__file__), '..', 'modules', 'ROOT', 'images')

ONE_CYC = 2**32
STEP = ONE_CYC // 16
LENGTH = 56


def phases(one_shot):
   # The phase the sample reads, post-increment as in sin(ph++)
   p, out = 0, []
   for _ in range(LENGTH):
      out.append(p)
      p = min(p + STEP, ONE_CYC - 1) if one_shot else (p + STEP) % ONE_CYC
   return np.array(out, dtype=float) / ONE_CYC


def style(ax):
   for s in ('top', 'right'):
      ax.spines[s].set_visible(False)
   ax.spines['left'].set_color(GREY)
   ax.spines['bottom'].set_color(GREY)
   ax.grid(True, linestyle='--', color=GREY, alpha=0.5)


def gen():
   n = np.arange(LENGTH)
   wrap = phases(False)
   once = phases(True)

   fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(10, 5.6), sharex=True)

   ax1.plot(n, wrap, color=SITE_ACCENT, linewidth=1.8,
            label='phase_iterator')
   ax1.plot(n, once, color=AMBER, linewidth=2.2, linestyle='--',
            label='one_shot_phase_iterator')
   ax1.set_ylabel('Phase (cycles)')
   ax1.set_ylim(-0.05, 1.45)
   ax1.set_title('The phase: wrapping, and holding at the end',
                 color='#333333', fontsize=11, loc='left')

   ax2.plot(n, np.sin(2 * np.pi * wrap), color=SITE_ACCENT, linewidth=1.8,
            label='sin(phase_iterator)')
   ax2.plot(n, np.sin(2 * np.pi * once), color=AMBER, linewidth=2.2,
            linestyle='--', label='sin(one_shot_phase_iterator)')
   ax2.set_ylabel('Value')
   ax2.set_ylim(-1.1, 1.75)
   ax2.set_xlabel('Time (samples)')
   ax2.set_title('A sine read from each', color='#333333', fontsize=11,
                 loc='left')

   for ax in (ax1, ax2):
      ax.set_xlim(0, LENGTH - 1)
      style(ax)
      ax.legend(loc='upper right', fontsize=9, ncol=2)

   fig.tight_layout()
   path = os.path.join(OUT_DIR, 'one_shot_phase_iterator.svg')
   fig.savefig(path, format='svg', bbox_inches=None)
   plt.close(fig)
   print('wrote', path)


if __name__ == '__main__':
   gen()
