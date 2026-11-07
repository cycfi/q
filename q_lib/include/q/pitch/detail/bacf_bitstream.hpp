/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(CYCFI_Q_DETAIL_BACF_BITSTREAM_NOVEMBER_7_2026)
#define CYCFI_Q_DETAIL_BACF_BITSTREAM_NOVEMBER_7_2026

#include <q/support/base.hpp>
#include <q/utility/bitset.hpp>
#include <q/utility/bitstream_acf.hpp>
#include <q/utility/ring_buffer.hpp>
#include <q/utility/zero_crossing_collector.hpp>
#include <infra/assert.hpp>

#include <algorithm>
#include <cstdint>
#include <cstring>

namespace cycfi::q::detail
{
   ////////////////////////////////////////////////////////////////////////////
   // bacf_pulse: the positive half of a cycle, from its leading edge to its
   // trailing edge. `lead` is its leading edge on the sample clock, `peak`
   // its highest sample, `width` the samples from the lead to the first
   // one under 30% of the peak (or to the trailing edge).
   ////////////////////////////////////////////////////////////////////////////
   struct bacf_pulse
   {
      std::uint32_t        lead;
      float                peak;
      float                width;
   };

   // Two pulses of one waveform: within 40% in height, 15% in width.
   bool                    similar(bacf_pulse const& a, bacf_pulse const& b);

   ////////////////////////////////////////////////////////////////////////////
   // bacf_list: the latest `size()` entries of a ring, newest first.
   ////////////////////////////////////////////////////////////////////////////
   template <typename T>
   struct bacf_list
   {
      std::size_t          size() const         { return _size; }
      T const&             operator[](std::size_t i) const { return _ring[i]; }

      ring_buffer<T> const& _ring;
      std::size_t          _size;
   };

   ////////////////////////////////////////////////////////////////////////////
   // bacf_bitstream: the front end of the BACF follower. A comparator
   // with hysteresis turns the signal into one bit a sample, kept as a
   // history of the latest `history` bits; its edges (leading: rising over
   // the hysteresis, trailing: falling under its negative) and its pulses
   // are kept in rings, newest first. The sample clock, now(), counts the
   // samples since construction and wraps.
   //
   // Each sample, operator() reports an edge to `on_edge` at that sample:
   // the pulse it ends already in pulses(), the edge itself not yet in
   // edges(), the sample's bit not yet in the history. mismatch() is the
   // bitstream autocorrelation the follower scores with: the latest `w` bits
   // against the `w` bits `lag` earlier, the end aligned down to a word.
   // Allocates at construction only.
   ////////////////////////////////////////////////////////////////////////////
   class bacf_bitstream
   {
   public:

      enum edge_kind { leading, trailing };
      using edge_list = bacf_list<std::uint32_t>;
      using pulse_list = bacf_list<bacf_pulse>;
      using word = bitset<>::value_type;

      static constexpr std::size_t  word_size = bitset<>::value_size;
      static constexpr float        width_level = 0.3f;  // of the peak

                           bacf_bitstream(
                              float hysteresis
                            , std::size_t history
                            , std::size_t capacity
                           );

                           template <typename F>
      void                 operator()(float s, F&& on_edge);

      std::uint32_t        now() const          { return _now; }
      std::size_t          history() const      { return _held; }
      bool                 has(std::uint32_t lag, std::uint32_t w) const;
      std::uint32_t        mismatch(std::uint32_t lag, std::uint32_t w) const;
      edge_list            edges(edge_kind k) const;
      pulse_list           pulses() const;
      void                 reset();

   private:

      using edge_ring = ring_buffer<std::uint32_t>;
      using pulse_ring = ring_buffer<bacf_pulse>;

      void                 compact();

