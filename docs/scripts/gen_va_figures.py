#!/usr/bin/env python3
"""
Generate the virtual analog reference figures.

Produces, in docs/modules/ROOT/images/:

   ladder_poles.svg     the magnitude response at 2, 4 and 6 poles, against
                        the prototype each one should follow
   ota_cell.svg         what the two cells do to a sine: the transistor
                        ladder's odd harmonics against the OTA cell's second
   analog_osc.svg       the three waveforms one core gives from one phase
   hard_sync.svg        a slave restarted by its master, with the restarts
                        marked
   triangle_symmetry.svg  the triangle as its symmetry is skewed, and the
                        even harmonics that come up with it
   vcf_sweep.svg        an animated SVG: a sawtooth as the filter closes
                        over it, one settled cycle a frame

Every figure is measured, not drawn: this compiles a probe against the
headers, runs it, and plots what comes back, so a figure cannot drift from
the code it illustrates.

Style and palette follow gen_fm_figures.py.

Usage: python3 docs/scripts/gen_va_figures.py
"""

import json
import os
import subprocess
import tempfile

import numpy as np
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt

SKY = '#64b5f6'
SITE_ACCENT = '#1565c0'
AMBER = '#ffb300'
GREEN = '#43a047'
PINK = '#d81b60'
GRAY = '#5d5d5d'

SPS = 48000.0
HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
OUT_DIR = os.path.join(HERE, '..', 'modules', 'ROOT', 'images')

