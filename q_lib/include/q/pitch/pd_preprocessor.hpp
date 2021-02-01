/*=============================================================================
   Copyright (c) 2014-2021 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(CYCFI_Q_PD_PREPROCESSOR_HPP_MARCH_12_2018)
#define CYCFI_Q_PD_PREPROCESSOR_HPP_MARCH_12_2018

#include <q/fx/signal_conditioner.hpp>
#include <q/fx/waveshaper.hpp>
#include <q/fx/moving_average.hpp>
#include <q/fx/noise_gate.hpp>

namespace cycfi::q
{
   ////////////////////////////////////////////////////////////////////////////
   class pd_preprocessor : public signal_conditioner
   {
   public:

      using config = signal_conditioner::config;

      template <typename Config>
      pd_preprocessor(
         Config const& conf
       , frequency lowest_freq
       , frequency highest_freq
       , std::uint32_t sps
      )
       : signal_conditioner{conf, sps}
       , _lp1{highest_freq * 2, sps}
       , _lp2{lowest_freq / 2, sps}
      {}

      float operator()(float s)
      {
         // Bandpass filter
         s = _lp1(s);
         s -= _lp2(s);

         return signal_conditioner::operator()(s);
      }

   private:

      one_pole_lowpass        _lp1;
      one_pole_lowpass        _lp2;
   };
}

#endif

