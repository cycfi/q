#!/usr/bin/env python3
"""
Generate the q::zero_crossing reference figure.

Produces, in docs/modules/ROOT/images/:

   zero_crossing.svg  -- two panels. Left: a signal with the hysteresis band
                         around zero, and the boolean pulse train that
                         zero_crossing emits (high while above zero, released
                         only after the signal clears the lower threshold).
                         Right: the zero_crossing_ex view, marking leading
                         edges, trailing edges, and the peak height of each
                         bounded pulse.

Style and palette follow gen_integrator_figures.py. Event markers are magenta
per the palette (splices, onsets, pitch marks).

Usage: /usr/bin/python3 docs/scripts/gen_zero_crossing_figures.py
"""

import os
import numpy as np
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt

SITE_ACCENT = '#1565c0'    # the input signal
AMBER = '#ffb300'          # hysteresis band
GREEN = '#43a047'          # boolean pulse output
MAGENTA = '#d81b60'        # event markers: leading/trailing edges
GREY = '#5d5d5d'


def zero_crossing(x, hysteresis):
   # Mirror of q::zero_crossing: _hysteresis is negative.
   h = -hysteresis
   state = 0
   out = np.zeros_like(x)
   for i, s in enumerate(x):
      s = s + h / 2
      if not state and s > 0.0:
         state = 1
      elif state and s < h:
         state = 0
      out[i] = state
   return out


def zero_crossing_ex(x, hysteresis):
   # Mirror of q::zero_crossing_ex: returns per-sample event codes and the
   # running peak of the current pulse.
   h = -hysteresis
   state = 0
   leads, trails, peaks = [], [], []
   peak = 0.0
   lead_i = None
   for i, s in enumerate(x):
      s = s + h / 2
      if s > 0.0:
         if not state:
            state = 1
            lead_i = i
            peak = s
         else:
            peak = max(peak, s)
      elif state and s < h:
         state = 0
         trails.append(i)
         leads.append(lead_i)
         peaks.append(peak)
   return leads, trails, peaks


def gen():
   n = 300
   t = np.arange(n)
   hyst = 0.15

   # A decaying tone: amplitude crosses through the hysteresis band, so late
   # pulses illustrate the min-height behavior of the detector.
   sig = np.sin(2 * np.pi * t / 50) * (0.9 * np.exp(-t / 260) + 0.1)

   pulse = zero_crossing(sig, hyst)
   leads, trails, peaks = zero_crossing_ex(sig, hyst)

   fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(10, 4.2))

   # Panel 1: signal, hysteresis band, boolean output.
   ax1.axhspan(-hyst, 0.0, color=AMBER, alpha=0.18, zorder=0,
               label='hysteresis band')
   ax1.plot(t, sig, color=SITE_ACCENT, linewidth=1.6,
            label='input', zorder=2)
   ax1.plot(t, pulse * 0.9, color=GREEN, linewidth=1.8,
            label='zero_crossing (bool)', zorder=3)
   ax1.axhline(0, color='#b0b0b0', linewidth=0.8, zorder=1)
   ax1.axhline(-hyst, color=GREY, linewidth=0.8, linestyle=':', zorder=1)
   ax1.set_title('Pulses at the zero crossings', color='#333333', fontsize=11)

   # Panel 2: zero_crossing_ex edges and per-pulse peak height.
   ax2.plot(t, sig, color=SITE_ACCENT, linewidth=1.6, label='input', zorder=2)
   ax2.axhline(0, color='#b0b0b0', linewidth=0.8, zorder=1)
   for j, (le, te, pk) in enumerate(zip(leads, trails, peaks)):
      ax2.axvline(le, color=MAGENTA, linewidth=1.0, linestyle='--',
                  alpha=0.8, zorder=3,
                  label='leading edge' if j == 0 else None)
      ax2.plot([le, te], [pk, pk], color=GREEN, linewidth=1.6, zorder=4,
               label='pulse height' if j == 0 else None)
   ax2.set_title('zero_crossing_ex: edges and pulse height',
                 color='#333333', fontsize=11)

   for ax in (ax1, ax2):
      ax.set_xlabel('Time (samples)')
      ax.set_xlim(0, n - 1)
      for s in ('top', 'right'):
         ax.spines[s].set_visible(False)
      ax.spines['left'].set_color('#b0b0b0')
      ax.spines['bottom'].set_color('#b0b0b0')
      ax.grid(True, linestyle='--', color='#b0b0b0', alpha=0.5)
      ax.legend(loc='upper right', fontsize=9)

   fig.tight_layout()
   path = os.path.join(OUT_DIR, 'zero_crossing.svg')
   fig.savefig(path, format='svg', bbox_inches=None)
   plt.close(fig)
   print('wrote', path)


OUT_DIR = os.path.join(
   os.path.dirname(__file__), '..', 'modules', 'ROOT', 'images')


if __name__ == '__main__':
   gen()
