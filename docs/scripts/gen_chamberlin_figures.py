#!/usr/bin/env python3
"""
Generate the chamberlin_filter reference figure.

Produces, in docs/modules/ROOT/images/:

   chamberlin_response.svg

The magnitude response of the ACTUAL filter: the Chamberlin recurrence is run in
Python exactly as in q/fx/svf.hpp (chamberlin_filter), impulse response -> FFT,
the lowpass at rising Q. The fs/6 stability ceiling is marked: the cutoff is
valid below it (the coefficient f = 2*sin(pi*fc/fs) reaches 1 at fs/6).

Style matches gen_svf_figures.py. Run with /usr/bin/python3.
"""

import os
import numpy as np
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt

GRID_COLOR = '#b0b0b0'
TEXT_COLOR = '#1a1a1a'
SIGNAL_RED = '#e53935'

OUT_DIR = os.path.join(
   os.path.dirname(__file__), '..', 'modules', 'ROOT', 'images')

SPS = 48000.0
FC = 1000.0
N = 1 << 16
MAX_COEFF = 0.99


def chamberlin_impulse(fc, Q, n=N, sps=SPS):
   f = 2.0 * np.sin(np.pi * fc / sps)
   f = min(f, MAX_COEFF)
   q = 1.0 / Q
   lp = bp = 0.0
   h = np.empty(n)
   for i in range(n):
      x = 1.0 if i == 0 else 0.0
      lp = lp + f * bp
      hp = x - lp - q * bp
      bp = bp + f * hp
      h[i] = lp
   return h


def magnitude_db(h, sps=SPS):
   H = np.fft.rfft(h)
   freq = np.fft.rfftfreq(len(h), 1.0 / sps)
   return freq, 20.0 * np.log10(np.maximum(np.abs(H), 1e-9))


def main():
   fig, ax = plt.subplots(figsize=(10, 6))
   curves = [
      (0.707, '#bbdefb', 'Q = 0.707'),
      (2.0,   '#64b5f6', 'Q = 2'),
      (5.0,   '#1e88e5', 'Q = 5'),
      (10.0,  '#0d47a1', 'Q = 10'),
   ]
   for Q, color, label in curves:
      freq, mag = magnitude_db(chamberlin_impulse(FC, Q))
      ax.plot(freq, mag, color=color, linewidth=1.7, label=label)

   ax.axvline(SPS / 6.0, color=SIGNAL_RED, linewidth=1.3, linestyle='--')
   ax.text(SPS / 6.0 * 0.97, 18, 'fs/6 limit', color=SIGNAL_RED,
           ha='right', fontsize=10)

   ax.set_xscale('log')
   ax.set_xlim(20, 20000)
   ax.set_ylim(-40, 24)
   ax.set_xlabel('Frequency (Hz)', color=TEXT_COLOR)
   ax.set_ylabel('Magnitude (dB)', color=TEXT_COLOR)
   ax.set_title('Chamberlin lowpass at rising Q (peak height = Q)', color=TEXT_COLOR)
   ax.grid(True, which='both', linestyle='--', linewidth=0.5,
           color=GRID_COLOR, alpha=0.7)
   ax.legend(loc='lower left')

   fig.tight_layout()
   path = os.path.join(OUT_DIR, 'chamberlin_response.svg')
   fig.savefig(path, format='svg', bbox_inches=None)
   plt.close(fig)
   print('wrote', path)


if __name__ == '__main__':
   main()
