#!/usr/bin/env python3
"""
Generate dot-based signal flow diagrams for the q examples.

Produces, in docs/modules/ROOT/images/:
   grain-freeze-flow.svg   -- grain freeze signal chain
   sustain-hold-flow.svg   -- sustain hold signal chain

Usage: python3 docs/scripts/gen_example_flow_figures.py
Run from the q repo root.
"""

import os
import re
import subprocess
import sys

# ─── palette ──────────────────────────────────────────────────────────────────
SITE_ACCENT = '#1565c0'
FILL_AUDIO  = '#e8f2fd'
GREEN       = '#43a047'
PINK        = '#d81b60'
FILL_PINK   = '#fdeaf2'
INK         = '#1a1a1a'
GREY        = '#5d5d5d'
FONT        = 'DejaVu Sans, Verdana, sans-serif'
FONT_DOT    = 'DejaVu Sans'

OUT_DIR = os.path.join(
    os.path.dirname(__file__), '..', 'modules', 'ROOT', 'images')


# ─── helper: dot → scaled SVG ─────────────────────────────────────────────────
def dot_to_svg(src, target_width=720):
    result = subprocess.run(
        ['dot', '-Tsvg'],
        input=src.encode(),
        capture_output=True)
    if result.returncode:
        print(result.stderr.decode(), file=sys.stderr)
        sys.exit(1)
    svg = result.stdout.decode()
    svg = svg.replace('font-family="%s"' % FONT_DOT, 'font-family="%s"' % FONT)
    svg = svg.replace("font-family='%s'" % FONT_DOT, 'font-family="%s"' % FONT)
    m  = re.search(r'width="([\d.]+)pt"',  svg)
    m2 = re.search(r'height="([\d.]+)pt"', svg)
    if m and m2:
        w_pt = float(m.group(1))
        h_pt = float(m2.group(1))
        h_px = round(h_pt * target_width / w_pt)
        svg = re.sub(r'width="[\d.]+pt"',  'width="%d"'  % target_width, svg, count=1)
        svg = re.sub(r'height="[\d.]+pt"', 'height="%d"' % h_px,         svg, count=1)
    return svg


def sub(text):
    """Grey subtitle fragment for HTML labels."""
    return '<FONT POINT-SIZE="9" COLOR="%s">%s</FONT>' % (GREY, text)


