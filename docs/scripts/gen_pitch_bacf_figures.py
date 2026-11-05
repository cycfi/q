#!/usr/bin/env python3
"""
Generate the BACF pitch detection figures from the test suite's own output.

test/pitch/pitch_detector_ex.cpp runs the pitch detector on the guitar
samples in test/audio_files and writes what it computes as 5-channel WAV
files, results/pitch_detect_<name>.wav in the build's test directory:

   1. the conditioned input (band-passed, compressed, clipped),
   2. the bitset bits (0 to 0.8),
   3. the BACF periodicity of each analysis, over the first half of its
      window, blank below the shortest period,
   4. the frequency, over the highest frequency,
   5. the predicted frequency, over the highest frequency.

The figures plot those channels as they are, on one time axis, after the
graphs of the 2018 and 2020 articles (cycfi.com, "Fast and Efficient Pitch
Detection"). Run the suite first (ctest from cmake-build-debug/test), so the
results are current.

Writes pitch_bacf_*.svg in docs/modules/ROOT/images/.

Usage: python3 docs/scripts/gen_pitch_bacf_figures.py [results_dir]
"""

import os
import sys

import numpy as np
import soundfile as sf
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
OUT_DIR = os.path.join(HERE, '..', 'modules', 'ROOT', 'images')
RESULTS = (sys.argv[1] if len(sys.argv) > 1 else
           os.path.join(REPO, 'cmake-build-debug', 'test', 'results'))

SITE_ACCENT = '#1565c0'    # signal
SKY = '#64b5f6'            # bitstream
AMBER = '#fb8c00'          # bitstream autocorrelation
MAGENTA = '#d81b60'        # event marker
GREY = '#5d5d5d'
INK = '#1a1a1a'

# The lowest note of each sample's test; the detector's range is 0.8 to 5
# times it (pitch_detector_ex.cpp).
LOW_E, A, D, G, B, HIGH_E = 82.41, 110.0, 146.83, 196.0, 246.94, 329.63


def load(name):
   x, sps = sf.read(os.path.join(RESULTS, 'pitch_detect_' + name + '.wav'))
   return x, sps


def onset(x, sps):
   """The first sample above 10% of the input's peak, in seconds."""
   s = np.abs(x[:, 0])
   return int(np.argmax(s > 0.1 * s.max())) / sps


def style(ax):
   for s in ('top', 'right'):
      ax.spines[s].set_visible(False)
   ax.spines['left'].set_color('#b0b0b0')
   ax.spines['bottom'].set_color('#b0b0b0')


def frame_starts(bacf):
   """The first sample of each analysis: where its blank begins."""
   on = bacf > 0
   return [i for i in range(1, len(on)) if on[i - 1] and not on[i]]


def view(name, after, dur, title, path, marks=None):
   """Input, bitset and BACF of a sample's test result, from `after`
   seconds past the onset, for `dur` seconds."""
   x, sps = load(name)
   a = int((onset(x, sps) + after) * sps)
   b = a + int(dur * sps)
   t = np.arange(a, b) / sps
   sig, bits, bacf = x[a:b, 0], x[a:b, 1] > 0.4, x[a:b, 2]

   fig, (a0, a1, a2) = plt.subplots(
      3, 1, figsize=(10, 5.6), sharex=True,
      gridspec_kw=dict(height_ratios=[1.5, 0.6, 1.3], hspace=0.3))
   for ax in (a0, a1, a2):
      style(ax)
   a0.plot(t, sig, color=SITE_ACCENT, lw=1.0)
   a0.axhline(0, color=GREY, lw=0.6, ls=':')
   a0.set_yticks([])
   a0.set_title(title, fontsize=11, color=INK, loc='left')
   a1.fill_between(t, bits, step='post', color=SKY, lw=0)
   a1.set_ylim(-0.1, 1.15)
   a1.set_yticks([])
   a1.set_title('Bitset', fontsize=9, color=GREY, loc='left')
   a2.plot(t, bacf, color=AMBER, lw=1.4)
   a2.set_ylim(0, 1.08)
   a2.set_yticks([0, 0.5, 1])
   a2.set_ylabel('periodicity', fontsize=8)
   a2.set_title('Bitstream autocorrelation, one analysis after another',
                fontsize=9, color=GREY, loc='left')
   a2.set_xlim(t[0], t[-1])
   a2.set_xlabel('seconds', fontsize=8)
   if marks:
      marks((a0, a1, a2), t, x[a:b])
   fig.savefig(os.path.join(OUT_DIR, path), format='svg',
               bbox_inches='tight')
   plt.close(fig)
   starts = frame_starts(bacf)
   half = starts[1] - starts[0] if len(starts) > 1 else 0
   print(f'wrote {path}: {t[0]:.4f} s, analyses at',
         [round(t[s], 4) for s in starts], 'half window', half)
   return x[a:b], t, sps


