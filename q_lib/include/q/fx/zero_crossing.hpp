/*=============================================================================
   Copyright (c) 2014-2021 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(CYCFI_Q_ZERO_CROSSING_DECEMBER_7_2018)
#define CYCFI_Q_ZERO_CROSSING_DECEMBER_7_2018

#include <q/support/base.hpp>
#include <q/support/decibel.hpp>

namespace cycfi::q
{
   ////////////////////////////////////////////////////////////////////////////
   // zero_crossing generates pulses that coincide with the zero crossings of
   // the signal. To minimize noise, 1) we apply some amount of hysteresis
   // and 2) constrain the time between transitions to a minumum given
   // min_period (or max_freq).
   //
   // Note: the result is a bool.
   ////////////////////////////////////////////////////////////////////////////
   struct zero_crossing
   {
      zero_crossing(float hysteresis)
       : _hysteresis(-hysteresis)
      {}

      zero_crossing(decibel hysteresis)
       : _hysteresis(-as_float(hysteresis))
      {}

      bool operator()(float s)
      {
         // Offset s by half of hysteresis, so that zero cross detection is
         // centered on the actual zero.
         s += _hysteresis / 2;

         if (!_state && s > 0.0f)
            _state = 1;
         else if (_state && s < _hysteresis)
            _state = 0;
         return _state;
      }

      bool operator()() const
      {
         return _state;
      }

      float _hysteresis;
      bool _state = 0;
   };

   ////////////////////////////////////////////////////////////////////////////
   // zero_crossing_ex is an extended version of zero_crossing with more
   // information about the saved for futher inspection.
   //
   // Zero-crossing information includes the maximum height of the waveform
   // bounded by the pulse, the pulse width, as well as the leading edge and
   // trailing edge frame positions (number of samples from the start) and y
   // coordinates (the sample values before and after each zero crossing) of
   // the zero crossings.
   //
   // This extended information can be obtained via the `get_info()` member
   // function. This information is only valid after the trailing edge of the
   // pulse.
   //
   // Note: the result is an int which can be 1 (leading edge), -1 (trailing
   // edge) and 0 (no state change).
   ////////////////////////////////////////////////////////////////////////////
   struct zero_crossing_ex
   {
      struct info
      {
         using crossing_data = std::pair<float, float>;
         static constexpr auto undefined_edge = int_min<int>();

         std::size_t       period(info const& next) const;
         float             fractional_period(info const& next) const;
         std::size_t       width() const;
         float             height() const;

         crossing_data     _crossing;
         float             _peak;
         int               _leading_edge = undefined_edge;
         int               _trailing_edge = undefined_edge;
      };

                           zero_crossing_ex(float hysteresis);
                           zero_crossing_ex(decibel hysteresis);

      int                  operator()(float s);
      bool                 operator()() const;
      info const&          get_info() const;

      float                _hysteresis;
      bool                 _state = 0;
      std::size_t          _frame = 0;
      float                _prev = 0.0f;
      info                 _info;
   };

   ////////////////////////////////////////////////////////////////////////////
   // Implementation
   ////////////////////////////////////////////////////////////////////////////
   inline std::size_t zero_crossing_ex::info::period(info const& next) const
   {
      CYCFI_ASSERT(_leading_edge <= next._leading_edge, "Invalid order.");
      return next._leading_edge - _leading_edge;
   }

   inline float zero_crossing_ex::info::fractional_period(info const& next) const
   {
      CYCFI_ASSERT(_leading_edge <= next._leading_edge, "Invalid order.");

      // Get the start edge
      auto prev1 = _crossing.first;
      auto curr1 = _crossing.second;
      auto dy1 = curr1 - prev1;
      auto dx1 = -prev1 / dy1;

      // Get the next edge
      auto prev2 = next._crossing.first;
      auto curr2 = next._crossing.second;
      auto dy2 = curr2 - prev2;
      auto dx2 = -prev2 / dy2;

      // Calculate the fractional period
      auto result = next._leading_edge - _leading_edge;
      return result + (dx2 - dx1);
   }

   std::size_t zero_crossing_ex::info::width() const
   {
      return _trailing_edge - _leading_edge;
   }

   float zero_crossing_ex::info::height() const
   {
      return _peak;
   }

   inline zero_crossing_ex::zero_crossing_ex(float hysteresis)
    : _hysteresis(-hysteresis)
   {}

   inline zero_crossing_ex::zero_crossing_ex(decibel hysteresis)
    : _hysteresis(-as_float(hysteresis))
   {}

   inline int zero_crossing_ex::operator()(float s)
   {
      // Offset s by half of hysteresis, so that zero cross detection is
      // centered on the actual zero.
      s += _hysteresis / 2;

      int result = 0;
      if (s > 0.0f)
      {
         if (!_state)
         {
            _state = 1;
            result = 1;
            _info = info{{ _prev, s }, s, int(_frame)};
         }
         else
         {
            _info._peak = std::max(s, _info._peak);
         }
      }
      else if (_state && s < _hysteresis)
      {
         _state = 0;
         result = -1;
         _info._trailing_edge = _frame;
      }
      ++_frame;
      _prev = s;
      return result;
   }

   inline bool zero_crossing_ex::operator()() const
   {
      return _state;
   }

   inline zero_crossing_ex::info const& zero_crossing_ex::get_info() const
   {
      return _info;
   }
}

#endif