# ─── figure 1: grain_freeze signal chain ──────────────────────────────────────
def build_grain_freeze_dot():
    return (
        'digraph grain_freeze {\n'
        '    graph [bgcolor=transparent splines=curved ranksep=0.55 nodesep=0.45\n'
        '           fontname="%(font)s" pad=0.18]\n'
        '    node  [fontname="%(font)s" fontsize=11 penwidth=1.5\n'
        '           style="rounded,filled" shape=box margin="0.18,0.12"]\n'
        '    edge  [fontname="%(font)s" fontsize=9  penwidth=1.3 arrowsize=0.7]\n'
        '    rankdir=LR\n'
        '\n'
        '    /* audio path */\n'
        '    _in   [label="input" shape=plaintext style="" fillcolor=transparent\n'
        '           color=transparent fontcolor="%(ink)s"]\n'
        '    buf   [label=<ring buffer<BR/>%(p_hermite)s>\n'
        '           fillcolor="%(fill)s" color="%(acc)s" fontcolor="%(ink)s"]\n'
        '    grains [label=<grains \xd72, OLA<BR/>%(p_grains)s>\n'
        '            fillcolor="%(fill)s" color="%(acc)s" fontcolor="%(ink)s"]\n'
        '    gain  [label=<\xd7 0.8 \xb7 gain<BR/>%(p_gain)s>\n'
        '           fillcolor="%(fill)s" color="%(acc)s" fontcolor="%(ink)s"]\n'
        '    mix   [label="+" shape=circle fixedsize=true width=0.40 height=0.40\n'
        '           style=filled fillcolor=white color="%(ink)s" fontsize=14 margin=0]\n'
        '    _out  [label="out" shape=plaintext style="" fillcolor=transparent\n'
        '           color=transparent fontcolor="%(ink)s"]\n'
        '\n'
        '    /* event trigger — pill shape via high corner radius */\n'
        '    freeze [label=<freeze at 1.2 s<BR/>%(p_freeze)s>\n'
        '            shape=box style="rounded,filled" margin="0.25,0.18"\n'
        '            fillcolor="%(fill_pink)s" color="%(pink)s" fontcolor="%(pink)s"]\n'
        '\n'
        '    /* control */\n'
        '    lfo   [label=<sin_cos_gen LFO<BR/>%(p_lfo)s>\n'
        '           fillcolor=white color="%(grn)s" fontcolor="%(ink)s"]\n'
        '    jitter [label=<jitter<BR/>%(p_jitter)s>\n'
        '            fillcolor=white color="%(grn)s" fontcolor="%(ink)s"]\n'
        '    fade  [label=<hann_downward_ramp<BR/>%(p_fade)s>\n'
        '           fillcolor=white color="%(grn)s" fontcolor="%(ink)s"]\n'
        '\n'
        '    /* pin control nodes to audio columns */\n'
        '    { rank=same; buf;    freeze }\n'
        '    { rank=same; grains; lfo;    jitter }\n'
        '    { rank=same; gain;   fade }\n'
        '\n'
        '    /* audio path edges */\n'
        '    _in    -> buf    [color="%(ink)s"]\n'
        '    buf    -> grains [color="%(ink)s"]\n'
        '    grains -> gain   [color="%(ink)s"]\n'
        '    gain   -> mix    [color="%(ink)s"]\n'
        '    mix    -> _out   [color="%(ink)s"]\n'
        '\n'
        '    /* dry bypass */\n'
        '    _in -> mix [constraint=false style=dashed color="%(grey)s"\n'
        '                xlabel="dry (passthrough)" fontcolor="%(grey)s"]\n'
        '\n'
        '    /* event trigger */\n'
        '    freeze -> grains [constraint=false style=dashed\n'
        '                      color="%(pink)s" fontcolor="%(pink)s"]\n'
        '\n'
        '    /* control edges */\n'
        '    lfo    -> grains [constraint=false color="%(grn)s"]\n'
        '    jitter -> grains [constraint=false color="%(grn)s"]\n'
        '    fade   -> gain   [constraint=false color="%(grn)s"]\n'
        '}\n'
    ) % dict(
        font=FONT_DOT, fill=FILL_AUDIO, acc=SITE_ACCENT,
        grn=GREEN, ink=INK, grey=GREY, pink=PINK, fill_pink=FILL_PINK,
        p_hermite=sub('hermite reads'),
        p_grains=sub('width 4096 \xb7 hop 2048'),
        p_gain=sub('hann fade \xb7 last 4 s'),
        p_freeze=sub('anchor = freeze_pos'),
        p_lfo=sub('0.3 Hz \xb7 \xb125 ms wander'),
        p_jitter=sub('\xb15 ms per-spawn'),
        p_fade=sub('4 s tail fade'),
    )


def gen_grain_freeze():
    svg = dot_to_svg(build_grain_freeze_dot())
    out = os.path.join(OUT_DIR, 'grain-freeze-flow.svg')
    with open(out, 'w') as f:
        f.write(svg)
    print('wrote', out)


