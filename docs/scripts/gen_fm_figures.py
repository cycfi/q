#!/usr/bin/env python3
"""
Generate the FM synthesis reference figures.

Produces, in docs/modules/ROOT/images/:

   fm_wave.svg        -- an animated SVG: one carrier cycle as the
                         modulation index rises from 0, a sine turning
                         into something with harmonics, and back
   fm_feedback.svg    -- an animated SVG: an operator modulating itself,
                         a sine sweeping toward a sawtooth as the
                         feedback rises to a DX7 carrier's most
   fm_sidebands.svg   -- a carrier modulated at three indices: the
                         sidebands that phase modulation puts either side
                         of the carrier, and how they spread with the index
   dx_envelope.svg    -- the dx_envelope_gen attack curve against a
                         constant slope, and a whole note's envelope

The signals are computed the way q computes them: the modulated phase is
sin(wc t + b sin(wm t)), which is what fm_operator does when fm_algorithm
hands it a phase offset; the envelope replays the law in
dx_envelope_gen.hpp (a decay falls at its rate, an attack climbs by the
rate times 2 plus the doublings below full, from attack_from).

Both were checked against the compiled library, 2026-09-27: a
dx_envelope_gen at the same rate matched this envelope to 0.0000 dB over
the attack, and two fm_operators through an fm_algorithm at index 2 matched
this spectrum within 0.01 dB at every sideband out to the third.

Style and palette follow gen_interpolation_figures.py (the canonical
PALETTE lives there).

Usage: python3 docs/scripts/gen_fm_figures.py
"""

import os
import numpy as np
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt

SKY = '#64b5f6'            # the carrier alone
SITE_ACCENT = '#1565c0'    # the modulated spectrum, the envelope
AMBER = '#ffb300'          # secondary series
GREEN = '#43a047'          # reference: the constant slope
PINK = '#d81b60'           # event markers

SPS = 48000.0

OUT_DIR = os.path.join(
   os.path.dirname(__file__), '..', 'modules', 'ROOT', 'images')

# dx_envelope_gen's constants, in decibels and seconds
FLOOR = -89.93
ATTACK_FROM = -49.95
RANGE = 89.93


def save(fig, name):
   path = os.path.join(OUT_DIR, name)
   fig.savefig(path, format='svg', bbox_inches=None)
   plt.close(fig)
   print('wrote', path)


def spectrum(fc, fm, beta, seconds=1.0):
   """The magnitude spectrum of a phase modulated sine, in dB."""
   n = int(SPS * seconds)
   t = np.arange(n) / SPS
   y = np.sin(2 * np.pi * fc * t + beta * np.sin(2 * np.pi * fm * t))
   mag = np.abs(np.fft.rfft(y * np.hanning(n))) / (n / 4)
   freq = np.fft.rfftfreq(n, 1 / SPS)
   return freq, 20 * np.log10(np.maximum(mag, 1e-6))


def fig_sidebands():
   fc, fm = 2000.0, 400.0
   fig, axes = plt.subplots(3, 1, figsize=(10, 8), sharex=True)

   for ax, beta, color in zip(
      axes, [0.5, 2.0, 5.0], [SKY, SITE_ACCENT, AMBER]):
      freq, db = spectrum(fc, fm, beta)
      keep = freq <= 6000
      ax.axvline(fc, color=PINK, linestyle=':', linewidth=1.0, zorder=0)
      ax.plot(freq[keep], db[keep], color=color, linewidth=1.2, zorder=2)
      ax.set_ylim(-70, 5)
      ax.set_ylabel('dB')
      ax.grid(True, linestyle='--', linewidth=0.5, color='#b0b0b0', alpha=0.8)
      ax.text(
         0.985, 0.86, f'index {beta:g} rad', transform=ax.transAxes
       , ha='right', fontsize=11, color=color)

   axes[0].text(
      fc + 70, -22, 'carrier 2 kHz', color=PINK, fontsize=9, va='top')
   axes[-1].set_xlabel('Frequency (Hz), modulator at 400 Hz')
   fig.tight_layout()
   save(fig, 'fm_sidebands.svg')


