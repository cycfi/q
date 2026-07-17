#!/usr/bin/env python3
"""
Generate the q::differentiator reference figure.

Produces, in docs/modules/ROOT/images/:

   differentiator.svg  -- three use-case panels, one per component, each
                          showing what the component is *for* rather than the
                          raw tradeoff:

     1. first_difference  -- edge detection. A gate turns on then off; the
                             first-difference fires a sharp impulse at each
                             transition (zero latency), the classic onset /
                             release detector.
     2. central_difference -- peak picking on a noisy signal. The derivative
                             crosses zero at the peak; central_difference's
                             crossing lands cleanly on the true peak where
                             first_difference's is buried in noise.
     3. slope             -- attack vs decay. Over a window m, slope reads a
                             plucked-note envelope as rising (attack) then
                             falling (decay), a trend classifier that ignores
                             sample-scale wiggle.

Style and palette follow gen_integrator_figures.py.

Usage: /usr/bin/python3 docs/scripts/gen_differentiator_figures.py
"""

import os
import numpy as np
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt

SITE_ACCENT = '#1565c0'    # the input signal
SKY = '#64b5f6'            # first_difference
AMBER = '#ffb300'          # central_difference / slope output
MAGENTA = '#d81b60'        # markers / detections
GREEN = '#2e9e5b'          # attack (slope > 0)
RED = '#e5533c'            # decay (slope < 0)

OUT_DIR = os.path.join(
   os.path.dirname(__file__), '..', 'modules', 'ROOT', 'images')


def first_difference(x):
   y = np.zeros_like(x)
   prev = 0.0
   for i, s in enumerate(x):
      y[i] = s - prev
      prev = s
   return y


def central_difference(x):
   y = np.zeros_like(x)
   d2 = [0.0, 0.0]
   for i, s in enumerate(x):
      y[i] = (s - d2[1]) / 2
      d2[1] = d2[0]
      d2[0] = s
   return y


def slope(x, m):
   y = np.zeros_like(x)
   for i in range(len(x)):
      prev = x[i - m] if i - m >= 0 else 0.0
      y[i] = x[i] - prev
   return y


