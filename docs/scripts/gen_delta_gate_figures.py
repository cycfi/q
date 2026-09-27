#!/usr/bin/env python3
"""
Generate the delta_gate reference figure.

Produces, in docs/modules/ROOT/images/:

   delta_gate.svg  -- top, delta_gate(6 dB, 20 ms) on the level of a
                      staccato guitar phrase (test/audio_files/GStaccato.wav,
                      its peak envelope, 50 ms release); bottom,
                      delta_gate_bipolar(a semitone, 20 ms) on a pitch
                      trace with vibrato, a whole-tone step and a gap with
                      no pitch.

The blocks are replayed with the arithmetic of q/fx/delta_gate.hpp,
sample_hold.hpp and envelope.hpp (the peak follower's release coefficient
is fast_exp3(-2 / (sps * release))).

NOTE: run with homebrew `python3` (has soundfile + matplotlib); the Xcode
`/usr/bin/python3` used by the other generators cannot read float WAVs.
Style and palette follow gen_interpolation_figures.py.

Usage: python3 docs/scripts/gen_delta_gate_figures.py
"""

import os
import numpy as np
import soundfile as sf
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt

SITE_ACCENT = '#1565c0'
AMBER = '#ffb300'
GREY = '#b0b0b0'

HERE = os.path.dirname(__file__)
OUT_DIR = os.path.join(HERE, '..', 'modules', 'ROOT', 'images')
AUDIO = os.path.join(HERE, '..', '..', 'test', 'audio_files',
                     'GStaccato.wav')

T0, T1 = 0.35, 2.45                         # the stretch drawn, seconds
PITCH_SPS = 1000.0                          # a control-rate pitch trace
SEMITONE = 2 ** (1 / 12)


def fast_exp3(x):
   return (6 + x * (6 + x * (3 + x))) * 0.16666666


def peak_envelope(x, release, sps):
   rel = fast_exp3(-2.0 / (sps * release))
   y, out = 0.0, np.empty(len(x))
   for i, s in enumerate(x):
      y = s if s > y else s + rel * (y - s)
      out[i] = y
   return out


def sample_hold(x, d, sps):
   # The reference each sample is compared with
   n = max(1, int(d * sps * 0.5))
   then = last = hold = 0.0
   tick, out = 0, np.empty(len(x))
   for i, s in enumerate(x):
      out[i] = then
      tick += 1
      if tick >= n:
         tick = 0
         then, last, hold = last, hold, s
   return out


def delta_gate(x, ratio, d, sps):
   ref = sample_hold(x, d, sps)
   return x > ref * ratio, ref


def delta_gate_bipolar(x, ratio, d, sps):
   ref = sample_hold(x, d, sps)
   fire = (ref > 0) & (x > 0) & ((x > ref * ratio) | (x < ref / ratio))
   return fire, ref


def level():
   x, sps = sf.read(AUDIO, dtype='float32')
   env = peak_envelope(np.abs(x[:int(T1 * sps)]), 0.05, sps)
   return env, sps


def pitch():
   t = np.arange(int(2.0 * PITCH_SPS)) / PITCH_SPS
   f = np.where(t < 0.9, 196.0, 220.0)       # G up a whole tone at 0.9 s
   f = f * 2 ** (15 / 1200 * np.sin(2 * np.pi * 5 * t))   # +-15 cents
   f[(t >= 1.4) & (t < 1.6)] = 0.0           # no pitch
   return t, f


def db(v):
   return 20 * np.log10(np.maximum(v, 1e-5))


def thin(x, n):
   # Every nth point, for drawing
   return x[::n]


def any_in(b, n):
   # True where any sample of a block of n is true, so short openings
   # survive the thinning
   m = len(b) // n * n
   return b[:m].reshape(-1, n).any(axis=1)


def style(ax):
   for s in ('top', 'right'):
      ax.spines[s].set_visible(False)
   ax.spines['left'].set_color(GREY)
   ax.spines['bottom'].set_color(GREY)
   ax.grid(True, linestyle='--', color=GREY, alpha=0.5)


def gen():
   env, sps = level()
   gate, ref = delta_gate(env, 10 ** (6 / 20), 0.02, sps)
   n = 40
   a, b = int(T0 * sps), int(T1 * sps)
   t = thin(np.arange(a, b) / sps, n)
   lv = thin(env[a:b], n)
   th = thin(ref[a:b], n) * 10 ** (6 / 20)
   on = any_in(gate[a:b], n)
   t, lv, th = t[:len(on)], lv[:len(on)], th[:len(on)]

   t2, f = pitch()
   step, fref = delta_gate_bipolar(f, SEMITONE, 0.02, PITCH_SPS)
   step = any_in(step, 4)
   t2, f, fref = thin(t2, 4)[:len(step)], thin(f, 4)[:len(step)], \
      thin(fref, 4)[:len(step)]

   fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(10, 6.4))

   ax1.fill_between(t, 0, 1, where=on, color=AMBER, alpha=0.35,
                    transform=ax1.get_xaxis_transform(), linewidth=0,
                    label='delta_gate true')
   ax1.plot(t, db(lv), color=SITE_ACCENT, linewidth=1.6,
            label='level (peak envelope)')
   ax1.plot(t, db(th), color='#5d5d5d', linewidth=1.0, linestyle='--',
            label='then() raised 6 dB')
   ax1.set_xlim(T0, T1)
   ax1.set_ylim(-70, 0)
   ax1.set_xlabel('Time (s)')
   ax1.set_ylabel('Level (dB)')
   ax1.set_title('delta_gate: a staccato phrase, 6 dB against ~20 ms ago',
                 color='#333333', fontsize=11, loc='left')
   ax1.legend(loc='center', fontsize=9)

   ok = fref > 0
   ax2.fill_between(t2, 0, 1, where=step, color=AMBER, alpha=0.35,
                    transform=ax2.get_xaxis_transform(), linewidth=0,
                    label='delta_gate_bipolar true')
   ax2.fill_between(t2, np.where(ok, fref / SEMITONE, np.nan),
                    np.where(ok, fref * SEMITONE, np.nan),
                    color=GREY, alpha=0.35, linewidth=0,
                    label='then() within a semitone')
   ax2.plot(t2, np.where(f > 0, f, np.nan), color=SITE_ACCENT,
            linewidth=1.6, label='pitch (0: none)')
   ax2.set_xlim(t2[0], t2[-1])
   ax2.set_ylim(170, 245)
   ax2.set_xlabel('Time (s)')
   ax2.set_ylabel('Frequency (Hz)')
   ax2.set_title('delta_gate_bipolar: a semitone either way',
                 color='#333333', fontsize=11, loc='left')
   ax2.text(1.5, 175, 'no pitch', ha='center', color='#333333', fontsize=9)
   ax2.legend(loc='upper left', fontsize=9)

   for ax in (ax1, ax2):
      style(ax)

   fig.tight_layout()
   path = os.path.join(OUT_DIR, 'delta_gate.svg')
   fig.savefig(path, format='svg', bbox_inches=None)
   plt.close(fig)
   print('wrote', path)


if __name__ == '__main__':
   gen()
