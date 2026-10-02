#!/usr/bin/env python3
"""
Generate the DX7 algorithm chart, docs/modules/ROOT/images/dx7_algorithms.svg.

The 32 algorithms drawn the way the DX7's own panel draws them: a box per
operator, a modulator sitting above what it modulates, the carriers along
the bottom, and the feedback loop where the algorithm has one.

The figure is drawn from the library, not from a copy of the chart: this
script compiles a small program against q/synth/fm/dx7_routing.hpp, reads
back each routing's edges, carriers and feedback, and lays that out. So the
picture cannot drift from dx7_routing, and a wrong row would show up as a
wrong picture.

Usage: python3 docs/scripts/gen_dx7_chart.py
"""

import json
import os
import subprocess
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)                       # docs/
REPO = os.path.dirname(ROOT)
OUT = os.path.join(ROOT, 'modules/ROOT/images/dx7_algorithms.svg')

CARRIER_FILL = '#dce7f2'                           # the shaded carriers
INK, GRAY = '#1a1a1a', '#5d5d5d'

DUMPER = r'''
#include <q/synth/fm/dx7_routing.hpp>
#include <cstdio>
namespace q = cycfi::q;
int main()
{
   std::printf("[");
   for (int a = 0; a != 32; ++a)
   {
      auto t = q::dx7_routing[a].get();
      std::printf("%s{\"n\":%d,\"edges\":[", a ? "," : "", a + 1);
      bool first = true;
      for (int to = 0; to != 6; ++to)
         for (int from = 0; from != 6; ++from)
            if (t.mod[to] & (1 << from))
            {
               std::printf("%s[%d,%d]", first ? "" : ",", from + 1, to + 1);
               first = false;
            }
      std::printf("],\"carriers\":[");
      first = true;
      for (int i = 0; i != 6; ++i)
         if (t.carriers & (1 << i))
         {
            std::printf("%s%d", first ? "" : ",", i + 1);
            first = false;
         }
      std::printf("],\"fb_src\":%d,\"fb_dst\":%d}", t.fb_src + 1, t.fb_dst + 1);
   }
   std::printf("]\n");
}
'''


def routings():
   """The 32 routings, read out of the compiled library."""
   with tempfile.TemporaryDirectory() as tmp:
      src = os.path.join(tmp, 'dump.cpp')
      exe = os.path.join(tmp, 'dump')
      with open(src, 'w') as f:
         f.write(DUMPER)
      subprocess.run(
         ['c++', '-std=c++20', '-O1', '-w',
          '-I', os.path.join(REPO, 'q_lib/include'),
          '-I', os.path.join(REPO, 'infra/include'),
          '-o', exe, src], check=True)
      return json.loads(subprocess.run([exe], capture_output=True,
                                       check=True).stdout)


def layout(alg):
   """Where each operator sits: (column, row), rows counted from the bottom.

   A carrier is on the bottom row, a modulator one row above the highest
   thing it modulates. Columns come from the operators below: a modulator
   sits over the middle of them, and operators that would land on the same
   spot are spread apart. Separate stacks are placed side by side, in
   operator order, the way the chart reads.
   """
   targets = {n: [] for n in range(1, 7)}
   for src, dst in alg['edges']:
      targets[src].append(dst)

   row = {}

   def depth(n):
      if n not in row:
         row[n] = 1 + max((depth(t) for t in targets[n]), default=-1)
      return row[n]

   for n in range(1, 7):
      depth(n)

   # the operators of each stack, found through the edges
   group = {n: n for n in range(1, 7)}

   def root(n):
      while group[n] != n:
         n = group[n]
      return n

   for src, dst in alg['edges']:
      a, b = root(src), root(dst)
      if a != b:
         group[max(a, b)] = min(a, b)

   stacks = {}
   for n in range(1, 7):
      stacks.setdefault(root(n), []).append(n)

   col = {}
   place = 0.0
   for _, ops in sorted(stacks.items()):
      local = {}
      slot = 0.0
      for n in sorted(ops):                      # the carriers, left to right
         if row[n] == 0:
            local[n] = slot
            slot += 1.0
      for r in range(1, max(row[n] for n in ops) + 1):
         level = sorted(n for n in ops if row[n] == r)
         if not level:
            continue
         want = [sum(local[t] for t in targets[n]) / len(targets[n])
                 for n in level]
         order = sorted(range(len(level)), key=lambda i: want[i])
         spread = list(want)
         for k in range(1, len(order)):          # keep them a box apart
            i, j = order[k - 1], order[k]
            if spread[j] - spread[i] < 1.0:
               spread[j] = spread[i] + 1.0
         shift = sum(want) / len(want) - sum(spread) / len(spread)
         for n, x in zip(level, spread):
            local[n] = x + shift
      low = min(local.values())
      for n in ops:
         col[n] = local[n] - low + place
      place += max(local.values()) - low + 1.0   # a column between stacks
   return col, row


def feedback_lane(alg, row, box, box_w, loop=9):
   """Where the feedback loop runs: beside its operators, clear of the
   others. box(n) gives an operator's center.
   """
   src, dst = alg['fb_src'], alg['fb_dst']
   sx, _ = box(src)
   dx, _ = box(dst)
   spanned = set(range(min(row[src], row[dst]), max(row[src], row[dst]) + 2))

   def free(lane):
      for n in range(1, 7):
         if n in (src, dst):
            continue
         cx, _ = box(n)
         if row[n] in spanned and abs(cx - lane) < box_w / 2 + 3:
            return False
         if row[n] == row[dst] + 1 and min(lane, dx) - 3 < cx < max(lane, dx) + 3:
            return False
      return True

   near = [box(n)[0] for n in range(1, 7) if row[n] in spanned]
   right, left = max(sx, dx) + box_w / 2, min(sx, dx) - box_w / 2
   return next(x for x in (right + loop, left - loop,
                           max(near) + box_w / 2 + loop,
                           min(near) - box_w / 2 - loop)
               if free(x))


