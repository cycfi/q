#!/usr/bin/env python3
"""
Generate the q::peak_picker reference figure.

Produces, in docs/modules/ROOT/images/:

   peak_picker.svg  -- an exponentially decaying sine, with the bare picker
                       (no qualifiers) marking one local maximum per cycle at
                       the exact crest.

The picker here mirrors the C++ core: a first-difference strict sign change,
reporting the apex one sample after the peak.

Style and palette follow gen_interpolation_figures.py.

Usage: /usr/bin/python3 docs/scripts/gen_peak_picker_figures.py
"""

import os
import numpy as np
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt

SITE_ACCENT = '#1565c0'    # the signal
MAGENTA = '#d81b60'        # peaks
GREY = '#5d5d5d'
LIGHT = '#9e9e9e'          # rejected local maxima
AMBER = '#ef6c00'          # a threshold / bar

OUT_DIR = os.path.join(
   os.path.dirname(__file__), '..', 'modules', 'ROOT', 'images')


def pick_maxima(sig):
   """peak_picker core: a strict first-difference sign change. Returns
   (apex_index, apex_value) for each local maximum."""
   prev = 0.0
   rising = False
   out = []
   for i, s in enumerate(sig):
      d = s - prev
      if rising and d < 0.0:
         out.append((i - 1, prev))
      if d > 0.0:
         rising = True
      elif d < 0.0:
         rising = False
      prev = s
   return out


def gen():
   period = 64
   cycles = 8
   n = period * cycles
   i = np.arange(n)
   sig = np.exp(-1.8 * i / n) * np.sin(2 * np.pi * i / period)

   picks = pick_maxima(sig)
   px = np.array([p[0] for p in picks], dtype=float)
   py = np.array([p[1] for p in picks], dtype=float)

   fig, ax = plt.subplots(figsize=(10, 4))

   ax.plot(i, sig, color=SITE_ACCENT, linewidth=1.5, label='signal', zorder=2)
   ax.vlines(px, 0.0, py, color=MAGENTA, linewidth=1.4, zorder=3)
   ax.plot(px, py, 'o', markersize=8, color=MAGENTA,
           markeredgecolor='white', markeredgewidth=0.8, label='peak',
           zorder=4)

   ax.axhline(0, color=GREY, lw=0.8, zorder=1)
   ax.set_xlabel('Time (samples)')
   ax.set_yticks([-1, 0, 1])
   ax.set_xlim(0, n - 1)
   ax.set_ylim(-1.1, 1.25)
   for sp in ('top', 'right', 'left'):
      ax.spines[sp].set_visible(False)
   ax.spines['bottom'].set_color('#b0b0b0')
   ax.legend(loc='upper right', fontsize=9)

   fig.tight_layout()
   path = os.path.join(OUT_DIR, 'peak_picker.svg')
   fig.savefig(path, format='svg', bbox_inches=None)
   plt.close(fig)
   print('wrote', path)
   print('peaks:', len(picks))


def run_positive(sig):
   """peak_picker core + peak_gate at level 0: keep only above-zero maxima."""
   prev = 0.0
   rising = False
   all_maxima, kept = [], []
   for i, s in enumerate(sig):
      d = s - prev
      if rising and d < 0.0:
         all_maxima.append((i - 1, prev))
         if prev > 0.0:
            kept.append((i - 1, prev))
      if d > 0.0:
         rising = True
      elif d < 0.0:
         rising = False
      prev = s
   return all_maxima, kept


