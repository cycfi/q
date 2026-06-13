#!/usr/bin/env python3

import argparse
import csv
from dataclasses import dataclass
from pathlib import Path

import numpy as np
import soundfile as sf
from scipy.fft import next_fast_len


DEFAULT_AUDIO_FILES = (
    "GLines1.wav",
    "Hammer-Pull High E.wav",
    "Tapping D.wav",
)


@dataclass(frozen=True)
class Config:
    hop_ms: float = 1.0
    frame_ms: float = 60.0
    envelope_ms: float = 10.0
    fmin: float = 70.0
    fmax: float = 1400.0
    mpm_peak_ratio: float = 1.0
    mpm_min_clarity: float = 0.35
    rbj_asdf_alpha: float = 0.01
    rbj_asdf_min_confidence: float = 0.55
    yin_threshold: float = 0.18
    min_confidence: float = 0.70
    min_envelope: float = 0.01
    attack_backfill_envelope: float = 0.05
    max_attack_backfill_ms: float = 300.0
    continuity_window_ms: float = 80.0
    max_continuity_deviation_cents: float = 450.0
    max_unvoiced_gap_ms: float = 40.0
    max_synth_amp: float = 0.80


def read_audio(path):
    audio, sample_rate = sf.read(path, dtype="float32", always_2d=True)
    mono = audio.mean(axis=1)
    return mono.astype(np.float32), sample_rate


def frame_at(signal, center, frame_size):
    start = center - frame_size // 2
    end = start + frame_size
    out = np.zeros(frame_size, dtype=np.float32)
    src_start = max(start, 0)
    src_end = min(end, len(signal))
    if src_end > src_start:
        dst_start = src_start - start
        out[dst_start : dst_start + (src_end - src_start)] = signal[src_start:src_end]
    return out


def normalized_envelopes(signal, sample_positions, sample_rate, config):
    size = max(1, int(round(config.envelope_ms * sample_rate / 1000.0)))
    half = size // 2
    squared = np.square(signal.astype(np.float64))
    prefix = np.concatenate(([0.0], np.cumsum(squared)))
    envelope = np.empty(len(sample_positions), dtype=np.float64)

    for i, pos in enumerate(sample_positions):
        start = max(0, pos - half)
        end = min(len(signal), start + size)
        if end <= start:
            envelope[i] = 0.0
        else:
            envelope[i] = np.sqrt((prefix[end] - prefix[start]) / (end - start))

    peak = envelope.max(initial=0.0)
    if peak > 0.0:
        envelope /= peak
    return np.clip(envelope, 0.0, 1.0)


def parabolic_minimum(values, index):
    if index <= 0 or index >= len(values) - 1:
        return float(index)
    left = values[index - 1]
    center = values[index]
    right = values[index + 1]
    denom = left - 2.0 * center + right
    if abs(denom) < 1e-20:
        return float(index)
    return float(index) + 0.5 * (left - right) / denom


def yin_frequency(frame, sample_rate, min_lag, max_lag, threshold):
    frame = frame.astype(np.float64, copy=False)
    frame -= frame.mean()
    frame *= np.hanning(len(frame))

    energy = np.dot(frame, frame)
    if energy <= 1e-12:
        return -1.0, 0.0

    fft_size = next_fast_len(2 * len(frame))
    spectrum = np.fft.rfft(frame, fft_size)
    autocorr = np.fft.irfft(spectrum * np.conj(spectrum), fft_size)[: max_lag + 1]

    squared = frame * frame
    prefix = np.concatenate(([0.0], np.cumsum(squared)))
    taus = np.arange(max_lag + 1)
    left_energy = prefix[len(frame) - taus] - prefix[0]
    right_energy = prefix[len(frame)] - prefix[taus]
    difference = left_energy + right_energy - 2.0 * autocorr
    difference[0] = 0.0

    cumulative = np.cumsum(difference)
    cmnd = np.ones_like(difference)
    cmnd[1:] = difference[1:] * taus[1:] / np.maximum(cumulative[1:], 1e-20)

    search = cmnd[min_lag : max_lag + 1]
    if len(search) == 0:
        return -1.0, 0.0

    below = np.flatnonzero(search < threshold)
    if len(below):
        lag = int(below[0] + min_lag)
        while lag + 1 <= max_lag and cmnd[lag + 1] < cmnd[lag]:
            lag += 1
    else:
        lag = int(np.argmin(search) + min_lag)

    confidence = max(0.0, min(1.0, 1.0 - float(cmnd[lag])))
    if confidence <= 0.0:
        return -1.0, confidence

    refined_lag = parabolic_minimum(cmnd, lag)
    if refined_lag <= 0.0:
        return -1.0, confidence
    return float(sample_rate / refined_lag), confidence