def sub_collector_marks(axes, t, x):
   """p1, p2, p3 in the first whole analysis of the span, as in the 2020
   article: candidates between the first leading edges."""
   a0, a1, a2 = axes
   bacf, bits = x[:, 2], x[:, 1] > 0.4
   s0, s1 = frame_starts(bacf)[:2]
   half = s1 - s0
   rises = [i for i in range(s0 + 1, s0 + 2 * half)
            if bits[i] and not bits[i - 1]]
   pairs = []
   for i, e1 in enumerate(rises[:2]):
      for e2 in rises[i + 1:]:
         if e2 - e1 < half:
            pairs.append((e1, e2))
   pairs = pairs[:3]
   for k, (e1, e2) in enumerate(pairs):
      y = 1.25 + 0.4 * k
      a1.annotate('', xy=(t[e2], y), xytext=(t[e1], y),
                  arrowprops=dict(arrowstyle='<|-|>', color=INK, lw=0.9))
      a1.text(t[e2] + (t[1] - t[0]) * 8, y, f'p{k + 1}', fontsize=8,
              color=INK, ha='left', va='center')
      lag = s0 + (e2 - e1)
      y2 = 0.08 + 0.13 * k
      a2.annotate('', xy=(t[lag], y2), xytext=(t[s0], y2),
                  arrowprops=dict(arrowstyle='<|-|>', color=INK, lw=0.8))
      a2.text(t[lag] + (t[1] - t[0]) * 8, y2, f'p{k + 1}', fontsize=8,
              color=INK, ha='left', va='center')
      a2.axvline(t[lag], color=MAGENTA, lw=0.8, ls='--')
      print(f'  p{k + 1}: {e2 - e1} samples, periodicity '
            f'{bacf[lag]:.3f}')
   a2.axvline(t[s0], color=MAGENTA, lw=0.8, ls='--')
   a1.set_ylim(-0.1, 1.45 + 0.4 * len(pairs))


def gen_pattern():
   """A guitar low E and its zero crossings: several per cycle."""
   x, sps = load('1a-Low-E')
   a = int((onset(x, sps) + 1.0) * sps)
   n = int(3 / LOW_E * sps)
   t = np.arange(a, a + n) / sps
   s = x[a:a + n, 0]
   fig, (a0, a1) = plt.subplots(
      2, 1, figsize=(10, 3.6), sharex=True,
      gridspec_kw=dict(height_ratios=[1.5, 0.6], hspace=0.3))
   for ax in (a0, a1):
      style(ax)
   a0.plot(t, s, color=SITE_ACCENT, lw=1.1)
   a0.axhline(0, color=GREY, lw=0.6, ls=':')
   a0.set_yticks([])
   a0.set_title('A guitar low E, a second after the pick', fontsize=11,
                color=INK, loc='left')
   a1.fill_between(t, s > 0, step='post', color=SKY, lw=0)
   a1.set_ylim(-0.1, 1.15)
   a1.set_yticks([])
   a1.set_title('Zero crossings', fontsize=9, color=GREY, loc='left')
   a1.set_xlim(t[0], t[-1])
   a1.set_xlabel('seconds', fontsize=8)
   fig.savefig(os.path.join(OUT_DIR, 'pitch_bacf_pattern.svg'),
               format='svg', bbox_inches='tight')
   plt.close(fig)
   print('wrote pitch_bacf_pattern.svg:', round(t[0], 4), 's,',
         int(np.sum(np.diff((s > 0).astype(int)) != 0)), 'crossings')


