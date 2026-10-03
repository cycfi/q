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
   peak_offset.svg
   lagrange6_accuracy.svg
   zero_projection.svg

All figures share one style (matplotlib, blue palette below, dashed grid,
Index/Value axes) and one irregular dataset, chosen on purpose: it makes
each type's distinguishing trait visible: cosine's zero slope at the
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
   # fractional offsets -- the same setup as test/utility/interpolation.cpp.
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
   # Measured by test/benchmark/interpolation_bench.cpp (see its header for
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


def peak_offset_figure():
   # The inverse problem: the samples bracket a maximum, but the maximum
   # itself falls between them. The lobe is sampled so its true peak sits
   # at a deliberately non-integer position; the integer maximum is then
   # the wrong answer, and the parabola through its two neighbours recovers
   # the remaining fraction of a sample.
   peak_at = 3.35
   width = 2.1

   def signal(t):
      return np.exp(-(((t - peak_at) / width) ** 2))

   xs = np.arange(8)
   ys = signal(xs.astype(float))

   k = int(np.argmax(ys))                 # the integer maximum
   y0, y1, y2 = ys[k-1], ys[k], ys[k+1]
   d = y0 - 2.0*y1 + y2
   delta = 0.5 * (y0 - y2) / d            # peak_offset
   a, b = 0.5 * d, 0.5 * (y2 - y0)
   vertex_y = a*delta**2 + b*delta + y1

   fig, ax = new_axes()

   tc = np.linspace(k - 2, k + 2, 400)
   ax.plot(tc, signal(tc), color=PALETTE['powder'], linewidth=2.0,
           label='Underlying signal', zorder=1)

   tp = np.linspace(k - 1.3, k + 1.3, 200)
   ax.plot(tp, a*(tp - k)**2 + b*(tp - k) + y1, color=CURVE_COLOR,
           linewidth=1.75, label='Parabola through y0, y1, y2', zorder=2)

   ax.plot([k-1, k, k+1], [y0, y1, y2], 'o', color=POINTS_COLOR,
           markersize=11, label='y0, y1, y2', zorder=4)
   for xi, yi, nm in ((k-1, y0, 'y0'), (k, y1, 'y1'), (k+1, y2, 'y2')):
      ax.annotate(nm, (xi, yi), textcoords='offset points', xytext=(0, 12),
                  ha='center', color=PALETTE['royal'], fontsize=11)

   ax.axvline(k, color=PALETTE['signal_red'], linestyle=':', linewidth=1.4,
              zorder=2)
   ax.plot([peak_at], [1.0], 'x', color=PALETTE['green'], markersize=13,
           markeredgewidth=2.4, label='True peak', zorder=5)
   ax.plot([k + delta], [vertex_y], '*', color=PALETTE['magenta'],
           markersize=20, label='Located peak', zorder=6)

   span = max(1.0, vertex_y) - min(y0, y2)
   lo, hi = min(y0, y2) - 0.10*span, max(1.0, vertex_y) + 0.10*span
   ay = y1 - 0.10*span
   ax.annotate('', xy=(k + delta, ay), xytext=(k, ay),
               arrowprops=dict(arrowstyle='<->', color=PALETTE['magenta'],
                               linewidth=1.6))
   ax.text(k + delta/2, ay - 0.012*span,
           f'peak_offset = {delta:+.2f} samples',
           ha='center', va='top', color=PALETTE['magenta'], fontsize=11)
   ax.text(k - 0.03, hi - 0.012*span, 'integer maximum ', ha='right',
           va='top', color=PALETTE['signal_red'], fontsize=10)

   ax.set_xlim(k - 1.45, k + 1.45)
   ax.set_ylim(lo, hi)
   ax.set_xticks([k-1, k, k+1])
   ax.legend(loc='lower center', fontsize=9)
   save(fig, 'peak_offset.svg')


# The 4- and 6-point primitives as q/utility/interpolation_primitives.hpp
# writes them: mu in [0, 1) between the middle pair.
def cubic_p(y0, y1, y2, y3, mu):
   c1 = y2 - y0/3 - y1/2 - y3/6
   c2 = (y0 + y2)/2 - y1
   c3 = (y3 - y0)/6 + (y1 - y2)/2
   return ((c3*mu + c2)*mu + c1)*mu + y1


def hermite_p(y0, y1, y2, y3, mu):
   c1 = (y2 - y0) * 0.5
   c2 = y0 - 2.5*y1 + 2*y2 - 0.5*y3
   c3 = (y3 - y0) * 0.5 + (y1 - y2) * 1.5
   return ((c3*mu + c2)*mu + c1)*mu + y1


def lagrange6_p(y0, y1, y2, y3, y4, y5, mu):
   c1 = y0/20 - y1/2 - y2/3 + y3 - y4/4 + y5/30
   c2 = (16*(y1 + y3) - (y0 + y4))/24 - 1.25*y2
   c3 = (10*y2 - 14*y3 + 7*y4 - y0 - y1 - y5)/24
   c4 = (y0 + y4)/24 - (y1 + y3)/6 + y2/4
   c5 = (y1 - y4)/24 + (y3 - y2)/12 + (y5 - y0)/120
   return ((((c5*mu + c4)*mu + c3)*mu + c2)*mu + c1)*mu + y2


