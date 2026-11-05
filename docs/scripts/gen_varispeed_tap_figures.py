#!/usr/bin/env python3
"""
Generate the varispeed_tap reference figures.

Produces, in docs/modules/ROOT/images/:

   varispeed_tap_heads.svg  -- the delay line as boxes of samples, with
                               the write head and the read head, at
                               ratios 1, 0.5 and 2 (drawn by hand, as SVG).
   varispeed_tap_splice.svg -- a splice on a 200 Hz tone with two
                               harmonics, over a 10 ms crossfade: top, a
                               jump of one period back; bottom, a jump of
                               half a period back.

The splice data comes from the real q::varispeed_tap: a small C++ probe is
compiled against q_lib (header-only) and run, as gen_dx7_chart.py does.

Style and palette follow gen_interpolation_figures.py.

Usage: python3 docs/scripts/gen_varispeed_tap_figures.py
"""

import json
import os
import subprocess
import tempfile

import numpy as np
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt

SITE_ACCENT = '#1565c0'
AMBER = '#ffb300'
SIGNAL_RED = '#e53935'
GREY = '#b0b0b0'

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
OUT_DIR = os.path.join(HERE, '..', 'modules', 'ROOT', 'images')

SPS = 44100.0

# Reads "n max_delay splice_at splice_to fade", then n (sample, ratio)
# pairs; prints each sample's output and delay. splice_to is relative to
# the delay at the splice.
PROBE = r'''
#include <q/fx/varispeed_tap.hpp>
#include <cstdio>
#include <vector>
int main()
{
   std::size_t n, max_delay, at, fade;
   float to;
   std::scanf("%zu %zu %zu %f %zu", &n, &max_delay, &at, &to, &fade);
   cycfi::q::varispeed_tap tap{max_delay};
   std::printf("{\"y\":[");
   std::vector<float> d(n);
   for (std::size_t i = 0; i != n; ++i)
   {
      float s, r;
      std::scanf("%f %f", &s, &r);
      if (i == at && fade)
         tap.splice(tap.delay() + to, fade);
      std::printf(i ? ",%.7g" : "%.7g", tap(s, r));
      d[i] = tap.delay();
   }
   std::printf("],\"delay\":[");
   for (std::size_t i = 0; i != n; ++i)
      std::printf(i ? ",%.7g" : "%.7g", d[i]);
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


def run(exe, x, r, max_delay, at=0, to=0.0, fade=0):
   lines = [f'{len(x)} {max_delay} {at} {to} {fade}']
   lines += [f'{s:.9g} {q:.9g}' for s, q in zip(x, r)]
   out = subprocess.run([exe], input='\n'.join(lines), text=True,
                        capture_output=True, check=True).stdout
   d = json.loads(out)
   return np.array(d['y']), np.array(d['delay'])


def style(ax):
   for s in ('top', 'right'):
      ax.spines[s].set_visible(False)
   ax.spines['left'].set_color(GREY)
   ax.spines['bottom'].set_color(GREY)
   ax.grid(True, linestyle='--', color=GREY, alpha=0.5)


FONT = ('-apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, '
        'Helvetica, Arial, sans-serif')
INK = '#1a1a1a'
MUTED = '#5d5d5d'
BOX_FILL, BOX_LINE = '#f3f3f3', '#9e9e9e'
HOT_FILL = '#e8f2fd'


def gen_heads():
   # One pair of strips a ratio: the delay line now, and four samples
   # later. Boxes are samples of a wave, oldest left, newest right, the
   # same sample at the same place in both strips; the shaded boxes are
   # the delay, from the read head (below) to the write head (above).
   n, later, w, pitch = 12, 4, 38, 40
   x0 = 150
   start = 4
   rows = [
      (1.0, 'Ratio 1: the read keeps pace with the write. '
            'The delay holds, and the pitch is unchanged.'),
      (0.5, 'Ratio 0.5: the read moves half as fast and falls behind. '
            'The delay grows, and the pitch drops an octave.'),
      (2.0, 'Ratio 2: the read moves twice as fast and catches up. '
            'At delay 0 it can go no further.'),
   ]
   strip_h, gap = 112, 40
   row_h = 54 + 2 * strip_h + gap
   width = x0 + (n + later) * pitch + 90
   height = 10 + row_h * len(rows)
   out = ['<?xml version="1.0" encoding="utf-8"?>',
          f'<svg xmlns="http://www.w3.org/2000/svg" '
          f'viewBox="0 0 {width} {height}" font-family=\'{FONT}\'>',
          '<defs><marker id="head" viewBox="0 0 10 10" refX="9" refY="5" '
          'markerWidth="6" markerHeight="6" orient="auto">'
          '<path d="M0,0 L10,5 L0,10 z" fill="context-stroke"/></marker>'
          '</defs>']
   cx = lambda k: x0 + k * pitch + w / 2

   def strip(y, count, read, label):
      newest = count - 1
      by = y + 34
      out.append(f'<text x="{x0 - 16}" y="{by + w / 2 + 5}" font-size="15" '
                 f'text-anchor="end" fill="{MUTED}">{label}</text>')
      for k in range(count):
         hot = read < k <= newest
         out.append(f'<rect x="{x0 + k * pitch}" y="{by}" width="{w}" '
                    f'height="{w}" rx="4" '
                    f'fill="{HOT_FILL if hot else BOX_FILL}" '
                    f'stroke="{SITE_ACCENT if hot else BOX_LINE}"/>')
         v = np.sin(2 * np.pi * k / 9)
         mid = by + w / 2
         top = mid - v * (w / 2 - 6)
         out.append(f'<polyline points="{cx(k):.1f},{mid} {cx(k):.1f},'
                    f'{top:.1f}" stroke="{MUTED}" stroke-width="1.5"/>')
         out.append(f'<circle cx="{cx(k):.1f}" cy="{top:.1f}" r="3" '
                    f'fill="{MUTED}"/>')
      # Write head above the newest sample, read head below its sample
      out.append(f'<text x="{cx(newest)}" y="{by - 24}" font-size="15" '
                 f'text-anchor="middle" fill="{INK}">write</text>')
      out.append(f'<line x1="{cx(newest)}" y1="{by - 20}" '
                 f'x2="{cx(newest)}" y2="{by - 3}" stroke="{INK}" '
                 f'stroke-width="2" marker-end="url(#head)"/>')
      out.append(f'<line x1="{cx(read)}" y1="{by + w + 20}" '
                 f'x2="{cx(read)}" y2="{by + w + 3}" stroke="{SIGNAL_RED}" '
                 f'stroke-width="2" marker-end="url(#head)"/>')
      out.append(f'<text x="{cx(read)}" y="{by + w + 36}" font-size="15" '
                 f'text-anchor="middle" fill="{SIGNAL_RED}">read</text>')
      d = newest - read
      if d > 0:
         out.append(f'<text x="{x0 + count * pitch + 6}" y="{by + w / 2 + 5}" '
                    f'font-size="15" fill="{SITE_ACCENT}">delay {d}</text>')
      else:
         out.append(f'<text x="{x0 + count * pitch + 6}" y="{by + w / 2 + 5}" '
                    f'font-size="15" fill="{SITE_ACCENT}">delay 0</text>')

   y = 10
   for ratio, text in rows:
      out.append(f'<text x="20" y="{y + 16}" font-size="15" '
                 f'fill="{INK}">{text}</text>')
      read = n - 1 - start
      strip(y + 30, n, read, 'now')
      moved = min(read + int(ratio * later), n + later - 1)
      strip(y + 30 + strip_h, n + later, moved, f'{later} samples later')
      y += row_h

   out.append('</svg>')
   path = os.path.join(OUT_DIR, 'varispeed_tap_heads.svg')
   with open(path, 'w') as f:
      f.write('\n'.join(out) + '\n')
   print('wrote', path)


def gen_splice(exe):
   f = 200.0
   n = int(0.06 * SPS)
   w = 2 * np.pi * f * np.arange(n) / SPS
   x = np.sin(w) + 0.5 * np.sin(2 * w + 0.6) + 0.3 * np.sin(3 * w + 1.9)
   x = (x / np.abs(x).max()).astype(np.float32)
   ratio = np.ones_like(x)
   period = SPS / f
   at = int(0.03 * SPS)
   fade = int(0.01 * SPS)
   max_delay = int(0.02 * SPS)

   # The ramps the tap crossfades with: the new read fades in as the old
   # read fades out, the two gains summing to 1
   g = np.zeros(n)
   g[at:at + fade] = 0.5 - 0.5 * np.cos(np.pi * np.arange(fade) / fade)
   g[at + fade:] = 1.0

   a, b = at - int(0.006 * SPS), at + fade + int(0.006 * SPS)
   t = (np.arange(a, b) - at) / SPS * 1000
   fig, axes = plt.subplots(2, 1, figsize=(10, 6.4), sharex=True)
   cases = [(period, 'Jump back one period: the two reads match'),
            (period / 2, 'Jump back half a period: the two reads differ')]
   for ax, (jump, title) in zip(axes, cases):
      y, _ = run(exe, x, ratio, max_delay, at, jump, fade)
      old = x[a:b]
      new = np.array([x[i - int(round(jump))] for i in range(a, b)])
      ax.axvspan(0, fade / SPS * 1000, color=AMBER, alpha=0.25,
                 linewidth=0, label='crossfade')
      ax.plot(t, np.where(g[a:b] < 1, old, np.nan), color='#90caf9',
              linewidth=5.0, label='old read (fading out)')
      ax.plot(t, np.where(g[a:b] > 0, new, np.nan), color=SIGNAL_RED,
              linewidth=1.6, linestyle=':', label='new read (fading in)')
      ax.plot(t, y[a:b], color='#222222', linewidth=1.6, label='output')
      ax.set_ylim(-1.35, 1.35)
      ax.set_ylabel('Amplitude')
      ax.set_title(title, color='#333333', fontsize=11, loc='left')
      style(ax)
   axes[0].legend(loc='upper right', fontsize=9, ncol=4,
                  bbox_to_anchor=(1.0, 1.02))
   axes[1].set_xlim(t[0], t[-1])
   axes[1].set_xlabel('Time from the start of the splice (ms)')
   fig.tight_layout()
   path = os.path.join(OUT_DIR, 'varispeed_tap_splice.svg')
   fig.savefig(path, format='svg', bbox_inches=None)
   plt.close(fig)
   print('wrote', path)


if __name__ == '__main__':
   gen_heads()
   with tempfile.TemporaryDirectory() as tmp:
      exe = build_probe(tmp)
      gen_splice(exe)
