#!/usr/bin/env python3
"""
Generate the Pitch Detection overview figure(s).

Produces, in docs/modules/ROOT/images/:

   pitch_zero_crossings.svg -- why zero crossings alone are not enough. Top: a
                              pure tone crosses zero exactly twice per cycle, so
                              the spacing gives the period directly. Bottom: a
                              harmonically rich tone of the same period crosses
                              zero many times per cycle, so raw crossing spacing
                              no longer reveals the period.

Style and palette follow gen_interpolation_figures.py (the canonical PALETTE
lives there).

Usage: /usr/bin/python3 docs/scripts/gen_pitch_overview_figures.py
"""

import os
import numpy as np
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt

SITE_ACCENT = '#1565c0'    # waveform
INK = '#1a1a1a'            # crossing marks
GREY = '#5d5d5d'

OUT_DIR = os.path.join(
   os.path.dirname(__file__), '..', 'modules', 'ROOT', 'images')


def crossings(x):
   """Indices i where the sign changes between i and i+1 (sub-sample x)."""
   out = []
   for i in range(len(x) - 1):
      if (x[i] < 0 <= x[i + 1]) or (x[i] >= 0 > x[i + 1]):
         out.append(i + (-x[i]) / (x[i + 1] - x[i]))
   return np.array(out)


def panel(ax, x, title, period):
   n = np.arange(len(x))
   ax.plot(n, x, color=SITE_ACCENT, linewidth=1.8, zorder=2)
   ax.axhline(0, color=GREY, lw=0.9, ls=':', zorder=1)
   xc = crossings(x)
   ax.plot(xc, np.zeros_like(xc), 'o', color=INK, markersize=6, zorder=3)
   # bracket one period
   ytop = x.max() * 1.30
   ax.annotate('', xy=(period, ytop), xytext=(0, ytop),
               arrowprops=dict(arrowstyle='<|-|>', color=GREY, lw=1.2))
   in_period = int(np.sum(xc < period))
   ax.text(period / 2, ytop + 0.04 * (x.max() - x.min()),
           f'one period: {in_period} crossings',
           ha='center', va='bottom', color=GREY, fontsize=10)
   ax.set_title(title, fontsize=11, color=INK, loc='left')
   ax.set_yticks([])
   ax.set_xlim(0, len(x) - 1)
   ax.set_ylim(x.min() * 1.2, x.max() * 1.65)
   for s in ('top', 'right', 'left'):
      ax.spines[s].set_visible(False)
   ax.spines['bottom'].set_color('#b0b0b0')


def gen():
   P = 80
   n = np.arange(0, 2 * P + 1)
   w = 2 * np.pi / P
   off = 10                                  # phase offset so n=0 is not on a zero
   pure = np.sin(w * (n + off))
   rich = (0.3 * np.sin(w * (n + off))
           + 0.4 * np.sin(2 * w * (n + off))
           + 0.3 * np.sin(3 * w * (n + off)))

   fig, (a, b) = plt.subplots(2, 1, figsize=(10, 6.0),
                              gridspec_kw=dict(hspace=0.45))
   panel(a, pure, 'A pure tone: two crossings per cycle, spacing gives the period', P)
   panel(b, rich, 'The same period with strong harmonics: many crossings per cycle', P)
   a.set_xlabel('')
   b.set_xlabel('Time (samples)')

   path = os.path.join(OUT_DIR, 'pitch_zero_crossings.svg')
   fig.savefig(path, format='svg', bbox_inches='tight')
   plt.close(fig)
   print('wrote', path)


MAGENTA = '#d81b60'


def bacf_count(bits, lag, window):
   a = bits[:window]
   b = bits[lag:lag + window]
   return int(np.count_nonzero(a ^ b))


def gen_noise():
   """BACF survives additive noise: the notch still lands on the period."""
   np.random.seed(7)                         # reproducible
   P = 80
   window = 2 * P
   max_lag = int(2.4 * P)
   n = max_lag + window + P
   t = np.arange(n)
   w = 2 * np.pi / P
   clean = (0.3 * np.sin(w * t) + 0.4 * np.sin(2 * w * t) + 0.3 * np.sin(3 * w * t))
   noisy = clean + np.random.normal(0.0, 0.25, size=n)
   bits = (noisy >= 0).astype(np.uint8)

   lags = np.arange(1, max_lag)
   counts = np.array([bacf_count(bits, int(L), window) for L in lags])

   fig, (ax_w, ax_c) = plt.subplots(
      2, 1, figsize=(10, 6.4),
      gridspec_kw=dict(height_ratios=[1.4, 2.0], hspace=0.42))

   show = 2 * P + 1
   ax_w.plot(np.arange(show), noisy[:show], color=SITE_ACCENT, linewidth=1.3)
   ax_w.axhline(0, color=GREY, lw=0.8, ls=':')
   ax_w.set_title('The harmonic signal with additive noise', fontsize=11,
                  color=INK, loc='left')
   ax_w.set_yticks([])
   ax_w.set_xlim(0, show - 1)
   for s in ('top', 'right', 'left'):
      ax_w.spines[s].set_visible(False)
   ax_w.spines['bottom'].set_color('#b0b0b0')

   ax_c.plot(lags, counts, color=SITE_ACCENT, linewidth=1.8, zorder=2)
   ax_c.axvline(P, color=MAGENTA, lw=1.4, ls='--', zorder=3)
   ax_c.plot([P], [counts[P - 1]], 'o', color=MAGENTA, markersize=11, zorder=4)
   ax_c.annotate('the notch still lands\non the true period',
                 xy=(P, counts[P - 1]),
                 xytext=(P + 6, counts.max() * 0.5),
                 fontsize=10, color=MAGENTA, va='center',
                 arrowprops=dict(arrowstyle='-', color=MAGENTA, lw=0.9))
   ax_c.set_title('XOR mismatch count vs lag: noise lifts the floor but the notch survives',
                  fontsize=11, color=INK, loc='left')
   ax_c.set_xlabel('Lag (samples)')
   ax_c.set_ylabel('Mismatch count')
   ax_c.set_xlim(0, max_lag - 1)
   ax_c.set_ylim(-2, counts.max() * 1.08)
   ax_c.grid(True, linestyle='--', linewidth=0.5, color='#b0b0b0', alpha=0.8)
   for s in ('top', 'right'):
      ax_c.spines[s].set_visible(False)

   path = os.path.join(OUT_DIR, 'pitch_noise_robust.svg')
   fig.savefig(path, format='svg', bbox_inches='tight')
   plt.close(fig)
   print('wrote', path)


if __name__ == '__main__':
   gen()
   gen_noise()
