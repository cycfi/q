#!/usr/bin/env python3
"""
Generate the tutorials' block diagrams with blockdiag.py.

Produces, in docs/modules/ROOT/images/ (or the directory given):

   square_synth_flow.svg     -- the square synth: MIDI in, the voice, out
   poly_synth_flow.svg       -- the same front end, sixteen voices summed
   va_voice_flow.svg         -- the notes from a file, out to a file or device
   fm_routing_chart.svg      -- the DX7's algorithm 1, as its chart draws it
   q-layers.svg              -- Q's layers: q_plug and q_io over q_lib
   q-io-stack.svg            -- where q_io sits, from the application to the OS

Usage: python3 docs/scripts/gen_block_figures.py [out_dir]
"""

import os
import sys

from blockdiag import Figure, Block, GAP, FONT, PAD, text_width

HERE = os.path.dirname(os.path.abspath(__file__))
OUT = (sys.argv[1] if len(sys.argv) > 1 else
       os.path.join(HERE, '..', 'modules', 'ROOT', 'images'))


def front_end(f, cx=None):
    """The MIDI input the synth tutorials share: the device and the stream
    across the top, the processor under the stream, centred on cx."""
    cx = f.width / 2 if cx is None else cx
    dev = Block('q::midi_device', 'input')
    stream = Block('q::midi_input_stream', 'input')
    proc = Block('my_midi_processor', 'yours')
    f.row([dev, stream], y=f.margin, x=cx - stream.w / 2 - GAP - dev.w)
    f.place(proc, cx - proc.w / 2, stream.bottom + GAP)
    f.arrow(dev, stream, 'midi')
    f.arrow(stream, proc, 'midi')
    return proc


def square_synth():
    f = Figure()
    proc = front_end(f)

    # the synth: one row for the signal, the envelope under it
    phase = Block('phase_iterator')
    square = Block('q::square')
    svf = Block('q::svf')
    vca = Block('×', shape='circle')
    clip = Block('q::cubic_clip')
    chain = [phase, square, svf, vca, clip]
    top = proc.bottom + GAP + 50
    f.row(chain, y=top)
    vca.y = top + (phase.h - vca.h) / 2
    for a, b in zip(chain, chain[1:]):
        f.arrow(a, b)

    env = Block('q::adsr_envelope_gen')
    env.w = 3 * (vca.cx - svf.cx)           # its thirds meet the two
    f.place(env, svf.cx - env.w / 3, phase.bottom + GAP)
    f.arrow(env, svf, 'control', enter=svf.cx)
    f.arrow(env, vca, 'control', enter=vca.cx)

    synth = f.container('my_square_synth : q::audio_stream', chain + [env])
    f.arrow(proc, synth, 'control')

    out = Block('q::audio_device')
    f.place(out, clip.cx - out.w / 2, synth.bottom + GAP)
    f.arrow(clip, out)

    return f.write(os.path.join(OUT, 'square_synth_flow.svg'))


def poly_synth():
    f = Figure()
    proc = front_end(f)

    top = proc.bottom + GAP + 50
    array = f.stack('std::array<voice, 16>', ['voice', 'voice'])
    alloc = Block('allocate()', 'yours')
    mix = Block('Σ', shape='circle')
    clip = Block('q::cubic_clip')
    f.row([alloc, array, mix, clip], cy=top + array.h / 2)
    f.arrow(alloc, array, 'control')
    f.arrow(array, mix)
    f.arrow(mix, clip)

    synth = f.container('poly_synth : q::audio_stream',
                        [alloc, array, mix, clip])
    f.arrow(proc, synth, 'control')

    out = Block('q::audio_device')
    f.place(out, clip.cx - out.w / 2, synth.bottom + GAP)
    f.arrow(clip, out)

    return f.write(os.path.join(OUT, 'poly_synth_flow.svg'))


