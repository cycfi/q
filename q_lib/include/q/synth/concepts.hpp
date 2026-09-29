/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(CYCFI_Q_SYNTH_CONCEPTS_HPP_MAY_12_2023)
#define CYCFI_Q_SYNTH_CONCEPTS_HPP_MAY_12_2023

#include <q/support/basic_concepts.hpp>
#include <q/support/phase.hpp>
#include <q/support/decibel.hpp>
#include <q/support/duration.hpp>

namespace cycfi::q::concepts
{
   template <typename T>
   concept Oscillator =
      std::copy_constructible<T> &&
      std::assignable_from<T&, T> &&
      std::default_initializable<T> &&
      requires(T o, T a, T b, phase_iterator pi)
      {
         o(pi);            // Generate a periodic waveform given `{phase_iterator}`, `pi`.
      };

   template <typename T>
   concept BasicOscillator =
      Oscillator<T> &&
      requires(T o, phase ph)
      {
         o(ph);            // Generate a periodic waveform given `{phase}`, `pi`.
      };

   template <typename T>
   concept BandwidthLimitedOscillator =
      Oscillator<T> &&
      requires(T o, phase ph, phase dt)
      {
         o(ph, dt);        // Generate a periodic waveform given `{phase}`, `pi`
                           // and another `{phase}`, `dt` representing the delta
                           // phase between two samples of the waveform (this is
                           // equivalent to the `_step` member function of the
                           // `{phase_iterator}`). (ph))`
      };

   template <typename T>
   concept Generator =
      std::copy_constructible<T> &&
      std::assignable_from<T&, T> &&
      requires(T g, T a, T b)
      {
         g();              // Generate a signal.
      };

   template <typename T>
   concept Ramp =
      Generator<T> &&
      requires(T v, duration w, float sps)
      {
         T(w, sps);        // Construct a `Ramp` given `duration`, `w`, and `sps`.
         v.reset();        // Reset the Ramp to the start.
         v.config(w, sps); // Configure a `Ramp` given `duration`, `w`, and `sps`.
      };

   // An envelope generator: a Generator that is started and stopped, and
   // says where it is. `config` is its parameters, set at construction or
   // in place (a note retriggered keeps its level); `gain` is a master
   // volume on the output.
   template <typename T>
   concept EnvelopeGenerator =
      Generator<T> &&
      requires(T e, typename T::config const& cfg, float sps, float g)
      {
         T(cfg, sps);      // Construct from a config at a sample rate.
         e.set(cfg, sps);  // Reconfigure in place, the state kept.
         e.attack();       // Start.
         e.release();      // Stop: run out from wherever it is.
         e.reset();        // Idle and silent.
         { e.in_idle_phase() } -> std::convertible_to<bool>;
         { e.in_release_phase() } -> std::convertible_to<bool>;
         e.gain(g);
         { e.gain() } -> std::convertible_to<float>;
      };

   // What an `adsr_envelope_gen` needs from the config it is handed. Any
   // type with these four members will do; `adsr_envelope_gen::config` is
   // just one default kind.
   //
   // A sustain rate is deliberately not required. A config that carries
   // one gets a sustain that runs down over that time, which is Q's own
   // idea; a config without one gets a sustain that holds its level until
   // the note is released, which is the classic ADSR.
   template <typename T>
   concept ADSRConfig =
      requires(T const& cfg)
      {
         { cfg.attack_rate } -> std::convertible_to<duration>;
         { cfg.decay_rate } -> std::convertible_to<duration>;
         { cfg.sustain_level } -> std::convertible_to<decibel>;
         { cfg.release_rate } -> std::convertible_to<duration>;
      };
}

#endif