PROBE = r'''
#include <q/fx/ladder.hpp>
#include <q/synth/va/analog_osc.hpp>
#include <q/synth/va/sync.hpp>
#include <q/synth/va/triangle_osc.hpp>
#include <q/support/literals.hpp>
#include <cmath>
#include <cstdio>
#include <vector>

namespace q = cycfi::q;
using namespace q::literals;
constexpr float sps = 48000.0f;

// The settled magnitude of a filter at one frequency, in dB.
template <typename F>
double response(F& filt, double hz)
{
   auto step = 2.0 * M_PI * hz / sps;
   double peak = 0;
   for (int i = 0; i != 40000; ++i)
   {
      auto y = filt(float(std::sin(step * i)));
      if (i > 20000)
         peak = std::max(peak, std::abs(double(y)));
   }
   filt = 0.0f;
   return 20.0 * std::log10(std::max(peak, 1e-9));
}

// Harmonic magnitudes of a filter's output for a sine in.
template <typename F>
void harmonics(F& filt, double f0, double amp, double* out, int n)
{
   int len = 48000;
   std::vector<double> y(len);
   for (int i = 0; i != len * 2; ++i)
   {
      auto v = filt(float(amp * std::sin(2 * M_PI * f0 * i / sps)));
      if (i >= len)
         y[i - len] = v;
   }
   for (int k = 1; k <= n; ++k)
   {
      double re = 0, im = 0;
      for (int i = 0; i != len; ++i)
      {
         auto p = 2 * M_PI * f0 * k * i / sps;
         re += y[i] * std::cos(p);
         im += y[i] * std::sin(p);
      }
      out[k - 1] = 2.0 * std::hypot(re, im) / len;
   }
}

int main()
{
   std::printf("{\n");

   // 1. the response at three pole counts, with no resonance
   std::printf("\"poles\": {");
   {
      q::basic_ladder<q::detail::moog_cell, 2> two{1_kHz, sps};
      q::basic_ladder<q::detail::moog_cell, 4> four{1_kHz, sps};
      q::basic_ladder<q::detail::moog_cell, 6> six{1_kHz, sps};
      char const* names[] = {"2", "4", "6"};
      for (int which = 0; which != 3; ++which)
      {
         std::printf("%s\"%s\": [", which ? ", " : "", names[which]);
         for (int i = 0; i != 40; ++i)
         {
            double hz = 100.0 * std::pow(10.0, i / 39.0 * 2.2);
            double db = which == 0? response(two, hz) :
                        which == 1? response(four, hz) : response(six, hz);
            std::printf("%s[%.2f, %.3f]", i ? ", " : "", hz, db);
         }
         std::printf("]");
      }
   }
   std::printf("},\n");

   // 2. the two cells, driven, and what they add
   {
      // Both at a working resonance: the ladder's cell sits in the
      // feedback path, so with no resonance there is no feedback and
      // nothing to saturate.
      double moog[8], ota[8];
      q::moog_ladder m{6_kHz, sps, 0.3f, true};
      harmonics(m, 500.0, 0.8, moog, 8);
      q::ota_ladder o{6_kHz, sps, 0.3f};
      o.drive(1.0f);
      harmonics(o, 500.0, 0.8, ota, 8);
      std::printf("\"cells\": {\"moog\": [");
      for (int i = 0; i != 8; ++i)
         std::printf("%s%.6f", i ? ", " : "", moog[i]);
      std::printf("], \"ota\": [");
      for (int i = 0; i != 8; ++i)
         std::printf("%s%.6f", i ? ", " : "", ota[i]);
      std::printf("]},\n");
   }

   // 3. one core, three waveforms, one phase
   {
      q::analog_osc osc{0.3f};
      osc.symmetry(0.5f);
      q::phase_iterator pi{200_Hz, sps};
      std::printf("\"waves\": [");
      for (int i = 0; i != 480; ++i, ++pi)
         std::printf("%s[%.4f, %.4f, %.4f]", i ? ", " : "",
            osc.saw(pi), osc.pulse(pi), osc.triangle(pi));
      std::printf("],\n");
   }

   // 4. hard sync: the slave restarted by the master
   {
      q::analog_osc osc;
      q::phase_iterator m{200_Hz, sps};
      q::phase_iterator s{560_Hz, sps};
      q::hard_sync sync;
      std::printf("\"sync\": [");
      for (int i = 0; i != 480; ++i)
      {
         ++m; ++s;
         bool at = sync(s, m);
         std::printf("%s[%.4f, %d]", i ? ", " : "", osc.saw(s), at ? 1 : 0);
      }
      std::printf("],\n");
   }

   // 5. a sawtooth through the filter, at a sweep of cutoffs
   {
      std::printf("\"vcf\": {\"cutoff\": [");
      int frames = 28, points = 480;
      std::vector<double> hz(frames);
      for (int k = 0; k != frames; ++k)
      {
         hz[k] = 100.0 * std::pow(120.0, double(k) / (frames - 1));
         std::printf("%s%.1f", k ? ", " : "", hz[k]);
      }
      std::printf("], \"frames\": [");
      for (int k = 0; k != frames; ++k)
      {
         q::ota_ladder f{q::frequency{hz[k]}, sps, 0.1f};
         f.drive(1.0f);
         q::analog_osc osc;
         q::phase_iterator pi{200_Hz, sps};
         for (int i = 0; i != 4800; ++i, ++pi)   // let it settle
            f(osc.saw(pi) * 0.7f);
         std::printf("%s[", k ? ", " : "");
         for (int i = 0; i != points; ++i, ++pi)
            std::printf("%s%.4f", i ? ", " : "", f(osc.saw(pi) * 0.7f));
         std::printf("]");
      }
      std::printf("], \"raw\": [");
      {
         q::analog_osc osc;
         q::phase_iterator pi{200_Hz, sps};
         for (int i = 0; i != points; ++i, ++pi)
            std::printf("%s%.4f", i ? ", " : "", osc.saw(pi) * 0.7f);
      }
      std::printf("]},\n");
   }

   // 6. the triangle as its symmetry is skewed
   {
      std::printf("\"triangle\": {");
      float syms[] = {0.5f, 0.25f, 0.1f};
      for (int k = 0; k != 3; ++k)
      {
         q::basic_triangle_osc osc{syms[k]};
         q::phase_iterator pi{200_Hz, sps};
         std::printf("%s\"%.2f\": {\"wave\": [", k ? ", " : "", syms[k]);
         for (int i = 0; i != 240; ++i, ++pi)
            std::printf("%s%.4f", i ? ", " : "", osc(pi));
         std::printf("], \"harm\": [");

         int len = 4800;                    // 20 cycles of 200 Hz
         std::vector<double> y(len);
         q::phase_iterator p2{200_Hz, sps};
         for (int i = 0; i != len; ++i, ++p2)
            y[i] = osc(p2);
         for (int h = 1; h <= 8; ++h)
         {
            double re = 0, im = 0;
            for (int i = 0; i != len; ++i)
            {
               auto ph = 2 * M_PI * 200.0 * h * i / sps;
               re += y[i] * std::cos(ph);
               im += y[i] * std::sin(ph);
            }
            std::printf("%s%.6f", h > 1 ? ", " : "",
               2.0 * std::hypot(re, im) / len);
         }
         std::printf("]}");
      }
      std::printf("}\n");
   }

   std::printf("}\n");
}
'''