def gen_acf():
   """Standard autocorrelation of the same low E: a peak at the period."""
   x, sps = load('1a-Low-E')
   a = int((onset(x, sps) + 1.0) * sps)
   s = x[a:a + int(0.06 * sps), 0]
   P = sps / LOW_E
   w = int(2 * P)
   lags = np.arange(int(1.6 * P))
   r = np.array([np.dot(s[:w], s[L:L + w]) for L in lags])
   r /= r[0]
   peak = int(lags[int(0.8 * P) + np.argmax(r[int(0.8 * P):])])
   fig, (a0, a1) = plt.subplots(
      2, 1, figsize=(10, 4.4),
      gridspec_kw=dict(height_ratios=[1.2, 1.6], hspace=0.45))
   for ax in (a0, a1):
      style(ax)
   n = int(2.4 * P)
   L = int(round(P * 0.35))
   a0.plot(np.arange(n), s[:n], color=SITE_ACCENT, lw=1.1, label='signal')
   a0.plot(np.arange(n), s[L:L + n], color=AMBER, lw=1.0,
           label=f'delayed by {L} samples')
   a0.set_yticks([])
   a0.set_xlim(0, n)
   a0.legend(loc='upper right', fontsize=8, frameon=False)
   a0.set_title('A guitar low E against a delayed copy of itself',
                fontsize=11, color=INK, loc='left')
   a1.plot(lags, r, color=AMBER, lw=1.6)
   a1.axhline(0, color=GREY, lw=0.6, ls=':')
   a1.axvline(peak, color=MAGENTA, lw=1.0, ls='--')
   a1.annotate(f'period {peak} samples', xy=(peak, r[peak]),
               xytext=(peak + 10, 0.85), color=MAGENTA, fontsize=9)
   a1.set_xlim(0, lags[-1])
   a1.set_xlabel('lag (samples)', fontsize=8)
   a1.set_ylabel('correlation', fontsize=8)
   a1.set_title('Sum of products against lag, normalized', fontsize=9,
                color=GREY, loc='left')
   fig.savefig(os.path.join(OUT_DIR, 'pitch_acf.svg'), format='svg',
               bbox_inches='tight')
   plt.close(fig)
   print('wrote pitch_acf.svg: delay', L, 'peak at', peak, 'samples')


def gen_low_e_onset():
   """The input and the predicted frequency at the pick."""
   x, sps = load('1a-Low-E')
   on = int(onset(x, sps) * sps)
   a, b = on - int(0.01 * sps), on + int(0.06 * sps)
   t = np.arange(a, b) / sps
   pred = x[a:b, 4] * LOW_E * 5
   fig, (a0, a1) = plt.subplots(
      2, 1, figsize=(10, 4.2), sharex=True,
      gridspec_kw=dict(height_ratios=[1.3, 1.4], hspace=0.3))
   for ax in (a0, a1):
      style(ax)
   a0.plot(t, x[a:b, 0], color=SITE_ACCENT, lw=0.9)
   a0.set_yticks([])
   a0.set_title('A guitar low E, from the pick', fontsize=11, color=INK,
                loc='left')
   a1.plot(t, np.where(pred > 0, pred, np.nan), color=AMBER, lw=1.4)
   a1.axhline(LOW_E, color=GREY, lw=0.6, ls=':')
   a1.set_ylim(0, 200)
   a1.set_ylabel('Hz', fontsize=8)
   a1.set_title('Predicted frequency', fontsize=9, color=GREY, loc='left')
   a1.set_xlabel('seconds', fontsize=8)
   a1.set_xlim(t[0], t[-1])
   fig.savefig(os.path.join(OUT_DIR, 'pitch_bacf_low_e_onset.svg'),
               format='svg', bbox_inches='tight')
   plt.close(fig)
   p = np.nonzero(pred > 0)[0]
   steps = [(round(t[i], 4), round(float(pred[i]), 2)) for i in p
            if i == p[0] or pred[i] != pred[i - 1]]
   print('wrote pitch_bacf_low_e_onset.svg: onset', round(on / sps, 4),
         's, predictions', steps[:8])


if __name__ == '__main__':
   gen_pattern()
   gen_acf()
   view('1a-Low-E', 1.0, 0.075, 'A guitar low E, a second after the pick',
        'pitch_bacf_low_e.svg')
   for name, title, path in [
      ('2a-A', 'The A string, open', 'pitch_bacf_a.svg'),
      ('3a-D', 'The D string, open', 'pitch_bacf_d.svg'),
      ('4a-G', 'The G string, open', 'pitch_bacf_g.svg'),
      ('5a-B', 'The B string, open', 'pitch_bacf_b.svg'),
      ('6a-High-E', 'The high E string, open', 'pitch_bacf_high_e.svg'),
      ('1b-Low-E-12th', 'The low E string at the 12th fret',
       'pitch_bacf_low_e_12th.svg'),
   ]:
      view(name, 1.0, 0.05, title + ', a second after the pick', path)
   view('1a-Low-E', 1.0, 0.075, 'A guitar low E, a second after the pick',
        'pitch_bacf_sub_collector.svg', marks=sub_collector_marks)
   view('1c-Low-E-24th', 1.0, 0.05,
        'The low E string at the 24th fret, two octaves up, a second after '
        'the pick', 'pitch_bacf_multiple_peaks.svg')
   view('3a-D', 14.0, 0.055, 'The D string, 14 seconds after the pick',
        'pitch_bacf_d_string.svg')
   gen_low_e_onset()
