#!/usr/bin/env python3
"""
Generate the formant preservation reference figures.

Produces, in docs/modules/ROOT/images/:

   formant_envelope.svg   -- the envelope q::formant_learner learns from a
                             Stratocaster recording (its average spectrum
                             faint, the model F bold), and the same model
                             as a shift an octave up and down leaves it,
                             F(f/r): the pickup's resonance moves with the
                             pitch.
   formant_correction.svg -- the correction q::formant_corrector applies,
                             F(f) / F(f/r), at +-5 and +-12 semitones,
                             with the boost cap.

The data is from the real classes: a small C++ probe is compiled against
q_lib (header-only), learns from the recording and prints the fitted
table's rows, as gen_transient_detector_figures.py does. The recording is
Joel's Strat (an open E strummed, then ringing), given as the argument.

NOTE: run with homebrew `python3` (has soundfile + matplotlib). Style and
palette follow gen_transient_detector_figures.py.

Usage: python3 docs/scripts/gen_formant_preservation_figures.py strat.wav
"""

import json
import os
import subprocess
import sys
import tempfile

import numpy as np
import soundfile as sf
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt

# The wide frame of the documentation conventions, drawn at its placed
# size (a point is a pixel), with 15 px text
WIDE = 750
plt.rcParams['font.size'] = 15

SITE_ACCENT = '#1565c0'
POWDER = '#bbdefb'
AMBER = '#ffb300'
SIGNAL_RED = '#e53935'
GREY = '#b0b0b0'
INK = '#333333'

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
OUT_DIR = os.path.join(HERE, '..', 'modules', 'ROOT', 'images')
SEMITONES = (0, 12, -12, 5, -5)

