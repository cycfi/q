#!/usr/bin/env python3
"""
Generate the square_synth example figures.

Produces, in docs/modules/ROOT/images/:

   square_synth_adsr.svg       -- the example's exact ADSR envelope
                                  (100 ms attack, 1 s decay, -12 dB
                                  sustain, 5 s sustain rate, 1 s release)
   square_synth_spectrum.svg   -- why bandwidth-limited: spectrum of a
                                  naive square vs. the poly_blep square
                                  the example plays (C6 at 44.1 kHz);
                                  the naive one folds aliases back into
                                  the audio band

Style and palette follow gen_interpolation_figures.py (the canonical
PALETTE lives there).

Usage: python3 docs/scripts/gen_square_synth_figures.py
"""

import os
import numpy as np
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt

SKY = '#64b5f6'
SITE_ACCENT = '#1565c0'    # the band-limited series
AMBER = '#ffb300'          # secondary series
RED = '#e53935'            # errors: the aliases
MAGENTA = '#d81b60'        # event markers: note on / note off

OUT_DIR = os.path.join(
   os.path.dirname(__file__), '..', 'modules', 'ROOT', 'images')


def new_axes(xlabel='Sample', ylabel='Value'):
   fig, ax = plt.subplots(figsize=(10, 6))
   ax.set_xlabel(xlabel)
   ax.set_ylabel(ylabel)
   ax.grid(True, linestyle='--', linewidth=0.5, color='#b0b0b0', alpha=0.8)
   return fig, ax


def save(fig, name):
   path = os.path.join(OUT_DIR, name)
   fig.savefig(path, format='svg', bbox_inches=None)
   plt.close(fig)
   print('wrote', path)


def adsr_figure():
   # The example's envelope configuration. Each segment is an
   # exponential approach toward its target, the q envelope_gen way:
   # the rate is the time to (almost) reach it.
   sps = 1000.0                     # 1 ms steps are plenty for a figure
   attack, decay, sustain_rate, release = 0.1, 1.0, 5.0, 1.0
   sustain_level = 10.0 ** (-12.0 / 20.0)          # -12 dB
   note_off = 3.0                                  # key released at 3 s

   def approach(y0, target, rate, n):
      # within 99.9% of the target after `rate` seconds
      k = np.exp(-np.log(1000.0) / (rate * sps))
      t = k ** np.arange(n)
      return target + (y0 - target) * t

   a = approach(0.0, 1.0, attack, int(attack * sps))
   d = approach(1.0, sustain_level, decay, int(decay * sps))
   s = approach(d[-1], 0.0, sustain_rate * 5,      # slow sustain decay
                int((note_off - attack - decay) * sps))
   r = approach(s[-1], 0.0, release, int(1.5 * release * sps))
   env = np.concatenate([a, d, s, r])
   t = np.arange(len(env)) / sps

   fig, ax = new_axes(xlabel='Time (s)', ylabel='Envelope level')
   ax.plot(t, env, color=SITE_ACCENT, linewidth=1.75, label='ADSR envelope')
   ax.axhline(sustain_level, color='#b0b0b0', linewidth=1.0, linestyle='--')
   ax.axvline(0, color=MAGENTA, linewidth=1.0, linestyle=':',
              label='Note on / off')
   ax.axvline(note_off, color=MAGENTA, linewidth=1.0, linestyle=':')

   style = dict(color='#333333', ha='center')
   ax.text(attack / 2, 1.04, 'attack\n100 ms', **style)
   ax.text(attack + decay / 2, 0.62, 'decay\n1 s', **style)
   ax.text(attack + decay + 0.85, sustain_level + 0.06,
           'sustain\n−12 dB', **style)
   ax.text(note_off + release / 2, 0.32, 'release\n1 s', **style)
   ax.set_ylim(-0.04, 1.12)
   ax.legend(loc='center right')
   save(fig, 'square_synth_adsr.svg')


def poly_blep(t, dt):
   r = np.zeros_like(t)
   lo = t < dt
   x = t[lo] / dt
   r[lo] = x + x - x * x - 1.0
   hi = t > 1.0 - dt
   x = (t[hi] - 1.0) / dt
   r[hi] = x * x + x + x + 1.0
   return r


def spectrum(sig, sps):
   w = np.hanning(len(sig))
   mag = np.abs(np.fft.rfft(sig * w))
   mag /= mag.max()
   f = np.fft.rfftfreq(len(sig), 1.0 / sps)
   return f, 20 * np.log10(np.maximum(mag, 1e-7))


def spectrum_figure():
   sps, freq, n = 44100.0, 1046.5, 1 << 15        # C6
   ph = (np.arange(n) * freq / sps) % 1.0
   dt = freq / sps

   naive = np.where(ph < 0.5, 1.0, -1.0)
   blep = naive + poly_blep(ph, dt) - poly_blep((ph + 0.5) % 1.0, dt)

   fn, mn = spectrum(naive, sps)
   fb, mb = spectrum(blep, sps)

   fig, ax = new_axes(xlabel='Frequency (kHz)', ylabel='Magnitude (dB)')
   ax.plot(fn / 1000, mn, color=RED, linewidth=1.0, alpha=0.8,
           label='Naive square (aliased)')
   ax.plot(fb / 1000, mb, color=SITE_ACCENT, linewidth=1.0,
           label='q::square (poly_blep)')
   ax.set_xlim(0, sps / 2000)
   ax.set_ylim(-100, 3)
   ax.legend(loc='best')
   save(fig, 'square_synth_spectrum.svg')


if __name__ == '__main__':
   adsr_figure()
   spectrum_figure()
