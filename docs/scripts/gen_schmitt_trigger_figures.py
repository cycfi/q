#!/usr/bin/env python3
"""Generate Schmitt trigger hysteresis figure for the reference page."""

import numpy as np
import matplotlib.pyplot as plt
import os

SITE_ACCENT = '#1565c0'
SKY         = '#64b5f6'
SIGNAL_RED  = '#e53935'
MAGENTA     = '#d81b60'
GREEN       = '#43a047'
AMBER       = '#ffb300'
TEXT        = '#1a1a1a'
SUBTEXT     = '#333333'
GRID        = '#b0b0b0'

N   = 300
t   = np.linspace(0, 3 * np.pi, N)

# Input: slow sine + mild noise
rng = np.random.default_rng(42)
signal = np.sin(t) + 0.12 * rng.standard_normal(N)

ref        = 0.0
hysteresis = 0.25
upper      = ref + hysteresis
lower      = ref - hysteresis

# Simulate schmitt_trigger
output = np.zeros(N)
state  = False
for i in range(N):
    s = signal[i]
    if s > ref + hysteresis:
        state = True
    elif s < ref - hysteresis:
        state = False
    output[i] = 1.0 if state else 0.0

fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(10, 6), sharex=True)
fig.subplots_adjust(hspace=0.08)

# ── top: signal + thresholds ──────────────────────────────────────────────────
ax1.plot(t, signal, color=SITE_ACCENT, linewidth=1.4, label='input signal s')
ax1.axhline(ref,   color=SUBTEXT,     linewidth=0.8, linestyle='--', label='ref')
ax1.axhline(upper, color=GREEN,       linewidth=1.0, linestyle='--',
            label=f'ref + hysteresis ({hysteresis:+.2f})')
ax1.axhline(lower, color=SIGNAL_RED,  linewidth=1.0, linestyle='--',
            label=f'ref − hysteresis ({-hysteresis:+.2f})')
ax1.fill_between(t, lower, upper, alpha=0.08, color=AMBER, label='hysteresis band')
ax1.set_ylabel('amplitude', color=TEXT, fontsize=11)
ax1.set_ylim(-1.6, 1.6)
ax1.grid(color=GRID, linestyle='--', linewidth=0.5)
ax1.spines['top'].set_visible(False)
ax1.spines['right'].set_visible(False)
ax1.legend(loc='upper right', fontsize=9, framealpha=0.85)
ax1.set_title('Schmitt Trigger with hysteresis', color=TEXT, fontsize=12, pad=6)

# ── bottom: output ────────────────────────────────────────────────────────────
ax2.step(t, output, where='post', color=MAGENTA, linewidth=1.5, label='output')
ax2.fill_between(t, output, step='post', alpha=0.15, color=MAGENTA)
ax2.set_ylim(-0.25, 1.55)
ax2.set_yticks([0, 1])
ax2.set_yticklabels(['false', 'true'], color=SUBTEXT, fontsize=10)
ax2.set_ylabel('output', color=TEXT, fontsize=11)
ax2.set_xlabel('time', color=TEXT, fontsize=11)
ax2.grid(axis='x', color=GRID, linestyle='--', linewidth=0.5)
ax2.spines['top'].set_visible(False)
ax2.spines['right'].set_visible(False)
ax2.set_xticks([])

script_dir = os.path.dirname(os.path.abspath(__file__))
out_dir = os.path.join(script_dir, '..', 'modules', 'ROOT', 'images')
os.makedirs(out_dir, exist_ok=True)
out = os.path.join(out_dir, 'schmitt_trigger.svg')
fig.savefig(out, format='svg', bbox_inches='tight')
print(f'Generated {out}')
plt.close(fig)
