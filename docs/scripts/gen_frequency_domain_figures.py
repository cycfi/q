#!/usr/bin/env python3
"""
Generate the Frequency Domain landing page figure.

Produces, in docs/modules/ROOT/images/:

   frequency_domain_waterfall.svg -- a strummed Stratocaster, its waveform
                                     above and its spectrum below, frame
                                     after frame as a waterfall: each strum
                                     a ridge across the band, the strings'
                                     harmonics the lines running back as
                                     the chord rings.

The spectra are from the real class: a small C++ probe is compiled against
q_lib (header-only), Hann-windows 4096-sample frames of the recording and
runs them through q::rfft, as gen_formant_preservation_figures.py does. The
recording is Joel's Strat (an open E strummed, then ringing), given as the
argument.

NOTE: run with homebrew `python3` (has soundfile + matplotlib). Palette
follows gen_fft_figures.py.

Usage: python3 docs/scripts/gen_frequency_domain_figures.py strat.wav
"""

import os
import re
import subprocess
import sys
import tempfile

import numpy as np
import soundfile as sf
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
from matplotlib.colors import LinearSegmentedColormap

SITE_ACCENT = '#1565c0'
SKY = '#64b5f6'
POWDER = '#bbdefb'
GREY = '#b0b0b0'
SUB = '#333333'

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
OUT_DIR = os.path.join(HERE, '..', 'modules', 'ROOT', 'images')

N = 4096            # frame size: 93 ms at 44.1 kHz
SECONDS = 7.0       # the strums and the start of the ring
FRAMES = 36         # one every 190 ms or so
F_LO, F_HI = 60.0, 12000.0
FLOOR_DB = -75.0

# Reads "n hop frames", then n * frames worth of samples; prints each
# frame's magnitudes in dB, n/2 + 1 of them a line.
PROBE = r'''
#include <q/fft/fft.hpp>
#include <cmath>
#include <cstdio>
#include <numbers>
#include <vector>
namespace q = cycfi::q;
int main()
{
   std::size_t n, hop, frames;
   std::scanf("%zu %zu %zu", &n, &hop, &frames);
   std::size_t const len = hop * (frames - 1) + n;
   std::vector<float> x(len);
   for (auto& s : x)
      std::scanf("%f", &s);

   q::rfft fft{n};
   std::vector<float> w(n), data(n + 2);
   for (std::size_t i = 0; i != n; ++i)
      w[i] = float(0.5 - 0.5 * std::cos(2 * std::numbers::pi * i / n));
   for (std::size_t f = 0; f != frames; ++f)
   {
      for (std::size_t i = 0; i != n; ++i)
         data[i] = x[f * hop + i] * w[i];
      fft.forward(data.data());
      for (std::size_t b = 0; b <= n / 2; ++b)
      {
         auto const re = data[2 * b], im = data[2 * b + 1];
         std::printf(b ? " %.6g" : "%.6g",
            10 * std::log10(re * re + im * im + 1e-30));
      }
      std::printf("\n");
   }
}
'''


def build_probe(tmp):
   src = os.path.join(tmp, 'probe.cpp')
   exe = os.path.join(tmp, 'probe')
   with open(src, 'w') as f:
      f.write(PROBE)
   subprocess.run(
      ['c++', '-std=c++20', '-O2', '-w',
       '-I', os.path.join(REPO, 'q_lib/include'),
       '-I', os.path.join(REPO, 'infra/include'),
       '-o', exe, src], check=True)
   return exe


def spectra(exe, x, hop):
   need = hop * (FRAMES - 1) + N
   x = np.pad(x, (0, max(0, need - len(x))))[:need]
   text = f'{N} {hop} {FRAMES}\n' + '\n'.join(f'{s:.7g}' for s in x)
   out = subprocess.run([exe], input=text, text=True, capture_output=True,
                        check=True).stdout
   return np.array([[float(v) for v in l.split()] for l in out.splitlines()])


def on_log_axis(db, sps, f):
   """Each frame on the log frequency grid f: where a cell spans several
   bins, the loudest, so the harmonics' peaks survive the thinning; where
   it is narrower than a bin (the bass), interpolated between bins."""
   bins = np.arange(db.shape[1]) * sps / N
   edges = np.sqrt(f[:-1] * f[1:])
   edges = np.concatenate(([f[0] / 1.01], edges, [f[-1] * 1.01]))
   idx = np.searchsorted(bins, edges)
   out = np.empty((db.shape[0], len(f)))
   for i in range(db.shape[0]):
      out[i] = np.interp(f, bins, db[i])
   for j in range(len(f)):
      a, b = idx[j], idx[j + 1]
      if b - a >= 2:
         out[:, j] = db[:, a:b].max(axis=1)
   return out