def gen_positive():
   # A real window of a conditioned guitar staccato note (the `cond` column of
   # the peak_picker test's GStaccato output), one value per sample. The picker
   # runs on it here exactly as in C++, so the marks are the real ones: one
   # dominant positive crest per cycle, with a shallow maximum down in each
   # negative excursion. peak_gate at level 0 keeps the crests, drops the negatives.
   data = os.path.join(os.path.dirname(__file__), 'gstaccato_cond.txt')
   cond = np.loadtxt(data)
   i = np.arange(len(cond))

   all_maxima, kept = run_positive(cond)
   ax_all = np.array([p[0] for p in all_maxima], dtype=float)
   ay_all = np.array([p[1] for p in all_maxima], dtype=float)
   kx = np.array([p[0] for p in kept], dtype=float)
   ky = np.array([p[1] for p in kept], dtype=float)

   fig, ax = plt.subplots(figsize=(10, 4))
   ax.plot(i, cond, color=SITE_ACCENT, linewidth=1.5,
           label='conditioned signal', zorder=2)
   ax.plot(ax_all, ay_all, 'o', markersize=6, markerfacecolor='none',
           markeredgecolor=LIGHT, markeredgewidth=1.3, label='local maxima',
           zorder=3)
   ax.vlines(kx, 0.0, ky, color=MAGENTA, linewidth=1.4, zorder=3)
   ax.plot(kx, ky, 'o', markersize=8, color=MAGENTA, markeredgecolor='white',
           markeredgewidth=0.8, label='kept (peak_gate, level 0)', zorder=4)

   ax.axhline(0, color=GREY, lw=0.8, zorder=1)
   ax.set_xlabel('Time (samples)')
   ax.set_yticks([-0.5, 0, 0.5])
   ax.set_xlim(0, len(cond) - 1)
   for sp in ('top', 'right', 'left'):
      ax.spines[sp].set_visible(False)
   ax.spines['bottom'].set_color('#b0b0b0')
   ax.legend(loc='upper right', fontsize=9)

   fig.tight_layout()
   path = os.path.join(OUT_DIR, 'peak_picker_positive.svg')
   fig.savefig(path, format='svg', bbox_inches=None)
   plt.close(fig)
   print('wrote', path)
   print('local maxima:', len(all_maxima), ' kept:', len(kept))


def run_min_slope(sig, threshold, m):
   """peak_picker core + min_slope: keep a peak only if the rise over the last
   m samples (a q::slope window) clears threshold."""
   prev = 0.0
   rising = False
   all_maxima, kept = [], []
   for i, s in enumerate(sig):
      d = s - prev
      if rising and d < 0.0:
         all_maxima.append((i - 1, prev))
         rise = s - (sig[i - m] if i >= m else 0.0)
         if rise >= threshold:
            kept.append((i - 1, prev))
      if d > 0.0:
         rising = True
      elif d < 0.0:
         rising = False
      prev = s
   return all_maxima, kept


def gen_min_slope():
   sps = 48000.0
   n = 2800
   i = np.arange(n)
   # A crossfade from a low, slow wave into a high, fast one, centered on a
   # shared crest so the transition sits at a peak (slope zero on both sides).
   # A smooth weight, not a hard splice, so there is no seam; the crest simply
   # grows from slow to fast.
   f_slow = 70.0
   isp = int(round(sps / (4 * f_slow) + 2 * sps / f_slow))   # a slow-wave crest
   slow = 0.2 * np.sin(2 * np.pi * f_slow * i / sps)         # low, slow
   fast = 1.0 * np.cos(2 * np.pi * 300.0 * (i - isp) / sps)  # high, fast; peaks at isp
   half = 170.0
   w = np.clip((i - (isp - half)) / (2 * half), 0.0, 1.0)
   w = 3.0 * w ** 2 - 2.0 * w ** 3                           # smoothstep crossfade
   sig = (1.0 - w) * slow + w * fast

   m = int(0.5e-3 * sps)                            # 0.5 ms slope window
   all_maxima, kept = run_min_slope(sig, threshold=0.15, m=m)
   ax_all = np.array([p[0] for p in all_maxima], dtype=float)
   ay_all = np.array([p[1] for p in all_maxima], dtype=float)
   kx = np.array([p[0] for p in kept], dtype=float)
   ky = np.array([p[1] for p in kept], dtype=float)

   fig, ax = plt.subplots(figsize=(10, 4))
   ax.plot(i, sig, color=SITE_ACCENT, linewidth=1.2, label='signal', zorder=2)
   ax.plot(ax_all, ay_all, 'o', markersize=6, markerfacecolor='none',
           markeredgecolor=LIGHT, markeredgewidth=1.3, label='local maxima',
           zorder=3)
   ax.plot(kx, ky, 'o', markersize=8, color=MAGENTA, markeredgecolor='white',
           markeredgewidth=0.8, label='kept (peak_min_slope)', zorder=4)

   ax.axhline(0, color=GREY, lw=0.8, zorder=1)
   ax.set_xlabel('Time (samples)')
   ax.set_yticks([-1, 0, 1])
   ax.set_xlim(0, n - 1)
   ax.set_ylim(-1.15, 1.4)
   for sp in ('top', 'right', 'left'):
      ax.spines[sp].set_visible(False)
   ax.spines['bottom'].set_color('#b0b0b0')
   ax.legend(loc='upper center', fontsize=9, ncol=3)

   fig.tight_layout()
   path = os.path.join(OUT_DIR, 'peak_picker_min_slope.svg')
   fig.savefig(path, format='svg', bbox_inches=None)
   plt.close(fig)
   print('wrote', path)
   print('local maxima:', len(all_maxima), ' kept:', len(kept))