def probe():
   """Compile the probe against the headers and read back its measurements."""
   with tempfile.TemporaryDirectory() as tmp:
      src = os.path.join(tmp, 'probe.cpp')
      exe = os.path.join(tmp, 'probe')
      with open(src, 'w') as f:
         f.write(PROBE)
      subprocess.run(
         ['c++', '-std=c++20', '-O2', '-w',
          '-I', os.path.join(REPO, 'q_lib/include'),
          '-I', os.path.join(REPO, 'infra/include'),
          '-o', exe, src], check=True)
      return json.loads(subprocess.run([exe], capture_output=True,
                                       check=True).stdout)


def save(fig, name):
   out = os.path.join(OUT_DIR, name)
   fig.savefig(out, format='svg', bbox_inches='tight')
   plt.close(fig)
   print('wrote', os.path.normpath(out))


def fig_poles(data):
   fig, ax = plt.subplots(figsize=(9, 4.2))
   colors = {'2': GREEN, '4': SITE_ACCENT, '6': AMBER}

   for poles, points in data['poles'].items():
      hz = np.array([p[0] for p in points])
      db = np.array([p[1] for p in points])
      ax.semilogx(hz, db, color=colors[poles], linewidth=1.8,
                  label=f'{poles} poles')

      # what the prototype says it should be, at the prewarped ratio
      r = np.tan(np.pi * hz / SPS) / np.tan(np.pi * 1000.0 / SPS)
      ideal = -10.0 * int(poles) * np.log10(1 + r * r)
      ax.semilogx(hz, ideal, color=colors[poles], linewidth=0.9,
                  linestyle='--', alpha=0.7)

   ax.axvline(1000, color=PINK, linestyle=':', linewidth=1.0)
   ax.text(1030, 4, 'cutoff', color=PINK, fontsize=9)
   ax.set_xlim(100, 16000)
   ax.set_ylim(-80, 8)
   ax.set_xlabel('Frequency (Hz), cutoff at 1 kHz')
   ax.set_ylabel('Magnitude (dB)')
   ax.grid(True, which='both', linestyle='--', linewidth=0.5,
           color='#b0b0b0', alpha=0.8)
   ax.legend(loc='lower left', fontsize=9)
   save(fig, 'ladder_poles.svg')


def fig_cells(data):
   fig, ax = plt.subplots(figsize=(9, 3.8))
   moog = np.array(data['cells']['moog'])
   ota = np.array(data['cells']['ota'])

   def db(x):
      return 20 * np.log10(np.maximum(x / x[0], 1e-6))

   k = np.arange(1, 9)
   width = 0.38
   floor = -90
   ax.bar(k - width / 2, db(moog) - floor, width, bottom=floor,
          color=SITE_ACCENT, label="transistor ladder cell")
   ax.bar(k + width / 2, db(ota) - floor, width, bottom=floor,
          color=AMBER, label="variable gain (OTA) cell")

   ax.set_xticks(k)
   ax.set_xlabel('Harmonic of the input sine')
   ax.set_ylabel('Level against the fundamental (dB)')
   ax.set_ylim(-90, 4)
   ax.set_title(
      'a 500 Hz sine through each filter at resonance 0.3, cutoff 6 kHz,'
      ' the OTA cell at drive 1',
      fontsize=9, color=GRAY)
   ax.grid(True, axis='y', linestyle='--', linewidth=0.5, color='#b0b0b0',
           alpha=0.8)
   ax.legend(loc='upper right', fontsize=9)
   save(fig, 'ota_cell.svg')


