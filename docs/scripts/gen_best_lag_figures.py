#!/usr/bin/env python3
"""
Generate the q::best_lag reference figures.

Produces, in docs/modules/ROOT/images/:

   best_lag_selfsim.svg  -- a complex repeating waveform with the newest
                            comparison window and the matching window one lag
                            (= period) back, and the lag marked between them.
   best_lag_ncc.svg      -- a zoomed view of the NCC peak (period 100.25):
                            integer-lag scores, the parabola fit, and the
                            refined peak a quarter sample right of the integer
                            best.

Style and palette follow gen_interpolation_figures.py (the canonical
PALETTE lives there).

Usage: python3 docs/scripts/gen_best_lag_figures.py
"""

import os
import numpy as np
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt

SKY = '#64b5f6'            # integer-lag scores / window shading
SITE_ACCENT = '#1565c0'    # waveform / parabola fit
MAGENTA = '#d81b60'        # event marker: refined peak / the lag
GREY = '#5d5d5d'
INK = '#1a1a1a'

OUT_DIR = os.path.join(
   os.path.dirname(__file__), '..', 'modules', 'ROOT', 'images')


def ncc(x, window, lag):
   a = x[-window:]
   b = x[-window-lag:-lag]
   den = np.sqrt(np.dot(a, a) * np.dot(b, b))
   return float(np.dot(a, b) / den) if den > 0 else 0.0


def gen_selfsim():
   """A repeating waveform: the newest window matches the window one lag back."""
   P = 100
   n = np.arange(300)
   w = 2 * np.pi / P
   x = np.sin(w*n) + 0.6*np.sin(2*w*n + 0.8) + 0.35*np.sin(3*w*n + 2.0)

   wlen, end = 70, 285
   nw0, nw1 = end - wlen, end       # newest window
   mw0, mw1 = nw0 - P, nw1 - P      # same window, one lag (= period) back

   fig, ax = plt.subplots(figsize=(10, 4.5))
   ax.plot(n, x, color=SITE_ACCENT, linewidth=1.6, zorder=2)
   for a, b in ((mw0, mw1), (nw0, nw1)):
      ax.axvspan(a, b, color=SKY, alpha=0.22, zorder=1)
      ax.axvline(a, color=GREY, lw=1.0, ls=':', zorder=1)
      ax.axvline(b, color=GREY, lw=1.0, ls=':', zorder=1)

   lo, hi = x.min(), x.max()
   ytop = hi + 0.5
   ax.annotate('', xy=(nw0, ytop), xytext=(mw0, ytop),
               arrowprops=dict(arrowstyle='<|-|>', color=MAGENTA, lw=1.6))
   ax.text((mw0 + nw0) / 2, ytop + 0.06, 'lag = period', ha='center',
           va='bottom', color=MAGENTA, fontsize=12)
   ax.text((mw0 + mw1) / 2, lo - 0.35, 'window one lag back', ha='center',
           va='top', fontsize=10, color=INK)
   ax.text((nw0 + nw1) / 2, lo - 0.35, 'newest window', ha='center',
           va='top', fontsize=10, color=INK)

   ax.set_xlabel('Time (samples), newest at right')
   ax.set_yticks([])
   ax.set_xlim(0, 299)
   ax.set_ylim(lo - 1.1, ytop + 0.6)
   for s in ('top', 'right', 'left'):
      ax.spines[s].set_visible(False)
   ax.spines['bottom'].set_color('#b0b0b0')
   path = os.path.join(OUT_DIR, 'best_lag_selfsim.svg')
   fig.savefig(path, format='svg', bbox_inches=None)
   plt.close(fig)
   print('wrote', path)


def gen_ncc():
   """Zoomed NCC peak with parabolic sub-sample refinement."""
   period = 100.25
   n = np.arange(4096)
   x = np.sin(2 * np.pi * n / period) \
      + 0.4 * np.sin(4 * np.pi * n / period + 0.7)

   window = 512
   lags = np.arange(80, 131)
   scores = np.array([ncc(x, window, int(L)) for L in lags])

   i = int(np.argmax(scores))
   L0 = int(lags[i])
   c0, c1, c2 = scores[i-1], scores[i], scores[i+1]
   d = c0 - 2*c1 + c2
   off = 0.5 * (c0 - c2) / d if d < 0 else 0.0
   refined = L0 + off
   peak = c1 - 0.125 * (c0 - c2)**2 / d

   fig, ax = plt.subplots(figsize=(9, 5.5))
   ax.set_xlabel('Lag (samples)')
   ax.set_ylabel('Normalized correlation')
   ax.grid(True, linestyle='--', linewidth=0.5, color='#b0b0b0', alpha=0.8)

   u = np.linspace(-3, 3, 300)
   par = c1 + 0.5 * (c2 - c0) * u + 0.5 * d * u**2
   ax.plot(L0 + u, par, color=SITE_ACCENT, linewidth=1.9,
           label='parabola fit', zorder=2)

   m = (lags >= L0 - 3) & (lags <= L0 + 3)
   ax.plot(lags[m], scores[m], 'o', color=SKY, markersize=10,
           label='NCC at integer lags', zorder=3)

   ymin = float(scores[m].min())
   span = peak - ymin
   ax.plot([L0, L0], [ymin - span, peak], color=GREY, lw=1.0, ls=':', zorder=1)
   ax.axvline(refined, color=MAGENTA, linewidth=1.3, linestyle='--', zorder=1)
   ax.plot([refined], [peak], 'o', color=MAGENTA, markersize=13,
           label=f'refined peak ({refined:.2f})', zorder=5)

   y_arrow = ymin + 0.10 * span
   ax.annotate('', xy=(refined, y_arrow), xytext=(L0, y_arrow),
               arrowprops=dict(arrowstyle='<|-|>', color=GREY, lw=1.3,
                               shrinkA=0, shrinkB=0))
   ax.text((L0 + refined) / 2, y_arrow + 0.03 * span, f'+{off:.2f} sample',
           ha='center', va='bottom', fontsize=11, color=GREY)
   ax.annotate(f'integer best ({L0})', xy=(L0, c1),
               xytext=(L0 - 2.3, peak + 0.16 * span),
               ha='center', va='bottom', fontsize=10, color=INK,
               arrowprops=dict(arrowstyle='-', color=GREY, lw=0.8))

   ax.set_xlim(L0 - 3.5, L0 + 3.6)
   ax.set_ylim(ymin - 0.18 * span, peak + 0.42 * span)
   ax.set_xticks(range(L0 - 3, L0 + 4))
   ax.legend(loc='upper right')
   path = os.path.join(OUT_DIR, 'best_lag_ncc.svg')
   fig.savefig(path, format='svg', bbox_inches=None)
   plt.close(fig)
   print('wrote', path)


if __name__ == '__main__':
   gen_selfsim()
   gen_ncc()
