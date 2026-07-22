#!/usr/bin/env python3
"""Generate the before/after figure for the q signal_conditioner reference page.

The data is the REAL output of the C++ `signal_conditioner`, not a Python
reimplementation. The source WAV is produced by the signal_conditioner test,
which writes a 3-channel file per input:

    channel 0 = original signal
    channel 1 = conditioned signal (signal_conditioner output)
    channel 2 = signal envelope after gate + compressor (signal_env())
    channel 3 = smoothed tap (smoothed(): post-smoother, pre-clip)

Regenerate the source WAV with:

    cd build/test && ./test_signal_conditioner
    # -> build/test/results/signal_conditioner_GStaccato.wav

We overlay a few cycles of the attack of one pluck ("1a-Low-E", the low E at
~82 Hz, which is harmonically rich): raw, the smoothed() tap, and the
conditioned output on a single shared axis. The smoothed trace keeps the
crest shapes the tanh rail saturates away, which is the tap's reason to
exist. The raw pluck has sharp transient spikes and high-frequency hash; the
conditioner tames the spikes (pre-clip), cleans the hash (high pass + dynamic
smoother), and lifts the level (makeup gain), while preserving the periodic
structure. Sharing the axis makes the waveshaping and the gain visible directly.

NOTE: run with homebrew `python3` (has soundfile + matplotlib); the Xcode
`/usr/bin/python3` used by the other generators cannot read float WAVs. Palette
and style otherwise match docs/scripts/gen_interpolation_figures.py.
"""

import numpy as np
import soundfile as sf
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt

PALETTE = {
    "site_accent":  "#1565c0",
    "amber":        "#fb8c00",
    "grid":         "#b0b0b0",
    "text":         "#1a1a1a",
    "sub":          "#333333",
}

SRC = "../../build/test/results/signal_conditioner_1a-Low-E.wav"
OUT = "../modules/ROOT/images/signal_conditioner.svg"

T0, DUR = 0.998, 0.060      # window: a few cycles of the attack (~5 periods)

d, sr = sf.read(SRC)
a, b = int(T0 * sr), int((T0 + DUR) * sr)
t_ms = (np.arange(a, b) - a) / sr * 1000.0
orig = d[a:b, 0]
cond = d[a:b, 1]
smoothed = d[a:b, 3]

fig, ax = plt.subplots(figsize=(10, 6))

ax.plot(t_ms, orig, color=PALETTE["amber"], linewidth=1.4, alpha=0.6, label="raw")
ax.plot(t_ms, smoothed, color="#2e7d32", linewidth=1.6, label="smoothed tap")
ax.plot(t_ms, cond, color=PALETTE["site_accent"], linewidth=1.8, label="conditioned")

ax.axhline(0, color=PALETTE["grid"], linewidth=0.8)
ax.set_title("The attack of one pluck: raw, smoothed tap, conditioned",
             color=PALETTE["text"], fontsize=13)
ax.set_xlabel("Time (ms)", color=PALETTE["sub"], fontsize=11)
ax.set_ylabel("Amplitude", color=PALETTE["sub"], fontsize=11)
ax.set_xlim(0, DUR * 1000.0)
ax.set_ylim(-0.75, 0.75)
ax.legend(loc="upper right", fontsize=11)
ax.grid(True, linestyle="--", color=PALETTE["grid"], alpha=0.7)
ax.tick_params(colors=PALETTE["sub"])
for spine in ax.spines.values():
    spine.set_color(PALETTE["grid"])

fig.tight_layout()
fig.savefig(OUT, format="svg", bbox_inches="tight")
print(f"wrote {OUT}")
