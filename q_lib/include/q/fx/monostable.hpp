/*=============================================================================
   Copyright (c) 2014-2023 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(CYCFI_Q_MONOSTABLE_HPP_DECEMBER_24_2015)
#define CYCFI_Q_MONOSTABLE_HPP_DECEMBER_24_2015

#include <q/support/base.hpp>
#include <q/support/literals.hpp>

namespace cycfi::q
{
   ////////////////////////////////////////////////////////////////////////////
   // monostable is a one shot pulse generator. A single pulse input generates
   // a timed pulse of the given duration. `basic_monostable` is the template
   // class it is based on. It has a `retriggerable` parameter that allows
   // retriggering. Typedefs are provided for non-retriggerable `monostable`
   // and retriggerable `retriggerable_monostable` types.
   ////////////////////////////////////////////////////////////////////////////
   template <bool retriggerable>
   struct basic_monostable
   {
      basic_monostable(duration d, float sps)
       : _n_samples(as_float(d) * sps)
      {}

      bool operator()(bool val)
      {
         // If we got a 1
         if (val)
         {
            // If retriggerable, always start
            if constexpr(retriggerable)
               start();

            // Else, start only if we are at the end
            else if (_ticks == 0)
               start();
         }
         // Count down
         if (_ticks)
            --_ticks;
         return _ticks != 0;
      }

      bool operator()() const
      {
         return _ticks != 0;
      }

      void start()
      {
         _ticks = _n_samples;
      }

      void stop()
      {
         _ticks = 0;
      }

      std::uint32_t  _n_samples;
      std::uint32_t  _ticks = 0;
   };

   using monostable = basic_monostable<false>;
   using retriggerable_monostable = basic_monostable<true>;
}

#endif