# Reads "n sps", then n samples; learns, fits, prints the chosen rows.
PROBE = r'''
#include <q/fft/formant_learner.hpp>
#include <cmath>
#include <cstdio>
#include <memory>
#include <vector>
namespace q = cycfi::q;
int main()
{
   std::size_t n;
   float sps;
   std::scanf("%zu %f", &n, &sps);
   std::vector<float> x(n);
   for (auto& s : x)
      std::scanf("%f", &s);

   q::formant_learner learner{sps};
   learner(x);
   auto table = std::make_unique<q::formant_table>();
   learner.fit(*table);

   int const st[] = {0, 12, -12, 5, -5};
   std::printf("{\"coverage\":%g,\"rows\":[", learner.coverage());
   for (int i = 0; i != 5; ++i)
   {
      std::vector<float> k(table->order());
      float level;
      table->at(std::exp2(st[i] / 12.0f), k, level);
      std::printf(i ? ",{" : "{");
      std::printf("\"level\":%.9g,\"k\":[", level);
      for (std::size_t m = 0; m != k.size(); ++m)
         std::printf(m ? ",%.9g" : "%.9g", k[m]);
      std::printf("]}");
   }
   std::printf("]}\n");
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


def mono(path):
   x, sps = sf.read(path, dtype='float32')
   if x.ndim > 1:
      x = x.mean(axis=1)
   return x, sps


def run(exe, x, sps):
   text = f'{len(x)} {sps}\n' + '\n'.join(f'{s:.8g}' for s in x)
   out = subprocess.run([exe], input=text, text=True, capture_output=True,
                        check=True).stdout
   return json.loads(out)


def model_db(row, f, sps):
   """The row's spectral envelope at f, in dB: its level over |A|."""
   k = row['k']
   a = np.zeros(len(k) + 1)
   a[0] = 1
   for m in range(1, len(k) + 1):
      prev = a.copy()
      for j in range(1, m):
         a[j] = prev[j] + k[m - 1] * prev[m - j]
      a[m] = k[m - 1]
   w = np.exp(-1j * np.outer(2 * np.pi * f / sps, np.arange(len(a))))
   return 20 * np.log10(row['level'] / np.abs(w @ a))


def average_spectrum(x, sps):
   """The input's average spectrum, frames at unit power, for display."""
   frame = int(0.05 * sps)
   n = 1 << int(np.ceil(np.log2(frame)))
   win = np.hanning(frame)
   acc = np.zeros(n // 2 + 1)
   for a in range(0, len(x) - frame, frame // 2):
      seg = x[a:a + frame]
      if np.sqrt(np.mean(seg ** 2)) < 10 ** (-50 / 20):
         continue
      p = np.abs(np.fft.rfft(seg * win, n)) ** 2
      acc += p / p.sum()
   return np.fft.rfftfreq(n, 1 / sps), acc


def style(ax):
   for s in ('top', 'right'):
      ax.spines[s].set_visible(False)
   ax.spines['left'].set_color(GREY)
   ax.spines['bottom'].set_color(GREY)
   ax.grid(True, which='both', linestyle='--', color=GREY, alpha=0.4)
   ax.set_xscale('log')
   ax.set_xlim(60, 16000)
   ax.set_xticks([100, 300, 1000, 3000, 10000])
   ax.set_xticklabels(['100', '300', '1k', '3k', '10k'])
   ax.set_xlabel('Frequency (Hz)')


def gen_envelope(d, x, sps):
   f = np.geomspace(60, 16000, 600)
   rows = dict(zip(SEMITONES, d['rows']))
   fs, p = average_spectrum(x, sps)
   base = model_db(rows[0], f, sps)
   lift = base.max() - 10 * np.log10(p[(fs > 60) & (fs < 16000)].max())

   fig, ax = plt.subplots(figsize=(WIDE / 72, 360 / 72))
   m = (fs > 60) & (fs < 16000)
   ax.plot(fs[m], 10 * np.log10(p[m] + 1e-20) + lift, color=POWDER,
           linewidth=0.8, label='average spectrum')
   ax.plot(f, base, color=SITE_ACCENT, linewidth=2.2,
           label='F, the learned spectral envelope')
   ax.plot(f, model_db(rows[12], f, sps), color=SIGNAL_RED, linewidth=1.8,
           linestyle='--', label='F(f/r), an octave up')
   ax.plot(f, model_db(rows[-12], f, sps), color=AMBER, linewidth=1.8,
           linestyle='--', label='F(f/r), an octave down')
   ax.set_ylim(base.max() - 70, base.max() + 8)
   ax.set_ylabel('Level (dB)')
   ax.legend(loc='lower left', fontsize=15)
   style(ax)
   fig.tight_layout()
   path = os.path.join(OUT_DIR, 'formant_envelope.svg')
   fig.savefig(path, format='svg')
   plt.close(fig)
   print('wrote', path)


def gen_correction(d, sps):
   f = np.geomspace(60, 16000, 600)
   rows = dict(zip(SEMITONES, d['rows']))
   base = model_db(rows[0], f, sps)
   fig, ax = plt.subplots(figsize=(WIDE / 72, 330 / 72))
   for st, color, ls in ((12, SIGNAL_RED, '-'), (5, SIGNAL_RED, ':'),
                         (-12, AMBER, '-'), (-5, AMBER, ':')):
      c = base - model_db(rows[st], f, sps)
      ax.plot(f, c, color=color, linewidth=1.8, linestyle=ls,
              label=f'{st:+d} semitones')
   ax.axhline(0, color=GREY, linewidth=1)
   ax.set_ylabel('Correction (dB)')
   ax.legend(loc='lower left', fontsize=15, ncol=2)
   style(ax)
   fig.tight_layout()
   path = os.path.join(OUT_DIR, 'formant_correction.svg')
   fig.savefig(path, format='svg')
   plt.close(fig)
   print('wrote', path)


if __name__ == '__main__':
   x, sps = mono(sys.argv[1])
   with tempfile.TemporaryDirectory() as tmp:
      d = run(build_probe(tmp), x, sps)
   print('coverage', d['coverage'])
   gen_envelope(d, x, sps)
   gen_correction(d, sps)
