#!/usr/bin/env python3
"""
Generate the bacf_pitch_shifter reference figures.

Produces, in docs/modules/ROOT/images/:

   bacf_pitch_shifter_jump.svg   -- one splice on a 196 Hz tone with two
                                    harmonics, shifted down a fourth:
                                    top, a varispeed_tap jumping a fixed
                                    2.5 periods; bottom, the shifter
                                    jumping a period.
   bacf_pitch_shifter_cycles.svg -- the same tone, each cycle painted by
                                    the input period it is read from:
                                    the input, then the shifter's output
                                    up a fifth (a cycle plays twice) and
                                    down a fourth (a cycle is skipped).

The data for both comes from the real q::bacf_pitch_shifter and
q::varispeed_tap: a small C++ probe is compiled against q_lib
(header-only) and run, as gen_varispeed_tap_figures.py does. The probe
reads the shifter's tap position directly, to say where each output
sample comes from.

   bacf_pitch_shifter_splices.svg -- the delay line as boxes of samples of
                                     a wave with a period of 5 samples,
                                     with the write head and the read
                                     head: going up, the read nears the
                                     write and splices back one period;
                                     going down, it falls behind and
                                     splices forward one period. Drawn by
                                     hand, as SVG, in the style of
                                     varispeed_tap_heads.svg.

Usage: python3 docs/scripts/gen_bacf_pitch_shifter_figures.py
"""

import os
import subprocess
import tempfile

import numpy as np
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt

# The wide frame of the documentation conventions, drawn at its placed
# size (a point is a pixel), with 15 px text
WIDE = 750
plt.rcParams['font.size'] = 15

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
OUT_DIR = os.path.join(HERE, '..', 'modules', 'ROOT', 'images')

FONT = ('-apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, '
        'Helvetica, Arial, sans-serif')
INK = '#1a1a1a'
MUTED = '#5d5d5d'
SITE_ACCENT = '#1565c0'
SIGNAL_RED = '#e53935'
BOX_FILL, BOX_LINE = '#f3f3f3', '#9e9e9e'
HOT_FILL = '#e8f2fd'
MATCH_FILL, MATCH_LINE = '#fff3cd', '#ffb300'
GREY = '#b0b0b0'
CYCLE_COLORS = ['#1565c0', '#ffb300', '#e53935', '#43a047', '#8e24aa',
                '#00897b']

SPS = 44100.0
F0 = 196.0
PERIOD = SPS / F0