def parabolic_maximum(values, index):
    if index <= 0 or index >= len(values) - 1:
        return float(index)
    left = values[index - 1]
    center = values[index]
    right = values[index + 1]
    denom = left - 2.0 * center + right
    if abs(denom) < 1e-20:
        return float(index)
    return float(index) + 0.5 * (left - right) / denom


def mpm_frequency(frame, sample_rate, min_lag, max_lag, config):
    frame = frame.astype(np.float64, copy=False)
    frame -= frame.mean()
    frame *= np.hanning(len(frame))

    energy = np.dot(frame, frame)
    if energy <= 1e-12:
        return -1.0, 0.0

    frame_size = len(frame)
    fft_size = next_fast_len(2 * frame_size)
    spectrum = np.fft.rfft(frame, fft_size)
    autocorr = np.fft.irfft(spectrum * np.conj(spectrum), fft_size)[: max_lag + 1]

    squared = frame * frame
    prefix = np.concatenate(([0.0], np.cumsum(squared)))
    taus = np.arange(max_lag + 1)
    denominator = (prefix[frame_size - taus] - prefix[0]) + (
        prefix[frame_size] - prefix[taus]
    )
    nsdf = np.zeros_like(autocorr)
    valid = denominator > 1e-20
    nsdf[valid] = 2.0 * autocorr[valid] / denominator[valid]

    peak_lags = []
    for lag in range(min_lag + 1, max_lag):
        if nsdf[lag] > 0.0 and nsdf[lag] > nsdf[lag - 1] and nsdf[lag] >= nsdf[lag + 1]:
            peak_lags.append(lag)

    if not peak_lags:
        return -1.0, 0.0

    peak_scores = np.asarray([nsdf[lag] for lag in peak_lags], dtype=np.float64)
    best_score = float(np.max(peak_scores))
    if best_score < config.mpm_min_clarity:
        return -1.0, best_score

    score_threshold = best_score * config.mpm_peak_ratio
    eligible_lags = [lag for lag in peak_lags if nsdf[lag] >= score_threshold]
    lag = min(eligible_lags)
    refined_lag = parabolic_maximum(nsdf, lag)
    if refined_lag <= 0.0:
        return -1.0, best_score
    return float(sample_rate / refined_lag), best_score


def rbj_asdf_frequency(frame, sample_rate, min_lag, max_lag, config):
    frame = frame.astype(np.float64, copy=False)
    frame -= frame.mean()
    frame *= np.hanning(len(frame))

    frame_size = len(frame)
    energy = np.dot(frame, frame) / frame_size
    if energy <= 1e-12:
        return -1.0, 0.0

    asdf = np.zeros(max_lag + 1, dtype=np.float64)
    for lag in range(1, max_lag + 1):
        start = (frame_size + lag) // 2
        x0 = frame[start - lag : start - lag + frame_size]
        x1 = frame[start : start + frame_size]

        if len(x0) < frame_size or len(x1) < frame_size:
            padded0 = np.zeros(frame_size, dtype=np.float64)
            padded1 = np.zeros(frame_size, dtype=np.float64)
            padded0[: len(x0)] = x0
            padded1[: len(x1)] = x1
            x0 = padded0
            x1 = padded1

        delta = x0 - x1
        asdf[lag] = np.dot(delta, delta) / frame_size

    autocorr_like = energy - 0.5 * asdf
    normalized = autocorr_like / energy

    peak_lags = []
    for lag in range(min_lag + 1, max_lag):
        if (
            normalized[lag] > 0.0
            and normalized[lag] > normalized[lag - 1]
            and normalized[lag] >= normalized[lag + 1]
        ):
            peak_lags.append(lag)

    if not peak_lags:
        return -1.0, 0.0

    weighted_scores = np.asarray(
        [
            normalized[lag] * (lag ** (-config.rbj_asdf_alpha))
            for lag in peak_lags
        ],
        dtype=np.float64,
    )
    best = int(np.argmax(weighted_scores))
    lag = peak_lags[best]
    confidence = float(normalized[lag])
    if confidence < config.rbj_asdf_min_confidence:
        return -1.0, confidence

    refined_lag = parabolic_maximum(normalized, lag)
    if refined_lag <= 0.0:
        return -1.0, confidence
    return float(sample_rate / refined_lag), confidence