def va_synth():
    f = Figure()
    top = f.margin + 44 + GAP + 50         # where the container's row starts

    stack = f.stack('std::vector<voice>', ['voice', 'voice'])
    mix = Block('Σ', shape='circle')
    gain = Block('patch.gain')

    # the two places a buffer can go, to the right of the container
    wav = Block('q::wav_writer')
    player = Block('player', 'yours')
    wav.w = player.w = max(wav.w, player.w)
    inner = stack.w + mix.w + gain.w + 2 * GAP
    total = inner + 40 + 44 + wav.w
    f.row([stack, mix, gain], cy=top + stack.h / 2,
          x=(f.width - total) / 2 + 20)
    f.arrow(stack, mix)
    f.arrow(mix, gain)
    synth = f.container('render(file, patch)', [stack, mix, gain])

    fork = synth.right + 14
    f.place(wav, fork + 36, gain.cy - 66 - wav.h / 2)
    f.place(player, fork + 36, gain.cy + 66 - player.h / 2)
    f.path([(gain.right, gain.cy), (fork, gain.cy)], head=False)
    f.dot(fork, gain.cy)
    for b in (wav, player):
        f.path([(fork, gain.cy), (fork, b.cy), (b.left, b.cy)])

    # the notes, from a file this time
    midi = Block('q::midi_file', 'input')
    rec = Block('recorder', 'yours')
    f.row([midi, rec], y=f.margin, x=synth.cx - rec.w / 2 - GAP - midi.w)
    f.arrow(midi, rec, 'midi')
    f.arrow(rec, synth, 'control')

    return f.write(os.path.join(OUT, 'va_voice_flow.svg'))


def fm_synth():
    f = Figure()
    top = f.margin + 2 * (44 + GAP) + 50

    stack = f.stack('std::vector<slot>', ['slot', 'slot'])
    mix = Block('Σ', shape='circle')
    hp = Block('20 Hz high-pass')
    wav = Block('q::wav_writer')
    f.row([stack, mix, hp, wav], cy=top + stack.h / 2)
    f.arrow(stack, mix)
    f.arrow(mix, hp)
    f.arrow(hp, wav)
    synth = f.container('render(file, patch)', [stack, mix, hp, wav])

    # the patch on the left, the notes on the right, each pair a column
    patch = Block('q::dx_patch')
    patcher = Block('q::dx_patcher')
    midi = Block('q::midi_file', 'input')
    rec = Block('recorder', 'yours')
    for k, (a, b) in enumerate(((patch, patcher), (midi, rec))):
        cx = synth.left + synth.w * (k + 1) / 3
        f.place(a, cx - a.w / 2, f.margin)
        f.place(b, cx - b.w / 2, a.bottom + GAP)
    f.arrow(patch, patcher)
    f.arrow(midi, rec, 'midi')
    f.arrow(patcher, synth, 'control')
    f.arrow(rec, synth, 'control')
    return f.write(os.path.join(OUT, 'fm_voice_flow.svg'))


def midi_monitor():
    f = Figure()
    dev = Block('MIDI device', 'input')
    stream = Block('midi_input_stream')
    proc = Block('midi::processor')
    out = Block('std::cout', 'plain')
    f.row([dev, stream, proc, out], y=f.margin)
    f.arrow(dev, stream, 'midi')
    f.arrow(stream, proc)
    f.arrow(proc, out)
    return f.write(os.path.join(OUT, 'midi-monitor-flow.svg'))


def list_devices():
    f = Figure()
    root = Block('list_devices', 'yours')
    f.row([root], y=f.margin)
    columns = [
        ['audio_device::list()', 'RtAudio', 'CoreAudio, WASAPI, ALSA'],
        ['midi_device::list()', 'libremidi', 'CoreMIDI, Windows MIDI, ALSA seq'],
    ]
    for cx, names in zip((f.width / 2 - 205, f.width / 2 + 205), columns):
        blocks = [Block(names[0])] + [Block(n, 'plain') for n in names[1:]]
        above = root
        for b in blocks:
            f.place(b, cx - b.w / 2, above.bottom + 56)
            if above is root:                   # one trunk, then the split
                b.y += 8
                lane = root.bottom + 22
                f.path([(root.cx, root.bottom), (root.cx, lane), (b.cx, lane),
                        (b.cx, b.top)])
            else:
                f.arrow(above, b)
            above = b
    return f.write(os.path.join(OUT, 'list-devices-stack.svg'))


