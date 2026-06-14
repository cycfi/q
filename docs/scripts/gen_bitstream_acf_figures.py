#!/usr/bin/env python3
"""
Generate the q::bitstream_acf reference figures.

Produces, in docs/modules/ROOT/images/:

   bitstream_acf_count.svg -- the signature three-panel view of the algorithm:
                              top, a harmonically rich periodic signal (1st,
                              2nd, 3rd partials at 0.3/0.4/0.3); middle, its
                              one-bit-per-sample bitstream (set while the signal
                              is positive); bottom, the XOR mismatch count vs
                              lag, which dips to its deepest notch at the true
                              period and to shallower notches at multiples.

The bottom panel runs the actual algorithm: quantize to a bitstream, then for
each lag count the set bits of (bitstream XOR bitstream-shifted-by-lag) over a
half-window, exactly what bitstream_acf does with XOR + popcount.

Style and palette follow gen_interpolation_figures.py (the canonical PALETTE
lives there).

Usage: /usr/bin/python3 docs/scripts/gen_bitstream_acf_figures.py
"""

import os
import numpy as np
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt

SKY = '#64b5f6'            # bitstream fill
SITE_ACCENT = '#1565c0'    # waveform / count curve
MAGENTA = '#d81b60'        # event marker: the detected period
GREY = '#5d5d5d'
INK = '#1a1a1a'

OUT_DIR = os.path.join(
   os.path.dirname(__file__), '..', 'modules', 'ROOT', 'images')


def signal(period, n):
   """Harmonically rich tone: 1st/2nd/3rd partials at 0.3/0.4/0.3."""
   t = np.arange(n)
   w = 2 * np.pi / period
   return (0.3 * np.sin(w * t)
           + 0.4 * np.sin(2 * w * t)
           + 0.3 * np.sin(3 * w * t))


def bacf_count(bits, lag, window):
   """Set-bit count of (bits XOR bits<<lag) over `window`, the bitstream_acf op."""
   a = bits[:window]
   b = bits[lag:lag + window]
   return int(np.count_nonzero(a ^ b))


def gen_count():
   period = 80
   window = 2 * period                      # half of the captured history
   max_lag = int(2.4 * period)
   n = max_lag + window + period            # enough history for the largest lag
   x = signal(period, n)
   bits = (x >= 0).astype(np.uint8)        # one bit per sample

   lags = np.arange(1, max_lag)
   counts = np.array([bacf_count(bits, int(L), window) for L in lags])

   fig, (ax_w, ax_b, ax_c) = plt.subplots(
      3, 1, figsize=(10, 8.2),
      gridspec_kw=dict(height_ratios=[2.0, 0.8, 2.2], hspace=0.38))

   show = 2 * period + 1                     # show two periods of signal/bits

   # --- top: the raw signal ---
   ax_w.plot(np.arange(show), x[:show], color=SITE_ACCENT, linewidth=1.8)
   ax_w.axhline(0, color=GREY, lw=0.8, ls=':')
   ax_w.set_title('A harmonically rich signal (partials 0.3, 0.4, 0.3)',
                  fontsize=11, color=INK, loc='left')
   ax_w.set_yticks([])
   ax_w.set_xlim(0, show - 1)
   for s in ('top', 'right', 'left'):
      ax_w.spines[s].set_visible(False)
   ax_w.spines['bottom'].set_color('#b0b0b0')

   # --- middle: the bitstream ---
   ax_b.fill_between(np.arange(show), bits[:show], step='post',
                     color=SKY, alpha=0.9, linewidth=0)
   ax_b.set_title('Its one-bit-per-sample bitstream (1 while positive)',
                  fontsize=11, color=INK, loc='left')
   ax_b.set_yticks([])
   ax_b.set_xlim(0, show - 1)
   ax_b.set_ylim(-0.1, 1.15)
   for s in ('top', 'right', 'left'):
      ax_b.spines[s].set_visible(False)
   ax_b.spines['bottom'].set_color('#b0b0b0')

   # --- bottom: the BACF mismatch count ---
   ax_c.plot(lags, counts, color=SITE_ACCENT, linewidth=1.8, zorder=2)
   ax_c.axvline(period, color=MAGENTA, lw=1.4, ls='--', zorder=3)
   ax_c.plot([period], [counts[period - 1]], 'o', color=MAGENTA,
             markersize=11, zorder=4)
   ax_c.annotate('period: deepest notch\n(count = 0, perfect match)',
                 xy=(period, counts[period - 1]),
                 xytext=(period + 6, counts.max() * 0.45),
                 fontsize=10, color=MAGENTA, va='center',
                 arrowprops=dict(arrowstyle='-', color=MAGENTA, lw=0.9))
   if 2 * period < max_lag:
      ax_c.annotate('2x period', xy=(2 * period, counts[2 * period - 1]),
                    xytext=(2 * period - 6, counts.max() * 0.62),
                    fontsize=9, color=GREY, ha='right', va='center',
                    arrowprops=dict(arrowstyle='-', color=GREY, lw=0.8))
   ax_c.set_title('XOR mismatch count vs lag: low count = strong periodicity',
                  fontsize=11, color=INK, loc='left')
   ax_c.set_xlabel('Lag (samples)')
   ax_c.set_ylabel('Mismatch count')
   ax_c.set_xlim(0, max_lag - 1)
   ax_c.set_ylim(-2, counts.max() * 1.08)
   ax_c.grid(True, linestyle='--', linewidth=0.5, color='#b0b0b0', alpha=0.8)
   for s in ('top', 'right'):
      ax_c.spines[s].set_visible(False)

   path = os.path.join(OUT_DIR, 'bitstream_acf_count.svg')
   fig.savefig(path, format='svg', bbox_inches='tight')
   plt.close(fig)
   print('wrote', path)


if __name__ == '__main__':
   gen_count()
