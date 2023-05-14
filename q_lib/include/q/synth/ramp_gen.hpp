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
   namespace concepts
   {
      /////////////////////////////////////////////////////////////////////////
      // The ramp concept requirements
      /////////////////////////////////////////////////////////////////////////
      template <typename T>
      concept ramp = requires(T v)
      {
         v();
         v.reset();
         v.config(1_ms, 44100.0);
      };
   }

   ////////////////////////////////////////////////////////////////////////////
   // Ramp generator abstract base class. This is provided so we can hold
   // references (pointers, smart pointers, etc.) to ramp generator in std
   // containers.
   ////////////////////////////////////////////////////////////////////////////
   struct ramp_base
   {
      virtual        ~ramp_base() = default;
      virtual float  operator()(float offset, float scale) = 0;
      virtual bool   done() const = 0;
      virtual void   reset() = 0;

      virtual void   config(duration width, float sps) = 0;
   };

   ////////////////////////////////////////////////////////////////////////////
   // Ramp generators are generic components used to compose segments of an
   // envelope. Multiple ramp segments with distinct shape characteristics
   // may be used to construct ADSR envelopes, AD envelopes, etc. The common
   // feature of a ramp generator is the ability to specify the ramp's width.
   // Available ramp shape forms include exponential, linear, blackman, hold,
   // and hann, both upward and downward variants of each.
   ////////////////////////////////////////////////////////////////////////////
   template <concepts::ramp Base>
   struct ramp_gen : ramp_base, Base
   {
                     ramp_gen(duration width, float sps);

      virtual float  operator()(float offset, float scale) override;
      virtual bool   done() const override;
      virtual void   config(duration width, float sps) override;

      virtual void   reset() override;

   private:

      std::size_t    _time = 0;
      std::size_t    _end = 0;
   };

   ////////////////////////////////////////////////////////////////////////////
   // Inline Implementation
   ////////////////////////////////////////////////////////////////////////////
   inline void ramp_base::config(duration width, float sps)
   {
      this->config(width, sps);
   }

   template <concepts::ramp Base>
   inline ramp_gen<Base>::ramp_gen(duration width, float sps)
    : Base{width, sps}
    , _end(std::ceil(as_float(width) * sps))
   {
   }

   template <concepts::ramp Base>
   inline float ramp_gen<Base>::operator()(float offset, float scale)
   {
      ++_time;
      return offset + (Base::operator()() * scale);
   }

   template <concepts::ramp Base>
   inline bool ramp_gen<Base>::done() const
   {
      return _time >= _end;
   }

   template <concepts::ramp Base>
   inline void ramp_gen<Base>::config(
      duration width, float sps)
   {
      Base::config(width, sps);
      _end = std::ceil(as_float(width) * sps);

      Base::reset();
      reset();
   }

   template <concepts::ramp Base>
   inline void ramp_gen<Base>::reset()
   {
      Base::reset();
      _time = 0;
   }
}

#endif