# "sola r": the shifter at ratio r. "fixed r jump bound fade": a
# varispeed_tap at ratio r, spliced forward by `jump` seconds whenever it
# falls `bound` behind. Reads n, then n samples; prints per sample the
# output, the input position of the main read, and while splicing the
# incoming read's position (else -1).
PROBE = r'''
#define private public
#include <q/fx/bacf_pitch_shifter.hpp>
#undef private
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>
namespace q = cycfi::q;
int main(int argc, char** argv)
{
   std::size_t n;
   float const sps = 44100;
   float const r = std::atof(argv[2]);
   std::scanf("%zu", &n);
   std::vector<float> x(n);
   for (auto& s : x)
      std::scanf("%f", &s);
   if (std::strcmp(argv[1], "sola") == 0)
   {
      q::bacf_pitch_shifter sh{sps};
      for (std::size_t i = 0; i != n; ++i)
      {
         auto y = sh(x[i], r);
         auto const& t = sh._tap;
         std::printf("%.7g %.7g %.7g\n", y, float(i) - t._delay,
            t._fade ? float(i) - t._to : -1.0f);
      }
   }
   else
   {
      auto const jump = std::atof(argv[3]) * sps;
      auto const bound = std::atof(argv[4]) * sps;
      auto const fade = std::size_t(std::atof(argv[5]) * sps);
      q::varispeed_tap t{std::size_t(0.1 * sps)};
      for (std::size_t i = 0; i != n; ++i)
      {
         auto y = t(x[i], r);
         if (!t.splicing() && t.delay() > bound)
            t.splice(t.delay() - jump, fade);
         std::printf("%.7g %.7g %.7g\n", y, float(i) - t.delay(),
            t.splicing() ? float(i) - t.target() : -1.0f);
      }
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


def tone(n):
   t = np.arange(n)
   return (0.5 * np.sin(2 * np.pi * F0 * t / SPS)
           + 0.25 * np.sin(4 * np.pi * F0 * t / SPS + 0.8))


def run(exe, x, *args):
   inp = f'{len(x)}\n' + '\n'.join(f'{v:.8g}' for v in x)
   out = subprocess.run([exe, *args], input=inp, text=True,
                        capture_output=True, check=True).stdout
   return np.array([[float(v) for v in l.split()]
                    for l in out.split('\n') if l])


def style(ax):
   for side in ('top', 'right'):
      ax.spines[side].set_visible(False)
   ax.spines['left'].set_color(GREY)
   ax.spines['bottom'].set_color(GREY)
   ax.grid(True, linestyle='--', color=GREY, alpha=0.5)
   ax.set_yticks([])


def gen_jump(exe):
   # One splice going down a fourth, zoomed: a fixed jump of 2.5 periods
   # lands half a cycle off, and the crossfade sums two copies out of
   # phase; a jump of a period lands on the same point of the cycle.
   n = int(0.6 * SPS)
   x = tone(n)
   jump, fade = 2.5 * PERIOD / SPS, 0.010
   rows = [
      (run(exe, x, 'fixed', '0.75', f'{jump}', f'{jump + 0.002}',
           f'{fade}'),
       'A fixed jump, 2.5 periods: the crossfade sums two copies half a '
       'cycle apart'),
      (run(exe, x, 'sola', '0.75'),
       'The shifter: a jump of one period, the same point of the cycle'),
   ]
   fig, axs = plt.subplots(2, 1, figsize=(WIDE / 72, 400 / 72), sharex=True)
   for ax, (d, title) in zip(axs, rows):
      y, splicing = d[:, 0], d[:, 2] >= 0
      starts = np.nonzero(splicing[1:] & ~splicing[:-1])[0] + 1
      s0 = [s for s in starts if s > 0.3 * SPS][0]
      a, b = s0 - int(0.010 * SPS), s0 + int(0.020 * SPS)
      ms = (np.arange(a, b) - s0) / SPS * 1000
      ax.fill_between(ms, -0.85, 0.85, where=splicing[a:b],
                      color='#eeeeee', lw=0, label='crossfade')
      ax.plot(ms, y[a:b], color=SITE_ACCENT, lw=1.6)
      ax.set_ylim(-0.85, 0.85)
      ax.set_title(title, fontsize=15, loc='left')
      style(ax)
   axs[1].set_xlabel('ms from the splice')
   fig.tight_layout()
   path = os.path.join(OUT_DIR, 'bacf_pitch_shifter_jump.svg')
   fig.savefig(path)
   plt.close(fig)
   print('wrote', path)


def gen_cycles(exe):
   # Six cycles of the tone, each painted by the input period it comes
   # from: the input, then the output up a fifth (a cycle is played
   # twice) and down a fourth (a cycle is skipped). Grey: a crossfade.
   n = int(0.6 * SPS)
   x = tone(n)
   a = int(0.40 * SPS)
   b = a + int(6 * PERIOD)
   ms = (np.arange(a, b) - a) / SPS * 1000

   def paint(ax, y, pos):
      for i in range(a, b - 1):
         k = int(pos[i - a] // PERIOD)
         ax.plot(ms[i - a:i - a + 2], y[i:i + 2],
                 color=CYCLE_COLORS[k % len(CYCLE_COLORS)], lw=2)

   fig, axs = plt.subplots(3, 1, figsize=(WIDE / 72, 540 / 72), sharex=True)
   paint(axs[0], x, np.arange(a, b))
   axs[0].set_title('The input, each cycle its own color', fontsize=15,
                    loc='left')
   for ax, r, title in [
      (axs[1], '1.5', 'Up a fifth: read faster, and a cycle played twice'),
      (axs[2], '0.75', 'Down a fourth: read slower, and a cycle skipped'),
   ]:
      d = run(exe, x, 'sola', r)
      y, main, incoming = d[:, 0], d[:, 1], d[:, 2]
      splicing = incoming >= 0
      # Within a crossfade, the cycle is the read heard more: the
      # outgoing one in the first half, the incoming one after.
      pos = main[a:b].copy()
      run_start = None
      for i in range(a, b):
         if splicing[i]:
            if run_start is None:
               run_start = i
         elif run_start is not None:
            mid = (run_start + i) // 2
            pos[mid - a:i - a] = incoming[mid:i]
            run_start = None
      ax.fill_between(ms, -0.85, 0.85, where=splicing[a:b],
                      color='#eeeeee', lw=0)
      paint(ax, y, pos)
      ax.set_title(title, fontsize=15, loc='left')
   for ax in axs:
      ax.set_ylim(-0.85, 0.85)
      style(ax)
   axs[2].set_xlabel('ms')
   fig.tight_layout()
   path = os.path.join(OUT_DIR, 'bacf_pitch_shifter_cycles.svg')
   fig.savefig(path)
   plt.close(fig)
   print('wrote', path)


def gen():
   n, w, pitch, period = 14, 38, 40, 5
   x0 = (WIDE - n * pitch - 40) / 2
   rows = [
      ('Pitch up: the read catches up. Before it reaches the write, '
       'it jumps back.', 1),
      ('Pitch down: the read falls behind. Past a period behind, it jumps '
       'forward.', 9),
   ]
   row_h = 190
   width = WIDE
   height = 10 + row_h * len(rows)
   out = ['<?xml version="1.0" encoding="utf-8"?>',
          f'<svg xmlns="http://www.w3.org/2000/svg" '
          f'viewBox="0 0 {width} {height}" font-family=\'{FONT}\'>',
          '<defs><marker id="head" viewBox="0 0 10 10" refX="9" refY="5" '
          'markerWidth="6" markerHeight="6" orient="auto">'
          '<path d="M0,0 L10,5 L0,10 z" fill="context-stroke"/></marker>'
          '</defs>']
   cx = lambda k: x0 + k * pitch + w / 2
   newest = n - 1

   y = 10
   for text, delay in rows:
      read = newest - delay
      up = delay < period
      to = read - period if up else read + period
      out.append(f'<text x="{x0}" y="{y + 16}" font-size="15" '
                 f'fill="{INK}">{text}</text>')
      by = y + 64
      for k in range(n):
         match = k in (read, to)
         hot = read < k <= newest
         fill = MATCH_FILL if match else HOT_FILL if hot else BOX_FILL
         line = MATCH_LINE if match else SITE_ACCENT if hot else BOX_LINE
         out.append(f'<rect x="{x0 + k * pitch}" y="{by}" width="{w}" '
                    f'height="{w}" rx="4" fill="{fill}" stroke="{line}"/>')
         ph = 2 * np.pi * k / period
         v = (np.sin(ph) + 0.4 * np.sin(2 * ph + 0.8)) / 1.3
         mid = by + w / 2
         top = mid - v * (w / 2 - 6)
         out.append(f'<polyline points="{cx(k):.1f},{mid} {cx(k):.1f},'
                    f'{top:.1f}" stroke="{MUTED}" stroke-width="1.5"/>')
         out.append(f'<circle cx="{cx(k):.1f}" cy="{top:.1f}" r="3" '
                    f'fill="{MUTED}"/>')

      # Write head above the newest sample
      out.append(f'<text x="{cx(newest)}" y="{by - 24}" font-size="15" '
                 f'text-anchor="middle" fill="{INK}">write</text>')
      out.append(f'<line x1="{cx(newest)}" y1="{by - 20}" '
                 f'x2="{cx(newest)}" y2="{by - 3}" stroke="{INK}" '
                 f'stroke-width="2" marker-end="url(#head)"/>')

      # Read head below its sample
      out.append(f'<line x1="{cx(read)}" y1="{by + w + 20}" '
                 f'x2="{cx(read)}" y2="{by + w + 3}" stroke="{SIGNAL_RED}" '
                 f'stroke-width="2" marker-end="url(#head)"/>')
      out.append(f'<text x="{cx(read)}" y="{by + w + 36}" font-size="15" '
                 f'text-anchor="middle" fill="{SIGNAL_RED}">read</text>')

      # The splice: an arc above the boxes from the read to its new place
      ay = by - 8
      out.append(f'<path d="M{cx(read)},{ay} Q{(cx(read) + cx(to)) / 2},'
                 f'{ay - 34} {cx(to)},{ay}" fill="none" stroke="{SIGNAL_RED}" '
                 f'stroke-width="1.6" stroke-dasharray="5 3" '
                 f'marker-end="url(#head)"/>')
      label = 'splice: back one period' if up else \
         'splice: forward one period'
      out.append(f'<text x="{(cx(read) + cx(to)) / 2}" y="{ay - 26}" '
                 f'font-size="15" text-anchor="middle" '
                 f'fill="{SIGNAL_RED}">{label}</text>')
      out.append(f'<text x="{x0 + n * pitch + 6}" y="{by + w / 2 + 5}" '
                 f'font-size="15" fill="{SITE_ACCENT}">delay {delay}</text>')
      y += row_h

   out.append(f'<text x="{x0}" y="{height - 14}" font-size="15" '
              f'fill="{MUTED}">The amber samples are one period apart: the '
              f'same point in the cycle.</text>')
   out.append('</svg>')
   path = os.path.join(OUT_DIR, 'bacf_pitch_shifter_splices.svg')
   with open(path, 'w') as f:
      f.write('\n'.join(out) + '\n')
   print('wrote', path)


if __name__ == '__main__':
   gen()
   with tempfile.TemporaryDirectory() as tmp:
      exe = build_probe(tmp)
      gen_jump(exe)
      gen_cycles(exe)
