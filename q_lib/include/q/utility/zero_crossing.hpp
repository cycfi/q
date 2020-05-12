/*=============================================================================
   Copyright (c) 2014-2020 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(CYCFI_Q_ZERO_CROSSING_HPP_MARCH_12_2018)
#define CYCFI_Q_ZERO_CROSSING_HPP_MARCH_12_2018

#include <q/support/base.hpp>
#include <q/utility/bitset.hpp>
#include <q/utility/ring_buffer.hpp>
#include <q/support/decibel.hpp>
#include <infra/assert.hpp>
#include <cmath>

namespace cycfi::q
{
   ////////////////////////////////////////////////////////////////////////////
   // The zero_crossing class saves zero-crossing information necessary to
   // extract accurate timing information such as periods between pulses for
   // performing analysis such as bitstream autocorrelation.
   //
   // Each zero crossing pulse is saved in a ring buffer of info elements.
   // Data include the maximum height of the waveform bounded by the pulse,
   // the pulse width, as well as the leading edge and trailing edge frame
   // positions (number of samples from the start) and y coordinates (the
   // sample values before and after each zero crossing) of the zero
   // crossings.
   //
   // Only the latest few (finite amount) of zero crossing information is
   // saved, given by the window constructor parameter. The window is the
   // number of frames (samples) of information held by the zero_crossing
   // data structure.
   //
   // Each call to the function operator, given a sample s, returns the
   // zero-crossing state (bool). is_ready() returns true when we have
   // sufficient info to perform analysis. is_ready() returns true after
   // every window/2 frames. Information about each zero crossing can be
   // obtained using the index operator[]. The leftmost edge (oldest) is at
   // the 0th index while the rightmost edge (latest) is at index
   // num_edges()-1.
   //
   // After window/2 frames, the leading edge and trailing edge frame
   // positions are shifted by -window/2 such that an edge at frame index N
   // will be shifted to N-window/2. For example, if the window size is 100
   // and the leading edge is at frame 45, it will be shifted to -5 (45-50).
   //
   // This procedure is done to ensure seamless operation from one window to
   // the next. In the example above, frame index -5 is already past the left
   // side of the window, but will still be kept as long as the trailing edge
   // is still within the window. Take note that it is also possible for the
   // latest edge to have a trailing edge that goes past the right side of
   // the window. If for example, with the same window size 100, there can be
   // an edge with a leading edge at 95 and trailing edge at 120.
   ////////////////////////////////////////////////////////////////////////////
   class zero_crossing
   {
   public:

      static constexpr float pulse_height_diff = 0.8;
      static constexpr float pulse_width_diff = 0.85;
      static constexpr auto undefined_edge = int_min<int>();

      struct info
      {
         using crossing_data = std::pair<float, float>;

         void              update_peak(float s, std::size_t frame);
         std::size_t       period(info const& next) const;
         float             fractional_period(info const& next) const;
         int               width() const;
         bool              similar(info const& next) const;

         crossing_data     _crossing;
         float             _peak;
         int               _leading_edge = undefined_edge;
         int               _trailing_edge = undefined_edge;
         float             _width = 0.0f;
      };

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
      bool                 is_reset() const;

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
      float                _peak_update = 0.0f;
      float                _peak = 0.0f;
   };

   ////////////////////////////////////////////////////////////////////////////
   // Implementation
   ////////////////////////////////////////////////////////////////////////////
   namespace detail
   {
      inline std::size_t adjust_window_size(std::size_t window)
      {
         return (window + bitset<>::value_size - 1) / bitset<>::value_size;
      }
   }

   inline zero_crossing::zero_crossing(decibel hysteresis, std::size_t window)
    : _hysteresis(-float(hysteresis))
    , _window_size(detail::adjust_window_size(window) * bitset<>::value_size)
   {}

   inline void zero_crossing::info::update_peak(float s, std::size_t frame)
   {
      _peak = std::max(s, _peak);
      if ((_width == 0.0f) && (s < (_peak * 0.3)))
         _width = frame - _leading_edge;
   }

   inline std::size_t zero_crossing::info::period(info const& next) const
   {
      CYCFI_ASSERT(_leading_edge <= next._leading_edge, "Invalid order.");
      return next._leading_edge - _leading_edge;
   }

   inline bool zero_crossing::info::similar(info const& next) const
   {
      return rel_within(_peak, next._peak, 1.0f-pulse_height_diff) &&
         rel_within(_width, next._width, 1.0f-pulse_width_diff);
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

   inline bool zero_crossing::is_reset() const
   {
      return _frame == 0;
   }

   inline bool zero_crossing::is_ready() const
   {
      return _ready;
   }

   inline float zero_crossing::peak_pulse() const
   {
      return std::max(_peak, _peak_update);
   }

   inline void zero_crossing::update_state(float s)
   {
      if (_ready)
      {
         shift(_window_size / 2);
         _ready = false;
         _peak = _peak_update;
         _peak_update = 0.0f;
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
            _info[0].update_peak(s, _frame);
         }
         if (s > _peak_update)
         {
            _peak_update = s;
         }
      }
      else if (_state && s < _hysteresis)
      {
         _state = 0;
         auto& info = _info[0];
         info._trailing_edge = _frame;
         if (_peak == 0.0f)
            _peak = _peak_update;
      }

      _prev = s;
   }

   inline bool zero_crossing::operator()(float s)
   {
      // Offset s by half of hysteresis, so that zero cross detection is
      // centered on the actual zero.
      s += _hysteresis / 2;

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
}

#endif

