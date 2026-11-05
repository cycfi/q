#!/usr/bin/env python3
"""
Generate the bacf_lag_tracker reference animation and its poster.

Produces:

   docs/modules/ROOT/images/bacf_lag_tracker_strobe.svg
      -- the poster: the frame where the pattern freezes.
   docs/antora/build/media/bacf_lag_tracker_strobe.mp4
      -- the animation, for the media host (media.cycfi.com/q/
         bacf-lag-tracker/, uploaded by hand with
         --content-type video/mp4); not committed.

An E chord, ringing (0.52 s after its first strum, from
test/audio_files/E-Chord.wav), compared with itself one lag back as the
lag slides from 1 to 30 ms, the way a strobe tuner's pattern drifts until
it stands still. Top: the last 45 ms of the low-passed signal, with the
latest window and the window one lag back. Then the two windows overlaid,
their bitstreams and XOR, and the mismatch so far against the lag.

The bits and the window are the tracker's own: a small C++ probe is
compiled against q_lib (header-only) and run, as gen_varispeed_tap_figures
does. It low-passes the input at 1 kHz, as the bacf_pitch_shifter does,
feeds a bacf_lag_tracker, and reads its comparator state.

Usage: python3 docs/scripts/gen_bacf_lag_tracker_figures.py
"""

import os
import subprocess
import tempfile

import numpy as np
import soundfile as sf
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
from matplotlib.animation import FuncAnimation, FFMpegWriter

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
OUT_DIR = os.path.join(HERE, '..', 'modules', 'ROOT', 'images')
MEDIA_DIR = os.path.join(HERE, '..', 'antora', 'build', 'media')
CLIP = os.path.join(REPO, 'test', 'audio_files', 'E-Chord.wav')

SITE_ACCENT = '#1565c0'
AMBER = '#ffb300'
SIGNAL_RED = '#e53935'
GREEN = '#43a047'
GREY = '#b0b0b0'

AFTER = 0.52        # seconds after the first strum: the chord ringing
LOW_E = 82.41