# ─── figure 2: sustain_hold signal chain ──────────────────────────────────────
def build_sustain_hold_dot():
    return (
        'digraph sustain_hold {\n'
        '    graph [bgcolor=transparent splines=curved ranksep=0.55 nodesep=0.45\n'
        '           fontname="%(font)s" pad=0.18]\n'
        '    node  [fontname="%(font)s" fontsize=11 penwidth=1.5\n'
        '           style="rounded,filled" shape=box margin="0.18,0.12"]\n'
        '    edge  [fontname="%(font)s" fontsize=9  penwidth=1.3 arrowsize=0.7]\n'
        '    rankdir=LR\n'
        '\n'
        '    /* audio path */\n'
        '    _in   [label="input" shape=plaintext style="" fillcolor=transparent\n'
        '           color=transparent fontcolor="%(ink)s"]\n'
        '    buf   [label=<ring buffer<BR/>%(p_hermite)s>\n'
        '           fillcolor="%(fill)s" color="%(acc)s" fontcolor="%(ink)s"]\n'
        '    grains [label=<grains \xd72, OLA<BR/>%(p_grains)s>\n'
        '            fillcolor="%(fill)s" color="%(acc)s" fontcolor="%(ink)s"]\n'
        '    norm  [label=<norm<BR/>%(p_norm)s>\n'
        '           fillcolor="%(fill)s" color="%(acc)s" fontcolor="%(ink)s"]\n'
        '    comp  [label=<compressor<BR/>%(p_comp)s>\n'
        '           fillcolor="%(fill)s" color="%(acc)s" fontcolor="%(ink)s"]\n'
        '    _out  [label="out" shape=plaintext style="" fillcolor=transparent\n'
        '           color=transparent fontcolor="%(ink)s"]\n'
        '\n'
        '    /* control */\n'
        '    envelope [label=<envelope<BR/>%(p_env)s>\n'
        '              fillcolor=white color="%(grn)s" fontcolor="%(ink)s"]\n'
        '    blag  [label=<best_lag → P<BR/>%(p_blag)s>\n'
        '           fillcolor=white color="%(grn)s" fontcolor="%(ink)s"]\n'
        '    sched [label=<scheduler<BR/>%(p_sched)s>\n'
        '           fillcolor=white color="%(grn)s" fontcolor="%(ink)s"]\n'
        '\n'
        '    /* user input */\n'
        '    space [label="SPACE"\n'
        '           shape=box style="rounded,filled" margin="0.25,0.18"\n'
        '           fillcolor="%(fill_pink)s" color="%(pink)s" fontcolor="%(pink)s"]\n'
        '\n'
        '    /* pin control nodes to audio columns */\n'
        '    { rank=same; buf;    envelope }\n'
        '    { rank=same; grains; blag;    space }\n'
        '    { rank=same; norm;   sched }\n'
        '\n'
        '    /* audio path edges */\n'
        '    _in    -> buf    [color="%(ink)s"]\n'
        '    buf    -> grains [color="%(ink)s"]\n'
        '    grains -> norm   [color="%(ink)s"]\n'
        '    norm   -> comp   [color="%(ink)s"]\n'
        '    comp   -> _out   [color="%(ink)s"]\n'
        '\n'
        '    /* dry bypass */\n'
        '    _in -> comp [constraint=false style=dashed color="%(grey)s"\n'
        '                 xlabel="dry handover" fontcolor="%(grey)s"]\n'
        '\n'
        '    /* control flow */\n'
        '    buf      -> envelope [constraint=false color="%(grn)s"]\n'
        '    envelope -> blag     [constraint=false color="%(grn)s"]\n'
        '    blag     -> sched    [constraint=false color="%(grn)s"]\n'
        '    sched    -> grains   [constraint=false color="%(grn)s"\n'
        '                          xlabel="grain anchor"]\n'
        '\n'
        '    /* user input */\n'
        '    space -> blag [constraint=false style=dashed color="%(pink)s"\n'
        '                   fontcolor="%(pink)s" xlabel="hold / release"]\n'
        '}\n'
    ) % dict(
        font=FONT_DOT, fill=FILL_AUDIO, acc=SITE_ACCENT,
        grn=GREEN, ink=INK, grey=GREY, pink=PINK, fill_pink=FILL_PINK,
        p_hermite=sub('hermite reads'),
        p_grains=sub('width 16P \xb7 hop 8P'),
        p_norm=sub('RMS table'),
        p_comp=sub('AR side-chain + clip'),
        p_env=sub('attack keep-out'),
        p_blag=sub('NCC, sub-sample'),
        p_sched=sub('period grid \xb7 ping-pong'),
    )


def gen_sustain_hold():
    svg = dot_to_svg(build_sustain_hold_dot())
    out = os.path.join(OUT_DIR, 'sustain-hold-flow.svg')
    with open(out, 'w') as f:
        f.write(svg)
    print('wrote', out)


if __name__ == '__main__':
    os.makedirs(OUT_DIR, exist_ok=True)
    gen_grain_freeze()
    gen_sustain_hold()
