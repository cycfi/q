#!/usr/bin/env python3
"""
Generate the q noise-generator reference figure.

Produces, in docs/modules/ROOT/images/:

   noise_spectra.svg  -- Welch power spectra of white_noise_gen and
                         pink_noise_gen, on a log-frequency axis, showing the
                         flat white spectrum against pink's -3 dB/octave
                         (-10 dB/decade) roll-off, with an ideal reference
                         slope.

The generators are reproduced here exactly (the same 32-bit integer XOR/add
white core and the same three-pole pink weighting as
q_lib/include/q/synth/noise_gen.hpp); the sequences were verified bit-exact
against samples dumped from the compiled header. The DC component is removed
before the transform so the plot shows spectral colour, not the offset.

Style and palette follow gen_interpolation_figures.py.

Usage: /usr/bin/python3 docs/scripts/gen_noise_gen_figures.py
"""

import os
import numpy as np
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt

SITE_ACCENT = '#1565c0'    # white noise
ORANGE = '#fb8c00'         # pink noise
GREEN = '#43a047'          # ideal reference slope
GREY = '#5d5d5d'

SPS = 44100.0

OUT_DIR = os.path.join(
   os.path.dirname(__file__), '..', 'modules', 'ROOT', 'images')

MASK = 0xffffffff


def white_pink(n):
   """Exact reproduction of white_noise_gen / pink_noise_gen (noise_gen.hpp)."""
   x1, x2 = 0x67452301, 0xefcdab89
   scale = np.float32(2.0 / 0xffffffff)
   ps = 0.2
   c1 = np.float32(0.99765);   c2 = np.float32(0.0990460 * ps)
   c3 = np.float32(0.96300);   c4 = np.float32(0.2965164 * ps)
   c5 = np.float32(0.57000);   c6 = np.float32(1.0526913 * ps)
   c7 = np.float32(0.1848 * ps)
   b0 = b1 = b2 = np.float32(0.0)
   w = np.empty(n, dtype=np.float64)
   p = np.empty(n, dtype=np.float64)
   for i in range(n):
      x1 = (x1 ^ x2) & MASK
      s = np.float32(x2) * scale
      x2 = (x2 + x1) & MASK
      w[i] = s
      b0 = c1 * b0 + s * c2
      b1 = c3 * b1 + s * c4
      b2 = c5 * b2 + s * c6
      p[i] = b0 + b1 + b2 + s * c7
   return w, p


def welch_psd(x, seg=8192):
   """Averaged periodogram (Hann, 50% overlap), DC-removed, one-sided."""
   win = np.hanning(seg)
   norm = (win ** 2).sum()
   hop = seg // 2
   acc = np.zeros(seg // 2 + 1)
   count = 0
   for start in range(0, len(x) - seg + 1, hop):
      frame = x[start:start + seg]
      frame = (frame - frame.mean()) * win        # remove DC, apply window
      spec = np.fft.rfft(frame)
      acc += (np.abs(spec) ** 2) / norm
      count += 1
   psd = acc / count
   freqs = np.fft.rfftfreq(seg, 1.0 / SPS)
   return freqs, psd


def gen():
   n = 1 << 20
   w, p = white_pink(n)
   fw, pw = welch_psd(w)
   fp, pp = welch_psd(p)

   # dB, normalised so each curve reads 0 dB at 1 kHz (shape, not absolute level)
   def to_db_at_1k(f, psd):
      db = 10.0 * np.log10(psd + 1e-30)
      ref = np.interp(1000.0, f, db)
      return db - ref

   wdb = to_db_at_1k(fw, pw)
   pdb = to_db_at_1k(fp, pp)

   fig, ax = plt.subplots(figsize=(10, 6))

   fmin, fmax = 20.0, SPS / 2
   m = (fw >= fmin) & (fw <= fmax)

   # ideal -10 dB/decade (= -3 dB/octave) reference through 1 kHz, 0 dB
   fref = np.array([fmin, fmax])
   pref = -10.0 * np.log10(fref / 1000.0)
   ax.plot(fref, pref, color=GREEN, linewidth=1.4, linestyle='--',
           label='ideal -3 dB/octave', zorder=2)

   ax.plot(fw[m], wdb[m], color=SITE_ACCENT, linewidth=1.3,
           label='white_noise_gen', zorder=4)
   ax.plot(fp[m], pdb[m], color=ORANGE, linewidth=1.3,
           label='pink_noise_gen', zorder=3)

   ax.set_xscale('log')
   ax.set_xlim(fmin, fmax)
   ax.set_ylim(-30, 22)
   ax.set_xlabel('Frequency (Hz)')
   ax.set_ylabel('Power (dB, relative to 1 kHz)')
   ax.grid(True, which='both', linestyle='--', color='#b0b0b0', linewidth=0.6)
   ax.set_axisbelow(True)
   for s in ('top', 'right'):
      ax.spines[s].set_visible(False)
   ax.spines['bottom'].set_color('#b0b0b0')
   ax.spines['left'].set_color('#b0b0b0')
   ax.set_xticks([20, 50, 100, 200, 500, 1000, 2000, 5000, 10000, 20000])
   ax.set_xticklabels(['20', '50', '100', '200', '500', '1k', '2k',
                       '5k', '10k', '20k'])
   ax.legend(loc='upper right')
   ax.set_title('White and pink noise spectra (DC removed)',
                color='#1a1a1a', fontsize=11)

   path = os.path.join(OUT_DIR, 'noise_spectra.svg')
   fig.savefig(path, format='svg', bbox_inches='tight')
   plt.close(fig)
   print('wrote', path)


if __name__ == '__main__':
   gen()
