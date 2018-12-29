/*=============================================================================
   Copyright (c) 2014-2018 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(CYCFI_Q_ZERO_CROSSING_HPP_MARCH_12_2018)
#define CYCFI_Q_ZERO_CROSSING_HPP_MARCH_12_2018

#include <q/utility/bitstream.hpp>
#include <q/utility/ring_buffer.hpp>
#include <q/utility/bitstream.hpp>
#include <q/detail/count_bits.hpp>
#include <infra/assert.hpp>
#include <cmath>

namespace cycfi { namespace q
{
   ////////////////////////////////////////////////////////////////////////////
   // The zero_crossing class saves bit information for each sample in a
   // bitstream (see bitstream.hpp) for performing analysis such as
   // autocorrelation, as well as additional information about each zero
   // crossing event necessary to extract accurate timing information such as
   // periods between pulses.
   ////////////////////////////////////////////////////////////////////////////
   class zero_crossing
   {
   public:

      // Each zero crossing pulse is saved in a buffer with info elements
      // (below) which can be accessed anytime. The data includes the maximum
      // height of the waveform enclosed within the pulse as well as the x
      // (frame index) and y positions (the sample values before and after
      // each zero crossing) of the zero crossings. Only the latest few
      // (finite amount) of zero crossing information is saved, given by the
      // window constructor parameter.

      struct info
      {
         using crossing_data = std::pair<float, float>;

         void              update_peak(float s);

         crossing_data     _crossing;
         float             _peak;
         int               _leading_edge;
         int               _trailing_edge;
      };

                           zero_crossing(decibel hysteresis, std::size_t window);

      std::size_t          num_edges() const;
      std::size_t          capacity() const;
      std::size_t          frame() const;
      std::size_t          window_size() const;

      bool                 operator()(float s);
      bool                 operator()() const;
      info const&          operator[](std::size_t index) const;
      info&                operator[](std::size_t index);

   private:

      void                 estimate_period();
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
   };

   ////////////////////////////////////////////////////////////////////////////
   // Implementation
   ////////////////////////////////////////////////////////////////////////////
   namespace detail
   {
      inline std::size_t adjust_window_size(std::size_t window)
      {
         auto result = (window / bitstream<>::value_size) + 1;
         if (result % 2)
            ++result;
         return result;
      }
   }

   inline zero_crossing::zero_crossing(decibel hysteresis, std::size_t window)
    : _hysteresis(-float(hysteresis))
    , _window_size(detail::adjust_window_size(window))
   {}

   inline void zero_crossing::info::update_peak(float s)
   {
      _peak = std::max(s, _peak);
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

   void zero_crossing::reset()
   {
      _num_edges = 0;
      _state = false;
      _frame = 0;
   }

   inline void zero_crossing::update_state(float s)
   {
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
      }
      else if (_state && s < _hysteresis)
      {
         _state = 0;
         _info[0]._trailing_edge = _frame;
      }
      _prev = s;
   }

   inline void zero_crossing::estimate_period()
   {
      if (++_frame >= _window_size && !_state)
      {
         auto half = _window_size / 2;

         // Remove half the size from _frame, so we can continue seamlessly
         _frame -= half;

         // We need at least two rising edges.
         if (num_edges() > 1)
         {
            bitstream<> _bits{ _window_size };
            for (auto i = 0; i != num_edges(); ++i)
            {
               auto& info = (*this)[i];
               auto pos = std::max<int>(info._leading_edge, 0);
               auto n = info._trailing_edge - pos;
               _bits.set(pos, n, 1);
            }

            // Shift the edges by half the number of samples
            shift(half);
         }
         else
         {
            reset();
         }
      }
   }

   inline bool zero_crossing::operator()(float s)
   {
      if (num_edges() >= capacity())
         reset();

      update_state(s);
      estimate_period();
      return _state;
   };

   inline bool zero_crossing::operator()() const
   {
      return _state;
   }

   inline zero_crossing::info const& zero_crossing::operator[](std::size_t index) const
   {
      return _info[(_num_edges-1)-index];
   }

   inline zero_crossing::info& zero_crossing::operator[](std::size_t index)
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

   struct auto_correlator
   {
      static constexpr auto value_size = bitstream<>::value_size;

      auto_correlator(bitstream<> const& bits)
       : _bits(bits)
       , _size(bits.size())
       , _mid_array(((_size / value_size) / 2) - 1)
      {}

      std::size_t mid_point() const { return _size / 2; }

      inline std::size_t operator()(std::size_t pos)
      {
         auto const index = pos / value_size;
         auto const shift = pos % value_size;

         auto const* p1 = _bits.data();
         auto const* p2 = _bits.data() + index;
         auto count = 0;

         if (shift == 0)
         {
            for (auto i = 0; i != _mid_array; ++i)
               count += detail::count_bits(*p1++ ^ *p2++);
         }
         else
         {
            auto shift2 = value_size - shift;
            for (auto i = 0; i != _mid_array; ++i)
            {
               auto v = *p2++ >> shift;
               v |= *p2 << shift2;
               count += detail::count_bits(*p1++ ^ v);
            }
         }
         return count;
      };

      bitstream<> const&   _bits;
      std::size_t const    _size;
      std::size_t const    _mid_array;
   };
}}

#endif