def envelope(rates, levels, gate, total, curved=True):
   """dx_envelope_gen, sampled. rates in seconds for the whole range,
   levels in dB; gate is when release() is called."""
   step = [RANGE / (r * SPS) for r in rates]
   target = [max(l, FLOOR) for l in levels]
   n = int(total * SPS)
   out = np.empty(n)
   db = FLOOR
   seg = 0                          # 0..2 attack, 3 release, 4 hold, 5 idle
   rising = True
   released = False

   def start(i, db):
      t = target[3 if i == 3 else i]
      rise = t > db
      if rise and db < ATTACK_FROM:
         db = min(ATTACK_FROM, t)
      return db, rise

   db, rising = start(0, db)
   for i in range(n):
      if i == int(gate * SPS) and not released:
         released = True
         seg = 3
         db, rising = start(3, db)
      if seg < 4:
         k = 3 if seg == 3 else seg
         t = target[k]
         if db != t:
            if rising:
               below = np.floor(-db / 6.0206) if curved else 0.0
               factor = max(1.0, 2.0 + below) if curved else 1.0
               db = min(db + step[k] * factor, t)
            else:
               db = max(db - step[k], t)
            if db == t:
               if seg == 3:
                  seg = 5 if db <= FLOOR else 3
               else:
                  seg = 4 if seg == 2 else seg + 1
                  if seg < 3:
                     db, rising = start(seg, db)
      out[i] = db
   return out


def fig_envelope():
   fig, (top, bottom) = plt.subplots(2, 1, figsize=(10, 7))

   # The attack law against a constant slope, same rate
   rate = [1.0, 1e-6, 1e-6, 1e-6]
   level = [0.0, 0.0, 0.0, FLOOR]
   curve = envelope(rate, level, gate=10.0, total=0.2)
   flat = envelope(rate, level, gate=10.0, total=0.2, curved=False)
   t = np.arange(len(curve)) / SPS * 1000

   top.plot(t, curve, color=SITE_ACCENT, linewidth=1.6
    , label='the attack curve')
   top.plot(t, flat, color=GREEN, linewidth=1.4, linestyle='--'
    , label='a constant slope, same rate')
   top.axhline(-6, color=PINK, linestyle=':', linewidth=1.0)
   top.text(
      2, -4.5, 'the last 6 dB take 27% of the attack', color=PINK
    , fontsize=9)
   top.set_xlim(0, 200)
   top.set_ylim(-60, 4)
   top.set_xlabel('Time (ms), at a rate of one second for the whole range')
   top.set_ylabel('Level (dB)')
   top.grid(True, linestyle='--', linewidth=0.5, color='#b0b0b0', alpha=0.8)
   top.legend(loc='lower right', fontsize=9)

   # The DX7's own front panel diagram of the envelope, redrawn: the four
   # rates and the four levels, key on to key off. A schematic, as the
   # panel's is; what the attack really does is the plot above.
   ink = '#1a1a1a'
   corners = [
      (0.0, 1.0), (1.0, 1.0), (2.0, 3.7), (3.0, 2.0), (4.2, 1.35),
      (5.2, 1.35), (6.2, 1.0), (7.0, 1.0)]
   bottom.plot(*zip(*corners), color=ink, linewidth=2.0, zorder=3,
               solid_joinstyle='miter')

   for x, y, label, ha, va in [
      (0.9, 1.1, 'L4', 'right', 'bottom'), (2.0, 3.8, 'L1', 'center', 'bottom'),
      (2.95, 1.9, 'L2', 'right', 'top'), (4.2, 1.25, 'L3', 'center', 'top'),
      (6.3, 1.1, 'L4', 'left', 'bottom'),
      (1.35, 2.3, 'R1', 'right', 'center'), (2.65, 2.9, 'R2', 'left', 'center'),
      (3.7, 1.75, 'R3', 'left', 'center'), (5.8, 1.45, 'R4', 'left', 'center')]:
      bottom.text(x, y, label, fontsize=13, color=ink, ha=ha, va=va, zorder=4)

   for x, label in [(1.0, 'KEY ON'), (5.2, 'KEY OFF')]:
      bottom.plot([x], [0.86], marker='^', markersize=9, color=ink, zorder=4)
      bottom.plot([x, x], [0.0, 0.5], color=ink, linewidth=1.2, zorder=4)
      bottom.text(x, 0.62, label, fontsize=12, color=ink, ha='center',
                  va='center', zorder=4)

   bottom.set_xlim(0, 7)
   bottom.set_ylim(0, 4)
   bottom.set_xticks(range(8))
   bottom.set_yticks(range(5))
   bottom.tick_params(labelleft=False, labelbottom=False, length=0)
   bottom.grid(True, linewidth=0.9, color='#b0b0b0')
   for spine in bottom.spines.values():
      spine.set_color('#b0b0b0')
   bottom.set_xlabel(
      "The DX7's own diagram of the envelope: four rates, four levels",
      color='#5d5d5d')

   fig.tight_layout()
   save(fig, 'dx_envelope.svg')