      float                _hysteresis;
      std::size_t          _keep;         // whole words
      std::size_t          _capacity;
      bitset<>             _bits;
      edge_ring            _lead;
      edge_ring            _trail;
      pulse_ring           _pulses;
      std::size_t          _num_lead = 0;
      std::size_t          _num_trail = 0;
      std::size_t          _num_pulses = 0;
      std::size_t          _pos = 0;      // the next bit
      std::size_t          _held = 0;     // bits held, saturating at _keep
      std::uint32_t        _now = 0;
      bool                 _state = false;
      bacf_pulse           _pulse{};      // the pulse in progress
   };

   ////////////////////////////////////////////////////////////////////////////
   // Inlines
   ////////////////////////////////////////////////////////////////////////////
   inline bool similar(bacf_pulse const& a, bacf_pulse const& b)
   {
      return rel_within(a.peak, b.peak, 0.4f)
         && rel_within(a.width, b.width
          , 1.0f - zero_crossing_collector::pulse_width_diff);
   }

   inline bacf_bitstream::bacf_bitstream(
      float hysteresis
    , std::size_t history
    , std::size_t capacity
   )
    : _hysteresis{hysteresis}
    , _keep{(history + word_size - 1) / word_size * word_size}
    , _capacity{capacity}
    , _bits{2 * _keep + word_size}
    , _lead(capacity)
    , _trail(capacity)
    , _pulses(capacity)
   {}

   template <typename F>
   inline void bacf_bitstream::operator()(float s, F&& on_edge)
   {
      if (_pos == 2 * _keep)
         compact();

      if (!_state && s > _hysteresis)
      {
         _state = true;
         _pulse = {_now, s, 0.0f};
         on_edge(leading);
         _lead.push(_now);
         _num_lead = std::min(_num_lead + 1, _capacity);
      }
      else if (_state && s < -_hysteresis)
      {
         _state = false;
         if (_pulse.width == 0.0f)
            _pulse.width = float(_now - _pulse.lead);
         _pulses.push(_pulse);
         _num_pulses = std::min(_num_pulses + 1, _capacity);
         on_edge(trailing);
         _trail.push(_now);
         _num_trail = std::min(_num_trail + 1, _capacity);
      }

      if (_state)
      {
         _pulse.peak = std::max(_pulse.peak, s);
         if (_pulse.width == 0.0f && s < _pulse.peak * width_level)
            _pulse.width = float(_now - _pulse.lead);
      }

      _bits.set(_pos++, _state);
      ++_now;
      _held = std::min(_held + 1, _keep);
   }

   // Enough history for the latest `w` bits against those `lag` earlier.
   inline bool bacf_bitstream::has(
      std::uint32_t lag, std::uint32_t w) const
   {
      return _held >= w + lag + word_size;
   }

   // `w` is whole words; the bits must reach back `w + lag`.
   inline std::uint32_t bacf_bitstream::mismatch(
      std::uint32_t lag, std::uint32_t w) const
   {
      CYCFI_ASSERT(w + lag <= _pos, "bacf_bitstream: not enough history");
      auto const start = (_pos - w - lag) / word_size;
      bitstream_acf<> const acf{_bits};
      return std::uint32_t(acf(lag, start, w / word_size));
   }

   inline bacf_bitstream::edge_list
   bacf_bitstream::edges(edge_kind k) const
   {
      return k == leading
         ? edge_list{_lead, _num_lead}
         : edge_list{_trail, _num_trail};
   }

   inline bacf_bitstream::pulse_list bacf_bitstream::pulses() const
   {
      return {_pulses, _num_pulses};
   }

   // Empties the history, the edges and the pulses; the clock runs on.
   inline void bacf_bitstream::reset()
   {
      _bits.clear();
      _num_lead = _num_trail = _num_pulses = 0;
      _pos = 0;
      _held = 0;
      _state = false;
   }

   // Slide the bits: keep the latest _keep, moved to the start.
   inline void bacf_bitstream::compact()
   {
      auto* p = _bits.data();
      auto const words = _keep / word_size;
      std::memmove(p, p + words, words * sizeof(word));
      std::fill(p + words, p + _bits.size() / word_size, word{0});
      _pos = _keep;
   }
}

#endif