def delay():
    f = Figure()
    y = 90
    plus = Block('+', shape='circle')
    f.place(plus, 200 - plus.w / 2, y - plus.h / 2)
    start = 14 + text_width('input') + 8
    f.label('input', 14, y + 0.35 * FONT)
    f.path([(start, y), (plus.left, y)])

    # the dry copy goes straight to the left channel
    dry = start + 30
    f.dot(dry, y, 'dry')
    f.path([(dry, y), (dry, 40), (590, 40)], 'dry', dashed=True)
    f.label('left, dry', 598, 40 + 0.35 * FONT)
    f.path([(plus.right, y), (590, y)])
    f.label('right, wet', 598, y + 0.35 * FONT)

    # and the wet one comes back round, through the delay
    tap = 520
    f.dot(tap, y)
    mul = Block('× 0.85')
    delay = Block('delay')
    f.place(mul, 400 - mul.w / 2, y + 50)
    f.place(delay, plus.cx - delay.w / 2, y + 50)
    f.path([(tap, y), (tap, mul.cy), (mul.right, mul.cy)])
    f.arrow(mul, delay)
    f.arrow(delay, plus)
    return f.write(os.path.join(OUT, 'delay_signal_chain.svg'))


def grain_freeze():
    f = Figure()
    y = 72
    ring = Block('ring buffer')
    grains = Block('grains, overlap-add', width=216)
    mul = Block('× 0.8')
    plus = Block('+', shape='circle')
    f.row([ring, grains, mul, plus], cy=y, x=104)
    f.label('input', 14, y + 0.35 * FONT)
    start = 14 + text_width('input') + 8
    f.path([(start, y), (ring.left, y)])
    for a, b in zip([ring, grains, mul], [grains, mul, plus]):
        f.arrow(a, b)
    f.sink(plus, 'out', length=30)

    # the dry signal, round the top into the sum
    dry = start + 14
    f.dot(dry, y, 'dry')
    f.path([(dry, y), (dry, 20), (plus.cx, 20), (plus.cx, plus.top)],
           'dry', dashed=True)

    # what steers the grains, from below
    freeze = Block('freeze', 'input', shape='pill')
    lfo = Block('sin_cos_gen LFO', 'yours')
    jitter = Block('jitter', 'yours')
    ramp = Block('hann_downward_ramp', 'yours')
    port = [grains.left + grains.w * k / 4 for k in (1, 2, 3)]
    f.place(freeze, ring.cx - freeze.w / 2, 200)
    f.place(jitter, port[1] - jitter.w / 2, 200)
    f.place(ramp, mul.cx - ramp.w / 2, 200)
    f.place(lfo, port[2] - lfo.w / 2, ramp.bottom + 14)
    f.path([(freeze.cx, freeze.top), (freeze.cx, 150), (port[0], 150),
            (port[0], grains.bottom)], 'control')
    f.arrow(jitter, grains, 'control', enter=port[1])
    f.arrow(lfo, grains, 'control', enter=port[2])
    f.arrow(ramp, mul, 'control', enter=mul.cx)
    return f.write(os.path.join(OUT, 'grain-freeze-flow.svg'))


def pitch_detection():
    f = Figure()
    x = 14 + text_width('samples') + 6 + 34
    zcc = Block('zero_crossing_collector')
    acf = Block('bitstream_acf')
    bacf = Block('bacf_period_detector')
    pd = Block('pitch_detector')
    f.row([zcc, acf], y=f.margin, x=x)
    f.row([bacf, pd], y=zcc.bottom + 60, x=x)
    f.source('samples', zcc)
    f.arrow(zcc, acf)
    f.arrow(acf, bacf)
    f.arrow(bacf, pd)
    f.sink(pd, 'frequency')
    return f.write(os.path.join(OUT, 'pitch_detection_flow.svg'))


