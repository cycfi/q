#!/usr/bin/env python3
"""
Generate the adsr_envelope_gen sustain figure.

Produces, in docs/modules/ROOT/images/:

   adsr_sustain.svg  -- one note, released at 1.2 s, through two
                        adsr_envelope_gen configs that differ only in
                        whether they carry a sustain_rate.

The envelope is replayed with the arithmetic of q/synth/envelope_gen.hpp,
exponential_gen.hpp and linear_gen.hpp: the four segments, each entered
from the level the last one left, and a segment ending once it has run
ceil(width * sps) samples. A config with a sustain_rate gets a linear
downward ramp for its sustain; one without gets a constant segment.

Style and palette follow gen_interpolation_figures.py.

Usage: /usr/bin/python3 docs/scripts/gen_envelope_gen_figures.py
"""

import math
import os
import numpy as np
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt

SITE_ACCENT = '#1565c0'
AMBER = '#ffb300'
MAGENTA = '#d81b60'
GREY = '#b0b0b0'

OUT_DIR = os.path.join(
   os.path.dirname(__file__), '..', 'modules', 'ROOT', 'images')

SPS = 48000.0
ATTACK, DECAY, RELEASE = 0.020, 0.150, 0.300
SUSTAIN_DB = -12.0
SUSTAIN_RATE = 2.0
NOTE_OFF = 1.2
LENGTH = 1.7


class ExpUp:
   def __init__(self, width, cv=0.95):
      self.tau = -math.log(1 - cv)
      self.full = 1 / cv
      self.rate = math.exp(-self.tau / (SPS * width))
      self.y = 0.0

   def __call__(self):
      self.y = self.full + self.rate * (self.y - self.full)
      return self.y


class ExpDown(ExpUp):
   def __call__(self):
      return 1 - ExpUp.__call__(self)


class LinDown:
   def __init__(self, width):
      self.rate = 1 / (width * SPS)
      self.y = 0.0

   def __call__(self):
      self.y += self.rate
      return 1 - self.y


class Segment:
   def __init__(self, ramp, width, level):
      self.ramp, self.level = ramp, level
      self.end = math.ceil(width * SPS) if width else None
      self.time = 0
      self.offset = self.scale = 0.0

   def start(self, prev):
      self.offset = min(self.level, prev)
      self.scale = abs(self.level - prev)

   def __call__(self):
      self.time += 1
      if self.ramp is None:                   # constant segment
         return self.offset + self.scale
      return self.offset + self.ramp() * self.scale

   def done(self):
      return self.end is not None and self.time >= self.end


def adsr(sustain_rate):
   sus = 10 ** (SUSTAIN_DB / 20)
   if sustain_rate:
      sustain = Segment(LinDown(sustain_rate), sustain_rate, 0.0)
   else:
      sustain = Segment(None, None, sus)
   return [
      Segment(ExpUp(ATTACK), ATTACK, 1.0),
      Segment(ExpDown(DECAY), DECAY, sus),
      sustain,
      Segment(ExpDown(RELEASE), RELEASE, 0.0),
   ]


def run(sustain_rate):
   segs = adsr(sustain_rate)
   i, y = 0, 0.0
   segs[0].start(y)
   out = []
   for n in range(int(LENGTH * SPS)):
      if n == int(NOTE_OFF * SPS) and i != len(segs) - 1:
         i = len(segs) - 1                    # release()
         segs[i].start(y)
      if i == len(segs):
         out.append(0.0)
         continue
      y = segs[i]()
      if segs[i].done():
         prev = i
         i += 1
         if i != len(segs):
            segs[i].start(segs[prev].level)
      out.append(y)
   return np.array(out)


def gen():
   decaying = run(SUSTAIN_RATE)
   holding = run(None)
   t = np.arange(len(decaying)) / SPS
   k = slice(None, None, 24)                  # 2 kHz is plenty to draw

   fig, ax = plt.subplots(figsize=(10, 4.6))
   ax.plot(t[k], holding[k], color=SITE_ACCENT, linewidth=1.8,
           label='config without sustain_rate: holds')
   ax.plot(t[k], decaying[k], color=AMBER, linewidth=1.8,
           label=f'config with sustain_rate = {SUSTAIN_RATE:g} s: runs down')
   ax.axvline(NOTE_OFF, color=MAGENTA, linestyle=':', linewidth=1.4)
   ax.text(NOTE_OFF + 0.01, 0.95, 'release()', color=MAGENTA, fontsize=9,
           va='top')
   ax.set_xlim(0, LENGTH)
   ax.set_ylim(0, 1.05)
   ax.set_xlabel('Time (s)')
   ax.set_ylabel('Level')
   for s in ('top', 'right'):
      ax.spines[s].set_visible(False)
   ax.spines['left'].set_color(GREY)
   ax.spines['bottom'].set_color(GREY)
   ax.grid(True, linestyle='--', color=GREY, alpha=0.5)
   ax.legend(loc='center right', fontsize=9)

   fig.tight_layout()
   path = os.path.join(OUT_DIR, 'adsr_sustain.svg')
   fig.savefig(path, format='svg', bbox_inches=None)
   plt.close(fig)
   print('wrote', path)


if __name__ == '__main__':
   gen()
