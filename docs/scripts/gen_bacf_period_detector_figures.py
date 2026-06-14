#!/usr/bin/env python3
"""
Generate the q::bacf_period_detector reference figure.

Produces, in docs/modules/ROOT/images/:

   bacf_missing_fundamental.svg -- the detector's robustness: a signal built
                                  from the 2nd, 3rd and 4th harmonics only (no
                                  energy at the fundamental) still repeats at
                                  the fundamental period, and the bitstream
                                  correlation's deepest notch lands there. Top:
                                  the waveform (note the sub-structure within a
                                  period). Bottom: the XOR mismatch count vs
                                  lag, deepest at the true fundamental period.

This runs the real algorithm (sign bitstream, XOR + popcount), so the notch
position is computed, not drawn.

Style and palette follow gen_interpolation_figures.py (the canonical PALETTE
lives there).

Usage: /usr/bin/python3 docs/scripts/gen_bacf_period_detector_figures.py
"""

import os
import numpy as np
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt

SKY = '#64b5f6'
SITE_ACCENT = '#1565c0'
MAGENTA = '#d81b60'        # event marker: the detected fundamental
GREY = '#5d5d5d'
INK = '#1a1a1a'

OUT_DIR = os.path.join(
   os.path.dirname(__file__), '..', 'modules', 'ROOT', 'images')


def missing_fundamental(period, n):
   """2nd/3rd/4th harmonics, no fundamental. Still repeats at `period`."""
   t = np.arange(n)
   w = 2 * np.pi / period
   return (0.5 * np.sin(2 * w * t)
           + 0.4 * np.sin(3 * w * t)
           + 0.3 * np.sin(4 * w * t))


def bacf_count(bits, lag, window):
   a = bits[:window]
   b = bits[lag:lag + window]
   return int(np.count_nonzero(a ^ b))


def gen_missing():
   period = 80
   window = 2 * period
   max_lag = int(2.4 * period)
   n = max_lag + window + period
   x = missing_fundamental(period, n)
   bits = (x >= 0).astype(np.uint8)

   lags = np.arange(1, max_lag)
   counts = np.array([bacf_count(bits, int(L), window) for L in lags])

   fig, (ax_w, ax_c) = plt.subplots(
      2, 1, figsize=(10, 6.6),
      gridspec_kw=dict(height_ratios=[1.5, 2.0], hspace=0.40))

   show = 2 * period + 1
   ax_w.plot(np.arange(show), x[:show], color=SITE_ACCENT, linewidth=1.8)
   ax_w.axhline(0, color=GREY, lw=0.8, ls=':')
   # mark the true period span
   ax_w.annotate('', xy=(period, x.max() * 1.18), xytext=(0, x.max() * 1.18),
                 arrowprops=dict(arrowstyle='<|-|>', color=MAGENTA, lw=1.4))
   ax_w.text(period / 2, x.max() * 1.24, 'one fundamental period',
             ha='center', va='bottom', color=MAGENTA, fontsize=10)
   ax_w.set_title('A signal with no fundamental (2nd, 3rd, 4th harmonics only)',
                  fontsize=11, color=INK, loc='left')
   ax_w.set_yticks([])
   ax_w.set_xlim(0, show - 1)
   ax_w.set_ylim(x.min() * 1.1, x.max() * 1.5)
   for s in ('top', 'right', 'left'):
      ax_w.spines[s].set_visible(False)
   ax_w.spines['bottom'].set_color('#b0b0b0')

   ax_c.plot(lags, counts, color=SITE_ACCENT, linewidth=1.8, zorder=2)
   ax_c.axvline(period, color=MAGENTA, lw=1.4, ls='--', zorder=3)
   ax_c.plot([period], [counts[period - 1]], 'o', color=MAGENTA,
             markersize=11, zorder=4)
   ax_c.annotate('fundamental period found here,\nnot at a harmonic',
                 xy=(period, counts[period - 1]),
                 xytext=(period + 6, counts.max() * 0.5),
                 fontsize=10, color=MAGENTA, va='center',
                 arrowprops=dict(arrowstyle='-', color=MAGENTA, lw=0.9))
   ax_c.set_title('XOR mismatch count vs lag: deepest notch at the true period',
                  fontsize=11, color=INK, loc='left')
   ax_c.set_xlabel('Lag (samples)')
   ax_c.set_ylabel('Mismatch count')
   ax_c.set_xlim(0, max_lag - 1)
   ax_c.set_ylim(-2, counts.max() * 1.08)
   ax_c.grid(True, linestyle='--', linewidth=0.5, color='#b0b0b0', alpha=0.8)
   for s in ('top', 'right'):
      ax_c.spines[s].set_visible(False)

   path = os.path.join(OUT_DIR, 'bacf_missing_fundamental.svg')
   fig.savefig(path, format='svg', bbox_inches='tight')
   plt.close(fig)
   print('wrote', path)


if __name__ == '__main__':
   gen_missing()
