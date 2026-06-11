#!/usr/bin/env python3
"""
Generate the sample_interpolation reference figures.

Produces, in docs/modules/ROOT/images/:

   no_interpolation.svg
   linear_interpolation.svg
   cosine_interpolation.svg
   cubic_interpolation.svg
   hermite_interpolation.svg
   bspline_interpolation.svg
   interpolation_quality.svg
   interpolation_cpu.svg

All figures share one style (matplotlib, blue palette below, dashed grid,
Index/Value axes) and one irregular dataset, chosen on purpose: it makes
each type's distinguishing trait visible — cosine's zero slope at the
samples, hermite passing smoothly through them, bspline smoothing past
them. A 0/1 zigzag would render the cubics as near-identical S-curves.

Usage: python3 docs/scripts/gen_interpolation_figures.py
"""

import os
import numpy as np
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt

# -----------------------------------------------------------------------------
# Q docs figure palette (agreed 2026-06-11). Primaries are blue, anchored on
# the docs site accent (#1565c0); accents are reserved for highlighting and
# at most 1-2 appear in any one figure.
# -----------------------------------------------------------------------------
PALETTE = {
   # Primaries (blues), light -> deep
   'powder':         '#bbdefb',
   'baby_blue':      '#90caf9',
   'sky':            '#64b5f6',   # default: sample points
   'soft_sky':       '#7ec8e3',
   'cerulean_light': '#4fc3f7',
   'cyan':           '#26c6da',
   'cornflower':     '#42a5f5',
   'bright_blue':    '#2196f3',
   'strong_blue':    '#1e88e5',
   'azure':          '#1a73e8',
   'ocean':          '#0288d1',
   'deep_ocean':     '#0277bd',
   'site_accent':    '#1565c0',   # default: curves (matches docs UI accent)
   'royal':          '#0d47a1',
   'cobalt_navy':    '#0f4c81',
   'indigo':         '#283593',

   # Accents (highlights & markers)
   'signal_red':     '#e53935',   # errors, overshoot, invalid regions
   'vermilion':      '#f4511e',
   'orange':         '#fb8c00',
   'amber':          '#ffb300',   # secondary data series
   'green':          '#43a047',   # reference / ideal
   'teal':           '#00897b',
   'magenta':        '#d81b60',   # event markers: splices, onsets, pitch marks
   'purple':         '#8e24aa',
}

POINTS_COLOR = PALETTE['sky']
CURVE_COLOR = PALETTE['site_accent']

# Irregular sample set: differences between the types are clearly visible.
# The high-pair/low-pair shape is deliberate: the Lagrange (cubic) mid-
# segment deviation from the chord is ((y1+y2) - (y0+y3))/16, so adjacent
# highs flanked by lows make the cubics visibly curve between samples.
# (Alternating 0/1 data zeroes that term and renders the cubics as chords.)
Y = np.array([0.1, 0.95, 0.9, 0.15, 0.2, 0.85, 0.8, 0.1])
X = np.arange(len(Y))

OUT_DIR = os.path.join(
   os.path.dirname(__file__), '..', 'modules', 'ROOT', 'images')


def cosine(y, i):
   k = int(np.floor(i))
   mu = i - k
   m = (1 - np.cos(mu * np.pi)) / 2
   return y[k] + m * (y[k+1] - y[k])


def hermite(y, i):
   k = int(np.floor(i))
   mu = i - k
   y0, y1, y2, y3 = y[k-1], y[k], y[k+1], y[k+2]
   c1 = 0.5 * (y2 - y0)
   c2 = y0 - 2.5*y1 + 2.0*y2 - 0.5*y3
   c3 = 0.5 * (y3 - y0) + 1.5 * (y1 - y2)
   return ((c3*mu + c2)*mu + c1)*mu + y1


def cubic(y, i):
   k = int(np.floor(i))
   mu = i - k
   y0, y1, y2, y3 = y[k-1], y[k], y[k+1], y[k+2]
   c1 = y2 - y0/3.0 - y1/2.0 - y3/6.0
   c2 = (y0 + y2)/2.0 - y1
   c3 = (y3 - y0)/6.0 + (y1 - y2)/2.0
   return ((c3*mu + c2)*mu + c1)*mu + y1


def bspline(y, i):
   k = int(np.floor(i))
   mu = i - k
   y0, y1, y2, y3 = y[k-1], y[k], y[k+1], y[k+2]
   c0 = (y0 + 4.0*y1 + y2) / 6.0
   c1 = (y2 - y0) * 0.5
   c2 = (y0 - 2.0*y1 + y2) * 0.5
   c3 = (y3 - y0)/6.0 + (y1 - y2)*0.5
   return ((c3*mu + c2)*mu + c1)*mu + c0


def linear(y, i):
   k = int(np.floor(i))
   mu = i - k
   return y[k] + mu * (y[k+1] - y[k])


def new_axes():
   fig, ax = plt.subplots(figsize=(10, 6))
   ax.set_xlabel('Index')
   ax.set_ylabel('Value')
   ax.grid(True, linestyle='--', linewidth=0.5, color='#b0b0b0', alpha=0.8)
   return fig, ax


