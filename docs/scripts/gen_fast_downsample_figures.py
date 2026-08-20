#!/usr/bin/env python3
"""
Generate the q::fast_downsample_3 / q::fast_downsample_5 reference figures.

Produces, in docs/modules/ROOT/images/:

   fast_downsample.svg    -- the magnitude response of the { 0.25, 0.5, 0.25 }
                             half-band pre-filter that fast_downsample_3
                             applies before decimating by two: a raised-cosine
                             rolloff with a null at the old Nyquist frequency.

   fast_downsample_5.svg  -- the same response in dB against that of
                             fast_downsample_5's { 1, 4, 6, 4, 1 } / 16
                             kernel, which is the same rolloff squared.

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
   # Both kernels are repeated two-sample averages, so the 5-tap response is
   # the 3-tap response squared. Shown in dB, because the interesting part is
   # the stopband, which a linear axis flattens against zero.
   f = np.linspace(0, 0.5, 1024)
   cos2 = np.cos(np.pi * f) ** 2
   floor = 1e-5
   db3 = 20 * np.log10(np.maximum(cos2, floor))
   db5 = 20 * np.log10(np.maximum(cos2 ** 2, floor))

   fig, ax = plt.subplots(figsize=(10, 4.5))

   ax.axvspan(0, 0.25, color=GREEN, alpha=0.08, zorder=0)
   ax.plot(f, db3, color=SITE_ACCENT, linewidth=2.2, zorder=3,
           label='fast_downsample_3   { 1, 2, 1 } / 4')
   ax.plot(f, db5, color=MAGENTA, linewidth=2.2, zorder=3,
           label='fast_downsample_5   { 1, 4, 6, 4, 1 } / 16')

   ax.axvline(0.25, color=GREY, linestyle='--', linewidth=1.1, zorder=2)

   # The two documented reference points.
   for x, y3, y5 in ((0.25, -6.02, -12.04), (0.40, -20.41, -40.82)):
      ax.scatter([x, x], [y3, y5], s=26,
                 color=[SITE_ACCENT, MAGENTA], zorder=4)

   ax.annotate('new Nyquist (fs/4)\n-6.0 dB', xy=(0.25, -6.02),
               xytext=(0.145, -17.0), color=GREY, fontsize=9,
               arrowprops=dict(arrowstyle='-', color=GREY, lw=0.8))
   ax.annotate('-12.0 dB', xy=(0.25, -12.04),
               xytext=(0.145, -26.0), color=GREY, fontsize=9,
               arrowprops=dict(arrowstyle='-', color=GREY, lw=0.8))
   ax.annotate('at 0.40 fs the extra\nround of averaging\nis worth 20 dB',
               xy=(0.40, -40.82), xytext=(0.265, -60.0), color=GREY,
               fontsize=9, arrowprops=dict(arrowstyle='-', color=GREY, lw=0.8))

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

   path = os.path.join(OUT_DIR, 'fast_downsample_5.svg')
   fig.savefig(path, format='svg', bbox_inches=None)
   plt.close(fig)
   print('wrote', path)


if __name__ == '__main__':
   gen()
   gen_compare()
