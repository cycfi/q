#!/usr/bin/env python3
"""
Generate the q::pitch_detector reference figure.

Produces, in docs/modules/ROOT/images/:

   pitch_detector_bias.svg -- why pitch_detector needs the bias section. As a
                            note rings, the 2nd harmonic strengthens and its
                            periodicity overtakes the fundamental's. Top: the
                            two candidate periodicities crossing over. Bottom:
                            the reported frequency. Without history, detection
                            flips up an octave at the crossover; the bias keeps
                            it on the true fundamental.

This figure is illustrative (a model of the periodicity evolution), not a run
of the detector, so the curves are smooth idealizations of the effect the
article describes.

Style and palette follow gen_interpolation_figures.py (the canonical PALETTE
lives there).

Usage: /usr/bin/python3 docs/scripts/gen_pitch_detector_figures.py
"""

import os
import numpy as np
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt

SITE_ACCENT = '#1565c0'    # fundamental / biased (correct) result
AMBER = '#fb8c00'          # the competing 2nd-harmonic candidate
SIGNAL_RED = '#e53935'     # the wrong (octave-jumped) result
MAGENTA = '#d81b60'        # event marker: the crossover
GREY = '#5d5d5d'
INK = '#1a1a1a'

OUT_DIR = os.path.join(
   os.path.dirname(__file__), '..', 'modules', 'ROOT', 'images')


def gen_bias():
   t = np.linspace(0, 400, 400)            # ms into the note
   s = t / 400.0
   p_fund = 0.99 - 0.09 * s                 # fundamental: strong, slow decline
   p_oct = 0.84 + 0.14 * s                  # 2nd harmonic: rising
   # crossover
   sc = (0.99 - 0.84) / (0.14 + 0.09)
   tc = sc * 400.0

   f0 = 110.0
   biased = np.full_like(t, f0)
   naive = np.where(t < tc, f0, 2 * f0)

   fig, (ax_p, ax_f) = plt.subplots(
      2, 1, figsize=(10, 7.0), sharex=True,
      gridspec_kw=dict(height_ratios=[1.0, 1.0], hspace=0.18))

   # --- top: candidate periodicities ---
   ax_p.plot(t, p_fund, color=SITE_ACCENT, linewidth=2.0,
             label='fundamental (period P)')
   ax_p.plot(t, p_oct, color=AMBER, linewidth=2.0,
             label='2nd harmonic (period P/2)')
   ax_p.axvline(tc, color=MAGENTA, lw=1.2, ls='--', zorder=1)
   ax_p.plot([tc], [0.99 - 0.09 * sc], 'o', color=MAGENTA, markersize=9, zorder=3)
   ax_p.text(tc + 6, 0.86, 'crossover: the harmonic\nnow scores higher',
             color=MAGENTA, fontsize=10, va='center')
   ax_p.set_ylabel('Periodicity')
   ax_p.set_title('As the note rings, the 2nd harmonic overtakes the fundamental',
                  fontsize=11, color=INK, loc='left')
   ax_p.set_ylim(0.80, 1.005)
   ax_p.legend(loc='lower left')
   ax_p.grid(True, linestyle='--', linewidth=0.5, color='#b0b0b0', alpha=0.8)
   for sp in ('top', 'right'):
      ax_p.spines[sp].set_visible(False)

   # --- bottom: reported frequency ---
   ax_f.plot(t, naive, color=SIGNAL_RED, linewidth=2.0, ls='--',
             label='without bias: jumps up an octave')
   ax_f.plot(t, biased, color=SITE_ACCENT, linewidth=2.4,
             label='with bias: holds the fundamental')
   ax_f.axvline(tc, color=MAGENTA, lw=1.2, ls='--', zorder=1)
   ax_f.annotate('octave error', xy=(tc + 40, 2 * f0),
                 xytext=(tc + 40, 2 * f0 + 14), color=SIGNAL_RED, fontsize=10,
                 ha='center')
   ax_f.set_ylabel('Reported frequency (Hz)')
   ax_f.set_xlabel('Time into the note (ms)')
   ax_f.set_ylim(95, 245)
   ax_f.set_yticks([110, 220])
   ax_f.legend(loc='center left')
   ax_f.grid(True, linestyle='--', linewidth=0.5, color='#b0b0b0', alpha=0.8)
   for sp in ('top', 'right'):
      ax_f.spines[sp].set_visible(False)

   path = os.path.join(OUT_DIR, 'pitch_detector_bias.svg')
   fig.savefig(path, format='svg', bbox_inches='tight')
   plt.close(fig)
   print('wrote', path)


if __name__ == '__main__':
   gen_bias()
