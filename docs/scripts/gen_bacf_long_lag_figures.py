#!/usr/bin/env python3
"""
Generate the internals figure for bacf_provisional's check after a long
lag.

Produces, in docs/modules/internals/images/:

   bacf_long_lag.svg -- a re-strum of an open E chord on a Strat (Joel's
                        own recording), low-passed at 1 kHz as the tracker
                        sees it. Top: the signal after the drop, with the
                        pair of like pulses that gave the estimate (5.03
                        ms) and the chord's period (24.2 ms), timed from
                        the pair's first pulse. Bottom: the normalized
                        mismatch of the bits over one lag against the lag
                        before, at each lag, 65 ms after the drop (0 a
                        perfect match, noise 0.5), on the same axis, so
                        each span above ends over its lag.

The data, strat_restrum_lp.txt, is the tracker's input from 100 ms before
the drop (at 5.5219 s in the recording, sample 243517 at 44.1 kHz) to 72 ms
after, starting on a 64-sample word boundary so the windows align as in
q::detail::bacf_bitstream. The comparator and the match are recomputed
here and checked against q's own: 57.4% at 222 samples and 93.7% at 1067
(the tracker's lock), from bacf_lag_tracker at the same sample.

Usage: python3 docs/scripts/gen_bacf_long_lag_figures.py
"""

import os

import numpy as np
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt

# The wide frame of the documentation conventions, drawn at its placed
# size (a point is a pixel), with 15 px text
WIDE = 750
plt.rcParams['font.size'] = 15

HERE = os.path.dirname(os.path.abspath(__file__))
OUT_DIR = os.path.join(HERE, '..', 'modules', 'internals', 'images')
DATA = os.path.join(HERE, 'strat_restrum_lp.txt')

INK = '#1a1a1a'
MUTED = '#5d5d5d'
CURVE = '#1565c0'
EVENT = '#d81b60'           # the estimate's pulses
REFERENCE = '#43a047'       # the chord's period
GREY = '#b0b0b0'

SPS = 44100.0
START = 239104              # the excerpt's first sample in the recording
DROP = 243517 - START       # the drop, in the excerpt
PAIR = (243659 - START, 243881 - START)   # the estimate's pulse leads
ESTIMATE = 222              # samples, 5.03 ms
LOCK = 1067                 # samples, 24.2 ms
WORD = 64                   # q::detail::bacf_bitstream::word_size (M2)
HYSTERESIS = 10 ** (-45 / 20)
MIN_WINDOW = int(0.002 * SPS)
AT = DROP + 2866            # where the curve is scored, 65 ms in
REPEAT = 0.85               # bacf_provisional::repeat


def bitstream(x):
    bits = np.zeros(len(x), dtype=bool)
    state = False
    for i, s in enumerate(x):
        if not state and s > HYSTERESIS:
            state = True
        elif state and s < -HYSTERESIS:
            state = False
        bits[i] = state
    return bits


def one_lag(lag):
    m = max(lag, MIN_WINDOW)
    return (m + WORD - 1) // WORD * WORD


def match(bits, lag, now):
    """bacf_slice's one-lag score: the latest one_lag(lag) bits before
    `now` against those `lag` earlier, the end aligned down to a word."""
    w = one_lag(lag)
    start = (now - w - lag) // WORD * WORD
    a = bits[start:start + w]
    b = bits[start + lag:start + lag + w]
    return 1.0 - np.count_nonzero(a != b) / w


def style(ax):
    for side in ('top', 'right'):
        ax.spines[side].set_visible(False)
    for side in ('left', 'bottom'):
        ax.spines[side].set_color(MUTED)
    ax.tick_params(colors=MUTED)


def bracket(ax, x0, x1, y, text, colour):
    ax.annotate('', (x0, y), (x1, y),
                arrowprops=dict(arrowstyle='<->', color=colour, lw=1.4,
                                shrinkA=0, shrinkB=0))
    ax.text((x0 + x1) / 2, y + 0.04, text, color=colour, ha='center',
            va='bottom')


def gen():
    x = np.loadtxt(DATA)
    bits = bitstream(x)

    lags = np.arange(110, 1301)
    curve = np.array([match(bits, int(l), AT) for l in lags])
    for lag, want in ((ESTIMATE, 0.574219), (LOCK, 0.936581)):
        got = match(bits, lag, AT)
        assert abs(got - want) < 1e-5, (lag, got, want)

    ms = lambda n: n / SPS * 1e3
    fig, (top, bottom) = plt.subplots(
        2, 1, figsize=(WIDE / 72, 460 / 72), sharex=True,
        gridspec_kw=dict(height_ratios=[1, 1], hspace=0.35))

    # One axis for both: the top is time from the pair's first pulse, the
    # bottom is the lag, so a span above ends over its lag below.
    first = PAIR[0]
    lo, hi = first - int(0.004 * SPS), first + int(0.030 * SPS)
    t = ms(np.arange(lo, hi) - first)
    y = x[lo:hi] / np.max(np.abs(x[lo:hi]))
    top.plot(t, y, color=CURVE, lw=1.2)
    top.axvline(ms(DROP - first), color=GREY, lw=1, ls='--')

    # The pair's leads, up to the bracket; the chord's period below.
    for p in PAIR:
        top.vlines(ms(p - first), y[p - lo], 1.25, color=EVENT, lw=1.2)
    bracket(top, 0, ms(ESTIMATE), 1.25, '5.03 ms', EVENT)
    for p in (first, first + LOCK):
        top.vlines(ms(p - first), -1.3, y[p - lo], color=REFERENCE, lw=1.2)
    bracket(top, 0, ms(LOCK), -1.3, '24.2 ms', REFERENCE)
    top.set_ylim(-1.5, 1.75)
    top.set_yticks([-1, 0, 1])
    top.set_ylabel('level')
    style(top)
    top.tick_params(labelbottom=True)
    top.set_xlabel('ms from the first pulse')

    # The normalized mismatch over one lag, at each lag, as Q's BACF
    # figures plot it: 0 is a perfect match, noise 0.5, a repeat a notch.
    miss = lambda lag: 1.0 - match(bits, lag, AT)
    bottom.plot(ms(lags), 1.0 - curve, color=CURVE, lw=1.4)
    bottom.axhline(1.0 - REPEAT, color=GREY, lw=1, ls='--')
    bottom.plot([ms(ESTIMATE)], [miss(ESTIMATE)], 'o', color=EVENT, ms=7)
    bottom.plot([ms(LOCK)], [miss(LOCK)], 'o', color=REFERENCE, ms=7)
    bottom.set_xlim(t[0], t[-1])
    bottom.set_ylim(0, max(0.7, 0.05 * np.ceil((1.0 - curve).max() * 20)))
    bottom.set_yticks([0, 0.15, 0.5])
    bottom.set_xlabel('lag, ms')
    bottom.set_ylabel('mismatch')
    style(bottom)

    fig.subplots_adjust(left=0.10, right=0.98, top=0.95, bottom=0.12)
    os.makedirs(OUT_DIR, exist_ok=True)
    path = os.path.join(OUT_DIR, 'bacf_long_lag.svg')
    fig.savefig(path)
    plt.close(fig)
    return path


if __name__ == '__main__':
    print(gen())