def lagrange6_figure():
   # Worst-case error reading a unit sinusoid between samples, over a
   # sweep of phases and fractional positions, against how coarsely the
   # sinusoid is sampled. 5.6 samples per period is the case
   # test/utility/interpolation.cpp checks.
   periods = np.geomspace(3, 40, 60)
   phases = np.linspace(0, 2*np.pi, 64, endpoint=False)
   mus = np.linspace(0, 1, 21)[:-1]

   def worst(fn, points, p):
      w = 2*np.pi / p
      err = 0.0
      for ph in phases:
         y = [np.sin(w*(i - (points//2 - 1)) + ph) for i in range(points)]
         for mu in mus:
            err = max(err, abs(fn(*y, mu) - np.sin(w*mu + ph)))
      return err

   fig, ax = plt.subplots(figsize=(10, 5.2))
   for fn, pts, name, color in (
         (cubic_p, 4, 'cubic_interpolate', PALETTE['baby_blue']),
         (hermite_p, 4, 'hermite_interpolate', PALETTE['site_accent']),
         (lagrange6_p, 6, 'lagrange6_interpolate', PALETTE['amber'])):
      ax.loglog(periods, [worst(fn, pts, p) for p in periods],
                color=color, linewidth=2.0, label=name)
   ax.axvline(5.6, color='#b0b0b0', linestyle=':', linewidth=1.4)
   ax.text(5.75, 2e-6, '5.6 samples\nper period', color='#333333',
           fontsize=9, va='bottom')
   ax.set_xlabel('Samples per period')
   ax.set_ylabel('Worst-case error (unit sinusoid)')
   ax.set_xlim(periods[0], periods[-1])
   ax.set_xticks([3, 4, 5, 6, 8, 10, 15, 20, 30, 40])
   ax.set_xticklabels(['3', '4', '5', '6', '8', '10', '15', '20', '30', '40'])
   ax.minorticks_off()
   ax.grid(True, which='both', linestyle='--', linewidth=0.5,
           color='#b0b0b0', alpha=0.8)
   ax.legend(loc='upper right')
   save(fig, 'lagrange6_accuracy.svg')


def zero_projection_figure():
   # The scene test/utility/interpolation.cpp checks: a raised-cosine pulse, its
   # left edge at x = 24, riding a slow ramp that is positive everywhere
   # past x = 0. The composite's own zero crossing is the ramp's, at 0;
   # the tangent at the steepest ascent sample projects to the pulse.
   def pulse(x):
      u = (np.asarray(x, dtype=float) - 30.0) / 6.0
      return np.where(np.abs(u) < 1, 0.5*(1 + np.cos(np.pi*u)), 0.0)

   def ramp(x):
      return 0.004 * np.asarray(x, dtype=float)

   xs = np.arange(40)
   y = pulse(xs) + ramp(xs)

   ks = max(range(1, 39), key=lambda k: y[k] - y[k-1])
   g0, g1, g2 = y[ks-1] - y[ks-2], y[ks] - y[ks-1], y[ks+1] - y[ks]
   d = g0 - 2*g1 + g2
   frac = 0.5*(g0 - g2)/d if d < 0 else 0.0          # peak_offset
   xi = (ks - 0.5) + frac
   yi = y[ks]*(0.5 + frac) + y[ks-1]*(0.5 - frac)
   found = xi + (-yi/g1 if g1 != 0 else 0.0)         # zero_projection

   fig, ax = new_axes()
   xc = np.linspace(0, 39, 800)
   ax.plot(xc, pulse(xc) + ramp(xc), color=PALETTE['powder'],
           linewidth=2.0, label='Waveform: pulse on a slow ramp', zorder=1)
   ax.plot(xc, pulse(xc), color='#b0b0b0', linewidth=1.2, linestyle='--',
           label='The pulse alone', zorder=1)
   ax.plot(xs, y, 'o', color=POINTS_COLOR, markersize=6,
           label='Samples', zorder=3)
   xt = np.linspace(found - 0.5, xi + 2.5, 50)
   ax.plot(xt, yi + g1*(xt - xi), color=CURVE_COLOR, linewidth=1.75,
           label='Tangent at the steepest sample', zorder=2)
   ax.axhline(0, color='#b0b0b0', linewidth=0.8, zorder=0)
   ax.plot([0], [0], 'x', color=PALETTE['signal_red'], markersize=12,
           markeredgewidth=2.2, label='Waveform crosses zero', zorder=5)
   ax.plot([found], [0], '*', color=PALETTE['magenta'], markersize=18,
           label=f'zero_projection lands at {found:.1f}', zorder=6)
   ax.set_xlim(-1, 39)
   ax.set_ylim(-0.1, 1.2)
   ax.legend(loc='upper left', fontsize=9)
   save(fig, 'zero_projection.svg')


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
   peak_offset_figure()
   lagrange6_figure()
   zero_projection_figure()
