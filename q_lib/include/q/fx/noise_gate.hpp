/*=============================================================================
   Copyright (c) 2014-2023 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(CYCFI_Q_NOISE_GATE_HPP_JANUARY_17_2021)
#define CYCFI_Q_NOISE_GATE_HPP_JANUARY_17_2021

#include <q/support/base.hpp>
#include <q/support/literals.hpp>
#include <q/fx/envelope.hpp>
#include <q/fx/differentiator.hpp>

namespace cycfi::q
{
   ////////////////////////////////////////////////////////////////////////////
   // Noise_gate with an onset_threshold, a release_threshold, and an optional
   // attack_window.
   //
   // Constructor examples :
   //            attack_window        onset_threshold  release_threshold
   // noise_gate <10>          _gate{ -33_dB,          -45_dB            };
   // noise_gate               _gate{ -33_dB,          -45_dB            };
   // noise_gate               _gate{                  -45_dB            };
   //
   // To process new sample s : calculate envelope, then calculate gate
   //
   // auto envelope = _peak_envelope_follower(std::abs(s));
   // auto gate = _gate(envelope);
   //
   // _gate(envelope) returns true (if it's open) or false (if it's closed).
   //
   // On onsets, the noise_gate opens up if the signal envelope goes above
   // the specified onset threshold (defaults to +12dB above
   // release_threshold).
   //
   // On note release, the noise_gate turns off if the signal goes below a
   // specified absolute release threshold.
   //
   // basic_noise_gate<attack_window> is a template class that implements
   // noise_gate. The parameter `attack_window` (defaults to 0) specifies a
   // moving sum window width. If `attack_window` is non-zero, the envelope's derivative
   // must exceed a consecutive run of `attack_window` samples that add up to
   // a value greater than the required onset threshold. This prevents slow
   // moving attacks to pass as valid onsets.
   //
   // The attack_window may be used to directly specify the attack window
   // size (in number of samples). Alrenatively, an arbitrary non-zero number
   // may be used if you want to set the actual attack window in the
   // constructor instead. For this case, a constructor taking in a duration
   // and the sps (samples per second) is provided.
   ////////////////////////////////////////////////////////////////////////////
   namespace detail
   {
      template <std::size_t attack_window>
      struct noise_gate_base
      {
         noise_gate_base() {}
         noise_gate_base(duration attack_width, float sps)
          : _sum{attack_width, sps}
         {}

         differentiator          _diff;
         moving_sum              _sum{attack_window};
      };

      template <>
      struct noise_gate_base<0>
      {
         noise_gate_base() {}
         noise_gate_base(duration attack_width, float sps)
         {}
      };
   }

   template <std::size_t attack_window = 0>
   class basic_noise_gate : detail::noise_gate_base<attack_window>
   {
      using base_type = detail::noise_gate_base<attack_window>;

   public:
                              basic_noise_gate(
                                 decibel onset_threshold
                               , decibel release_threshold
                              );

                              basic_noise_gate(
                                 decibel onset_threshold
                               , decibel release_threshold
                               , duration attack_width
                               , float sps
                              );

                              basic_noise_gate(decibel release_threshold);

      bool                    operator()(float env);
      bool                    operator()() const;

      void                    onset_threshold(decibel onset_threshold);
      void                    onset_threshold(float onset_threshold);
      void                    release_threshold(decibel release_threshold);
      void                    release_threshold(float release_threshold);

      float                   onset_threshold() const;
      float                   release_threshold() const;

   private:

      bool                    _state = 0;
      float                   _onset_threshold;
      float                   _release_threshold;
   };

   using noise_gate = basic_noise_gate<>;

   ////////////////////////////////////////////////////////////////////////////
   // Inline implementation
   ////////////////////////////////////////////////////////////////////////////
   template <std::size_t attack_window>
   inline basic_noise_gate<attack_window>::basic_noise_gate(
      decibel onset_threshold
    , decibel release_threshold
   )
    : _release_threshold{as_float(release_threshold)}
    , _onset_threshold{as_float(onset_threshold)}
   {
   }

   template <std::size_t attack_window>
   inline basic_noise_gate<attack_window>::basic_noise_gate(
      decibel onset_threshold
    , decibel release_threshold
    , duration attack_width
    , float sps
   )
    : base_type{attack_width, sps}
    , _release_threshold{as_float(release_threshold)}
    , _onset_threshold{as_float(onset_threshold)}
   {
      static_assert(attack_window > 0,
         "This constructor requires a non-zero attack_window.");
   }

   template <std::size_t attack_window>
   inline basic_noise_gate<attack_window>::basic_noise_gate(decibel release_threshold)
    : _release_threshold{as_float(release_threshold)}
    , _onset_threshold{as_float(release_threshold + 12_dB)}
   {}

   template <std::size_t attack_window>
   inline bool basic_noise_gate<attack_window>::operator()(float env)
   {
      if constexpr(attack_window > 0)
      {
         if (!_state && this->_sum(this->_diff(env)) > _onset_threshold)
            _state = 1;
         else if (_state && env < _release_threshold)
            _state = 0;
      }
      else
      {
         if (!_state && env > _onset_threshold)
            _state = 1;
         else if (_state && env < _release_threshold)
            _state = 0;
      }

      return _state;
   }

   template <std::size_t attack_window>
   inline bool basic_noise_gate<attack_window>::operator()() const
   {
      return _state;
   }

   template <std::size_t attack_window>
   inline void basic_noise_gate<attack_window>::onset_threshold(decibel onset_threshold)
   {
      _onset_threshold = as_float(onset_threshold);
   }

   template <std::size_t attack_window>
   inline void basic_noise_gate<attack_window>::release_threshold(decibel release_threshold)
   {
      _release_threshold = as_float(release_threshold);
   }

   template <std::size_t attack_window>
   inline void basic_noise_gate<attack_window>::onset_threshold(float onset_threshold)
   {
      _onset_threshold = onset_threshold;
   }

   template <std::size_t attack_window>
   inline void basic_noise_gate<attack_window>::release_threshold(float release_threshold)
   {
      _release_threshold = release_threshold;
   }

   template <std::size_t attack_window>
   inline float basic_noise_gate<attack_window>::onset_threshold() const
   {
      return _onset_threshold;
   }

   template <std::size_t attack_window>
   inline float basic_noise_gate<attack_window>::release_threshold() const
   {
      return _release_threshold;
   }
}

#endif