def gen():
   fig, (ax1, ax2, ax3) = plt.subplots(1, 3, figsize=(13, 4.2))

   # ----------------------------------------------------------------------
   # Panel 1: first_difference as an edge detector.
   # A gate switches on then off; the first-difference impulses mark the
   # onset and release, with no latency.
   # ----------------------------------------------------------------------
   n = 220
   t = np.arange(n)
   gate = 0.5 * (np.tanh((t - 45) / 3.5) - np.tanh((t - 165) / 3.5))
   fd = first_difference(gate)

   ax1.plot(t, gate, color=SITE_ACCENT, linewidth=1.8,
            label='input (gate)', zorder=2)
   ax1.plot(t, fd * 6, color=SKY, linewidth=1.6,
            label='first_difference (x6)', zorder=3)
   i_on = int(np.argmax(fd))
   i_off = int(np.argmin(fd))
   ax1.annotate('onset', xy=(i_on, fd[i_on] * 6), xytext=(i_on + 8, 0.78),
                color=MAGENTA, fontsize=9,
                arrowprops=dict(arrowstyle='->', color=MAGENTA, lw=1.2))
   ax1.annotate('release', xy=(i_off, fd[i_off] * 6),
                xytext=(i_off - 66, -0.92),
                color=MAGENTA, fontsize=9,
                arrowprops=dict(arrowstyle='->', color=MAGENTA, lw=1.2))
   ax1.set_title('first_difference: edge detection', color='#333333',
                 fontsize=11)
   ax1.set_ylim(-1.15, 1.3)

   # ----------------------------------------------------------------------
   # Panel 2: central_difference vs first_difference, frequency response.
   # first_difference's gain climbs monotonically and peaks at Nyquist, so
   # it amplifies high-frequency noise; central_difference's gain rolls off
   # and nulls at Nyquist. Both approximate the ideal derivative |w| at low
   # frequency.
   # ----------------------------------------------------------------------
   f = np.linspace(0, 1, 512)        # frequency as a fraction of Nyquist
   w2 = np.pi * f
   mag_first = 2 * np.sin(w2 / 2)    # |1 - e^-jw|
   mag_central = np.sin(w2)          # |(1 - e^-2jw) / 2|
   mag_ideal = w2                    # |w|, the ideal differentiator

   ax2.plot(f, mag_ideal, color='#9e9e9e', linewidth=1.4,
            linestyle=(0, (4, 3)), label='ideal derivative  |ω|', zorder=2)
   ax2.plot(f, mag_first, color=SKY, linewidth=2.2,
            label='first_difference  2·sin(ω/2)', zorder=3)
   ax2.plot(f, mag_central, color=AMBER, linewidth=2.2,
            label='central_difference  sin ω', zorder=4)
   ax2.plot([1], [2], 'o', color=SKY, markersize=6, zorder=5)
   ax2.plot([1], [0], 'o', color=AMBER, markersize=6, zorder=5)
   ax2.annotate('peaks at Nyquist\n(amplifies HF noise)', xy=(1.0, 2.0),
                xytext=(0.50, 2.45), color='#333333', fontsize=8.5, ha='left',
                arrowprops=dict(arrowstyle='->', color=SKY, lw=1.2))
   ax2.annotate('rolls off,\nnulls Nyquist', xy=(1.0, 0.0),
                xytext=(0.63, 0.58), color='#333333', fontsize=8.5, ha='left',
                arrowprops=dict(arrowstyle='->', color=AMBER, lw=1.2))
   ax2.set_title('central_difference: HF rolloff', color='#333333',
                 fontsize=11)
   ax2.set_xlabel('Frequency (× Nyquist)')
   ax2.set_ylabel('Gain')
   ax2.set_xlim(0, 1)
   ax2.set_ylim(0, 3.35)
   for s in ('top', 'right'):
      ax2.spines[s].set_visible(False)
   ax2.spines['left'].set_color('#b0b0b0')
   ax2.spines['bottom'].set_color('#b0b0b0')
   ax2.grid(True, linestyle='--', color='#b0b0b0', alpha=0.5)
   ax2.legend(loc='upper left', fontsize=8)

   # ----------------------------------------------------------------------
   # Panel 3: slope for attack vs decay over a window m.
   # A plucked-note envelope: slope > 0 during the attack, < 0 during decay.
   # ----------------------------------------------------------------------
   n3 = 260
   t3 = np.arange(n3)
   t0 = 25
   a = np.maximum(t3 - t0, 0)
   env = (1 - np.exp(-a / 13.0)) * np.exp(-a / 95.0)
   env[t3 < t0] = 0.0
   m = 22
   sl = slope(env, m)
   ss = 2.0  # display scale for slope

   ax3.plot(t3, env, color=SITE_ACCENT, linewidth=1.8,
            label='input (pluck envelope)', zorder=3)
   ax3.plot(t3, sl * ss, color=AMBER, linewidth=1.8,
            label=f'slope, m = {m} (x{ss:g})', zorder=4)
   ax3.fill_between(t3, 0, sl * ss, where=(sl > 0), color=GREEN, alpha=0.18,
                    zorder=1, label='attack (slope > 0)')
   ax3.fill_between(t3, 0, sl * ss, where=(sl < 0), color=RED, alpha=0.14,
                    zorder=1, label='decay (slope < 0)')
   ax3.set_title('slope: attack vs decay', color='#333333', fontsize=11)
   ax3.set_ylim(-0.55, 1.35)

   for ax in (ax1, ax3):
      ax.set_xlabel('Time (samples)')
      ax.set_xlim(0, ax.get_lines()[0].get_xdata()[-1])
      ax.axhline(0, color='#b0b0b0', linewidth=0.8, zorder=1)
      for s in ('top', 'right'):
         ax.spines[s].set_visible(False)
      ax.spines['left'].set_color('#b0b0b0')
      ax.spines['bottom'].set_color('#b0b0b0')
      ax.grid(True, linestyle='--', color='#b0b0b0', alpha=0.5)
      ax.legend(loc='upper right', fontsize=8)

   fig.tight_layout()
   path = os.path.join(OUT_DIR, 'differentiator.svg')
   fig.savefig(path, format='svg', bbox_inches=None)
   plt.close(fig)
   print('wrote', path)


if __name__ == '__main__':
   gen()
