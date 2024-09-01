/*=============================================================================
   Copyright (c) 2014-2024 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(CYCFI_Q_SYNTH_CONCEPTS_HPP_MAY_12_2023)
#define CYCFI_Q_SYNTH_CONCEPTS_HPP_MAY_12_2023

#include <q/support/basic_concepts.hpp>
#include <q/support/phase.hpp>

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
}

#endif