def fig_waves(data):
   w = np.array(data['waves'])
   t = np.arange(len(w)) / SPS * 1000
   fig, ax = plt.subplots(figsize=(9, 3.4))
   ax.plot(t, w[:, 0], color=SITE_ACCENT, linewidth=1.6, label='saw')
   ax.plot(t, w[:, 1], color=AMBER, linewidth=1.6, label='pulse (width 0.3)')
   ax.plot(t, w[:, 2], color=GREEN, linewidth=1.6, label='triangle')
   ax.set_xlim(0, t[-1])
   ax.set_ylim(-1.35, 1.35)
   ax.set_xlabel('Time (ms), one oscillator at 200 Hz')
   ax.set_ylabel('Amplitude')
   ax.grid(True, linestyle='--', linewidth=0.5, color='#b0b0b0', alpha=0.8)
   ax.legend(loc='upper right', fontsize=9, ncol=3)
   save(fig, 'analog_osc.svg')


def fig_sync(data):
   s = np.array(data['sync'])
   y, marks = s[:, 0], s[:, 1]
   t = np.arange(len(y)) / SPS * 1000
   fig, ax = plt.subplots(figsize=(9, 3.4))
   ax.plot(t, y, color=SITE_ACCENT, linewidth=1.6)
   for at in t[marks > 0.5]:
      ax.axvline(at, color=PINK, linestyle=':', linewidth=1.1)
   ax.plot([], [], color=PINK, linestyle=':', linewidth=1.1,
           label="the master's cycle, where the slave restarts")
   ax.set_xlim(0, t[-1])
   ax.set_ylim(-1.35, 1.35)
   ax.set_xlabel('Time (ms), master at 200 Hz and slave at 560 Hz')
   ax.set_ylabel('Amplitude')
   ax.grid(True, linestyle='--', linewidth=0.5, color='#b0b0b0', alpha=0.8)
   ax.legend(loc='upper right', fontsize=9)
   save(fig, 'hard_sync.svg')


def fig_triangle(data):
   fig, (top, bottom) = plt.subplots(2, 1, figsize=(9, 5.6))
   colors = [SITE_ACCENT, AMBER, GREEN]

   for color, (sym, d) in zip(colors, sorted(data['triangle'].items(),
                                             reverse=True)):
      wave = np.array(d['wave'])
      t = np.arange(len(wave)) / SPS * 1000
      top.plot(t, wave, color=color, linewidth=1.6, label=f'symmetry {sym}')

      harm = np.array(d['harm'])
      db = 20 * np.log10(np.maximum(harm / harm[0], 1e-6))
      bottom.plot(np.arange(1, 9), db, 'o-', color=color, linewidth=1.4,
                  markersize=4, label=f'symmetry {sym}')

   top.set_xlim(0, 10)
   top.set_ylim(-1.2, 1.2)
   top.set_xlabel('Time (ms) at 200 Hz')
   top.set_ylabel('Amplitude')
   top.grid(True, linestyle='--', linewidth=0.5, color='#b0b0b0', alpha=0.8)
   top.legend(loc='upper right', fontsize=9, ncol=3)

   bottom.set_xlabel('Harmonic')
   bottom.set_ylabel('Level against the fundamental (dB)')
   bottom.set_ylim(-80, 4)
   bottom.grid(True, linestyle='--', linewidth=0.5, color='#b0b0b0',
               alpha=0.8)
   bottom.legend(loc='upper right', fontsize=9)

   fig.tight_layout()
   save(fig, 'triangle_symmetry.svg')


