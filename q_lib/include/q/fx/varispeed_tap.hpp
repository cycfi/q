/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(CYCFI_Q_VARISPEED_TAP_OCTOBER_31_2026)
#define CYCFI_Q_VARISPEED_TAP_OCTOBER_31_2026

#include <q/synth/gen/hann_gen.hpp>
#include <q/utility/interpolation.hpp>
#include <q/utility/ring_buffer.hpp>

#include <algorithm>
#include <cstddef>
#include <vector>

namespace cycfi::q
{
   ////////////////////////////////////////////////////////////////////////////
   // basic_varispeed_tap: a delay line read at a varying speed, the way
   // tape replays faster or slower. Each call pushes a sample, reads at the
   // delay, then moves the read `ratio` samples along, so the delay changes
   // by 1 - ratio a call: below 1 the read falls behind (pitch down), above
   // 1 it catches up (pitch up), and the output's pitch is the input's
   // times the ratio. The delay stays within 0 .. max_delay; held at either
   // end, the pitch is the input's.
   //
   // At delay 0 the output is the input itself, so a tap that starts there
   // continues the dry signal without a seam. Reads use `Interpolation`, a
   // sample_interpolation policy, linear below a delay of 1, where a
   // 4-point policy has no newer sample to read. varispeed_tap uses
   // Hermite.
   //
   // The delay line is `Buffer`, a mirrored_ring_buffer or any type with
   // its interface, built with max_delay + 4 elements and a mirror of 3,
   // so the 4 samples a read takes are contiguous and the memory is what
   // the delay needs.
   //
   // splice(to, fade) jumps the read to another delay, crossfading the two
   // reads over `fade` samples with constant-amplitude Hann ramps, both
   // moving at the ratio: a splice for a time-domain pitch shifter, the
   // jump a period, so the two reads are alike. The material behind the
   // read is there to match: operator[] reads at any delay, interpolated
   // for a float, the sample itself for a whole number.
   ////////////////////////////////////////////////////////////////////////////
   template <
      typename Interpolation
    , typename Buffer = mirrored_ring_buffer<float>
   >
   class basic_varispeed_tap
   {
   public:

      using interpolation_type = Interpolation;
      using buffer_type = Buffer;

      explicit             basic_varispeed_tap(std::size_t max_delay);

      float                operator()(float s, float ratio);

      float                delay() const;
      void                 delay(float d);
      std::size_t          max_delay() const;
      void                 reset();

      void                 splice(float to, std::size_t fade);
      bool                 splicing() const;
      float                target() const;
      float                operator[](float delay) const;
      float                operator[](std::size_t delay) const;

   private:

      float                read(float delay) const;

      // The policy reads ages first .. first + 3 through this.
      struct window
      {
         float const*      data;
         std::size_t       first;
         float operator[](std::size_t age) const { return data[age - first]; }
      };

      Buffer               _buffer;
      std::size_t          _max_delay;
      float                _delay = 0.0f;
      float                _to = 0.0f;
      std::size_t          _fade = 0;
      basic_hann_upward_ramp_gen<float> _ramp{duration{1.0}, 1.0f};
   };

   using varispeed_tap = basic_varispeed_tap<sample_interpolation::hermite>;

   ////////////////////////////////////////////////////////////////////////////
   // Inlines
   ////////////////////////////////////////////////////////////////////////////
   template <typename Interpolation, typename Buffer>
   inline basic_varispeed_tap<Interpolation, Buffer>::basic_varispeed_tap(
      std::size_t max_delay)
    : _buffer(max_delay + 4, 3)
    , _max_delay{max_delay}
   {}

   template <typename Interpolation, typename Buffer>
   inline float basic_varispeed_tap<Interpolation, Buffer>::operator()(
      float s, float ratio)
   {
      _buffer.push(s);
      auto const top = float(_max_delay);
      auto const step = 1.0f - ratio;
      if (_fade == 0)
      {
         auto const y = read(_delay);
         _delay = std::clamp(_delay + step, 0.0f, top);
         return y;
      }

      float const g = _ramp();
      float const y = (1.0f - g) * read(_delay) + g * read(_to);
      _delay = std::clamp(_delay + step, 0.0f, top);
      _to = std::clamp(_to + step, 0.0f, top);
      if (--_fade == 0)
         _delay = _to;
      return y;
   }

   template <typename Interpolation, typename Buffer>
   inline float basic_varispeed_tap<Interpolation, Buffer>::read(
      float delay) const
   {
      if (delay < 1.0f)
      {
         auto const p = _buffer.span(0);
         return p[0] + delay * (p[1] - p[0]);
      }
      auto const first = std::size_t(delay) - 1;
      return Interpolation{}(window{_buffer.span(first), first}, delay);
   }

   template <typename Interpolation, typename Buffer>
   inline float basic_varispeed_tap<Interpolation, Buffer>::delay() const
   {
      return _delay;
   }

   // Setting the delay ends a splice in flight: the read is there now.
   template <typename Interpolation, typename Buffer>
   inline void basic_varispeed_tap<Interpolation, Buffer>::delay(float d)
   {
      _delay = std::clamp(d, 0.0f, float(_max_delay));
      _fade = 0;
   }

   template <typename Interpolation, typename Buffer>
   inline std::size_t
   basic_varispeed_tap<Interpolation, Buffer>::max_delay() const
   {
      return _max_delay;
   }

   template <typename Interpolation, typename Buffer>
   inline void basic_varispeed_tap<Interpolation, Buffer>::reset()
   {
      _buffer.clear();
      _delay = 0.0f;
      _fade = 0;
   }

   // A splice in flight is finished at once: the read is where it was
   // going, and the new fade starts from there.
   template <typename Interpolation, typename Buffer>
   inline void basic_varispeed_tap<Interpolation, Buffer>::splice(
      float to, std::size_t fade)
   {
      if (_fade)
         _delay = _to;
      _to = std::clamp(to, 0.0f, float(_max_delay));
      _fade = std::max<std::size_t>(fade, 1);
      _ramp.config(duration{double(_fade)}, 1.0f, true);
   }

   template <typename Interpolation, typename Buffer>
   inline bool basic_varispeed_tap<Interpolation, Buffer>::splicing() const
   {
      return _fade != 0;
   }

   // Where a splice in flight lands, or the delay when there is none.
   template <typename Interpolation, typename Buffer>
   inline float basic_varispeed_tap<Interpolation, Buffer>::target() const
   {
      return _fade ? _to : _delay;
   }

   template <typename Interpolation, typename Buffer>
   inline float basic_varispeed_tap<Interpolation, Buffer>::operator[](
      float delay) const
   {
      return read(std::clamp(delay, 0.0f, float(_max_delay)));
   }

   template <typename Interpolation, typename Buffer>
   inline float basic_varispeed_tap<Interpolation, Buffer>::operator[](
      std::size_t delay) const
   {
      return _buffer[std::min(delay, _max_delay)];
   }
}

#endif