def fig_wave():
   """An animated SVG: the waveform as the index rises and falls.

   SMIL rather than a script, because Antora loads an image::  in an <img>
   tag, where scripts do not run but declarative animation does. Every
   frame has the same point count, so the path morphs smoothly between
   them.
   """
   width, height = 700, 300
   left, right = 60, 660
   mid, amp = 130, 88
   cycles = 2.0                  # of the modulator, which is the fundamental
   points = 220
   frames = 26                   # up; the way back reuses them in reverse
   top_index = 6.0
   seconds = 9.0

   x = np.linspace(0.0, cycles, points)
   px = left + (right - left) * x / cycles

   def path(beta):
      y = np.sin(2 * np.pi * x + beta * np.sin(2 * np.pi * x))
      py = mid - amp * y
      return 'M ' + ' L '.join(
         f'{a:.1f},{b:.1f}' for a, b in zip(px, py))

   betas = [top_index * i / (frames - 1) for i in range(frames)]
   values = [path(b) for b in betas]
   values = values + values[-2::-1]           # and back, so it loops

   meter_left, meter_right = left, left + 220
   meter_y = 250
   widths = [(meter_right - meter_left) * b / top_index for b in betas]
   widths = widths + widths[-2::-1]

   def anim(attr, vals, fmt='{}'):
      joined = ';'.join(fmt.format(v) for v in vals)
      return (
         f'<animate attributeName="{attr}" dur="{seconds}s" '
         f'repeatCount="indefinite" calcMode="linear" '
         f'values="{joined}"/>')

   svg = f"""<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 {width} {height}" width="{width}" height="{height}" font-family="Helvetica, Arial, sans-serif">

  <!-- Generated by docs/scripts/gen_fm_figures.py. One cycle pair of a
       carrier phase modulated at its own frequency, as the index rises
       from 0 to {top_index:g} radians and falls back. SMIL animation, so it
       plays inside an Antora image:: (an <img> tag, where scripts do
       not run). -->

  <line x1="{left}" y1="{mid}" x2="{right}" y2="{mid}" stroke="#b0b0b0" stroke-width="1" stroke-dasharray="4 4"/>

  <!-- where it started: the unmodulated sine -->
  <path d="{path(0.0)}" fill="none" stroke="#64b5f6" stroke-width="1.2" opacity="0.45"/>

  <!-- the waveform, morphing -->
  <path fill="none" stroke="#1565c0" stroke-width="2.2" stroke-linejoin="round">
    {anim('d', values)}
  </path>

  <!-- how far the index has risen -->
  <rect x="{meter_left}" y="{meter_y}" width="{meter_right - meter_left}" height="8" rx="4" fill="#eef0f9"/>
  <rect x="{meter_left}" y="{meter_y}" height="8" rx="4" fill="#1565c0">
    {anim('width', widths, '{:.1f}')}
  </rect>
  <text x="{meter_left}" y="{meter_y + 28}" font-size="13" fill="#5d5d5d">0</text>
  <text x="{meter_right}" y="{meter_y + 28}" font-size="13" fill="#5d5d5d" text-anchor="end">{top_index:g} rad</text>
  <text x="{meter_right + 22}" y="{meter_y + 8}" font-size="14" fill="#1a1a1a">modulation index</text>
</svg>
"""
   out = os.path.join(OUT_DIR, 'fm_wave.svg')
   with open(out, 'w') as f:
      f.write(svg)
   print('wrote', out)


def feedback_cycle(fb, per_cycle=110, cycles=60):
   """One settled cycle of an operator modulating itself.

   The recurrence fm_algorithm runs: the source's last two outputs are
   averaged, scaled by `feedback` in radians, and added to its own phase.
   Checked against the compiled library (one fm_operator through an
   fm_algorithm with feedback), 2026-09-27: within 0.0006 at every
   feedback value the figure draws.
   """
   y1 = y2 = 0.0
   out = []
   for n in range(per_cycle * cycles):
      y = np.sin(2 * np.pi * (n % per_cycle) / per_cycle + fb * (y1 + y2) / 2)
      y2, y1 = y1, y
      out.append(y)
   return np.array(out[-per_cycle:])


