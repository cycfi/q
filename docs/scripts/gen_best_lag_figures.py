#!/usr/bin/env python3
"""
Generate the q::best_lag reference figure.

Produces, in docs/modules/ROOT/images/:

   best_lag_ncc.svg  -- normalized cross-correlation vs lag for a periodic
                        signal (period 100.25): integer-lag samples, the
                        correlation curve, and the parabolic sub-sample
                        peak refinement

Style and palette follow gen_interpolation_figures.py (the canonical
PALETTE lives there).

Usage: python3 docs/scripts/gen_best_lag_figures.py
"""

import os
import numpy as np
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt

SKY = '#64b5f6'            # integer-lag scores
SITE_ACCENT = '#1565c0'    # correlation curve
MAGENTA = '#d81b60'        # event marker: the refined peak

OUT_DIR = os.path.join(
   os.path.dirname(__file__), '..', 'modules', 'ROOT', 'images')


def ncc(x, window, lag):
   a = x[-window:]
   b = x[-window-lag:-lag]
   den = np.sqrt(np.dot(a, a) * np.dot(b, b))
   return float(np.dot(a, b) / den) if den > 0 else 0.0


if __name__ == '__main__':
   period = 100.25
   n = np.arange(4096)
   x = np.sin(2 * np.pi * n / period) \
      + 0.4 * np.sin(4 * np.pi * n / period + 0.7)

   window = 512
   lags = np.arange(80, 131)
   scores = np.array([ncc(x, window, int(L)) for L in lags])

   # Parabolic refinement around the integer peak (as in best_lag)
   i = int(np.argmax(scores))
   c0, c1, c2 = scores[i-1], scores[i], scores[i+1]
   d = c0 - 2*c1 + c2
   refined = lags[i] + (0.5 * (c0 - c2) / d if d < 0 else 0.0)
   peak = c1 - 0.125 * (c0 - c2)**2 / d

   fig, ax = plt.subplots(figsize=(10, 6))
   ax.set_xlabel('Lag (samples)')
   ax.set_ylabel('Normalized correlation')
   ax.grid(True, linestyle='--', linewidth=0.5, color='#b0b0b0', alpha=0.8)

   ax.plot(lags, scores, color=SITE_ACCENT, linewidth=1.75,
           label='NCC at integer lags')
   ax.plot(lags, scores, 'o', color=SKY, markersize=6)
   ax.axvline(refined, color=MAGENTA, linewidth=1.2, linestyle='--')
   ax.plot([refined], [peak], 'o', color=MAGENTA, markersize=8,
           label=f'Refined peak: lag {refined:.2f}')
   ax.legend(loc='best')

   path = os.path.join(OUT_DIR, 'best_lag_ncc.svg')
   fig.savefig(path, format='svg', bbox_inches=None)
   print('wrote', path)
