#!/usr/bin/env python3
"""
Generate the spectral_flux reference figure.

Produces, in docs/modules/ROOT/images/:

   spectral_flux.svg  -- a plucked low E (test/audio_files/1a-Low-E.wav):
                         its full-band level against spectral_flux(3 kHz,
                         50 ms), in dB, and where a delta_gate of 12 dB on
                         the flux fires.

The blocks are replayed with the arithmetic of q/fx/spectral_flux.hpp: the
RBJ high-pass biquad of q/fx/biquad.hpp (Q 0.707), then the peak envelope
follower of q/fx/envelope.hpp, and the gate of q/fx/delta_gate.hpp.

NOTE: run with homebrew `python3` (has soundfile + matplotlib); the Xcode
`/usr/bin/python3` used by the other generators cannot read float WAVs.
Style and palette follow gen_interpolation_figures.py.

Usage: python3 docs/scripts/gen_spectral_flux_figures.py
"""

import math
import os
import numpy as np
import soundfile as sf
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt

SITE_ACCENT = '#1565c0'
AMBER = '#ffb300'
MAGENTA = '#d81b60'
GREY = '#b0b0b0'

HERE = os.path.dirname(__file__)
OUT_DIR = os.path.join(HERE, '..', 'modules', 'ROOT', 'images')
AUDIO = os.path.join(HERE, '..', '..', 'test', 'audio_files',
                     '1a-Low-E.wav')

T0, T1 = 0.85, 3.0
CUTOFF, WINDOW = 3000.0, 0.05


def fast_exp3(x):
   return (6 + x * (6 + x * (3 + x))) * 0.16666666


def peak_envelope(x, release, sps):
   rel = fast_exp3(-2.0 / (sps * release))
   y, out = 0.0, np.empty(len(x))
   for i, s in enumerate(x):
      y = s if s > y else s + rel * (y - s)
      out[i] = y
   return out


def delta_gate(x, ratio, d, sps):
   # The reference each sample is compared with: a point sample every
   # d / 2, two holds back, as q/fx/sample_hold.hpp takes it
   n = max(1, int(d * sps * 0.5))
   then = last = hold = 0.0
   tick, ref = 0, np.empty(len(x))
   for i, s in enumerate(x):
      ref[i] = then
      tick += 1
      if tick >= n:
         tick = 0
         then, last, hold = last, hold, s
   return x > ref * ratio, ref


def highpass(x, f, sps, q=0.707):
   w = 2 * math.pi * f / sps
   c, s = math.cos(w), math.sin(w)
   alpha = s / (2 * q)
   a0 = 1 + alpha
   b0, b1, b2 = (1 + c) / 2 / a0, -(1 + c) / a0, (1 + c) / 2 / a0
   a1, a2 = -2 * c / a0, (1 - alpha) / a0
   x1 = x2 = y1 = y2 = 0.0
   out = np.empty(len(x))
   for i, v in enumerate(x):
      r = b0 * v + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2
      x2, x1, y2, y1 = x1, v, y1, r
      out[i] = r
   return out


def levels():
   x, sps = sf.read(AUDIO, dtype='float32')
   x = x[:int(T1 * sps)]
   full = peak_envelope(np.abs(x), WINDOW, sps)
   flux = peak_envelope(np.abs(highpass(x, CUTOFF, sps)), WINDOW, sps)
   return full, flux, sps


def db(v):
   return 20 * np.log10(np.maximum(v, 1e-5))


def gen():
   full, flux, sps = levels()
   gate, _ = delta_gate(flux, 10 ** (12 / 20), 0.02, sps)
   t = np.arange(len(full)) / sps
   k = slice(int(T0 * sps), int(T1 * sps), 20)

   # Rising edges of the gate inside the drawn stretch, after the first
   # 30 ms, where the reference is still zero and any level is a rise.
   start = int(T0 * sps)
   edges = [i for i in range(max(start, int(0.03 * sps)), len(gate))
            if gate[i] and not gate[i - 1]]

   fig, ax = plt.subplots(figsize=(10, 4.8))
   ax.plot(t[k], db(full[k]), color=SITE_ACCENT, linewidth=1.8,
           label='full-band level')
   ax.plot(t[k], db(flux[k]), color=AMBER, linewidth=1.8,
           label=f'spectral_flux, {CUTOFF / 1000:g} kHz and up')
   for i in edges:
      ax.axvline(t[i], color=MAGENTA, linestyle=':', linewidth=1.4)
   if edges:
      ax.text(t[edges[0]] + 0.02, -60,
              'delta_gate(12 dB)\non the flux fires',
              color=MAGENTA, fontsize=9, va='top')
   ax.set_xlim(T0, T1)
   ax.set_ylim(-70, 0)
   ax.set_xlabel('Time (s)')
   ax.set_ylabel('Level (dB)')
   for s in ('top', 'right'):
      ax.spines[s].set_visible(False)
   ax.spines['left'].set_color(GREY)
   ax.spines['bottom'].set_color(GREY)
   ax.grid(True, linestyle='--', color=GREY, alpha=0.5)
   ax.legend(loc='upper right', fontsize=9)

   fig.tight_layout()
   path = os.path.join(OUT_DIR, 'spectral_flux.svg')
   fig.savefig(path, format='svg', bbox_inches=None)
   plt.close(fig)
   print('wrote', path)


if __name__ == '__main__':
   gen()