def frequency_to_midi(frequency):
    return 69.0 + 12.0 * np.log2(frequency / 440.0)


def correct_period_errors(frequencies, config):
    corrected = frequencies.copy()
    radius = max(1, int(round(config.continuity_window_ms / config.hop_ms / 2.0)))
    factors = (0.25, 1.0 / 3.0, 0.5, 1.0, 2.0, 3.0, 4.0)

    for i, frequency in enumerate(corrected):
        if frequency <= 0.0:
            continue

        start = max(0, i - radius)
        end = min(len(corrected), i + radius + 1)
        neighborhood = corrected[start:end]
        voiced = neighborhood[neighborhood > 0.0]
        if len(voiced) < 5:
            continue

        median_midi = float(np.median(frequency_to_midi(voiced)))
        candidates = np.asarray(
            [
                frequency * factor
                for factor in factors
                if config.fmin <= frequency * factor <= config.fmax
            ],
            dtype=np.float64,
        )
        if len(candidates) == 0:
            continue

        candidate_midi = frequency_to_midi(candidates)
        best = int(np.argmin(np.abs(candidate_midi - median_midi)))
        current_error = abs(float(frequency_to_midi(frequency)) - median_midi) * 100.0
        best_error = abs(float(candidate_midi[best]) - median_midi) * 100.0

        if (
            best_error <= config.max_continuity_deviation_cents
            and best_error + 50.0 < current_error
        ):
            corrected[i] = candidates[best]

    return corrected


def fill_short_unvoiced_gaps(sample_positions, frequencies, envelopes, config):
    repaired = frequencies.copy()
    max_gap = max(0, int(round(config.max_unvoiced_gap_ms / config.hop_ms)))
    i = 0

    while i < len(repaired):
        if repaired[i] > 0.0:
            i += 1
            continue

        start = i
        while i < len(repaired) and repaired[i] <= 0.0:
            i += 1
        end = i

        has_left = start > 0 and repaired[start - 1] > 0.0
        has_right = end < len(repaired) and repaired[end] > 0.0
        gap_len = end - start

        if not has_left or not has_right or gap_len > max_gap:
            continue
        if np.max(envelopes[start:end], initial=0.0) < config.min_envelope:
            continue

        left_frequency = repaired[start - 1]
        right_frequency = repaired[end]
        left_sample = sample_positions[start - 1]
        right_sample = sample_positions[end]
        gap_samples = sample_positions[start:end]
        repaired[start:end] = np.interp(
            gap_samples,
            (left_sample, right_sample),
            (left_frequency, right_frequency),
        )

    return repaired


def correct_segment_harmonics(frequencies, config):
    corrected = frequencies.copy()
    factors = (0.25, 1.0 / 3.0, 0.5, 1.0, 2.0, 3.0, 4.0)
    max_error = config.max_continuity_deviation_cents / 3.0
    min_improvement = config.max_continuity_deviation_cents
    reference_midi = None
    i = 0

    while i < len(corrected):
        if corrected[i] <= 0.0:
            i += 1
            continue

        start = i
        while i < len(corrected) and corrected[i] > 0.0:
            i += 1
        end = i

        segment = corrected[start:end]
        segment_median = float(np.median(segment))
        if reference_midi is None:
            reference_midi = float(frequency_to_midi(segment_median))
            continue

        candidates = np.asarray(
            [
                segment_median * factor
                for factor in factors
                if config.fmin <= segment_median * factor <= config.fmax
            ],
            dtype=np.float64,
        )
        if len(candidates) == 0:
            continue

        candidate_midi = frequency_to_midi(candidates)
        errors = np.abs(candidate_midi - reference_midi) * 100.0
        best = int(np.argmin(errors))
        current_error = abs(float(frequency_to_midi(segment_median)) - reference_midi) * 100.0
        best_error = float(errors[best])

        if best_error <= max_error and best_error + min_improvement < current_error:
            correction = candidates[best] / segment_median
            corrected[start:end] *= correction
            segment_median = float(np.median(corrected[start:end]))

        reference_midi = float(frequency_to_midi(segment_median))

    return corrected


