#!/usr/bin/env python3
"""
Generate the q pitch_names reference figure.

Produces, in docs/modules/ROOT/images/:

   pitch_names_ladder.svg  -- the equal-tempered chromatic ladder. Note
                             frequencies plotted on a log axis against an
                             absolute semitone index: equal semitone steps
                             become equal vertical steps, and each octave is
                             an exact doubling. The natural A pitches
                             (A0..A7 = 27.5, 55, ... 3520 Hz) are marked, with
                             A4 = 440 Hz highlighted.

Mirrors q/support/pitch_names.hpp: next_frequency multiplies by the twelfth
root of two (1.059463...); oct_pitch anchors A on powers of two around 440.

Style and palette follow gen_interpolation_figures.py (the canonical
PALETTE lives there).

Usage: python3 docs/scripts/gen_pitch_names_figures.py
"""

import os
import numpy as np
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt

SITE_ACCENT = '#1565c0'    # the chromatic curve
SKY = '#64b5f6'            # note points
AMBER = '#ffb300'          # the A pitches
MAGENTA = '#d81b60'        # A4 = 440 marker (a single reference point)
GRID = '#b0b0b0'
INK = '#1a1a1a'

TWELFTH_ROOT = 1.059463094359295

NAMES = ['A', 'A#', 'B', 'C', 'C#', 'D', 'D#', 'E', 'F', 'F#', 'G', 'G#']

OUT_DIR = os.path.join(
   os.path.dirname(__file__), '..', 'modules', 'ROOT', 'images')

if __name__ == '__main__':
   # Semitone 0 == A0 == 27.5 Hz; step up by the twelfth root of two.
   n = 12 * 7 + 1                          # A0 up to A7 inclusive
   idx = np.arange(n)
   freqs = 27.5 * (TWELFTH_ROOT ** idx)

   fig, (ax, axk) = plt.subplots(
      1, 2, figsize=(10, 6), sharey=True,
      gridspec_kw={'width_ratios': [9, 1.0], 'wspace': 0.04})
   ax.set_yscale('log', base=2)
   ax.grid(True, which='both', linestyle='--', linewidth=0.5, color=GRID,
      alpha=0.8)

   # The chromatic curve: a straight line on the log axis.
   ax.plot(idx, freqs, color=SITE_ACCENT, linewidth=1.4, zorder=2,
      label='chromatic scale (12-TET)')
   ax.scatter(idx, freqs, s=16, color=SKY, zorder=3, edgecolor='white',
      linewidth=0.4)

   # Mark every natural A (semitone index multiple of 12): octave doublings.
   a_idx = np.arange(0, n, 12)
   ax.scatter(a_idx, freqs[a_idx], s=64, color=AMBER, zorder=4,
      edgecolor=INK, linewidth=0.6, label='A pitches (octave doublings)')
   for k, ai in enumerate(a_idx):
      ax.annotate(f'A{k}\n{freqs[ai]:.4g} Hz', xy=(ai, freqs[ai]),
         xytext=(ai + 1.2, freqs[ai] * 0.88), fontsize=8.5, color=INK,
         ha='left', va='top')

   # Highlight A4 = 440 Hz.
   a4 = 48                                 # 4 octaves above A0
   ax.scatter([a4], [freqs[a4]], s=120, color=MAGENTA, zorder=5,
      edgecolor='white', linewidth=1.0, label='A4 = 440 Hz')

   ax.set_xlabel('Absolute semitone index (0 = A0)')
   ax.set_ylabel('Frequency (Hz, log2)')
   ax.set_xlim(-2, n + 6)
   ax.set_ylim(24.0, 4300.0)
   ax.legend(loc='upper left')

   # ---- Piano keyboard aligned to the frequency axis -------------------
   # Each semitone occupies an equal slot on the log axis, so the keyboard
   # reads like a vertical piano roll: white keys for the naturals, shorter
   # black keys for the sharps. Slot i spans a half-semitone either side of
   # its frequency: [f_i * 2^(-1/24), f_i * 2^(+1/24)].
   root = TWELFTH_ROOT
   # Pitch class index with 0 == A. Naturals A B C D E F G are white.
   white_pc = {0, 2, 3, 5, 7, 8, 10}
   for i in range(0, n):
      y_lo = 27.5 * (root ** (i - 0.5))
      y_hi = 27.5 * (root ** (i + 0.5))
      pc = i % 12
      # White key body (drawn for every slot so black keys sit on a white
      # bed, as on a real keyboard). A pitches echo the plot's amber; A4 the
      # plot's magenta.
      if i == a4:
         body = MAGENTA
      elif pc == 0:
         body = AMBER
      else:
         body = '#ffffff'
      axk.axhspan(y_lo, y_hi, xmin=0.0, xmax=1.0, facecolor=body,
         edgecolor='#b8b8b8', linewidth=0.5, zorder=1)
      # Black key: a shorter, dark bar on the inner (left) side, with a small
      # vertical gap so each reads as a discrete key.
      if pc not in white_pc:
         y_lo_b = 27.5 * (root ** (i - 0.40))
         y_hi_b = 27.5 * (root ** (i + 0.40))
         axk.axhspan(y_lo_b, y_hi_b, xmin=0.0, xmax=0.58, facecolor='#1a1a1a',
            edgecolor='#1a1a1a', linewidth=0.4, zorder=3)

   # Clean frame around the keyboard.
   for spine in axk.spines.values():
      spine.set_edgecolor('#888888')
      spine.set_linewidth(1.0)
   axk.set_xlim(0.0, 1.0)
   axk.set_xticks([])
   axk.tick_params(labelleft=False, left=False)
   axk.set_xlabel('keys', fontsize=9, color=INK)

   os.makedirs(OUT_DIR, exist_ok=True)
   out = os.path.join(OUT_DIR, 'pitch_names_ladder.svg')
   fig.savefig(out, bbox_inches='tight', pad_inches=0.1)
   print('wrote', out)
