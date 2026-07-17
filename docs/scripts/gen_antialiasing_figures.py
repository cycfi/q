#!/usr/bin/env python3
"""
Generate the q antialiasing (PolyBLEP / PolyBLAMP) reference figures.

Produces, in docs/modules/ROOT/images/:

   antialias_waveforms.svg  -- time-domain overlay, the classic PolyBLEP view:
                               the plain waveform (green), the correction
                               residual (blue dashed), and their sum, the
                               band-limited output (red), drawn as continuous
                               curves at a high frequency (dt = 0.25) so the
                               rounded discontinuity is plainly visible. Top:
                               saw with poly_blep. Bottom: triangle with
                               poly_blamp.
   antialias_spectrum.svg   -- stacked log-frequency spectra of a band-unlimited
                               basic_saw (top) vs the PolyBLEP band-limited saw
                               (bottom). Harmonics past Nyquist (= fs/2) fold
                               back down as aliases: every peak that is not on a
                               true-harmonic marker is a fold. PolyBLEP pushes
                               them down. Leakage-free (exact-periodic, no
                               window) so the partials are real, not skirt.

The residual formulas mirror q/utility/antialiasing.hpp exactly, in
normalized phase (t in [0, 1)) with dt = freq / sps.

Style and palette follow gen_interpolation_figures.py (the canonical
PALETTE lives there).

Usage: python3 docs/scripts/gen_antialiasing_figures.py
"""

import os
import numpy as np
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt

SITE_ACCENT = '#1565c0'    # curves / band-limited
AMBER = '#ffb300'          # second series
GREEN = '#43a047'          # reference / ideal
SIGNAL_RED = '#e53935'     # not band-limited / aliasing
GRID = '#b0b0b0'

OUT_DIR = os.path.join(
   os.path.dirname(__file__), '..', 'modules', 'ROOT', 'images')


def poly_blep(p, dt):
   """Normalized PolyBLEP residual, mirroring antialiasing.hpp."""
   out = np.zeros_like(p)
   lo = p < dt
   hi = p > (1.0 - dt)
   t = p[lo] / dt
   out[lo] = t + t - t * t - 1.0
   t = -(1.0 - p[hi]) / dt
   out[hi] = t * t + t + t + 1.0
   return out


def poly_blamp(p, dt, scale):
   """Normalized PolyBLAMP residual, mirroring antialiasing.hpp."""
   out = np.zeros_like(p)
   lo = p < dt
   hi = p > (1.0 - dt)
   t = (p[lo] / dt) - 1.0
   out[lo] = -scale / 3.0 * dt * t * t * t
   t = -((1.0 - p[hi]) / dt) + 1.0
   out[hi] = scale / 3.0 * dt * t * t * t
   return out


def basic_triangle(p):
   """Normalized basic_triangle_osc: -1 at 0, +1 at 0.5, -1 at 1."""
   p = p % 1.0
   return np.where(p <= 0.5, 4.0 * p - 1.0, 3.0 - 4.0 * p)


def waveforms_figure():
   # High frequency (dt = 0.25, a quarter of the cycle per sample) so the
   # correction spans a visible fraction of the cycle. Plotted as continuous
   # curves: this is the analog shape the oscillator emits, before sampling.
   dt = 0.25
   scale = 4.0                     # poly_blamp scale for a triangle
   cycles = 2.0
   ph = np.linspace(0.0, cycles, 4001)
   pc = ph % 1.0                    # phase within the cycle

   # Saw: band-limited = plain - poly_blep at the wrap (mirrors saw_osc).
   # Plot the applied correction (band - plain) so "plain + correction =
   # output" holds in both panels.
   saw_plain = 2.0 * pc - 1.0
   saw_band = saw_plain - poly_blep(pc, dt)
   saw_res = saw_band - saw_plain

   # Triangle: naive part and two corner corrections (mirrors triangle_osc,
   # which evaluates at p + edge).
   edge1, edge2 = 0.25, 0.75
   tri_plain = basic_triangle(ph + edge1)
   tri_band = (tri_plain
               + poly_blamp((ph + edge1) % 1.0, dt, scale)
               - poly_blamp((ph + edge2) % 1.0, dt, scale))
   tri_res = tri_band - tri_plain

   fig, (ax_saw, ax_tri) = plt.subplots(2, 1, sharex=True, figsize=(10, 7))

   panels = [
      (ax_saw, saw_plain, saw_res, saw_band,
       'Saw:  poly_blep corrects the wrap (a step)'),
      (ax_tri, tri_plain, tri_res, tri_band,
       'Triangle:  poly_blamp corrects each peak (a corner)'),
   ]
   for ax, plain, res, band, label in panels:
      ax.grid(True, linestyle='--', linewidth=0.5, color=GRID, alpha=0.8)
      ax.axhline(0.0, color='#333333', linewidth=0.7)
      ax.plot(ph, plain, color=GREEN, linewidth=1.6, label='plain (aliased)')
      ax.plot(ph, res, color=SITE_ACCENT, linewidth=1.6, linestyle='--',
              label='correction residual')
      ax.plot(ph, band, color=SIGNAL_RED, linewidth=2.0,
              label='band-limited output')
      ax.set_ylim(-1.6, 1.6)
      ax.set_ylabel('Amplitude')
      ax.text(0.99, 0.96, label, transform=ax.transAxes, ha='right',
              va='top', fontsize=10, color='#333333')

   ax_saw.legend(loc='lower right', fontsize=9, ncol=3)
   ax_tri.set_xlabel('Phase (cycles)')
   ax_saw.set_title(
      'PolyBLEP in the time domain (dt = 0.25): the plain waveform plus the '
      'residual gives a rounded, band-limited edge', fontsize=11)

   fig.tight_layout()
   path = os.path.join(OUT_DIR, 'antialias_waveforms.svg')
   fig.savefig(path, format='svg', bbox_inches=None)
   print('wrote', path)