def correct_frame_harmonic_jumps(frequencies, config):
    corrected = frequencies.copy()
    factors = (0.25, 1.0 / 3.0, 0.5, 1.0, 2.0, 3.0, 4.0)
    max_error = config.max_continuity_deviation_cents / 3.0
    min_jump = config.max_continuity_deviation_cents
    previous = -1.0

    for i, frequency in enumerate(corrected):
        if frequency <= 0.0:
            continue
        if previous <= 0.0:
            previous = frequency
            continue

        previous_midi = float(frequency_to_midi(previous))
        current_midi = float(frequency_to_midi(frequency))
        current_error = abs(current_midi - previous_midi) * 100.0

        if current_error >= min_jump:
            candidates = np.asarray(
                [
                    frequency * factor
                    for factor in factors
                    if config.fmin <= frequency * factor <= config.fmax
                ],
                dtype=np.float64,
            )
            if len(candidates):
                candidate_midi = frequency_to_midi(candidates)
                errors = np.abs(candidate_midi - previous_midi) * 100.0
                best = int(np.argmin(errors))
                best_error = float(errors[best])
                if best_error <= max_error and best_error + min_jump < current_error:
                    frequency = float(candidates[best])
                    corrected[i] = frequency

        previous = frequency

    return corrected


def backfill_note_attacks(frequencies, envelopes, config):
    repaired = frequencies.copy()
    max_backfill = max(0, int(round(config.max_attack_backfill_ms / config.hop_ms)))
    i = 0

    while i < len(repaired):
        if repaired[i] <= 0.0:
            i += 1
            continue

        start = i
        while i < len(repaired) and repaired[i] > 0.0:
            i += 1

        fill_start = start
        limit = max(0, start - max_backfill)
        while (
            fill_start > limit
            and repaired[fill_start - 1] <= 0.0
            and envelopes[fill_start - 1] >= config.attack_backfill_envelope
        ):
            fill_start -= 1

        if fill_start < start:
            repaired[fill_start:start] = repaired[start]

    return repaired


def track_pitch(signal, sample_rate, config):
    frame_size = max(16, int(round(config.frame_ms * sample_rate / 1000.0)))
    hop_samples = config.hop_ms * sample_rate / 1000.0
    frame_count = int(np.floor((len(signal) - 1) / hop_samples)) + 1
    sample_positions = np.rint(np.arange(frame_count) * hop_samples).astype(np.int64)
    envelopes = normalized_envelopes(signal, sample_positions, sample_rate, config)

    min_lag = max(2, int(np.floor(sample_rate / config.fmax)))
    max_lag = min(frame_size - 2, int(np.ceil(sample_rate / config.fmin)))
    frequencies = np.full(len(sample_positions), -1.0, dtype=np.float64)

    for i, sample_no in enumerate(sample_positions):
        if envelopes[i] < config.min_envelope:
            continue
        frame = frame_at(signal, int(sample_no), frame_size)
        frequency, confidence = yin_frequency(
            frame, sample_rate, min_lag, max_lag, config.yin_threshold
        )
        if confidence >= config.min_confidence:
            frequencies[i] = frequency

    frequencies = correct_frame_harmonic_jumps(frequencies, config)
    frequencies = correct_period_errors(frequencies, config)
    frequencies = fill_short_unvoiced_gaps(
        sample_positions, frequencies, envelopes, config
    )
    frequencies = correct_segment_harmonics(frequencies, config)
    frequencies = correct_frame_harmonic_jumps(frequencies, config)
    frequencies = backfill_note_attacks(frequencies, envelopes, config)
    frequencies = correct_frame_harmonic_jumps(frequencies, config)
    frequencies = correct_period_errors(frequencies, config)
    frequencies = correct_frame_harmonic_jumps(frequencies, config)
    frequencies = correct_frame_harmonic_jumps(frequencies, config)
    return sample_positions, frequencies, envelopes