def gen(x, sps, db):
   f = np.geomspace(F_LO, F_HI, 320)
   lx = np.log10(f)
   spec = on_log_axis(db, sps, f)
   spec = np.clip(spec - spec.max(), FLOOR_DB, 0) - FLOOR_DB    # 0 .. 75
   height = spec / -FLOOR_DB                                    # 0 .. 1

   fig = plt.figure(figsize=(10, 7.4))
   wave = fig.add_axes([0.08, 0.83, 0.86, 0.13])
   fall = fig.add_axes([0.08, 0.08, 0.86, 0.64])

   # The waveform, the time domain.
   # Drawn as its range in each of 1500 columns, far finer than the page
   # shows and a fraction of the samples.
   seg = x[:int(SECONDS * sps)]
   cols = 1500
   seg = seg[:len(seg) // cols * cols].reshape(cols, -1)
   tc = (np.arange(cols) + 0.5) * SECONDS / cols
   wave.fill_between(tc, seg.min(axis=1), seg.max(axis=1),
                     color=SITE_ACCENT, linewidth=0)
   wave.set_xlim(0, SECONDS)
   peak = np.abs(seg).max()
   wave.set_ylim(-1.05 * peak, 1.05 * peak)
   wave.set_yticks([])
   wave.set_xlabel('Time (s)', color=SUB, fontsize=16, labelpad=2)
   wave.tick_params(colors=SUB, labelsize=15)
   for s in ('top', 'right', 'left'):
      wave.spines[s].set_visible(False)
   wave.spines['bottom'].set_color(GREY)

   # The waterfall, the frequency domain: the first frame in front, each
   # later one a step up and to the right, drawn back to front so a frame
   # hides what stands behind it.
   cmap = LinearSegmentedColormap.from_list('q', [SITE_ACCENT, SKY, POWDER])
   dy = 0.06
   dx = 0.008
   lift = 0.75
   for i in reversed(range(FRAMES)):
      xs = lx + i * dx
      base = i * dy
      ys = base + lift * height[i]
      c = cmap(i / (FRAMES - 1))
      # One path a frame: the ridge on top, white below it down to its
      # base, so it hides the frames behind; only the ridge is stroked.
      px = np.concatenate((xs, [xs[-1], xs[0]]))
      py = np.concatenate((ys, [base, base]))
      fall.fill(px, py, facecolor='white', edgecolor='none',
                zorder=2 * (FRAMES - i))
      fall.plot(xs, ys, color=c, linewidth=1.0,
                zorder=2 * (FRAMES - i) + 1)

   fall.set_xlim(lx[0], lx[-1] + FRAMES * dx)
   fall.set_ylim(-0.02, (FRAMES - 1) * dy + lift * height[-8:].max() + 0.05)
   ticks = [100, 300, 1000, 3000, 10000]
   fall.set_xticks(np.log10(ticks))
   fall.set_xticklabels(['100', '300', '1k', '3k', '10k'])
   fall.set_xlabel('Frequency (Hz)', color=SUB, fontsize=16)
   fall.set_yticks([])
   fall.tick_params(colors=SUB, labelsize=15)
   for s in ('top', 'right', 'left'):
      fall.spines[s].set_visible(False)
   fall.spines['bottom'].set_color(GREY)

   # The time axis of the waterfall, along its depth.
   x0, x1 = lx[-1] + 0.03, lx[-1] + 0.03 + (FRAMES - 1) * dx
   fall.annotate('', xy=(x1, (FRAMES - 1) * dy), xytext=(x0, 0),
                 arrowprops=dict(arrowstyle='->', color=GREY, lw=1))
   fall.text((x0 + x1) / 2 + 0.03, (FRAMES - 1) * dy / 2, 'time',
             color=SUB, fontsize=16, rotation=80, va='center')

   path = os.path.join(OUT_DIR, 'frequency_domain_waterfall.svg')
   fig.savefig(path, format='svg')
   plt.close(fig)

   # Coordinates to a tenth of a point, on a 720-point canvas: plenty, and
   # a fraction of the file. Numbers under 1 (the glyphs' scale factors)
   # keep their digits.
   svg = open(path).read()
   svg = re.sub(r'(?<![\d.])([1-9]\d*\.\d)\d+', r'\1', svg)
   open(path, 'w').write(svg)
   print('wrote', path, os.path.getsize(path) // 1024, 'KB')


if __name__ == '__main__':
   x, sps = sf.read(sys.argv[1], dtype='float32')
   if x.ndim > 1:
      x = x.mean(axis=1)
   hop = int(SECONDS * sps / FRAMES)
   with tempfile.TemporaryDirectory() as tmp:
      db = spectra(build_probe(tmp), x, hop)
   gen(x, sps, db)