def fig_feedback():
   """An animated SVG: a sine sweeping toward a sawtooth as feedback rises.

   Same SMIL approach as fig_wave, for the same reason.
   """
   width, height = 700, 300
   left, right = 60, 660
   mid, amp = 130, 88
   cycles = 2
   per_cycle = 110               # 440 Hz at 48 kHz, near enough
   frames = 26
   top_fb = np.pi / 2            # radians at full output, a DX7 carrier
                                 # at feedback 7
   seconds = 9.0

   px = left + (right - left) * np.arange(cycles * per_cycle) / (
      cycles * per_cycle - 1)

   def path(y):
      py = mid - amp * y
      return 'M ' + ' L '.join(f'{a:.1f},{b:.1f}' for a, b in zip(px, py))

   fbs = [top_fb * i / (frames - 1) for i in range(frames)]
   waves = [np.tile(feedback_cycle(fb, per_cycle), cycles) for fb in fbs]
   values = [path(w) for w in waves]
   values = values + values[-2::-1]

   # the sawtooth it sweeps toward, aligned to the top of the sweep
   saw = np.tile(np.linspace(1.0, -1.0, per_cycle), cycles)
   target = waves[-1]
   shift = max(
      range(cycles * per_cycle),
      key=lambda k: np.corrcoef(np.roll(saw, k), target)[0, 1])
   saw = np.roll(saw, shift)

   meter_left, meter_right = left, left + 220
   meter_y = 250
   widths = [(meter_right - meter_left) * fb / top_fb for fb in fbs]
   widths = widths + widths[-2::-1]

   def anim(attr, vals, fmt='{}'):
      joined = ';'.join(fmt.format(v) for v in vals)
      return (
         f'<animate attributeName="{attr}" dur="{seconds}s" '
         f'repeatCount="indefinite" calcMode="linear" '
         f'values="{joined}"/>')

   svg = f"""<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 {width} {height}" width="{width}" height="{height}" font-family="Helvetica, Arial, sans-serif">

  <!-- Generated by docs/scripts/gen_fm_figures.py. An operator modulating
       itself, as the feedback rises from 0 to {top_fb:.2f} radians and falls
       back: a sine at the bottom of the sweep, and near a sawtooth at
       the top, which is where a DX7 carrier at feedback 7 sits. Higher
       feedback keeps brightening and turns rough, which is why the
       sweep stops here. SMIL animation, so it plays
       inside an Antora image:: (an <img> tag, where scripts do not
       run). -->

  <line x1="{left}" y1="{mid}" x2="{right}" y2="{mid}" stroke="#b0b0b0" stroke-width="1" stroke-dasharray="4 4"/>

  <!-- the sawtooth it sweeps toward -->
  <path d="{path(saw)}" fill="none" stroke="#64b5f6" stroke-width="1.2" opacity="0.45"/>

  <!-- the operator's output, morphing -->
  <path fill="none" stroke="#1565c0" stroke-width="2.2" stroke-linejoin="round">
    {anim('d', values)}
  </path>

  <!-- how far the feedback has risen -->
  <rect x="{meter_left}" y="{meter_y}" width="{meter_right - meter_left}" height="8" rx="4" fill="#eef0f9"/>
  <rect x="{meter_left}" y="{meter_y}" height="8" rx="4" fill="#1565c0">
    {anim('width', widths, '{:.1f}')}
  </rect>
  <text x="{meter_right:.1f}" y="{meter_y - 10}" font-size="12" fill="#5d5d5d" text-anchor="end">as far as a DX7 carrier goes</text>
  <text x="{meter_left}" y="{meter_y + 28}" font-size="13" fill="#5d5d5d">0</text>
  <text x="{meter_right}" y="{meter_y + 28}" font-size="13" fill="#5d5d5d" text-anchor="end">&#960;/2 rad</text>
  <text x="{meter_right + 22}" y="{meter_y + 8}" font-size="14" fill="#1a1a1a">feedback</text>
</svg>
"""
   out = os.path.join(OUT_DIR, 'fm_feedback.svg')
   with open(out, 'w') as f:
      f.write(svg)
   print('wrote', out)



if __name__ == '__main__':
   fig_wave()
   fig_feedback()
   fig_sidebands()
   fig_envelope()
