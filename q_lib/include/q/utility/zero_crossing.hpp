/*=============================================================================
   Copyright (c) 2014-2018 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(CYCFI_Q_ZERO_CROSSING_HPP_MARCH_12_2018)
#define CYCFI_Q_ZERO_CROSSING_HPP_MARCH_12_2018

#include <q/utility/bitstream.hpp>
#include <q/utility/ring_buffer.hpp>
#include <infra/assert.hpp>
#include <cmath>

namespace cycfi { namespace q
{
   ////////////////////////////////////////////////////////////////////////////
   // The zero_crossing class saves zero-crossing information necessary to
   // extract accurate timing information such as periods between pulses for
   // performing analysis such as bitstream autocorrelation.
   //
   // Each zero crossing pulse is saved in a ring buffer of info elements.
   // Data includes the maximum height of the waveform bounded by the pulse
   // as well as the leading and trailing coordinates (frame index) and y
   // coordinates (the sample values before and after each zero crossing) of
   // the zero crossings.
   //
   // Only the latest few (finite amount) of zero crossing information is
   // saved, given by the window constructor parameter.
   //
   // Each call to the function operator, given a sample s, returns the
   // zero-crossing state (bool). is_ready() returns true when we have
   // sufficient info to perform analysis. is_ready() returns true after
   // every window/2 frames. Information about each zero crossing can be
   // obtained using the index operator[]. The leftmost edge is at the 0th
   // index while the rightmost edge is at index num_edges()-1.
   ////////////////////////////////////////////////////////////////////////////
   class zero_crossing
   {
   public:

      struct info
      {
         using crossing_data = std::pair<float, float>;

         void              update_peak(float s);
         std::size_t       period(info const& next) const;
         float             fractional_period(info const& next) const;

         crossing_data     _crossing;
         float             _peak;
         int               _leading_edge = -1;
         int               _trailing_edge = -1;
      };

      static constexpr float prediction_threshold = 0.06;
      static constexpr float prediction_pulse_threshold = 0.6;

                           zero_crossing(decibel hysteresis, std::size_t window);
                           zero_crossing(zero_crossing const& rhs) = default;
                           zero_crossing(zero_crossing&& rhs) = default;

      zero_crossing&       operator=(zero_crossing const& rhs) = default;
      zero_crossing&       operator=(zero_crossing&& rhs) = default;

      std::size_t          num_edges() const;
      std::size_t          capacity() const;
      std::size_t          frame() const;
      std::size_t          window_size() const;
      bool                 is_ready() const;
      float                peak_pulse() const;

      bool                 operator()(float s);
      bool                 operator()() const;
      info const&          operator[](std::size_t index) const;
      info&                operator[](std::size_t index);

   private:

      void                 update_state(float s);
      void                 shift(std::size_t n);
      void                 reset();

      using info_storage = ring_buffer<info, std::array<info, 64>>;

      float                _prev = 0.0f;
      float const          _hysteresis;
      bool                 _state = false;
      info_storage         _info;
      std::size_t          _num_edges = 0;
      std::size_t const    _window_size;
      std::size_t          _frame = 0;
      bool                 _ready = false;
      float                _peak_pulse = 0.0f;
   };

   ////////////////////////////////////////////////////////////////////////////
   // Implementation
   ////////////////////////////////////////////////////////////////////////////
   namespace detail
   {
      inline std::size_t adjust_window_size(std::size_t window)
      {
         return (window + bitstream<>::value_size - 1) / bitstream<>::value_size;
      }
   }

   inline zero_crossing::zero_crossing(decibel hysteresis, std::size_t window)
    : _hysteresis(-float(hysteresis))
    , _window_size(detail::adjust_window_size(window) * bitstream<>::value_size)
   {}

   inline void zero_crossing::info::update_peak(float s)
   {
      _peak = std::max(s, _peak);
   }

   inline std::size_t zero_crossing::info::period(info const& next) const
   {
      CYCFI_ASSERT(_leading_edge <= next._leading_edge, "Invalid order.");
      return next._leading_edge - _leading_edge;
   }

   inline float zero_crossing::info::fractional_period(info const& next) const
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

   inline std::size_t zero_crossing::num_edges() const
   {
      return _num_edges;
   }

   inline std::size_t zero_crossing::capacity() const
   {
      return _info.size();
   }

   inline std::size_t zero_crossing::frame() const
   {
      return _frame;
   }

   inline std::size_t zero_crossing::window_size() const
   {
      return _window_size;
   }

   inline void zero_crossing::reset()
   {
      _num_edges = 0;
      _state = false;
      _frame = 0;
   }

   inline bool zero_crossing::is_ready() const
   {
      return _ready;
   }

   inline float zero_crossing::peak_pulse() const
   {
      return _peak_pulse;
   }

   inline void zero_crossing::update_state(float s)
   {
      if (_ready)
      {
         shift(_window_size / 2);
         _ready = false;
         _peak_pulse = 0.0f;
      }

      if (num_edges() >= capacity())
         reset();

      if (s > 0.0f)
      {
         if (!_state)
         {
            CYCFI_ASSERT(_num_edges < _info.size(), "Bad _size");
            _info.push({ { _prev, s }, s, int(_frame) });
            ++_num_edges;
            _state = 1;
         }
         else
         {
            _info[0].update_peak(s);
         }
         if (s > _peak_pulse)
         {
            _peak_pulse = s;
         }
      }
      else if (_state && s < _hysteresis)
      {
         _state = 0;
         _info[0]._trailing_edge = _frame;
      }

      _prev = s;
   }

   inline bool zero_crossing::operator()(float s)
   {
      if (num_edges() >= capacity())
         reset();

      if ((_frame == _window_size/2) && num_edges() == 0)
         reset();

      update_state(s);

      if (++_frame >= _window_size && !_state)
      {
         // Remove half the size from _frame, so we can continue seamlessly
         _frame -= _window_size / 2;

         // We need at least two rising edges.
         if (num_edges() > 1)
            _ready = true;
         else
            reset();
      }

      return _state;
   };

   inline bool zero_crossing::operator()() const
   {
      return _state;
   }

   inline zero_crossing::info const&
   zero_crossing::operator[](std::size_t index) const
   {
      return _info[(_num_edges-1)-index];
   }

   inline zero_crossing::info&
   zero_crossing::operator[](std::size_t index)
   {
      return _info[(_num_edges-1)-index];
   }

   inline void zero_crossing::shift(std::size_t n)
   {
      _info[0]._leading_edge -= n;
      if (!_state)
         _info[0]._trailing_edge -= n;
      auto i = 1;
      for (; i != _num_edges; ++i)
      {
         _info[i]._leading_edge -= n;
         int edge = (_info[i]._trailing_edge -= n);
         if (edge < 0.0f)
            break;
      }
      _num_edges = i;
   }
}}

#endif