def sustain_hold():
    f = Figure()
    y = f.margin + 22
    ring = Block('ring buffer')
    grains = Block('grains ×2, OLA')
    norm = Block('norm')
    comp = Block('compressor')
    start = 14 + text_width('input') + 8
    f.row([ring, grains, norm, comp], cy=y, x=start + 40)
    f.label('input', 14, y + 0.35 * FONT)
    f.path([(start, y), (ring.left, y)])
    for a, b in zip([ring, grains, norm], [grains, norm, comp]):
        f.arrow(a, b)
    f.sink(comp, 'out', length=30)

    # the dry signal hands over to the held one
    dry = Block('dry handover', 'plain')
    f.place(dry, norm.cx - dry.w / 2, ring.bottom + 30)
    tap = start + 18
    f.dot(tap, y, 'dry')
    f.path([(tap, y), (tap, dry.cy), (dry.left, dry.cy)], 'dry', dashed=True)
    f.path([(dry.right, dry.cy), (comp.left + 22, dry.cy),
            (comp.left + 22, comp.bottom)], 'dry', dashed=True)

    # what decides where the grains come from
    env = Block('envelope', 'yours')
    lag = Block('best_lag → P', 'yours')
    sched = Block('scheduler', 'yours')
    f.row([env, lag, sched], y=dry.bottom + 50, x=ring.cx - env.w / 2)
    f.arrow(ring, env, 'control', enter=ring.cx)
    f.arrow(env, lag, 'control')
    f.arrow(lag, sched, 'control')
    lane = sched.top - 22
    f.path([(sched.cx, sched.top), (sched.cx, lane), (grains.cx, lane),
            (grains.cx, grains.bottom)], 'control')

    space = Block('SPACE', 'input', shape='pill')
    f.place(space, env.cx - space.w / 2, env.bottom + 30)
    f.path([(space.right, space.cy), (lag.cx, space.cy),
            (lag.cx, lag.bottom)], 'midi')
    return f.write(os.path.join(OUT, 'sustain-hold-flow.svg'))


def signal_conditioner():
    f = Figure()
    h = 44 + 17
    hp = Block('High pass', height=h, dashed=True)
    smooth = Block('Dynamic\nsmoother', height=h, dashed=True)
    clip = Block('Pre clip', height=h, dashed=True)
    gate = Block('Noise gate', height=h)
    comp = Block('Compressor +\nmakeup gain', height=h, dashed=True)
    chain = [hp, smooth, clip, gate, comp]
    tap = Block('gate() · gate_env()', 'accent')
    f.place(tap, 0, f.margin)
    gap = 28
    ends = text_width('in') + text_width('out') + 2 * (28 + 6)
    total = sum(b.w for b in chain) + gap * 4
    f.row(chain, y=tap.bottom + GAP, gap=gap,
          x=(f.width - total - ends) / 2 + text_width('in') + 34)
    tap.x = gate.cx - tap.w / 2
    f.source('in', hp, length=28)
    for a, b in zip(chain, chain[1:]):
        f.arrow(a, b)
    f.sink(comp, 'out', length=28)
    f.arrow(gate, tap, 'accent')

    # the level, taken after the clip, drives the gate and the compressor
    split = (clip.right + gate.left) / 2
    env = Block('Envelope: fast follower → peak', 'plain')
    f.place(env, split - env.w / 3, clip.bottom + 64)
    f.dot(split, clip.cy, 'plain')
    f.path([(split, clip.cy), (split, env.top)], 'plain')
    lane = clip.bottom + 34
    up = env.left + 2 * env.w / 3
    f.path([(up, env.top), (up, lane)], 'plain', head=False)
    f.dot(up, lane, 'plain')
    for b in (gate, comp):
        f.path([(up, lane), (b.cx, lane), (b.cx, b.bottom)], 'plain')
    return f.write(os.path.join(OUT, 'signal_conditioner_chain.svg'))


def fm_algorithm():
    f = Figure()
    y = 100
    plus = Block('+', shape='circle')
    index = Block('× index', 'plain')
    dev = Block('deviation', 'plain')
    op = Block('op[i]')
    x = 14 + text_width('modulators') + 6 + 34
    f.row([plus, index, dev, op], cy=y, x=x)
    f.source('modulators', plus)
    f.arrow(plus, index)
    f.arrow(index, dev)
    f.arrow(dev, op)
    out = op.right + 40
    f.dot(out, y)
    f.path([(op.right, y), (out + 34, y)])
    f.label('sum', out + 40, y + 0.35 * FONT)

    # on to the operators it modulates, further down the pass
    f.path([(out, y), (out, 40), (plus.cx, 40), (plus.cx, plus.top)])

    # and, where the feedback lands, back into itself
    avg = Block('average of\nits last two', 'accent')
    fb = Block('× feedback', 'accent')
    f.place(fb, plus.cx - fb.w / 2, op.bottom + 50)
    f.place(avg, dev.cx - avg.w / 2, fb.cy - avg.h / 2)
    f.path([(out, y), (out, avg.cy), (avg.right, avg.cy)], 'accent')
    f.arrow(avg, fb, 'accent')
    f.arrow(fb, plus, 'accent')
    return f.write(os.path.join(OUT, 'fm_algorithm_flow.svg'))


