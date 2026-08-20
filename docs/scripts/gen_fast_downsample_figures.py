#!/usr/bin/env python3
"""
Generate the q::fast_downsample_3 / q::fast_downsample_5 reference figures.

Produces, in docs/modules/ROOT/images/:

   fast_downsample.svg    -- the magnitude response of the { 0.25, 0.5, 0.25 }
                             half-band pre-filter that fast_downsample_3
                             applies before decimating by two: a raised-cosine
                             rolloff with a null at the old Nyquist frequency.

   fast_downsample_compare.svg -- the four named kernel responses in dB. All
                             are repeated two-sample averages, so they are one
                             rolloff raised to the powers 1 through 4.

The kernel h = [0.25, 0.5, 0.25] has response |H(f)| = cos^2(pi f / fs),
so it is -6 dB (0.5) at the new Nyquist fs/4 and a full null at the old
Nyquist fs/2. Style and palette follow gen_interpolation_figures.py.

Usage: /usr/bin/python3 docs/scripts/gen_fast_downsample_figures.py
"""

import os
import numpy as np
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt

SITE_ACCENT = '#1565c0'    # fast_downsample_3
SKY = '#64b5f6'            # fast_downsample_2
AMBER = '#ffb300'          # fast_downsample_4
MAGENTA = '#d81b60'        # fast_downsample_5
GREEN = '#43a047'          # the retained passband (below the new Nyquist)
GREY = '#5d5d5d'

OUT_DIR = os.path.join(
   os.path.dirname(__file__), '..', 'modules', 'ROOT', 'images')


def gen():
   # Frequency axis as a fraction of the input sample rate, 0 .. fs/2.
   f = np.linspace(0, 0.5, 512)
   mag = np.cos(np.pi * f) ** 2          # |H(f)| = cos^2(pi f / fs)

   fig, ax = plt.subplots(figsize=(10, 4.5))

   # Shade the band that survives decimation: everything below the new Nyquist.
   ax.axvspan(0, 0.25, color=GREEN, alpha=0.08, zorder=0)
   ax.plot(f, mag, color=SITE_ACCENT, linewidth=2.2,
           label='{ 0.25, 0.5, 0.25 } response', zorder=3)

   # New Nyquist (fs/4): the -6 dB point.
   ax.axvline(0.25, color=GREY, linestyle='--', linewidth=1.1, zorder=2)
   ax.scatter([0.25], [0.5], s=28, color=SITE_ACCENT, zorder=4)
   ax.annotate('new Nyquist (fs/4)\n0.5  (-6 dB)', xy=(0.25, 0.5),
               xytext=(0.30, 0.66), color=GREY, fontsize=9,
               arrowprops=dict(arrowstyle='-', color=GREY, lw=0.8))

   # Old Nyquist (fs/2): the null.
   ax.annotate('old Nyquist (fs/2)\nnull', xy=(0.5, 0.0),
               xytext=(0.40, 0.20), color=GREY, fontsize=9,
               arrowprops=dict(arrowstyle='-', color=GREY, lw=0.8))

   ax.text(0.115, 0.06, 'kept after\ndecimation', ha='center', va='bottom',
           color=GREEN, fontsize=9)

   ax.set_xlabel('Frequency (fraction of input sample rate)')
   ax.set_ylabel('Magnitude')
   ax.set_xlim(0, 0.5)
   ax.set_ylim(0, 1.05)
   ax.set_xticks([0, 0.125, 0.25, 0.375, 0.5])
   ax.grid(True, linestyle='--', color='#b0b0b0', alpha=0.5)
   for s in ('top', 'right'):
      ax.spines[s].set_visible(False)
   ax.legend(loc='upper right')

   path = os.path.join(OUT_DIR, 'fast_downsample.svg')
   fig.savefig(path, format='svg', bbox_inches=None)
   plt.close(fig)
   print('wrote', path)


def gen_compare():
   # All three kernels are repeated two-sample averages, so their responses are
   # cos^p(w/2) for p = 2, 3, 4. Shown in dB, because the interesting part is
   # the stopband, which a linear axis flattens against zero.
   f = np.linspace(0, 0.5, 1024)
   cos2 = np.cos(np.pi * f) ** 2
   floor = 1e-5

   series = [
      ('fast_downsample_2   { 1, 1 } / 2',           cos2**0.5, SKY,         -3.01, -10.20),
      ('fast_downsample_3   { 1, 2, 1 } / 4',        cos2,      SITE_ACCENT, -6.02, -20.40),
      ('fast_downsample_4   { 1, 3, 3, 1 } / 8',     cos2**1.5, AMBER,       -9.03, -30.60),
      ('fast_downsample_5   { 1, 4, 6, 4, 1 } / 16', cos2**2,   MAGENTA,    -12.04, -40.80),
   ]

   fig, ax = plt.subplots(figsize=(10, 4.5))
   ax.axvspan(0, 0.25, color=GREEN, alpha=0.08, zorder=0)

   for label, mag, color, at_quarter, at_040 in series:
      ax.plot(f, 20 * np.log10(np.maximum(mag, floor)), color=color,
              linewidth=2.2, zorder=3, label=label)
      ax.scatter([0.25, 0.40], [at_quarter, at_040], s=26, color=color, zorder=4)
      ax.text(0.241, at_quarter, f'{at_quarter:.1f}', ha='right',
              va='center', color=color, fontsize=8.5)
      ax.text(0.412, at_040, f'{at_040:.1f} dB', ha='left', va='center',
              color=color, fontsize=8.5)

   ax.axvline(0.25, color=GREY, linestyle='--', linewidth=1.1, zorder=2)
   ax.text(0.253, 1.0, 'new Nyquist (fs/4)', ha='left', va='bottom',
           color=GREY, fontsize=9)
   ax.text(0.40, -74.0, '0.40 fs', ha='center', va='bottom',
           color=GREY, fontsize=9)
   ax.text(0.125, 0.9, 'kept after decimation', ha='center', va='bottom',
           color=GREEN, fontsize=9)

   ax.set_xlabel('Frequency (fraction of input sample rate)')
   ax.set_ylabel('Magnitude (dB)')
   ax.set_xlim(0, 0.5)
   ax.set_ylim(-80, 4)
   ax.set_xticks([0, 0.125, 0.25, 0.375, 0.5])
   ax.grid(True, linestyle='--', color='#b0b0b0', alpha=0.5)
   for s_ in ('top', 'right'):
      ax.spines[s_].set_visible(False)
   ax.legend(loc='lower left')

   path = os.path.join(OUT_DIR, 'fast_downsample_compare.svg')
   fig.savefig(path, format='svg', bbox_inches=None)
   plt.close(fig)
   print('wrote', path)


if __name__ == '__main__':
   gen()
   gen_compare()
