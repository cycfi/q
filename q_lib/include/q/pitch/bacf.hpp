/*=============================================================================
   Copyright (c) 2014-2018 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(CYCFI_Q_BACF_HPP_MARCH_12_2018)
#define CYCFI_Q_BACF_HPP_MARCH_12_2018

#include <q/pitch/bitstream.hpp>
#include <q/detail/count_bits.hpp>
#include <q/utility/ring_buffer.hpp>
#include <infra/assert.hpp>
#include <cmath>

namespace cycfi { namespace q
{
   ////////////////////////////////////////////////////////////////////////////
   class edges
   {
   public:

      static constexpr float min_edge_deviation = 0.04;  // 4%

      struct info
      {
         using crossing_data = std::pair<float, float>;

         void              update_peak(float s);

         crossing_data     _crossing;
         float             _peak;
         int               _leading_edge;
         int               _trailing_edge;
         mutable bool      _inhibited = false;
      };

                           edges(float threshold)
                            : _threshold(-threshold)
                           {}

      struct span
      {
         explicit operator bool() const
         {
            return first && second;
         }

         float period() const;

         info const* first;
         info const* second;
      };

      bool                 operator()(float s, std::size_t index);
      bool                 operator()() const;
      info const&          operator[](std::size_t index) const;
      void                 reset();
      void                 shift(std::size_t n);
      std::size_t          size() const;
      bool                 is_full() const;
      span                 get_span(std::size_t period, bool all_edges = false) const;
      float                predict_period() const;

   private:

      using info_storage = ring_buffer<info, std::array<info, 64>>;

      float                _prev = 0.0f;
      float const          _threshold;
      bool                 _state = false;
      info_storage         _info;
      std::size_t          _size = 0;
      mutable float        _predicted_period = 0.0f;
   };

   ////////////////////////////////////////////////////////////////////////////
   template <typename T = std::uint32_t>
   class bacf
   {
   public:

      using correlation_vector = std::vector<std::uint16_t>;
      static constexpr float noise_threshold = 0.001;
      static constexpr float pulse_threshold = 0.6;

      struct info
      {
         correlation_vector   correlation;
         std::uint16_t        max_count = 0;
         std::uint16_t        min_count = 0;
         std::size_t          index = 0;
         float                periodicity;
      };

                              bacf(
                                 frequency lowest_freq
                               , frequency highest_freq
                               , std::uint32_t sps
                               , float threshold
                              );

                              bacf(bacf const& rhs) = default;
                              bacf(bacf&& rhs) = default;

      bacf&                   operator=(bacf const& rhs) = default;
      bacf&                   operator=(bacf&& rhs) = default;

                              template <typename F>
      bool                    operator()(float s, F f, std::size_t& extra);
      bool                    operator[](std::size_t i) const;

      std::size_t             size() const;
      info const&             result() const;
      bool                    is_start() const;
      std::size_t             position() const;
      std::size_t             minimum_period() const;
      void                    reset();

      edges const&            edges() const { return _edges; }
      edges::span             get_span(std::size_t index) const;
      edges::span             get_span() const;

   private:

      static std::size_t      buff_size(frequency freq, std::uint32_t sps);
      void                    set_bits(std::size_t from, std::size_t to);

      bitstream<T>            _bits;
      std::size_t             _size;
      std::size_t             _count = 0;
      std::size_t             _min_period;
      info                    _info;
      q::edges                _edges;
   };

   ////////////////////////////////////////////////////////////////////////////
   // Implementation
   ////////////////////////////////////////////////////////////////////////////
   template <typename T>
   inline std::size_t bacf<T>::buff_size(
      frequency freq, std::uint32_t sps)
   {
      auto period = sps / double(freq);
      return std::ceil(period) * 2;
   }

   template <typename T>
   inline bacf<T>::bacf(
      frequency lowest_freq
    , frequency highest_freq
    , std::uint32_t sps
    , float threshold
   )
    : _bits(buff_size(lowest_freq, sps))
    , _min_period(std::floor(sps / double(highest_freq)))
    , _edges(threshold)
   {
      _size = _bits.size();
      _info.correlation.resize(_size / 2, 0);
   }

   template <typename T, typename F>
   inline void auto_correlate(
      bitstream<T> const& bits, std::size_t start_pos, F f)
   {
      constexpr auto value_size = bitstream<T>::value_size;

      auto const size = bits.size();
      auto const array_size = size / value_size;
      auto const mid_pos = size / 2;
      auto const mid_array = (array_size / 2) - 1;

      auto index = start_pos / value_size;
      auto shift = start_pos % value_size;

      for (auto pos = start_pos; pos != mid_pos; ++pos)
      {
         auto* p1 = bits.data();
         auto* p2 = bits.data() + index;
         auto count = 0;

         if (shift == 0)
         {
            for (auto i = 0; i != mid_array; ++i)
               count += detail::count_bits(*p1++ ^ *p2++);
         }
         else
         {
            auto shift2 = value_size - shift;
            for (auto i = 0; i != mid_array; ++i)
            {
               auto v = *p2++ >> shift;
               v |= *p2 << shift2;
               count += detail::count_bits(*p1++ ^ v);
            }
         }
         ++shift;
         if (shift == value_size)
         {
            shift = 0;
            ++index;
         }

         f(pos, count);
      }
   }

   template <typename T>
   void  bacf<T>::set_bits(std::size_t from, std::size_t to)
   {
      // Get the highest peak
      float peak = 0;
      for (auto i = from; i != to; ++i)
      {
         if (_edges[i]._peak > peak)
            peak = _edges[i]._peak;
      }

      // Compute the threshold from the highest peak
      auto threshold = peak * pulse_threshold;

      // Set the bits
      for (auto i = from; i != to; ++i)
      {
         auto const& info = _edges[i];
         if (info._peak < threshold)
         {
            // inhibit weak pulses
            info._inhibited = true;
         }
         else
         {
            auto i = std::max<int>(info._leading_edge, 0);
            auto n = info._trailing_edge - i;
            _bits.set(i, n, 1);
         }
      }
   }

   template <typename T>
   template <typename F>
   inline bool bacf<T>::operator()(float s, F f, std::size_t& extra)
   {
      if (_edges.is_full())
      {
         _edges.reset();
         _count = 0;
         return false;
      }

      bool state = _edges(s, _count);
      if (++_count >= _size && !state)
      {
         _info.max_count = 0;
         _info.min_count = int_traits<uint16_t>::max;
         _info.index = 0;

         extra = _count - _size;
         auto half = _size / 2;

         // Remove half the size from _count, so we can continue seamlessly
         _count -= half;

         // We need at least two rising edges. No need to autocorrelate
         // if we do not have enough edges!
         if (_edges.size() > 1)
         {
            // Set the bits
            _bits.clear();
            set_bits(0, _edges.size());

            // Autocorrelate
            auto_correlate(_bits, _min_period,
               [&_info = this->_info](std::size_t pos, std::uint16_t count)
               {
                  _info.correlation[pos] = count;
                  _info.max_count = std::max(_info.max_count, count);
                  if (count < _info.min_count)
                  {
                     _info.min_count = count;
                     _info.index = pos;
                  }
               }
            );

            if (_info.correlation[_min_period] != _info.min_count)
            {
               _info.periodicity = 1.0 - (float(_info.min_count) / _info.max_count);
               f(); // Call the user function before shifting
            }
            else
            {
               _info.periodicity = 0.0f;
            }

            // Shift the edges by half the number of samples
            _edges.shift(half);

            // We are ready
            return true;
         }
         else
         {
            _edges.reset();
            return false;
         }
      }
      return false; // We're not ready yet.
   }

   template <typename T>
   inline bool bacf<T>::operator[](std::size_t i) const
   {
      return _bits.get(i);
   }

   template <typename T>
   inline std::size_t bacf<T>::size() const
   {
      return _bits.size();
   }

   template <typename T>
   inline typename bacf<T>::info const& bacf<T>::result() const
   {
      return _info;
   }

   template <typename T>
   inline bool bacf<T>::is_start() const
   {
      return _count == 0;
   }

   template <typename T>
   inline std::size_t bacf<T>::position() const
   {
      return _count;
   }

   template <typename T>
   inline std::size_t bacf<T>::minimum_period() const
   {
      return _min_period;
   }

   template <typename T>
   inline edges::span bacf<T>::get_span(std::size_t index) const
   {
      return _edges.get_span(index);
   }

   template <typename T>
   inline edges::span bacf<T>::get_span() const
   {
      return get_span(_info.index);
   }

   template <typename T>
   void bacf<T>::reset()
   {
      _edges.reset();
      _count = 0;
   }

   inline void edges::info::update_peak(float s)
   {
      _peak = std::max(s, _peak);
   }

   inline bool edges::operator()(float s, std::size_t index)
   {
      if (s > 0.0f)
      {
         if (!_state)
         {
            CYCFI_ASSERT(_size < _info.size(), "Bad _size");
            _info.push({ { _prev, s }, s, int(index) });
            ++_size;
            _state = 1;
            _predicted_period = 0.0f;
         }
         else
         {
            _info[0].update_peak(s);
         }
      }
      else if (_state && s < _threshold)
      {
         _state = 0;
         _info[0]._trailing_edge = index;
      }
      _prev = s;
      return _state;
   };

   inline bool edges::operator()() const
   {
      return _state;
   }

   inline edges::info const& edges::operator[](std::size_t index) const
   {
      return _info[(_size-1)-index];
   }

   inline void edges::reset()
   {
      _size = 0;
      _state = false;
      _predicted_period = 0.0f;
   }

   inline std::size_t edges::size() const
   {
      return _size;
   }

   inline bool edges::is_full() const
   {
      return _size >= _info.size();
   }

   inline void edges::shift(std::size_t n)
   {
      _info[0]._leading_edge -= n;
      if (!_state)
         _info[0]._trailing_edge -= n;
      auto i = 1;
      for (; i != _size; ++i)
      {
         _info[i]._leading_edge -= n;
         int edge = (_info[i]._trailing_edge -= n);
         if (edge < 0.0f)
            break;
      }
      _size = i;
      _predicted_period = 0.0f;
   }

   inline edges::span edges::get_span(std::size_t period, bool all_edges) const
   {
      float peak = 0.0f;
      std::size_t threshold = period * min_edge_deviation;
      edges::info const* first = nullptr;
      edges::info const* second = nullptr;

      for (int i = _size - 1; i >= 1; --i)
      {
         edges::info const& i_ = _info[i];
         if ((all_edges || !i_._inhibited) && i_._peak > peak)
         {
            for (int j = i - 1; j >= 0; --j)
            {
               edges::info const& j_ = _info[j];
               int span = j_._leading_edge - i_._leading_edge;
               if (std::abs(int(period) - span) <= threshold)
               {
                  first = &i_;
                  second = &j_;
                  peak = std::max(i_._peak, j_._peak);
                  break;
               }
               else if (span > period)
               {
                  break;
               }
            }
         }
      }
      auto r = span{ first, second };

      // If the first attempt fails, try to use all edges
      // if we haven't done so yet
      if (!r && !all_edges)
         return get_span(period, true);
      return r;
   }

   float edges::predict_period() const
   {
      if (_predicted_period != 0.0f)
         return _predicted_period;

      // We need at least two edges
      if (size() < 2)
         return 0.0f;

      // Get the first and second highest peaks
      info const* first_peak = 0;
      info const* second_peak = 0;
      for (int i = _size - 1; i >= 1; --i)
      {
         edges::info const& i_ = _info[i];
         if (!first_peak || i_._peak > first_peak->_peak)
         {
            second_peak = first_peak;
            first_peak = &i_;
         }
         else if (!second_peak || i_._peak > second_peak->_peak)
         {
            second_peak = &i_;
         }
      }

      // We got two peaks
      if (first_peak && second_peak)
      {
         // Arrange the edges
         if (second_peak->_leading_edge < first_peak->_leading_edge)
            std::swap(second_peak, first_peak);
         _predicted_period = span{first_peak, second_peak}.period();
         return _predicted_period;
      }

      return 0.0f;
   }

   float edges::span::period() const
   {
      if (!(*this))
         return 0.0f;

      // Get the start edge
      auto prev1 = first->_crossing.first;
      auto curr1 = first->_crossing.second;
      auto dy1 = curr1 - prev1;
      auto dx1 = -prev1 / dy1;

      // Get the next edge
      auto prev2 = second->_crossing.first;
      auto curr2 = second->_crossing.second;
      auto dy2 = curr2 - prev2;
      auto dx2 = -prev2 / dy2;

      // Calculate the frequency
      auto n_span = second->_leading_edge - first->_leading_edge;
      return n_span + (dx2 - dx1);
   }
}}

#endif