def fm_operator():
    f = Figure()
    y = f.margin + 70
    ratio = Block('× ratio', 'plain')
    phase = Block('phase', 'plain')
    plus = Block('+', shape='circle')
    osc = Block('osc')
    times = Block('×', shape='circle')
    x = 14 + text_width('master') + 6 + 34
    f.row([ratio, phase, plus, osc, times], cy=y, x=x)
    f.source('master', ratio)
    for a, b in zip([ratio, phase, plus, osc], [phase, plus, osc, times]):
        f.arrow(a, b)
    f.sink(times, 'out')
    f.source('modulation(mod)', plus, side='top')

    below = phase.bottom + 34
    f.path([(phase.cx - 60, below), (phase.cx, below),
            (phase.cx, phase.bottom)], 'plain', dashed=True)
    f.label('fixed(step)', phase.cx - 66, below + 0.35 * FONT, 'end')

    env = Block('env', 'yours')
    f.place(env, times.cx - env.w / 2, times.bottom + 50)
    f.arrow(env, times, 'control')
    f.source('attack, release, gain', env, 'control')
    return f.write(os.path.join(OUT, 'fm_operator_flow.svg'))


def fm_voice():
    f = Figure()
    pitch = Block('pitch env', 'yours')
    lfo = Block('LFO', 'accent')
    bent = Block('bent pitch', 'plain')
    ops = [Block('op 1'), Block('op 2'), Block('N')]
    left = 14 + text_width('master') + 6 + 34
    top = f.margin + 50

    # bent pitch takes its two inputs at its thirds
    gap = 24
    bent.w = 3 * (pitch.w / 2 + gap + lfo.w / 2)
    f.place(bent, left + 20, top + pitch.h + 44)
    f.place(pitch, bent.left + bent.w / 3 - pitch.w / 2, top)
    f.place(lfo, pitch.right + gap, top)
    f.arrow(pitch, bent, 'control', enter=pitch.cx)
    f.arrow(lfo, bent, 'control', enter=lfo.cx)

    # the operators, in the algorithm that routes them
    f.row(ops[:2], y=0, x=0, gap=14)
    f.place(ops[2], ops[1].right + 36, 0)
    alg = f.container('algorithm', ops, pad=14, kind='container')
    alg.move(bent.right + 40 - alg.left, bent.cy - alg.cy)
    f.label('…', (ops[1].right + ops[2].left) / 2, ops[1].cy + 0.35 * FONT,
            'middle')
    f.arrow(bent, alg)
    f.path([(lfo.right, lfo.cy), (alg.cx, lfo.cy), (alg.cx, alg.top)],
           'control')

    voice = f.container('fm_voice', [pitch, lfo, bent, alg], kind='plain',
                        dashed=True)
    f.source('master', bent, length=bent.left - left + 34)
    f.source('attack(note)', alg, 'control', side='bottom',
             length=voice.bottom - alg.bottom + 24)
    f.sink(alg, 'out', length=voice.right - alg.right + 24)
    return f.write(os.path.join(OUT, 'fm_voice_owns.svg'))


def ladder():
    f = Figure()
    y = f.margin + 90
    sigma = Block('Σ', shape='circle')
    poles = [Block('one-pole') for _ in range(4)]
    x = 14 + text_width('input') + 6 + 34
    f.row([sigma] + poles, cy=y, x=x)
    f.source('input', sigma)
    for a, b in zip([sigma] + poles, poles):
        f.arrow(a, b)
    tap = poles[-1].right + 40
    f.path([(poles[-1].right, y), (tap, y)], head=False)
    f.dot(tap, y)
    f.tap(tap, y, f.margin + 20, 'Lowpass')

    # the resonance, back round to the input
    k = Block('× k', 'accent')
    neg = Block('× −1', 'accent')
    f.place(neg, sigma.cx - neg.w / 2, sigma.bottom + 50)
    f.place(k, (poles[1].right + poles[2].left) / 2 - k.w / 2, neg.y)
    f.path([(tap, y), (tap, k.cy), (k.right, k.cy)], 'accent')
    f.arrow(k, neg, 'accent')
    f.arrow(neg, sigma, 'accent')
    return f.write(os.path.join(OUT, 'ladder_block.svg'))


