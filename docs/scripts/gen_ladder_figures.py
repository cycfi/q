#!/usr/bin/env python3
"""
Generate the moog_ladder reference figure.

Produces, in docs/modules/ROOT/images/:

   ladder_response.svg

The magnitude response of the ACTUAL filter: the linear ZDF ladder recurrence is
run in Python exactly as in q/fx/ladder.hpp, its impulse response is taken, and
the FFT magnitude is plotted at rising resonance. Shows the 24 dB/oct slope and
the resonant peak climbing toward self-oscillation as r -> 1.

Style matches gen_interpolation_figures.py / gen_svf_figures.py. Run with
/usr/bin/python3.
"""

import os
import numpy as np
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt

GRID_COLOR = '#b0b0b0'
TEXT_COLOR = '#1a1a1a'

OUT_DIR = os.path.join(
   os.path.dirname(__file__), '..', 'modules', 'ROOT', 'images')

SPS = 48000.0
FC = 1000.0
N = 1 << 16


def ladder_impulse(fc, r, n=N, sps=SPS):
   g = np.tan(np.pi * fc / sps)
   G = g / (1.0 + g)
   G2, G3, G4 = G * G, G * G * G, G * G * G * G
   k = 4.0 * r
   g_res = 1.0 / (1.0 + k * G4)
   one_m_g = 1.0 - G
   s1 = s2 = s3 = s4 = 0.0
   h = np.empty(n)
   for i in range(n):
      x = 1.0 if i == 0 else 0.0
      S = G3 * (one_m_g * s1) + G2 * (one_m_g * s2) + G * (one_m_g * s3) + (one_m_g * s4)
      u = (x - k * S) * g_res
      v = (u - s1) * G;  y1 = v + s1;  s1 = y1 + v
      v = (y1 - s2) * G; y2 = v + s2;  s2 = y2 + v
      v = (y2 - s3) * G; y3 = v + s3;  s3 = y3 + v
      v = (y3 - s4) * G; y4 = v + s4;  s4 = y4 + v
      h[i] = y4
   return h


def magnitude_db(h, sps=SPS):
   H = np.fft.rfft(h)
   f = np.fft.rfftfreq(len(h), 1.0 / sps)
   return f, 20.0 * np.log10(np.maximum(np.abs(H), 1e-9))


def main():
   fig, ax = plt.subplots(figsize=(10, 6))
   curves = [
      (0.0,  '#bbdefb', 'r = 0'),
      (0.5,  '#64b5f6', 'r = 0.5'),
      (0.85, '#1e88e5', 'r = 0.85'),
      (0.97, '#0d47a1', 'r = 0.97'),
   ]
   for r, color, label in curves:
      f, mag = magnitude_db(ladder_impulse(FC, r))
      ax.plot(f, mag, color=color, linewidth=1.7, label=label)

   ax.set_xscale('log')
   ax.set_xlim(20, 20000)
   ax.set_ylim(-60, 24)
   ax.set_xlabel('Frequency (Hz)', color=TEXT_COLOR)
   ax.set_ylabel('Magnitude (dB)', color=TEXT_COLOR)
   ax.set_title('4-pole lowpass (24 dB/oct) at rising resonance', color=TEXT_COLOR)
   ax.grid(True, which='both', linestyle='--', linewidth=0.5,
           color=GRID_COLOR, alpha=0.7)
   ax.legend(loc='lower left')

   fig.tight_layout()
   path = os.path.join(OUT_DIR, 'ladder_response.svg')
   fig.savefig(path, format='svg', bbox_inches=None)
   plt.close(fig)
   print('wrote', path)


if __name__ == '__main__':
   main()
