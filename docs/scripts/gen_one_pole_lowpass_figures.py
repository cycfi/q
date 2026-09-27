#!/usr/bin/env python3
"""
Generate the one pole low-pass reference figures.

Produces, in docs/modules/ROOT/images/:

   fixed_pt_leaky_integrator.svg  -- a noisy 12-bit reading smoothed by
                                     fixed_pt_leaky_integrator<16>.

The filter is replayed in integer arithmetic, as the C++ does it.

Style and palette follow gen_interpolation_figures.py.

Usage: /usr/bin/python3 docs/scripts/gen_one_pole_lowpass_figures.py
"""

import os
import numpy as np
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt

SITE_ACCENT = '#1565c0'
SKY = '#64b5f6'
GREY = '#b0b0b0'

OUT_DIR = os.path.join(
   os.path.dirname(__file__), '..', 'modules', 'ROOT', 'images')

def adc_readings(n=600):
   # A knob turned from 1000 to 3000, read with +-60 counts of noise from
   # a fixed LCG, so the figure is the same every run.
   state = 12345
   out = []
   for i in range(n):
      state = (state * 1103515245 + 12345) % 2**31
      noise = state % 121 - 60
      if i < 150:
         knob = 1000
      elif i < 350:
         knob = 1000 + (i - 150) * 10
      else:
         knob = 3000
      out.append(min(max(knob + noise, 0), 4095))
   return out


def fixed_pt_smooth(readings, k=16):
   # y += s - y / k in int, C++ division truncating toward zero
   y, out = 0, []
   for s in readings:
      y += s - int(y / k)
      out.append(int(y / k))
   return out


def style(ax):
   for s in ('top', 'right'):
      ax.spines[s].set_visible(False)
   ax.spines['left'].set_color(GREY)
   ax.spines['bottom'].set_color(GREY)
   ax.grid(True, linestyle='--', color=GREY, alpha=0.5)


def save(fig, name):
   fig.tight_layout()
   path = os.path.join(OUT_DIR, name)
   fig.savefig(path, format='svg', bbox_inches=None)
   plt.close(fig)
   print('wrote', path)


def fixed_pt_figure():
   raw = adc_readings()
   smooth = fixed_pt_smooth(raw)
   n = np.arange(len(raw))
   fig, ax = plt.subplots(figsize=(10, 4.6))
   ax.plot(n, raw, color=SKY, linewidth=0.9, label='ADC reading')
   ax.plot(n, smooth, color=SITE_ACCENT, linewidth=2.0,
           label='pot(reading) / pot.gain')
   ax.set_xlim(0, len(raw) - 1)
   ax.set_xlabel('Reading')
   ax.set_ylabel('Counts (12 bit)')
   style(ax)
   ax.legend(loc='upper left', fontsize=9)
   save(fig, 'fixed_pt_leaky_integrator.svg')


if __name__ == '__main__':
   fixed_pt_figure()
