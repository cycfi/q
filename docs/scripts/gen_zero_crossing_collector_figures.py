#!/usr/bin/env python3
"""
Generate the q::zero_crossing_collector reference figure.

Produces, in docs/modules/ROOT/images/:

   zero_crossing_event.svg -- a zoom on one positive pulse: the waveform, the
                             integer samples, the leading (rising) and trailing
                             (falling) crossings, and the sub-sample crossing
                             interpolated between the two samples P1 (last
                             negative) and P2 (first positive) that straddle it.

Style and palette follow gen_interpolation_figures.py (the canonical PALETTE
lives there).

Usage: /usr/bin/python3 docs/scripts/gen_zero_crossing_collector_figures.py
"""

import os
import numpy as np
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt

SKY = '#64b5f6'            # sample stems / dots
SITE_ACCENT = '#1565c0'    # waveform
MAGENTA = '#d81b60'        # event marker: the interpolated crossings
GREY = '#5d5d5d'
INK = '#1a1a1a'

OUT_DIR = os.path.join(
   os.path.dirname(__file__), '..', 'modules', 'ROOT', 'images')


def cross(x, i):
   """Sub-sample position of the crossing between samples i and i+1."""
   return i + (-x[i]) / (x[i + 1] - x[i])


def gen_event():
   P = 40
   n = np.arange(0, 96)
   # phase offset so the rising crossing lands between samples (for interpolation)
   ph = 0.6
   x = np.sin(2 * np.pi * (n + ph) / P) + 0.3 * np.sin(4 * np.pi * (n + ph) / P)

   rising = next(i for i in range(len(x) - 1) if x[i] < 0 <= x[i + 1])
   falling = next(i for i in range(rising + 1, len(x) - 1) if x[i] >= 0 > x[i + 1])
   xr, xf = cross(x, rising), cross(x, falling)
   peak_i = rising + 1 + int(np.argmax(x[rising + 1:falling + 1]))

   fig, ax = plt.subplots(figsize=(10, 5.2))

   # smooth curve for context
   nf = np.linspace(n[0], n[-1], 1200)
   xs = np.sin(2 * np.pi * (nf + ph) / P) + 0.3 * np.sin(4 * np.pi * (nf + ph) / P)
   ax.plot(nf, xs, color=SITE_ACCENT, linewidth=1.8, zorder=2)
   ax.axhline(0, color=GREY, lw=1.0, ls=':', zorder=1)

   # integer samples as stems
   ax.vlines(n, 0, x, color=SKY, lw=1.2, alpha=0.7, zorder=2)
   ax.plot(n, x, 'o', color=SKY, markersize=5, zorder=3)

   # the pulse span (leading -> trailing crossing)
   ax.axvspan(xr, xf, color=SKY, alpha=0.12, zorder=0)

   # P1 / P2 straddling the rising crossing
   for idx, lbl, dy in ((rising, 'P1', -0.16), (rising + 1, 'P2', 0.12)):
      ax.plot([n[idx]], [x[idx]], 'o', color=SITE_ACCENT, markersize=9, zorder=5)
      ax.annotate(lbl, xy=(n[idx], x[idx]),
                  xytext=(n[idx] - 1.4, x[idx] + dy),
                  color=INK, fontsize=11, ha='right')
   ax.plot([n[rising], n[rising + 1]], [x[rising], x[rising + 1]],
           color=MAGENTA, lw=1.0, ls='--', zorder=4)

   # interpolated crossings (event markers)
   for xc in (xr, xf):
      ax.plot([xc], [0], 'o', color=MAGENTA, markersize=10, zorder=6)
   ax.annotate('sub-sample crossing\n(interpolated between P1, P2)',
               xy=(xr, 0), xytext=(xr - 1.0, -0.95),
               color=MAGENTA, fontsize=10, ha='center', va='top',
               arrowprops=dict(arrowstyle='-', color=MAGENTA, lw=0.9))
   ax.annotate('trailing crossing', xy=(xf, 0), xytext=(xf + 1.0, -0.7),
               color=GREY, fontsize=10, ha='left', va='top',
               arrowprops=dict(arrowstyle='-', color=GREY, lw=0.8))

   # peak height of the pulse
   ax.annotate('', xy=(n[peak_i], x[peak_i]), xytext=(n[peak_i], 0),
               arrowprops=dict(arrowstyle='<|-|>', color=GREY, lw=1.2))
   ax.text(n[peak_i] + 0.6, x[peak_i] * 0.5, 'peak\nheight',
           color=GREY, fontsize=9, va='center')

   # pulse-width bracket
   ytop = x[peak_i] + 0.22
   ax.annotate('', xy=(xr, ytop), xytext=(xf, ytop),
               arrowprops=dict(arrowstyle='<|-|>', color=INK, lw=1.2))
   ax.text((xr + xf) / 2, ytop + 0.04, 'pulse width', color=INK,
           fontsize=10, ha='center', va='bottom')

   ax.set_xlabel('Time (samples)')
   ax.set_yticks([])
   ax.set_xlim(rising - 3, falling + 4)
   ax.set_ylim(-1.25, ytop + 0.4)
   for s in ('top', 'right', 'left'):
      ax.spines[s].set_visible(False)
   ax.spines['bottom'].set_color('#b0b0b0')

   path = os.path.join(OUT_DIR, 'zero_crossing_event.svg')
   fig.savefig(path, format='svg', bbox_inches='tight')
   plt.close(fig)
   print('wrote', path)


if __name__ == '__main__':
   gen_event()