def spectrum_figure():
   import matplotlib.ticker as mticker

   sr = 44100.0                            # fold point is Nyquist = sr/2
   nyquist = sr / 2.0
   # Exact periodicity, so no spectral leakage: dt = a/b is an exact fraction
   # (integer phase steps) and the buffer holds a whole number of b-sample
   # cycles, so no window is needed and every empty bin is truly empty. The
   # period b must not divide the samples-per-cycle evenly, or aliases would
   # fold onto harmonics and hide. dt = 4/401 is a 439.9 Hz fundamental (A4, a
   # realistic synth note) with a 401-sample period.
   a, b = 4, 401
   dt = a / b
   f = sr * a / b                          # 439.9 Hz (A4)
   n_fft = b * 200                          # 80200 samples = 200 whole cycles
   n = np.arange(n_fft)
   p = ((n * a) % b) / b                    # exact rational phase, no drift
   plain = 2.0 * p - 1.0                    # basic_saw, not band-limited
   band = plain - poly_blep(p, dt)          # saw, PolyBLEP band-limited

   def spectrum(x):
      mag = np.abs(np.fft.rfft(x))          # rectangular window (exact periodic)
      mag /= mag.max()
      return 20.0 * np.log10(np.maximum(mag, 1e-9))

   freqs = np.fft.rfftfreq(n_fft, 1.0 / sr)
   sp = spectrum(plain)
   sb = spectrum(band)
   harmonics = np.arange(f, nyquist, f)     # true harmonics below Nyquist
   floor = -84.0

   def harmonic_peaks(s):
      return np.array([s[np.argmin(np.abs(freqs - h))] for h in harmonics])

   # Stacked panels: band-unlimited on top, band-limited below, on a log
   # frequency axis (the analyzer view) so the folded aliases below the
   # fundamental are visible.
   fig, (ax_top, ax_bot) = plt.subplots(2, 1, sharex=True, figsize=(11, 7.4))

   panels = [
      (ax_top, sp, SIGNAL_RED, 'basic_saw  (not band-limited)'),
      (ax_bot, sb, SITE_ACCENT, 'saw  (band-limited, PolyBLEP)'),
   ]
   for ax, s, color, label in panels:
      vis = s > floor
      ax.fill_between(freqs, floor, s, color=color, alpha=0.25, linewidth=0)
      ax.vlines(freqs[vis], floor, s[vis], color=color, linewidth=0.7)
      ax.axvline(nyquist, color='#333333', linewidth=1.4)
      ax.text(nyquist * 0.985, floor + 4, 'Nyquist = fs/2 = 22.05 kHz',
              rotation=90, ha='right', va='bottom', fontsize=8.5,
              color='#333333')
      ax.plot(harmonics, harmonic_peaks(s), linestyle='none', marker='o',
              markersize=3, color=GREEN, zorder=5)
      ax.set_xscale('log')
      ax.set_xlim(60, nyquist)
      ax.set_ylim(floor, 6)
      ax.set_ylabel('Magnitude (dB)')
      ax.grid(True, which='major', linestyle='--', linewidth=0.4, color=GRID,
              alpha=0.6)
      ax.text(0.03, 0.94, label, transform=ax.transAxes, ha='left', va='top',
              fontsize=10.5, color=color, fontweight='bold')
      ax.xaxis.set_major_formatter(mticker.FuncFormatter(
         lambda x, _: ('%gk' % (x / 1000)) if x >= 1000 else '%g' % x))

   ax_top.plot([], [], linestyle='none', marker='o', markersize=3, color=GREEN,
               label='true harmonics (A4 = 440 Hz)')
   ax_top.legend(loc='lower right', fontsize=8.5)
   ax_bot.set_xlabel('Frequency (Hz, log)')
   ax_top.set_title(
      'Sawtooth at 440 Hz (A4), fs = 44.1 kSPS: harmonics past Nyquist fold '
      'back down as aliases (every peak off a green dot)', fontsize=10)

   fig.tight_layout()
   path = os.path.join(OUT_DIR, 'antialias_spectrum.svg')
   fig.savefig(path, format='svg', bbox_inches=None)
   print('wrote', path)


if __name__ == '__main__':
   waveforms_figure()
   spectrum_figure()