def fig_vcf_sweep(data):
   """An animated SVG: a sawtooth as the filter closes over it.

   SMIL rather than a script, because Antora loads an image:: in an <img>
   tag, where scripts do not run but declarative animation does. Every frame
   has the same point count, so the path morphs between them.
   """
   width, height = 700, 300
   left, right = 60, 660
   mid, amp = 130, 88
   seconds = 9.0

   cutoffs = data['vcf']['cutoff']
   frames = [np.array(f) for f in data['vcf']['frames']]
   raw = np.array(data['vcf']['raw'])

   # The filter delays what it passes, and by a different amount at every
   # cutoff, so without this every frame would sit at its own horizontal
   # offset and the wave would slide instead of change shape. Each frame is
   # rolled to put its steepest fall, the sawtooth's reset, in one place.
   def align(y, at=0.41):
      half = len(y) // 2                   # the reset in the first cycle
      drop = int(np.argmin(np.diff(y[:half])))
      return np.roll(y, int(at * len(y)) - drop)

   frames = [align(f) for f in frames]
   raw = align(raw)

   peak = max(np.max(np.abs(f)) for f in frames)

   px = left + (right - left) * np.arange(len(frames[0])) / (len(frames[0]) - 1)

   def path(y):
      py = mid - amp * (y / peak)
      return 'M ' + ' L '.join(f'{a:.1f},{b:.1f}' for a, b in zip(px, py))

   values = [path(f) for f in frames]
   values = values + values[-2::-1]              # and back, so it loops

   meter_left, meter_right = left, left + 220
   meter_y = 250
   span = np.log10(cutoffs[-1] / cutoffs[0])
   widths = [(meter_right - meter_left) * np.log10(c / cutoffs[0]) / span
             for c in cutoffs]
   widths = widths + widths[-2::-1]

   def anim(attr, vals, fmt='{}'):
      joined = ';'.join(fmt.format(v) for v in vals)
      return (f'<animate attributeName="{attr}" dur="{seconds}s" '
              f'repeatCount="indefinite" calcMode="linear" '
              f'values="{joined}"/>')

   svg = f"""<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 {width} {height}" width="{width}" height="{height}" font-family="Helvetica, Arial, sans-serif">

  <!-- Generated by docs/scripts/gen_va_figures.py. A 200 Hz sawtooth
       through q::ota_ladder at resonance 0.1, as the cutoff sweeps from
       {cutoffs[0]:.0f} Hz to {cutoffs[-1] / 1000:.1f} kHz and back. Every
       frame is the filter's real output, settled, at one common scale, so
       the level falling as the filter closes is the filter's own doing. SMIL animation, so it plays inside an Antora image:: (an
       <img> tag, where scripts do not run). -->

  <line x1="{left}" y1="{mid}" x2="{right}" y2="{mid}" stroke="#b0b0b0" stroke-width="1" stroke-dasharray="4 4"/>

  <!-- where it started: the sawtooth itself, unfiltered -->
  <path d="{path(raw)}" fill="none" stroke="{SKY}" stroke-width="1.2" opacity="0.5"/>

  <!-- the filter's output, morphing -->
  <path fill="none" stroke="{SITE_ACCENT}" stroke-width="2.2" stroke-linejoin="round">
    {anim('d', values)}
  </path>

  <!-- where the cutoff has reached -->
  <rect x="{meter_left}" y="{meter_y}" width="{meter_right - meter_left}" height="8" rx="4" fill="#eef0f9"/>
  <rect x="{meter_left}" y="{meter_y}" height="8" rx="4" fill="{SITE_ACCENT}">
    {anim('width', widths, '{:.1f}')}
  </rect>
  <text x="{meter_left}" y="{meter_y + 28}" font-size="13" fill="{GRAY}">{cutoffs[0]:.0f} Hz</text>
  <text x="{meter_right}" y="{meter_y + 28}" font-size="13" fill="{GRAY}" text-anchor="end">{cutoffs[-1] / 1000:.0f} kHz</text>
  <text x="{meter_right + 22}" y="{meter_y + 8}" font-size="14" fill="#1a1a1a">cutoff</text>
</svg>
"""
   out = os.path.join(OUT_DIR, 'vcf_sweep.svg')
   with open(out, 'w') as f:
      f.write(svg)
   print('wrote', os.path.normpath(out))


if __name__ == '__main__':
   d = probe()
   fig_poles(d)
   fig_cells(d)
   fig_waves(d)
   fig_sync(d)
   fig_triangle(d)
   fig_vcf_sweep(d)
