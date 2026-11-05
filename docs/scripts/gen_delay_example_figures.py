#!/usr/bin/env python3
"""
Generate the io_delay example figure.

Produces, in docs/modules/ROOT/images/:

   io_delay_echo_train.svg    -- the feedback delay's impulse response:
                                 echoes at the delay spacing, decaying by
                                 the feedback factor (io_delay: 500 ms, 0.4)

Style and palette follow gen_interpolation_figures.py (the canonical
PALETTE lives there).

Usage: python3 docs/scripts/gen_delay_example_figures.py
"""

import os
import numpy as np
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt

SITE_ACCENT = '#1565c0'    # output
AMBER = '#ffb300'          # secondary series: decay envelope
MAGENTA = '#d81b60'        # event markers

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
   io_delay_echo_figure()
