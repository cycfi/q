#!/usr/bin/env /usr/bin/python3
"""Generate the FFT magnitude-spectrum figure for the q FFT reference page.

q's `fft<N>` is a standard N-point complex DFT (verified against the header:
bin k lands at data[2k] (real), data[2k+1] (imag), no forward scaling). For a
real input signal, the magnitude spectrum |X[k]| = sqrt(re^2 + im^2) is exactly
numpy's rfft magnitude, so numpy faithfully reproduces what q computes. We plot
a composite of three tones so the peaks land on known bins.

Palette + style per docs/scripts/gen_interpolation_figures.py.
Run: /usr/bin/python3 gen_fft_figures.py
"""

import numpy as np
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt

PALETTE = {
    "sky":          "#64b5f6",
    "site_accent":  "#1565c0",
    "cornflower":   "#42a5f5",
    "amber":        "#ffb300",
    "magenta":      "#d81b60",
    "grid":         "#b0b0b0",
    "text":         "#1a1a1a",
    "sub":          "#333333",
}

OUT = "../modules/ROOT/images/fft-spectrum.svg"

# Composite signal: three tones at bins 10, 20, 30 with amplitudes 0.4, 0.5, 0.1
# over an N-point window (mirrors test/fft.cpp).
N = 256
bins  = [10, 20, 30]
amps  = [0.4, 0.5, 0.1]

n = np.arange(N)
x = np.zeros(N)
for k, a in zip(bins, amps):
    x += a * np.sin(2 * np.pi * k * n / N)

# Magnitude spectrum (real FFT). Normalize so a full-scale tone reads its
# amplitude: |X[k]| / (N/2) for 0 < k < N/2.
X = np.fft.rfft(x)
mag = np.abs(X) / (N / 2)
freq_bins = np.arange(len(mag))

fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(10, 6))

# --- Top: time-domain waveform ---
ax1.plot(n, x, color=PALETTE["site_accent"], linewidth=1.4)
ax1.set_title("Composite signal (three tones)", color=PALETTE["text"], fontsize=12)
ax1.set_xlabel("Sample index", color=PALETTE["sub"], fontsize=10)
ax1.set_ylabel("Amplitude", color=PALETTE["sub"], fontsize=10)
ax1.set_xlim(0, N - 1)
ax1.grid(True, linestyle="--", color=PALETTE["grid"], alpha=0.7)

# --- Bottom: magnitude spectrum ---
ax2.stem(
    freq_bins, mag,
    linefmt=PALETTE["cornflower"], markerfmt=" ", basefmt=" ",
)
# Mark the three peaks.
for k, a in zip(bins, amps):
    ax2.plot(k, mag[k], "o", color=PALETTE["magenta"], markersize=6, zorder=5)
    ax2.annotate(
        f"bin {k}",
        xy=(k, mag[k]), xytext=(k + 4, mag[k] + 0.02),
        color=PALETTE["sub"], fontsize=9,
    )
ax2.set_title("Magnitude spectrum |X[k]|", color=PALETTE["text"], fontsize=12)
ax2.set_xlabel("Frequency bin k", color=PALETTE["sub"], fontsize=10)
ax2.set_ylabel("Magnitude", color=PALETTE["sub"], fontsize=10)
ax2.set_xlim(0, 50)
ax2.set_ylim(0, 0.6)
ax2.grid(True, linestyle="--", color=PALETTE["grid"], alpha=0.7)

for ax in (ax1, ax2):
    ax.tick_params(colors=PALETTE["sub"])
    for spine in ax.spines.values():
        spine.set_color(PALETTE["grid"])

fig.tight_layout()
fig.savefig(OUT, format="svg", bbox_inches="tight")
print(f"wrote {OUT}")