def svf():
    f = Figure()
    y = f.margin + 90
    sigma = Block('Σ', shape='circle')
    i1 = Block('integrator')
    i2 = Block('integrator')
    x = 14 + text_width('input') + 6 + 34
    f.row([sigma, i1, i2], cy=y, x=x, gap=110)
    f.source('input', sigma)
    hp, bp, lp = sigma.right + 36, i1.right + 55, i2.right + 50
    f.path([(sigma.right, y), (i1.left, y)])
    f.path([(i1.right, y), (i2.left, y)])
    f.path([(i2.right, y), (lp, y)], head=False)
    for x, name in ((hp, 'Highpass'), (bp, 'Bandpass'), (lp, 'Lowpass')):
        f.dot(x, y)
        f.tap(x, y, f.margin + 20, name)

    # the damping and the lowpass, summed and subtracted from the input
    neg = Block('× −1', 'accent')
    f.place(neg, sigma.cx - neg.w / 2, sigma.bottom + 34)
    plus = Block('+', 'accent', shape='circle')
    k = Block('× k', 'accent')
    f.place(plus, sigma.cx - plus.w / 2, neg.bottom + 34)
    f.place(k, bp - k.w / 2, plus.cy - k.h / 2)
    f.path([(bp, y), (bp, k.top)], 'accent')
    f.arrow(k, plus, 'accent')
    lane = k.bottom + 26
    f.path([(lp, y), (lp, lane), (plus.cx, lane), (plus.cx, plus.bottom)],
           'accent')
    f.arrow(plus, neg, 'accent')
    f.arrow(neg, sigma, 'accent')
    return f.write(os.path.join(OUT, 'svf_block.svg'))


def midi_layers():
    f = Figure()
    x = 14 + text_width('packets') + 6 + 34
    rows = []
    for top, (reader, what) in zip(
            (f.margin, f.margin + 190),
            (('byte_reader', 'bytes'), ('packet_reader', 'packets'))):
        r = Block(reader)
        d = Block('dispatch')
        st = Block('stages')
        pr = Block('processor', 'yours')
        f.row([r, d], y=top, x=x)
        f.row([st, pr], y=top, x=d.right + 160)
        f.source(what, r)
        f.arrow(r, d)
        f.arrow(d, st)
        f.arrow(st, pr)
        rows.append((d, st))

    (d1, s1), (d2, s2) = rows
    c1, c2 = d1.right + 45, d1.right + 115
    up = Block('to_midi2')
    down = Block('to_midi1')
    f.place(up, c1 - up.w / 2, d1.bottom + 26)
    f.place(down, c2 - down.w / 2, d2.top - 26 - down.h)
    for c in (c1, c2):
        f.dot(c, d1.cy, 'plain')
        f.dot(c, d2.cy, 'plain')
    f.path([(c1, d1.cy), (c1, up.top)], 'plain', dashed=True)
    f.path([(c1, up.bottom), (c1, d2.cy)], 'plain', dashed=True, head=False)
    f.path([(c2, d2.cy), (c2, down.bottom)], 'plain', dashed=True)
    f.path([(c2, down.top), (c2, d1.cy)], 'plain', dashed=True, head=False)
    return f.write(os.path.join(OUT, 'midi-layers.svg'))