def track_pitch_mpm(signal, sample_rate, config):
    frame_size = max(16, int(round(config.frame_ms * sample_rate / 1000.0)))
    hop_samples = config.hop_ms * sample_rate / 1000.0
    frame_count = int(np.floor((len(signal) - 1) / hop_samples)) + 1
    sample_positions = np.rint(np.arange(frame_count) * hop_samples).astype(np.int64)
    envelopes = normalized_envelopes(signal, sample_positions, sample_rate, config)

    min_lag = max(2, int(np.floor(sample_rate / config.fmax)))
    max_lag = min(frame_size - 2, int(np.ceil(sample_rate / config.fmin)))
    frequencies = np.full(len(sample_positions), -1.0, dtype=np.float64)

    for i, sample_no in enumerate(sample_positions):
        if envelopes[i] < config.min_envelope:
            continue
        frame = frame_at(signal, int(sample_no), frame_size)
        frequency, clarity = mpm_frequency(frame, sample_rate, min_lag, max_lag, config)
        if clarity >= config.mpm_min_clarity:
            frequencies[i] = frequency

    frequencies = correct_frame_harmonic_jumps(frequencies, config)
    frequencies = correct_period_errors(frequencies, config)
    frequencies = fill_short_unvoiced_gaps(
        sample_positions, frequencies, envelopes, config
    )
    frequencies = correct_segment_harmonics(frequencies, config)
    frequencies = correct_frame_harmonic_jumps(frequencies, config)
    frequencies = backfill_note_attacks(frequencies, envelopes, config)
    frequencies = correct_period_errors(frequencies, config)
    frequencies = correct_frame_harmonic_jumps(frequencies, config)
    frequencies = correct_frame_harmonic_jumps(frequencies, config)
    return sample_positions, frequencies, envelopes


def track_pitch_rbj_asdf(signal, sample_rate, config):
    frame_size = max(16, int(round(config.frame_ms * sample_rate / 1000.0)))
    hop_samples = config.hop_ms * sample_rate / 1000.0
    frame_count = int(np.floor((len(signal) - 1) / hop_samples)) + 1
    sample_positions = np.rint(np.arange(frame_count) * hop_samples).astype(np.int64)
    envelopes = normalized_envelopes(signal, sample_positions, sample_rate, config)

    min_lag = max(2, int(np.floor(sample_rate / config.fmax)))
    max_lag = min(frame_size - 2, int(np.ceil(sample_rate / config.fmin)))
    frequencies = np.full(len(sample_positions), -1.0, dtype=np.float64)

    for i, sample_no in enumerate(sample_positions):
        if envelopes[i] < config.min_envelope:
            continue
        frame = frame_at(signal, int(sample_no), frame_size)
        frequency, confidence = rbj_asdf_frequency(
            frame, sample_rate, min_lag, max_lag, config
        )
        if confidence >= config.rbj_asdf_min_confidence:
            frequencies[i] = frequency

    frequencies = correct_period_errors(frequencies, config)
    frequencies = fill_short_unvoiced_gaps(
        sample_positions, frequencies, envelopes, config
    )
    frequencies = correct_segment_harmonics(frequencies, config)
    frequencies = correct_frame_harmonic_jumps(frequencies, config)
    frequencies = backfill_note_attacks(frequencies, envelopes, config)
    frequencies = correct_period_errors(frequencies, config)
    frequencies = correct_frame_harmonic_jumps(frequencies, config)
    frequencies = correct_frame_harmonic_jumps(frequencies, config)
    return sample_positions, frequencies, envelopes


def write_reference_csv(path, sample_positions, frequencies, envelopes, sample_rate):
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("w", newline="") as f:
        writer = csv.writer(f)
        writer.writerow(("time", "sample_no", "frequency", "envelope"))
        for sample_no, frequency, envelope in zip(sample_positions, frequencies, envelopes):
            writer.writerow(
                (
                    f"{sample_no / sample_rate:.6f}",
                    int(sample_no),
                    f"{frequency:.6f}",
                    f"{envelope:.6f}",
                )
            )


def synthesize_check_wav(csv_path, audio_path, output_path, config):
    original, sample_rate = read_audio(audio_path)
    sample_no = []
    frequency = []
    envelope = []

    with csv_path.open(newline="") as f:
        reader = csv.DictReader(f)
        for row in reader:
            sample_no.append(int(row["sample_no"]))
            frequency.append(float(row["frequency"]))
            envelope.append(float(row["envelope"]))

    sample_no = np.asarray(sample_no, dtype=np.float64)
    frequency = np.asarray(frequency, dtype=np.float64)
    envelope = np.asarray(envelope, dtype=np.float64)

    samples = np.arange(len(original), dtype=np.float64)
    interp_frequency = np.interp(samples, sample_no, np.maximum(frequency, 0.0))
    interp_envelope = np.interp(samples, sample_no, envelope)
    phase = np.cumsum(2.0 * np.pi * interp_frequency / sample_rate)
    synth = np.sin(phase) * interp_envelope * config.max_synth_amp
    synth[interp_frequency <= 0.0] = 0.0

    left = original.astype(np.float64)
    peak = np.max(np.abs(left), initial=0.0)
    if peak > 1.0:
        left /= peak

    stereo = np.column_stack((left, synth))
    output_path.parent.mkdir(parents=True, exist_ok=True)
    sf.write(output_path, stereo.astype(np.float32), sample_rate)


