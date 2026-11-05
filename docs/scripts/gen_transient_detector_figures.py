#!/usr/bin/env python3
"""
Generate the transient_detector reference figure.

Produces, in docs/modules/ROOT/images/:

   transient_detector.svg -- a pick on the low E string
                             (test/audio_files/1a-Low-E.wav) close up: the
                             signal, then its level above 4 kHz
                             (spectral_flux, 4 kHz, 1 ms, as the detector
                             uses it), with the detection; bottom, a
                             strummed E chord
                             (example/whammy/audio/E Chord.wav) and its
                             detections, one a strum.

The data comes from the real q::transient_detector and q::spectral_flux:
a small C++ probe is compiled against q_lib (header-only) and run, as
gen_dx7_chart.py does.

NOTE: run with homebrew `python3` (has soundfile + matplotlib). Style and
palette follow gen_interpolation_figures.py.

Usage: python3 docs/scripts/gen_transient_detector_figures.py
"""

import json
import os
import subprocess
import tempfile

import numpy as np
import soundfile as sf
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt

SITE_ACCENT = '#1565c0'
POWDER = '#bbdefb'
AMBER = '#ffb300'
SIGNAL_RED = '#e53935'
GREY = '#b0b0b0'

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
OUT_DIR = os.path.join(HERE, '..', 'modules', 'ROOT', 'images')
NOTE = os.path.join(REPO, 'test', 'audio_files', '1a-Low-E.wav')
CHORD = os.path.join(REPO, 'example', 'whammy', 'audio', 'E Chord.wav')

# Reads "n sps", then n samples; prints the flux level and the detections.
PROBE = r'''
#include <q/fx/transient_detector.hpp>
#include <cstdio>
#include <vector>
namespace q = cycfi::q;
int main()
{
   std::size_t n;
   float sps;
   std::scanf("%zu %f", &n, &sps);
   q::transient_detector detect{sps};
   q::spectral_flux flux{q::frequency{4000.0}, q::transient_detector::window, sps};
   std::vector<std::size_t> fired;
   std::printf("{\"flux\":[");
   for (std::size_t i = 0; i != n; ++i)
   {
      float s;
      std::scanf("%f", &s);
      std::printf(i ? ",%.6g" : "%.6g", flux(s));
      if (detect(s))
         fired.push_back(i);
   }
   std::printf("],\"fired\":[");
   for (std::size_t i = 0; i != fired.size(); ++i)
      std::printf(i ? ",%zu" : "%zu", fired[i]);
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


def run(exe, x, sps):
   text = f'{len(x)} {sps}\n' + '\n'.join(f'{s:.8g}' for s in x)
   out = subprocess.run([exe], input=text, text=True, capture_output=True,
                        check=True).stdout
   d = json.loads(out)
   return np.array(d['flux']), np.array(d['fired'], dtype=int)


def mono(path):
   x, sps = sf.read(path, dtype='float32')
   if x.ndim > 1:
      x = x.mean(axis=1)
   return x, sps


def db(v):
   return 20 * np.log10(np.maximum(v, 1e-6))


def style(ax):
   for s in ('top', 'right'):
      ax.spines[s].set_visible(False)
   ax.spines['left'].set_color(GREY)
   ax.spines['bottom'].set_color(GREY)
   ax.grid(True, linestyle='--', color=GREY, alpha=0.5)


def gen(exe):
   x, sps = mono(NOTE)
   flux, fired = run(exe, x, sps)
   at = fired[0]
   a, b = at - int(0.06 * sps), at + int(0.02 * sps)
   t = (np.arange(a, b) - at) / sps * 1000

   y, csps = mono(CHORD)
   _, strums = run(exe, y, csps)

   fig, (ax0, ax1, ax2) = plt.subplots(
      3, 1, figsize=(10, 8.4), gridspec_kw={'height_ratios': [1, 1.3, 1.3]})

   ax0.plot(t, x[a:b], color=SITE_ACCENT, linewidth=1.0, label='signal')
   ax0.axvline(0, color=SIGNAL_RED, linewidth=1.6, label='detected')
   ax0.set_xlim(t[0], t[-1])
   ax0.set_ylabel('Amplitude')
   ax0.set_title('A pick on the low E string, close up',
                 color='#333333', fontsize=11, loc='left')
   ax0.legend(loc='upper left', fontsize=9)

   lv = db(flux[a:b])
   ax1.plot(t, lv, color=SITE_ACCENT, linewidth=1.6,
            label='level above 4 kHz')
   ax1.axvline(0, color=SIGNAL_RED, linewidth=1.6, label='detected')
   pre = lv[(t > -50) & (t < -1)].max()
   ax1.annotate('the pick on the string', xy=(-25, pre - 2),
                xytext=(-48, pre + 22), fontsize=10, color='#333333',
                arrowprops=dict(arrowstyle='->', color='#5d5d5d'))
   ax1.annotate('the attack', xy=(0.6, lv[t >= 0][:40].max()),
                xytext=(5, -8), fontsize=10, color='#333333',
                arrowprops=dict(arrowstyle='->', color='#5d5d5d'))
   ax1.set_xlim(t[0], t[-1])
   ax1.set_ylim(-90, 5)
   ax1.set_xlabel('Time from the detection (ms)')
   ax1.set_ylabel('Level (dB)')
   ax1.legend(loc='upper left', fontsize=9)

   tc = np.arange(len(y)) / csps
   ax2.plot(tc[::20], y[::20], color=POWDER, linewidth=0.8,
            label='signal')
   for k, i in enumerate(strums):
      ax2.axvline(i / csps, color=SIGNAL_RED, linewidth=1.4,
                  label='detected' if k == 0 else None)
   ax2.set_xlim(0, tc[-1])
   ax2.set_xlabel('Time (s)')
   ax2.set_ylabel('Amplitude')
   ax2.set_title('A strummed E chord: one detection a strum',
                 color='#333333', fontsize=11, loc='left')
   ax2.legend(loc='upper right', fontsize=9)

   for ax in (ax0, ax1, ax2):
      style(ax)
   fig.tight_layout()
   path = os.path.join(OUT_DIR, 'transient_detector.svg')
   fig.savefig(path, format='svg', bbox_inches=None)
   plt.close(fig)
   print('wrote', path)
   print('note detected at %.4f s; strums at' % (at / sps),
         ' '.join('%.3f' % (i / csps) for i in strums))


if __name__ == '__main__':
   with tempfile.TemporaryDirectory() as tmp:
      gen(build_probe(tmp))