def per_note():
    f = Figure()
    mpe = Block('MPE zone')
    m2 = Block('MIDI 2.0')
    r1 = Block('mpe_reader')
    r2 = Block('per_note_reader')
    msgs = f.stack(None, ['note_pitch', 'note_pressure', 'note_timbre'])
    synth = Block('synth', 'yours')
    mpe.w = m2.w = max(mpe.w, m2.w)
    r1.w = r2.w = max(r1.w, r2.w)
    x = (f.width - (mpe.w + r1.w + msgs.w + synth.w + 3 * GAP + 60)) / 2
    f.row([mpe, r1], y=f.margin, x=x)
    f.row([m2, r2], y=mpe.bottom + 70, x=x)
    f.row([msgs, synth], cy=(mpe.cy + m2.cy) / 2, x=r1.right + GAP + 60)
    f.arrow(mpe, r1)
    f.arrow(m2, r2)
    f.arrow(r1, msgs)
    f.arrow(r2, msgs)
    f.arrow(msgs, synth)
    return f.write(os.path.join(OUT, 'per-note-sources.svg'))


def translation_directions():
    f = Figure()
    top = f.margin
    for stream, via in (('MIDI 2.0 stream', 'to_midi1'),
                        ('MIDI 1.0 stream', 'to_midi2')):
        row = [Block(stream), Block(via), Block('processor', 'yours')]
        f.row(row, y=top, gap=70)
        f.arrow(row[0], row[1])
        f.arrow(row[1], row[2])
        top = row[0].bottom + 30
    return f.write(os.path.join(OUT, 'translation-directions.svg'))


def translation_gathering():
    f = Figure()
    ccs = f.stack(None, ['cc 101 = 0', 'cc 100 = 0', 'cc 6 = 2',
                         'cc 38 = 0'])
    cc = ccs.children
    step = cc[1].cy - cc[0].cy
    to2 = Block('to_midi2', height=5 * step)
    rc = Block('registered_controller', 'yours')
    f.row([ccs, to2, rc], y=f.margin, gap=70)
    to2.y = cc[0].cy - step
    rc.y = to2.cy - rc.h / 2
    for i, c in enumerate(cc):
        last = i == len(cc) - 1
        f.path([(c.right, c.cy), (to2.left, c.cy)],
               'signal' if last else 'plain', dashed=not last)
    f.arrow(to2, rc)
    return f.write(os.path.join(OUT, 'translation-gathering.svg'))


def translation_substitutes():
    f = Figure()
    note = Block('note_on')
    to1 = Block('to_midi1')
    op = Block('operator()(note_on)', 'yours')
    f.place(op, 0, f.margin + 50)
    proc = f.container('processor', [op], pad=16, kind='yours')
    f.row([note, to1, proc], cy=op.cy, gap=60)
    note.y = to1.y = op.cy - note.h / 2
    f.arrow(note, to1)
    f.arrow(to1, op)
    return f.write(os.path.join(OUT, 'translation-substitutes.svg'))


def midi_processor_overloads():
    f = Figure()
    msgs = f.stack(None, ['note_on', 'note_on', 'control_change',
                          'program_change'], member='component')
    ops = [Block('operator()(note_on, time)', 'yours'),
           Block('operator()(control_change, time)', 'yours'),
           Block('operator()(message_base, time)', 'plain')]
    # the overloads on the same rows as the messages that reach them
    m = msgs.children
    w = max(b.w for b in ops)
    for i, b in enumerate(ops):
        b.w, b.h = w, m[0].h
        f.place(b, 0, i * (m[1].y - m[0].y))
    proc = f.container('my_midi_processor', ops, pad=16, kind='yours')
    f.row([msgs, proc], y=f.margin, gap=110)
    # a little above the rows, so the two note_on arrows meet their
    # overload evenly
    proc.move(0, m[1].y - ops[0].y - 12)
    f.arrow(m[0], ops[0])
    f.arrow(m[1], ops[0])
    f.arrow(m[2], ops[1])
    f.arrow(m[3], ops[2], 'plain', dashed=True)
    return f.write(os.path.join(OUT, 'midi-processor-overloads.svg'))


def ci_responder_chain():
    f = Figure()
    row = [Block('sysex_view'), Block('property_responder'),
           Block('profile_responder'), Block('responder')]
    f.row(row, y=f.margin)
    for a, b in zip(row, row[1:]):
        f.arrow(a, b)
    return f.write(os.path.join(OUT, 'ci-responder-chain.svg'))


