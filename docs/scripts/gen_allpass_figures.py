#!/usr/bin/env python3
"""
Generate the q all-pass reference figure.

Produces, in docs/modules/ROOT/images/:

   allpass.svg  -- two panels. Left: one_pole_allpass phase response for a few
                   pivot frequencies, each sweeping 0 to -180 degrees and
                   crossing -90 degrees exactly at its pivot f (magenta marks).
                   Right: polyphase_allpass phase response (a second-order
                   section in z^-2) for two coefficients. Both filters pass
                   every frequency at unity gain; only the phase changes.

Style and palette follow gen_differentiator_figures.py.

Usage: /usr/bin/python3 docs/scripts/gen_allpass_figures.py
"""

import os
import numpy as np
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt

SITE_ACCENT = '#1565c0'
SKY = '#64b5f6'
CORNFLOWER = '#42a5f5'
AMBER = '#ffb300'
GREEN = '#43a047'
MAGENTA = '#d81b60'

OUT_DIR = os.path.join(
   os.path.dirname(__file__), '..', 'modules', 'ROOT', 'images')

SPS = 44100.0


def one_pole_a(freq):
   # a = tan(pi*f/sps - pi/4): pivots the -90 deg point at freq.
   return np.tan(np.pi * freq / SPS - 0.25 * np.pi)


def one_pole_phase(a, w):
   # H(z) = (a + z^-1) / (1 + a z^-1)
   z1 = np.exp(-1j * w)
   H = (a + z1) / (1 + a * z1)
   return np.degrees(np.unwrap(np.angle(H)))


def polyphase_phase(a, w):
   # H(z) = (a - z^-2) / (1 - a z^-2)
   z2 = np.exp(-2j * w)
   H = (a - z2) / (1 - a * z2)
   return np.degrees(np.unwrap(np.angle(H)))


def gen():
   fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(10, 4.2))

   # -- Left: first-order all-pass, phase pivots at f --
   f = np.linspace(1.0, SPS / 2, 2000)
   w = 2 * np.pi * f / SPS
   pivots = [1000.0, 4000.0, 12000.0]
   colors = [SKY, CORNFLOWER, SITE_ACCENT]
   for fp, c in zip(pivots, colors):
      a = one_pole_a(fp)
      ph = one_pole_phase(a, w)
      ax1.plot(f / 1000, ph, color=c, linewidth=1.8,
               label=f'pivot = {fp/1000:g} kHz')
      ax1.plot([fp / 1000], [-90], 'o', color=MAGENTA, markersize=5,
               zorder=5)
   ax1.axhline(-90, color='#b0b0b0', linewidth=0.9, linestyle=':')
   ax1.set_title('one_pole_allpass: phase pivots at f',
                 color='#333333', fontsize=11)
   ax1.set_xlabel('Frequency (kHz)')
   ax1.set_ylabel('Phase (degrees)')
   ax1.set_xlim(0, SPS / 2 / 1000)
   ax1.set_ylim(-185, 5)
   ax1.set_yticks([0, -45, -90, -135, -180])

   # -- Right: second-order section (in z^-2) --
   f2 = np.linspace(1.0, SPS / 2, 2000)
   w2 = 2 * np.pi * f2 / SPS
   for a, c, lbl in ((0.4794, AMBER, 'a = 0.479'),
                     (0.8762, GREEN, 'a = 0.876')):
      ph = polyphase_phase(a, w2)
      ax2.plot(f2 / 1000, ph, color=c, linewidth=1.8, label=lbl)
   ax2.set_title('polyphase_allpass: second-order section (z$^{-2}$)',
                 color='#333333', fontsize=11)
   ax2.set_xlabel('Frequency (kHz)')
   ax2.set_ylabel('Phase (degrees)')
   ax2.set_xlim(0, SPS / 2 / 1000)

   for ax in (ax1, ax2):
      for s in ('top', 'right'):
         ax.spines[s].set_visible(False)
      ax.spines['left'].set_color('#b0b0b0')
      ax.spines['bottom'].set_color('#b0b0b0')
      ax.grid(True, linestyle='--', color='#b0b0b0', alpha=0.5)
      ax.legend(loc='best', fontsize=9)

   fig.tight_layout()
   path = os.path.join(OUT_DIR, 'allpass.svg')
   fig.savefig(path, format='svg', bbox_inches=None)
   plt.close(fig)
   print('wrote', path)


if __name__ == '__main__':
   gen()
