#!/usr/bin/env python3
"""
Generate the delay and io_delay example figures.

Produces, in docs/modules/ROOT/images/:

   delay_example_output.svg   -- the delay example's actual output: the
                                 example's own audio (Low E.wav) processed
                                 by the exact dsp (350 ms delay, 0.85
                                 feedback), input vs. output envelope
   io_delay_echo_train.svg    -- the feedback delay's impulse response:
                                 echoes at the delay spacing, decaying by
                                 the feedback factor (io_delay: 500 ms, 0.4)

Style and palette follow gen_interpolation_figures.py (the canonical
PALETTE lives there).

Usage: python3 docs/scripts/gen_delay_example_figures.py
"""

import os
import wave
import numpy as np
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt

SKY = '#64b5f6'            # input signal
SITE_ACCENT = '#1565c0'    # output
AMBER = '#ffb300'          # secondary series: decay envelope
MAGENTA = '#d81b60'        # event markers

OUT_DIR = os.path.join(
   os.path.dirname(__file__), '..', 'modules', 'ROOT', 'images')
WAV = os.path.join(
   os.path.dirname(__file__), '..', '..',
   'example', 'delay', 'audio', 'Low E.wav')


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


def feedback_delay(s, delay_len, feedback, tail):
   """y(n) = s(n) + feedback * y(n - delay_len), computed exactly,
   block-recursively (each block is one delay length long)."""
   x = np.concatenate([s, np.zeros(tail)])
   pad = (-len(x)) % delay_len
   x = np.concatenate([x, np.zeros(pad)])
   y = np.empty_like(x)
   prev = np.zeros(delay_len)
   for k in range(0, len(x), delay_len):
      y[k:k+delay_len] = x[k:k+delay_len] + feedback * prev
      prev = y[k:k+delay_len]
   return y


def envelope(s, sps, win_ms=10.0):
   """Peak envelope on a coarse grid, for plotting long waveforms."""
   win = int(sps * win_ms / 1000)
   pad = (-len(s)) % win
   m = np.abs(np.concatenate([s, np.zeros(pad)])).reshape(-1, win).max(axis=1)
   t = (np.arange(len(m)) + 0.5) * win / sps
   return t, m


def delay_example_figure():
   s, sps = load_wav(WAV)
   delay_len = int(sps * 0.350)
   y = feedback_delay(s, delay_len, 0.85, tail=int(6 * sps))

   t_in, env_in = envelope(s, sps)
   t_out, env_out = envelope(y, sps)

   fig, ax = new_axes(xlabel='Time (s)', ylabel='Amplitude')
   ax.fill_between(t_in, -env_in, env_in, color=SKY, alpha=0.65,
                   linewidth=0, label='Input (Low E.wav)')
   ax.plot(t_out, env_out, color=SITE_ACCENT, linewidth=1.5,
           label='Output envelope')
   ax.plot(t_out, -env_out, color=SITE_ACCENT, linewidth=1.5)
   ax.set_xlim(0, t_out[-1])
   ax.legend(loc='best')
   save(fig, 'delay_example_output.svg')


def io_delay_echo_figure():
   # Impulse response of the io_delay topology: unit impulse in, echoes
   # at the delay spacing, each feedback x the last
   delay_s, feedback = 0.5, 0.4
   n_echoes = 8
   t = np.arange(n_echoes) * delay_s
   a = feedback ** np.arange(n_echoes)

   fig, ax = new_axes(xlabel='Time (s)', ylabel='Amplitude')
   # The exponential decay law the echoes follow
   tt = np.linspace(0, t[-1], 400)
   ax.plot(tt, feedback ** (tt / delay_s), color=AMBER, linestyle='--',
           linewidth=1.5, label=r'feedback$^{\,t/delay}$ decay')
   ml, sl, bl = ax.stem(t, a, basefmt=' ')
   plt.setp(sl, color=SITE_ACCENT, linewidth=2.0)
   plt.setp(ml, color=SITE_ACCENT, markersize=7)
   ml.set_label('Echoes (0.4 feedback)')
   ax.axvline(0, color=MAGENTA, linewidth=1.0, linestyle=':',
              label='Input impulse')
   ax.set_xlim(-0.1, t[-1] + 0.25)
   ax.legend(loc='best')
   save(fig, 'io_delay_echo_train.svg')


if __name__ == '__main__':
   delay_example_figure()
   io_delay_echo_figure()
