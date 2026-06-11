#!/usr/bin/env python3
"""Generate monostable timing diagram for the reference page."""

import numpy as np
import matplotlib.pyplot as plt
import os

SITE_ACCENT = '#1565c0'
SKY         = '#64b5f6'
GREEN       = '#43a047'
SIGNAL_RED  = '#e53935'
MAGENTA     = '#d81b60'
TEXT        = '#1a1a1a'
SUBTEXT     = '#333333'
GRID        = '#b0b0b0'

N         = 130
N_SAMPLES = 30          # pulse width passed to constructor
TRIGGERS  = [10, 30, 75, 90]   # sample positions of rising edges

# ── simulate ──────────────────────────────────────────────────────────────────

trigger = np.zeros(N)
for t in TRIGGERS:
    trigger[t] = 1

def simulate(retriggerable):
    out   = np.zeros(N)
    ticks = 0
    for i in range(N):
        val = bool(trigger[i])
        if val:
            if retriggerable or ticks == 0:
                ticks = N_SAMPLES
        if ticks:
            ticks -= 1
        out[i] = 1 if ticks != 0 else 0
    return out

mono_nr = simulate(False)
mono_r  = simulate(True)

t = np.arange(N)

# ── plot ──────────────────────────────────────────────────────────────────────

fig, axes = plt.subplots(3, 1, figsize=(10, 5), sharex=True)
fig.subplots_adjust(hspace=0.08)

def style_ax(ax, ylabel):
    ax.set_ylim(-0.25, 1.55)
    ax.set_yticks([0, 1])
    ax.set_yticklabels(['0', '1'], color=SUBTEXT, fontsize=10)
    ax.set_ylabel(ylabel, color=TEXT, fontsize=10, labelpad=6)
    ax.grid(axis='x', color=GRID, linestyle='--', linewidth=0.5)
    ax.spines['top'].set_visible(False)
    ax.spines['right'].set_visible(False)

# Row 0 — trigger
ax = axes[0]
style_ax(ax, 'trigger')
ax.step(t, trigger, where='post', color=MAGENTA, linewidth=1.5)
ax.fill_between(t, trigger, step='post', alpha=0.15, color=MAGENTA)
ax.set_title('Monostable timing (pulse width = 30 samples)', color=TEXT, fontsize=12, pad=6)

# Row 1 — non-retriggerable
ax = axes[1]
style_ax(ax, 'monostable')
ax.step(t, mono_nr, where='post', color=SITE_ACCENT, linewidth=1.5)
ax.fill_between(t, mono_nr, step='post', alpha=0.18, color=SKY)

# annotate ignored triggers
for tr in TRIGGERS:
    if tr > 0 and mono_nr[tr - 1] == 1:
        ax.annotate('ignored', xy=(tr, 1.08), fontsize=8, color=SIGNAL_RED,
                    ha='center', va='bottom')
        ax.plot(tr, 0, marker='x', color=SIGNAL_RED, markersize=6, zorder=5)

# Row 2 — retriggerable
ax = axes[2]
style_ax(ax, 'retriggerable\nmonostable')
ax.step(t, mono_r, where='post', color=GREEN, linewidth=1.5)
ax.fill_between(t, mono_r, step='post', alpha=0.15, color=GREEN)

# annotate retrigger moments
for tr in TRIGGERS:
    if tr > 0 and mono_r[tr - 1] == 1:
        ax.annotate('retrigger', xy=(tr, 1.08), fontsize=8, color=GREEN,
                    ha='center', va='bottom')
        ax.axvline(tr, color=GREEN, linewidth=0.8, linestyle=':', alpha=0.7)

ax.set_xlabel('samples', color=TEXT, fontsize=11)

script_dir = os.path.dirname(os.path.abspath(__file__))
out_dir = os.path.join(script_dir, '..', 'modules', 'ROOT', 'images')
os.makedirs(out_dir, exist_ok=True)
out = os.path.join(out_dir, 'monostable_timing.svg')
fig.savefig(out, format='svg', bbox_inches='tight')
print(f'Generated {out}')
plt.close(fig)
