#!/usr/bin/env python3
"""
Generate the grain_freeze example figure.

Produces, in docs/modules/ROOT/images/:

   grain_freeze_timeline.svg   -- the example's own audio (Low E.wav) with
                                  the freeze point, the sustained grain
                                  texture region, and the closing fade

Style and palette follow gen_interpolation_figures.py (the canonical
PALETTE lives there).

Usage: python3 docs/scripts/gen_grain_freeze_example_figures.py
"""

import os
import wave
import numpy as np
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt

SKY = '#64b5f6'            # the input signal
SITE_ACCENT = '#1565c0'
AMBER = '#ffb300'          # secondary series: the texture gain
MAGENTA = '#d81b60'        # event markers: the freeze point

OUT_DIR = os.path.join(
   os.path.dirname(__file__), '..', 'modules', 'ROOT', 'images')
WAV = os.path.join(
   os.path.dirname(__file__), '..', '..',
   'example', 'grain_freeze', 'audio', 'Low E.wav')

FREEZE_AT, TAIL, FADE = 1.2, 6.0, 4.0     # the example's parameters


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


def load_wav(path):
   w = wave.open(path)
   sps = w.getframerate()
   data = np.frombuffer(
      w.readframes(w.getnframes()), dtype=np.int16).astype(float) / 32768.0
   w.close()
   return data, float(sps)


def envelope(s, sps, win_ms=10.0):
   win = int(sps * win_ms / 1000)
   pad = (-len(s)) % win
   m = np.abs(np.concatenate([s, np.zeros(pad)])).reshape(-1, win).max(axis=1)
   t = (np.arange(len(m)) + 0.5) * win / sps
   return t, m


def timeline_figure():
   s, sps = load_wav(WAV)
   t_end = len(s) / sps
   t_in, env_in = envelope(s, sps)

   fig, ax = new_axes(xlabel='Time (s)', ylabel='Amplitude')
   ax.fill_between(t_in, -env_in, env_in, color=SKY, alpha=0.65,
                   linewidth=0, label='Input (Low E.wav)')

   # The texture: grains re-read the frozen moment from the freeze point
   # until the end of the tail
   ax.axvline(FREEZE_AT, color=MAGENTA, linewidth=1.5, linestyle=':',
              label='Freeze point (1.2 s)')

   # The texture gain: unity, then the half-Hann fade over the last 4 s
   tg = np.linspace(0, t_end + TAIL, 1000)
   fade_start = t_end + TAIL - FADE
   gain = np.where(
      tg < fade_start, 1.0,
      0.5 * (1 + np.cos(np.pi * np.clip((tg - fade_start) / FADE, 0, 1))))
   gain[tg < FREEZE_AT] = 0.0
   ax.plot(tg, gain, color=AMBER, linewidth=1.75,
           label='Texture gain (4 s fade)')

   ax.set_xlim(0, t_end + TAIL)
   ax.set_ylim(-0.8, 1.1)
   ax.legend(loc='best')
   save(fig, 'grain_freeze_timeline.svg')


if __name__ == '__main__':
   timeline_figure()