def save(fig, name):
   path = os.path.join(OUT_DIR, name)
   fig.savefig(path, format='svg', bbox_inches=None)
   plt.close(fig)
   print('wrote', path)


def curve_figure(fn, label, name, lo, hi):
   # lo/hi: the type's valid index range over this dataset
   fig, ax = new_axes()
   xi = np.linspace(lo, hi, 600)
   yi = [fn(Y, i) for i in xi]
   ax.plot(X, Y, 'o', color=POINTS_COLOR, markersize=8,
           label='Original Points', zorder=3)
   ax.plot(xi, yi, color=CURVE_COLOR, linewidth=1.75,
           label=label + ' Interpolation')
   ax.legend(loc='best')
   save(fig, name)


def quality_figure():
   # Worst-case absolute error reading a 1 kHz sine sampled at 48 kHz at
   # fractional offsets -- the same setup as test/interpolation.cpp.
   w = 2 * np.pi * 1000.0 / 48000.0
   n = 512
   y = np.sin(w * np.arange(n))

   def max_err(fn):
      err = 0.0
      for k in range(2, 100):
         for frac in (0.25, 0.5, 0.75):
            i = k + frac
            # Buffer index i refers to sample (n-1) - i: newest first
            expected = np.sin(w * (n - 1 - i))
            err = max(err, abs(fn(y[::-1], i) - expected))
      return err

   def none_(y, i):
      return y[int(np.floor(i))]

   types = [
      ('none',     none_),
      ('cosine',   cosine),
      ('linear',   linear),
      ('bspline',  bspline),
      ('cubic',    cubic),
      ('hermite',  hermite),
   ]
   results = sorted(
      ((name, max_err(fn)) for name, fn in types),
      key=lambda r: r[1], reverse=True)
   names = [r[0] for r in results]
   errs = [r[1] for r in results]

   fig, ax = plt.subplots(figsize=(10, 6))
   ax.barh(names, errs, color=CURVE_COLOR)
   ax.set_xscale('log')
   ax.invert_yaxis()
   ax.set_xlabel('Worst-case error (1 kHz sine, 48 kHz sampling, log scale)')
   ax.grid(True, axis='x', linestyle='--', linewidth=0.5,
           color='#b0b0b0', alpha=0.8)
   for name_, err in zip(names, errs):
      ax.text(err * 1.2, name_, f'{err:.2e}', va='center',
              color='#333333', fontsize=10)
   ax.set_xlim(right=max(errs) * 30)
   save(fig, 'interpolation_quality.svg')


def cpu_figure():
   # Measured by docs/scripts/bench_interpolation.cpp (see its header for
   # method): ns per interpolated read, 1024-sample buffer, random
   # fractional indices. Apple M2, Apple clang 21, -O3. 2026-06-11.
   # cosine is computed via the quarter-wave folded sin lookup table
   # (2.63 with std::cos; 1.18 with the old full-cycle table -- the fold
   # costs ~0.2 ns here and buys a 4x smaller table).
   measured = {
      'none':     0.459,
      'linear':   0.630,
      'cosine':   1.385,
      'cubic':    1.982,
      'hermite':  1.417,
      'bspline':  1.801,
   }
   results = sorted(measured.items(), key=lambda r: r[1], reverse=True)
   names = [r[0] for r in results]
   costs = [r[1] for r in results]

   fig, ax = plt.subplots(figsize=(10, 6))
   ax.barh(names, costs, color=CURVE_COLOR)
   ax.invert_yaxis()
   ax.set_xlabel('ns per interpolated read (Apple M2, clang -O3)')
   ax.grid(True, axis='x', linestyle='--', linewidth=0.5,
           color='#b0b0b0', alpha=0.8)
   for name_, cost in zip(names, costs):
      ax.text(cost + 0.05, name_, f'{cost:.2f} ns', va='center',
              color='#333333', fontsize=10)
   ax.set_xlim(right=max(costs) * 1.25)
   save(fig, 'interpolation_cpu.svg')


def none_figure():
   # Zero-order hold: the value holds until the next integer index
   fig, ax = new_axes()
   ax.plot(X, Y, 'o', color=POINTS_COLOR, markersize=8,
           label='Original Points', zorder=3)
   ax.step(X, Y, where='post', color=CURVE_COLOR, linewidth=1.75,
           label='No Interpolation')
   ax.legend(loc='best')
   save(fig, 'no_interpolation.svg')


if __name__ == '__main__':
   n = len(Y)
   # 2-point types: [0, size-2]; 4-point types: [1, size-3]
   none_figure()
   curve_figure(linear, 'Linear', 'linear_interpolation.svg', 0, n-2)
   curve_figure(cosine, 'Cosine', 'cosine_interpolation.svg', 0, n-2)
   curve_figure(cubic, 'Cubic', 'cubic_interpolation.svg', 1, n-3)
   curve_figure(hermite, 'Hermite', 'hermite_interpolation.svg', 1, n-3)
   curve_figure(bspline, 'B-spline', 'bspline_interpolation.svg', 1, n-3)
   quality_figure()
   cpu_figure()