def fm_routing_chart():
    f = Figure()
    top = f.margin + 44
    w = text_width('OP6') + 2 * PAD + 24
    step = 44 + GAP

    # the stack of four on the right, two on the left, level at the bottom
    right = [Block('OP6', 'accent', width=w), Block('OP5', width=w),
             Block('OP4', width=w), Block('OP3', 'yours', width=w)]
    left = [Block('OP2', width=w), Block('OP1', 'yours', width=w)]
    for i, b in enumerate(right):
        f.place(b, f.width / 2 + 90 - w / 2, top + i * step)
    for i, b in enumerate(left):
        f.place(b, f.width / 2 - 90 - w / 2, top + (i + 2) * step)
    for a, b in zip(right, right[1:]):
        f.arrow(a, b)
    f.arrow(left[0], left[1])

    # OP6 back into itself
    op6 = right[0]
    loop, over = op6.right + 40, op6.top - 42
    f.path([(op6.right, op6.cy), (loop, op6.cy), (loop, over),
            (op6.cx, over), (op6.cx, op6.top)], 'accent')

    # the carriers, summed
    sigma = Block('Σ', shape='circle')
    f.place(sigma, f.width / 2 - sigma.w / 2, right[3].bottom + 40)
    for b in (left[1], right[3]):
        side = sigma.left if b.cx < sigma.cx else sigma.right
        f.path([(b.cx, b.bottom), (b.cx, sigma.cy), (side, sigma.cy)])
    f.path([(sigma.cx, sigma.bottom), (sigma.cx, sigma.bottom + 34)])
    f.label('out', sigma.cx, sigma.bottom + 34 + 18, 'middle')
    return f.write(os.path.join(OUT, 'fm_routing_chart.svg'))


def q_layers():
    # Three blocks fill little of the page's width, so the canvas is made
    # narrower and the page scales the whole figure up, text and all.
    f = Figure(width=460)
    plug = Block('q_plug (optional)')
    io = Block('q_io (optional)')
    plug.w = io.w = max(plug.w, io.w)
    f.row([plug, io], y=f.margin)

    # the core, the same size, centred under both
    lib = Block('q_lib', width=plug.w)
    f.place(lib, f.width / 2 - lib.w / 2, plug.bottom + GAP + 12)
    f.arrow(plug, lib, 'plain')
    f.arrow(io, lib, 'plain')
    return f.write(os.path.join(OUT, 'q-layers.svg'))


def q_io_stack():
    app = Block('your application', 'yours')
    io = Block('q_io')
    lib = Block('q_lib')
    audio = [Block('RtAudio', 'plain'),
             Block('CoreAudio,\nWASAPI, ALSA', 'plain')]
    midi = [Block('libremidi', 'plain'),
            Block('CoreMIDI, Windows\nMIDI, ALSA seq', 'plain')]
    everything = [app, io, lib] + audio + midi
    w = max(b.w for b in everything)
    for b in everything:
        b.w = w
    for b in (audio[1], midi[1]):
        b.h = max(audio[1].h, midi[1].h)

    # The canvas is cut to the drawing, so the page centres it and scales
    # it up, as for q-layers.
    f = Figure(width=2 * 14.0 + 2.5 * w + 1.5 * GAP)

    # q_io over its two backends, q_lib beside it, the application on top
    cx = f.margin + w + GAP / 2
    f.place(app, cx - w / 2, f.margin)
    f.place(io, cx - w / 2, app.bottom + GAP)
    f.place(lib, io.right + GAP, io.y)
    for k, (be, os_) in enumerate((audio, midi)):
        x = f.margin + k * (w + GAP)
        f.place(be, x, io.bottom + GAP + 12)
        f.place(os_, x, be.bottom + GAP)
        f.arrow(io, be, 'plain')
        f.arrow(be, os_, 'plain')
    f.arrow(app, io, 'plain')
    f.arrow(io, lib, 'plain')
    return f.write(os.path.join(OUT, 'q-io-stack.svg'))


FIGURES = [square_synth, poly_synth, va_synth, fm_synth, midi_monitor,
           list_devices, delay, grain_freeze, pitch_detection, sustain_hold,
           signal_conditioner, fm_algorithm, fm_operator, fm_voice, ladder,
           svf, midi_layers, per_note, fm_routing_chart, translation_directions,
           translation_gathering, translation_substitutes,
           midi_processor_overloads, ci_responder_chain, q_layers, q_io_stack]

if __name__ == '__main__':
    for make in FIGURES:
        print(make())