def process_file(audio_path, reference_dir, synthesized_dir, config, method):
    signal, sample_rate = read_audio(audio_path)
    if method == "yin":
        sample_positions, frequencies, envelopes = track_pitch(signal, sample_rate, config)
    elif method == "mpm":
        sample_positions, frequencies, envelopes = track_pitch_mpm(signal, sample_rate, config)
    elif method == "rbj-asdf":
        sample_positions, frequencies, envelopes = track_pitch_rbj_asdf(
            signal, sample_rate, config
        )
    else:
        raise ValueError(f"Unsupported pitch method: {method}")

    stem = f"{method}-{audio_path.stem}"
    csv_path = reference_dir / f"{stem}.csv"
    synth_path = synthesized_dir / f"{stem}.wav"

    write_reference_csv(csv_path, sample_positions, frequencies, envelopes, sample_rate)
    synthesize_check_wav(csv_path, audio_path, synth_path, config)

    voiced = int(np.count_nonzero(frequencies > 0.0))
    return csv_path, synth_path, len(sample_positions), voiced


def parse_args():
    parser = argparse.ArgumentParser(
        description="Extract 1 ms pitch/envelope reference CSVs and stereo sine-check WAVs."
    )
    parser.add_argument(
        "audio_files",
        nargs="*",
        type=Path,
        help="WAV files to process. Defaults to the initial pitch-reference fixture set.",
    )
    parser.add_argument(
        "--audio-dir",
        type=Path,
        default=Path("test/audio_files"),
        help="Directory used to resolve default or relative audio file names.",
    )
    parser.add_argument(
        "--reference-dir",
        type=Path,
        default=Path("test/reference"),
        help="Directory for generated reference CSV files.",
    )
    parser.add_argument(
        "--synthesized-dir",
        type=Path,
        default=Path("test/synthesized"),
        help="Directory for generated stereo verification WAV files.",
    )
    parser.add_argument(
        "--method",
        choices=("yin", "mpm", "rbj-asdf"),
        default="yin",
        help="Pitch tracker to use. The existing YIN FFT path remains the default.",
    )
    parser.add_argument(
        "--fmin",
        type=float,
        default=Config.fmin,
        help="Minimum search frequency in Hz.",
    )
    parser.add_argument(
        "--mpm-peak-ratio",
        type=float,
        default=Config.mpm_peak_ratio,
        help="MPM peak threshold relative to the best NSDF peak.",
    )
    parser.add_argument(
        "--mpm-min-clarity",
        type=float,
        default=Config.mpm_min_clarity,
        help="Minimum NSDF peak clarity for voiced MPM frames.",
    )
    parser.add_argument(
        "--rbj-asdf-alpha",
        type=float,
        default=Config.rbj_asdf_alpha,
        help="Lag penalty exponent for RBJ-ASDF peak scoring.",
    )
    parser.add_argument(
        "--rbj-asdf-min-confidence",
        type=float,
        default=Config.rbj_asdf_min_confidence,
        help="Minimum normalized RBJ-ASDF autocorrelation confidence.",
    )
    return parser.parse_args()


def main():
    args = parse_args()
    config = Config(
        fmin=args.fmin,
        mpm_peak_ratio=args.mpm_peak_ratio,
        mpm_min_clarity=args.mpm_min_clarity,
        rbj_asdf_alpha=args.rbj_asdf_alpha,
        rbj_asdf_min_confidence=args.rbj_asdf_min_confidence,
    )

    audio_files = args.audio_files
    if not audio_files:
        audio_files = [args.audio_dir / name for name in DEFAULT_AUDIO_FILES]
    else:
        audio_files = [
            path
            if path.is_absolute() or path.exists()
            else args.audio_dir / path
            for path in audio_files
        ]

    for audio_path in audio_files:
        if not audio_path.exists():
            raise FileNotFoundError(audio_path)
        csv_path, synth_path, frames, voiced = process_file(
            audio_path, args.reference_dir, args.synthesized_dir, config, args.method
        )
        print(
            f"{audio_path.name}: wrote {csv_path} and {synth_path} "
            f"({frames} frames, {voiced} voiced, method={args.method})"
        )


if __name__ == "__main__":
    main()