def cell(alg, x0, y0, unit, box_w, box_h, row_h, cell_mid):
   """One algorithm, drawn the way the DX7's own chart draws it.

   Plain lines, no arrowheads: a modulator sits above what it modulates
   and a line joins them, the carriers are shaded and tied together by
   the bus along the bottom, and feedback is the small rectangle that
   leaves an operator and returns to it.
   """
   col, row = layout(alg)
   carriers = set(alg['carriers'])
   out = []

   def box(n):
      cx = x0 + col[n] * unit + box_w / 2
      cy = y0 - row[n] * row_h - box_h / 2
      return cx, cy

   for src, dst in alg['edges']:                 # who modulates whom
      sx, sy = box(src)
      dx, dy = box(dst)
      out.append(f'  <line x1="{sx:.1f}" y1="{sy + box_h / 2:.1f}" '
                 f'x2="{dx:.1f}" y2="{dy - box_h / 2:.1f}" '
                 f'stroke="{INK}" stroke-width="1.2"/>')

   src, dst = alg['fb_src'], alg['fb_dst']       # and what feeds itself back
   sx, sy = box(src)
   dx, dy = box(dst)
   lane = feedback_lane(alg, row, box, box_w)
   leave = sx + box_w / 2 if lane > sx else sx - box_w / 2
   out.append(f'  <path d="M {leave:.1f} {sy:.1f} H {lane:.1f} '
              f'V {dy - box_h / 2 - 9:.1f} H {dx:.1f} V {dy - box_h / 2:.1f}" '
              f'fill="none" stroke="{INK}" stroke-width="1.2"/>')

   low = y0 + 11                                 # the bus under the carriers
   ordered = sorted(carriers, key=lambda n: col[n])
   ends = [box(n)[0] for n in ordered]
   for cx in ends:
      out.append(f'  <line x1="{cx:.1f}" y1="{y0:.1f}" x2="{cx:.1f}" '
                 f'y2="{low:.1f}" stroke="{INK}" stroke-width="1.2"/>')
   if len(ends) > 1:
      out.append(f'  <line x1="{ends[0]:.1f}" y1="{low:.1f}" '
                 f'x2="{ends[-1]:.1f}" y2="{low:.1f}" '
                 f'stroke="{INK}" stroke-width="1.2"/>')

   for n in range(1, 7):
      cx, cy = box(n)
      fill = CARRIER_FILL if n in carriers else '#ffffff'
      out.append(f'  <rect x="{cx - box_w / 2:.1f}" y="{cy - box_h / 2:.1f}" '
                 f'width="{box_w}" height="{box_h}" fill="{fill}" '
                 f'stroke="{INK}" stroke-width="1.2"/>')
      out.append(f'  <text x="{cx:.1f}" y="{cy + 5:.1f}" text-anchor="middle" '
                 f'font-size="13" fill="{INK}">{n}</text>')

   out.append(f'  <text x="{cell_mid:.0f}" y="{low + 30:.0f}" '
              f'text-anchor="middle" font-size="20" font-weight="bold" '
              f'fill="{INK}">{alg["n"]}</text>')
   return out, lane - x0


def main():
   algs = routings()
   for a in algs:                                # the picture's carriers are
      col, row = layout(a)                       # the library's carriers
      drawn = sorted(n for n in range(1, 7) if row[n] == 0)
      assert drawn == a['carriers'], (a['n'], drawn, a['carriers'])

   unit, box_w, box_h, row_h = 34, 34, 21, 34
   cols, per_col = 4, 8
   left = 16
   wide = max(max(layout(a)[0].values()) for a in algs)
   rows = max(max(layout(a)[1].values()) for a in algs)
   cell_w = wide * unit + box_w + 40
   cell_h = rows * row_h + box_h + 62             # the number sits below
   top = 24 + rows * row_h + box_h / 2   # the page carries the caption

   body = []
   for i, a in enumerate(algs):
      col, _ = layout(a)
      content = max(col.values()) * unit + box_w
      cell_left = left + (i % cols) * cell_w
      x0 = cell_left + (cell_w - 40 - content) / 2      # centred in its cell
      y0 = top + (i // cols) * cell_h
      drawn, lane = cell(a, x0, y0, unit, box_w, box_h, row_h,
                         cell_left + cell_w / 2)
      body += drawn
      assert (cell_left <= x0 + min(0.0, lane - 4)      # the loop stays in
              and x0 + max(content, lane + 4) <= cell_left + cell_w), \
         (a['n'], lane)

   width = left * 2 + cols * cell_w
   height = top + (per_col - 1) * cell_h + box_h + 46
   svg = (f'<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 {width:.0f} '
          f'{height:.0f}" width="{width:.0f}" height="{height:.0f}" '
          f'font-family="Helvetica, Arial, sans-serif">\n'
          f'\n  <!-- Generated by docs/scripts/gen_dx7_chart.py from the\n'
          f'       library\'s own dx7_routing table. -->\n'
          + '\n' + '\n'.join(body) + '\n</svg>\n')
   with open(OUT, 'w') as f:
      f.write(svg)
   print('wrote', OUT, f'({width:.0f} x {height:.0f})')


if __name__ == '__main__':
   main()
