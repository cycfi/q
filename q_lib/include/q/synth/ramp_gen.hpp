/*=============================================================================
   Copyright (c) 2014-2023 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(CYCFI_Q_RAMP_GEN_HPP_APRIL_29_2023)
#define CYCFI_Q_RAMP_GEN_HPP_APRIL_29_2023

#include <q/support/base.hpp>
#include <q/support/literals.hpp>

namespace cycfi::q
{
   ////////////////////////////////////////////////////////////////////////////
   // Ramp generator abstract base class. This is provided so we can hold
   // references (pointers, smart pointers, etc.) to ramp generator in std
   // containers.
   ////////////////////////////////////////////////////////////////////////////
   struct ramp_base
   {
      virtual float  operator()() = 0;
      virtual bool   done() const = 0;
      virtual void   reset() = 0;

      virtual void   config(
                        duration width
                      , float sps
                      , float start_level = 0.0f
                      , float end_level = 1.0f
                     ) = 0;

      void           config(
                        duration width
                      , float sps
                      , decibel start_level
                      , decibel end_level = 0_dB
                     );
   };

   ////////////////////////////////////////////////////////////////////////////
   // Ramp generators are generic components used to compose segments of an
   // envelope. Multiple ramp segments with distinct shape characteristics
   // may be used to construct ADSR envelopes, AD envelopes, etc. The common
   // feature of a ramp generator is the ability to specify the ramp's
   // width, start-level, and end-level. Available ramp shape forms include
   // exponential, linear, blackman, and hann.
   ////////////////////////////////////////////////////////////////////////////
   template <typename Base>
   struct ramp_gen : ramp_base, Base
   {
                     ramp_gen(
                        duration width
                      , float sps
                      , float start_level = 0.0f
                      , float end_level = 1.0f
                     );

                     ramp_gen(
                        duration width
                      , float sps
                      , decibel start_level
                      , decibel end_level = 0_dB
                     );

      virtual float  operator()() override;
      virtual bool   done() const override;
      virtual void   config(
                        duration width
                      , float sps
                      , float start_level = 0.0f
                      , float end_level = 1.0f
                     ) override;

      virtual void   reset() override;

   private:

      std::size_t _time = 0;
      std::size_t _end = 0;
      float _offset, _scale;
   };

   ////////////////////////////////////////////////////////////////////////////
   // Inline Implementation
   ////////////////////////////////////////////////////////////////////////////
   inline void ramp_base::config(
      duration width, float sps, decibel start_level, decibel end_level)
   {
      this->config(width, sps, as_float(start_level), as_float(end_level));
   }

   template <typename Base>
   inline ramp_gen<Base>::ramp_gen(
      duration width, float sps, float start_level, float end_level)
    : Base{width, sps}
    , _end(std::ceil(as_float(width) * sps))
    , _offset{std::min(start_level, end_level)}
    , _scale{std::abs(start_level-end_level)}
   {
   }

   template <typename Base>
   inline ramp_gen<Base>::ramp_gen(
      duration width, float sps, decibel start_level, decibel end_level)
    : ramp_gen{width, sps, as_float(start_level), as_float(end_level)}
   {
   }

   template <typename Base>
   inline float ramp_gen<Base>::operator()()
   {
      ++_time;
      return _offset + (Base::operator()() * _scale);
   }

   template <typename Base>
   inline bool ramp_gen<Base>::done() const
   {
      return _time >= _end;
   }

   template <typename Base>
   inline void ramp_gen<Base>::config(
      duration width, float sps, float start_level, float end_level)
   {
      Base::config(width, sps);
      _end = std::ceil(as_float(width) * sps);
      _offset = std::min(start_level, end_level);
      _scale = std::abs(start_level-end_level);

      Base::reset();
      reset();
   }

   template <typename Base>
   inline void ramp_gen<Base>::reset()
   {
      Base::reset();
      _time = 0;
   }
}

#endif