# Reads n and sps, then n samples; prints the tracker's window, then per
# sample the low-passed signal, the comparator bit, and 1 at a transient.
PROBE = r'''
#define private public
#include <q/pitch/bacf_lag_tracker.hpp>
#undef private
#include <q/fx/biquad.hpp>
#include <q/fx/transient_detector.hpp>
#include <q/support/literals.hpp>
#include <cstdio>
namespace q = cycfi::q;
using namespace q::literals;
int main()
{
   std::size_t n;
   float sps;
   std::scanf("%zu %f", &n, &sps);
   q::bacf_lag_tracker tr{2.5_ms, 45_ms, sps};
   q::lowpass lp{1_kHz, sps};
   q::transient_detector td{sps};
   std::printf("%zu\n", tr.window());
   for (std::size_t i = 0; i != n; ++i)
   {
      float x;
      std::scanf("%f", &x);
      bool const on = td(x);
      if (on)
         tr.drop();
      float const s = lp(x);
      tr(s);
      std::printf("%.7g %d %d\n", s, int(tr._state), int(on));
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


def load(exe):
   x, sps = sf.read(CLIP)
   if x.ndim > 1:
      x = x.mean(axis=1)
   x = x[:int(3 * sps)]
   inp = f'{len(x)} {sps}\n' + '\n'.join(f'{v:.8g}' for v in x)
   out = subprocess.run([exe], input=inp, text=True, capture_output=True,
                        check=True).stdout.split('\n')
   window = int(out[0])
   d = np.array([[float(v) for v in l.split()] for l in out[1:] if l])
   return sps, window, d[:, 0], d[:, 1].astype(bool), d[:, 2].astype(bool)


def gen(exe):
   sps, W, s, bits, transient = load(exe)
   T = int(round((np.nonzero(transient)[0][0] / sps + AFTER) * sps))
   lags = np.arange(int(0.001 * sps), int(0.030 * sps), 4)
   now_bits, now_sig = bits[T - W:T], s[T - W:T]
   mis = np.array([np.mean(now_bits != bits[T - W - L:T - L]) for L in lags])
   far = lags >= int(0.0135 * sps)          # past the low E's own period
   bi = int(np.nonzero(far)[0][0] + np.argmin(mis[far]))
   best = lags[bi]
   reach = lags[-1]
   peak = np.max(np.abs(s[T - W - reach:T]))
   ms = lambda n: n / sps * 1000

   frames = (list(range(bi + 1)) + [bi] * 80
             + list(range(bi + 1, len(lags))) + [len(lags) - 1] * 10)

   fig = plt.figure(figsize=(10, 7.6), dpi=90)
   gs = fig.add_gridspec(4, 1, height_ratios=[1.3, 1.6, 0.9, 1.5],
                         hspace=0.6)
   a0, a1, a2, a3 = (fig.add_subplot(gs[i]) for i in range(4))
   for ax in (a0, a1, a2, a3):
      for side in ('top', 'right'):
         ax.spines[side].set_visible(False)
      ax.spines['left'].set_color(GREY)
      ax.spines['bottom'].set_color(GREY)

   span = np.arange(T - W - reach - 40, T)
   a0.plot(ms(span - T), s[span] / peak, color=SITE_ACCENT, lw=0.8)
   a0.axvspan(-ms(W), 0, color=SITE_ACCENT, alpha=0.15)
   then_box = a0.axvspan(0, 0, color=AMBER, alpha=0.3)
   a0.set_xlim(ms(span[0] - T), 0)
   a0.set_ylim(-1.1, 1.1)
   a0.set_yticks([])
   a0.set_title('The latest window (blue) and the window one lag back '
                '(amber)', fontsize=10, loc='left')
   a0.set_xlabel('ms before now', fontsize=8)

   tw = ms(np.arange(W))
   a1.plot(tw, now_sig / peak, color=SITE_ACCENT, lw=1.4, label='now')
   then_line, = a1.plot(tw, now_sig / peak, color=AMBER, lw=1.2,
                        label='one lag back')
   a1.set_xlim(0, tw[-1])
   a1.set_ylim(-1.1, 1.1)
   a1.set_yticks([])
   a1.legend(loc='upper right', fontsize=8, frameon=False)
   title = a1.set_title('', fontsize=12, loc='left')

   rgb = lambda c: np.array(matplotlib.colors.to_rgb(c))
   blue, amber, red, light = (rgb(SITE_ACCENT), rgb(AMBER), rgb(SIGNAL_RED),
                              rgb('#f0f0f0'))
   rows = np.zeros((3, W, 3))
   img = a2.imshow(rows, aspect='auto', interpolation='nearest',
                   extent=[0, tw[-1], 3, 0])
   a2.set_yticks([0.5, 1.5, 2.5], ['now', 'lag back', 'XOR'], fontsize=8)
   a2.set_xlabel('ms in the window', fontsize=8)

   curve, = a3.plot([], [], color=GREEN, lw=1.5)
   cursor = a3.axvline(0, color=AMBER, lw=1)
   for ref, name in [(1000 / LOW_E, f'low E {1000 / LOW_E:.1f} ms'),
                     (ms(best), f'chord {ms(best):.1f} ms')]:
      a3.axvline(ref, color=SITE_ACCENT, lw=0.6, ls=':')
      a3.text(ref + 0.2, 0.92, name, fontsize=8, color=SITE_ACCENT)
   a3.set_xlim(0, ms(lags[-1]) + 0.5)
   a3.set_ylim(0, 1.0)
   a3.set_xlabel('lag (ms)', fontsize=8)
   a3.set_ylabel('mismatched', fontsize=8)
   a3.set_yticks([0, 0.25, 0.5, 0.75, 1.0],
                 ['0', '25%', '50%', '75%', '100%'])
   a3.axhline(0.5, color=GREY, lw=0.6, ls='--')
   freeze = a3.text(0, 0, '', fontsize=10, color=GREEN, ha='center')

   def draw(k):
      L = lags[k]
      then_bits = bits[T - W - L:T - L]
      then_line.set_ydata(s[T - W - L:T - L] / peak)
      rows[0] = np.where(now_bits[:, None], blue, light)
      rows[1] = np.where(then_bits[:, None], amber, light)
      rows[2] = np.where((now_bits != then_bits)[:, None], red, light)
      img.set_data(rows)
      then_box.set_x(-ms(W + L))
      then_box.set_width(ms(W))
      curve.set_data(ms(lags[:k + 1]), mis[:k + 1])
      cursor.set_xdata([ms(L)])
      title.set_text(f'Lag {ms(L):5.2f} ms: {100 * mis[k]:4.1f}% of the '
                     f'bits differ')
      freeze.set_position((ms(best), mis[bi] + 0.06))
      freeze.set_text('freeze' if k >= bi else '')
      return []

   os.makedirs(MEDIA_DIR, exist_ok=True)
   video = os.path.join(MEDIA_DIR, 'bacf_lag_tracker_strobe.mp4')
   FuncAnimation(fig, draw, frames=frames, interval=50).save(
      video, writer=FFMpegWriter(fps=20, bitrate=1800))
   print('wrote', video)

   draw(bi)
   poster = os.path.join(OUT_DIR, 'bacf_lag_tracker_strobe.svg')
   fig.savefig(poster)
   plt.close(fig)
   print('wrote', poster)
   print(f'freeze at {best} samples ({ms(best):.2f} ms), '
         f'{100 * mis[bi]:.1f}% mismatched; at the low E '
         f'{100 * mis[np.argmin(np.abs(ms(lags) - 1000 / LOW_E))]:.1f}%; '
         f'least and most: {100 * mis.min():.1f}% at '
         f'{ms(lags[np.argmin(mis)]):.2f} ms, {100 * mis.max():.1f}% at '
         f'{ms(lags[np.argmax(mis)]):.2f} ms')


if __name__ == '__main__':
   with tempfile.TemporaryDirectory() as tmp:
      gen(build_probe(tmp))