def run_z_score(sig, threshold, influence, lag):
   """peak_picker core + peak_z_score (Brakel): keep an apex only if it stands
   `threshold` deviations above a trailing mean; `influence` damps how much a
   flagged sample feeds the baseline."""
   ring = np.zeros(lag)
   ring_sq = np.zeros(lag)
   idx = 0
   s_sum = 0.0
   sq_sum = 0.0
   prev_f = 0.0
   prev = 0.0
   rising = False
   all_maxima, kept = [], []
   for j, s in enumerate(sig):
      mean = s_sum / lag
      sd = np.sqrt(max(sq_sum / lag - mean * mean, 0.0))
      bar = mean + threshold * sd
      d = s - prev
      if rising and d < 0.0:
         all_maxima.append((j - 1, prev))
         if prev > bar:
            kept.append((j - 1, prev))
      if d > 0.0:
         rising = True
      elif d < 0.0:
         rising = False
      prev = s
      f = influence * s + (1.0 - influence) * prev_f if s > bar else s
      prev_f = f
      s_sum += f - ring[idx]
      ring[idx] = f
      sq_sum += f * f - ring_sq[idx]
      ring_sq[idx] = f * f
      idx = (idx + 1) % lag
   return all_maxima, kept


def gen_z_score():
   # A band-limited noise floor with a few louder events on top. z_score
   # thresholds against the running noise level, so the floor is rejected and
   # the events kept. A warm-up prefix (not shown) fills the trailing window.
   warm, body = 500, 2800
   n = warm + body
   rng = np.random.RandomState(4)
   noise = np.convolve(rng.randn(n), np.ones(7) / 7, mode='same')
   noise *= 0.05 / noise.std()
   sig = noise.copy()
   for c, h in [(360, 0.8), (760, 0.62), (1230, 0.95),
                (1700, 0.7), (2160, 0.88), (2520, 0.55)]:
      for k in range(-16, 17):
         sig[warm + c + k] += h * 0.5 * (1.0 + np.cos(np.pi * k / 16))

   all_maxima, kept = run_z_score(sig, 4.5, 0.02, 400)
   all_maxima = [(x - warm, y) for x, y in all_maxima if x >= warm]
   kept = [(x - warm, y) for x, y in kept if x >= warm]
   sig = sig[warm:]
   i = np.arange(len(sig))

   fig, ax = plt.subplots(figsize=(10, 4))
   ax.plot(i, sig, color=SITE_ACCENT, linewidth=1.0, label='signal', zorder=2)
   ax.plot([p[0] for p in all_maxima], [p[1] for p in all_maxima], 'o',
           markersize=5, markerfacecolor='none', markeredgecolor=LIGHT,
           markeredgewidth=1.0, label='local maxima', zorder=3)
   kx = [p[0] for p in kept]
   ky = [p[1] for p in kept]
   ax.vlines(kx, 0.0, ky, color=MAGENTA, linewidth=1.3, zorder=3)
   ax.plot(kx, ky, 'o', markersize=8, color=MAGENTA, markeredgecolor='white',
           markeredgewidth=0.8, label='kept (peak_z_score)', zorder=4)

   ax.axhline(0, color=GREY, lw=0.8, zorder=1)
   ax.set_xlabel('Time (samples)')
   ax.set_yticks([0, 0.5, 1.0])
   ax.set_xlim(0, len(sig) - 1)
   ax.set_ylim(-0.3, 1.15)
   for sp in ('top', 'right', 'left'):
      ax.spines[sp].set_visible(False)
   ax.spines['bottom'].set_color('#b0b0b0')
   ax.legend(loc='upper right', fontsize=9)

   fig.tight_layout()
   path = os.path.join(OUT_DIR, 'peak_picker_z_score.svg')
   fig.savefig(path, format='svg', bbox_inches=None)
   plt.close(fig)
   print('wrote', path)
   print('local maxima:', len(all_maxima), ' kept:', len(kept))


def run_rms_gate(sig, ratio, window):
   """peak_picker core + peak_gate on RMS: keep an apex only if it is at least
   `ratio` times the running RMS over `window`. Returns (all_maxima, kept, bar)."""
   sq = sig * sig
   c = np.cumsum(np.insert(sq, 0, 0))
   ms = np.empty(len(sig))
   for i in range(len(sig)):
      lo = max(0, i + 1 - window)
      ms[i] = (c[i + 1] - c[lo]) / window          # boxcar mean square
   bar = ratio * np.sqrt(ms)
   prev = 0.0
   rising = False
   all_maxima, kept = [], []
   for i, s in enumerate(sig):
      d = s - prev
      if rising and d < 0.0:
         all_maxima.append((i - 1, prev))
         if prev >= bar[i - 1]:
            kept.append((i - 1, prev))
      if d > 0.0:
         rising = True
      elif d < 0.0:
         rising = False
      prev = s
   return all_maxima, kept, bar


def gen_rms_gate():
   # A real window of a conditioned guitar note (1a-Low-E), harmonically rich
   # so each cycle has a fundamental crest and strong harmonic humps. The RMS
   # over one fundamental period is a stable level; the crest clears twice it,
   # the humps do not, so the gate keeps one landmark per cycle without creep.
   data = os.path.join(os.path.dirname(__file__), 'low_e_cond.txt')
   cond = np.loadtxt(data)
   period = 527                                     # one period of low E in this file

   all_maxima, kept, bar = run_rms_gate(cond, 2.0, period)
   i = np.arange(len(cond))

   fig, ax = plt.subplots(figsize=(10, 4))
   ax.plot(i, cond, color=SITE_ACCENT, linewidth=1.2,
           label='conditioned signal', zorder=2)
   ax.plot(i, bar, color=AMBER, linewidth=1.1, linestyle='--',
           label='2 x RMS (one period)', zorder=2)
   ax.plot([p[0] for p in all_maxima], [p[1] for p in all_maxima], 'o',
           markersize=5, markerfacecolor='none', markeredgecolor=LIGHT,
           markeredgewidth=1.1, label='local maxima', zorder=3)
   kx = [p[0] for p in kept]
   ky = [p[1] for p in kept]
   ax.vlines(kx, 0.0, ky, color=MAGENTA, linewidth=1.3, zorder=3)
   ax.plot(kx, ky, 'o', markersize=8, color=MAGENTA, markeredgecolor='white',
           markeredgewidth=0.8, label='kept (peak_gate on RMS)', zorder=4)

   ax.axhline(0, color=GREY, lw=0.8, zorder=1)
   ax.set_xlabel('Time (samples)')
   ax.set_yticks([-0.5, 0, 0.5])
   ax.set_xlim(0, len(cond) - 1)
   for sp in ('top', 'right', 'left'):
      ax.spines[sp].set_visible(False)
   ax.spines['bottom'].set_color('#b0b0b0')
   # Above the axes, single row: the crests fill the upper right, so an in-plot
   # legend obscures the data.
   ax.legend(loc='lower center', bbox_to_anchor=(0.5, 1.01), ncol=4,
             fontsize=9, frameon=False)

   fig.tight_layout()
   path = os.path.join(OUT_DIR, 'peak_picker_rms_gate.svg')
   fig.savefig(path, format='svg', bbox_inches='tight')
   plt.close(fig)
   print('wrote', path)
   print('local maxima:', len(all_maxima), ' kept:', len(kept))


if __name__ == '__main__':
   gen()
   gen_positive()
   gen_min_slope()
   gen_z_score()
   gen_rms_gate()
