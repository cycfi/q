/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(CYCFI_Q_DETAIL_BACF_SLICE_NOVEMBER_7_2026)
#define CYCFI_Q_DETAIL_BACF_SLICE_NOVEMBER_7_2026

#include <q/pitch/detail/bacf_bitstream.hpp>

#include <algorithm>
#include <cstdint>

namespace cycfi::q::detail
{
   ////////////////////////////////////////////////////////////////////////////
   // bacf_slice: the signal since the last drop, on the sample clock. It
   // keeps the drop's time, its hint (the last good lag, 0 for none) and the
   // strongest pulse since, and decides the window a score runs over: one
   // lag of the slice (at least `min_window`, whole words) until the full
   // window, a lag and a word are all after the drop, so a score never
   // mixes the slice with what came before it.
   //
   // Before any drop, and after reset(), there is no slice yet: scores run
   // over the full window and no pulse is after it, at any clock.
   ////////////////////////////////////////////////////////////////////////////
   class bacf_slice
   {
   public:

                           bacf_slice(
                              std::uint32_t window
                            , std::uint32_t min_window
                           );

      void                 drop(std::uint32_t now, float hint);
      void                 reset();
      void                 observe(bacf_pulse const& p);

      bool                 after(std::uint32_t t) const;
      float                hint() const         { return _hint; }
      float                peak() const         { return _peak; }
      std::uint32_t        one_lag(std::uint32_t lag) const;
      std::uint32_t        window(std::uint32_t lag, std::uint32_t now) const;
      bool                 ready(std::uint32_t lag, std::uint32_t now) const;

   private:

      static constexpr std::uint32_t ws = bacf_bitstream::word_size;

      std::uint32_t        _window;       // bits, whole words
      std::uint32_t        _min_window;
      std::uint32_t        _drop = 0;
      bool                 _dropped = false;
      float                _hint = 0.0f;
      float                _peak = 0.0f;
   };

   ////////////////////////////////////////////////////////////////////////////
   // Inlines
   ////////////////////////////////////////////////////////////////////////////
   inline bacf_slice::bacf_slice(
      std::uint32_t window
    , std::uint32_t min_window
   )
    : _window{window}
    , _min_window{min_window}
   {}

   inline void bacf_slice::drop(std::uint32_t now, float hint)
   {
      _drop = now;
      _dropped = true;
      _hint = hint;
      _peak = 0.0f;
   }

   inline void bacf_slice::reset()
   {
      _drop = 0;
      _dropped = false;
      _hint = 0.0f;
      _peak = 0.0f;
   }

   inline void bacf_slice::observe(bacf_pulse const& p)
   {
      if (after(p.lead))
         _peak = std::max(_peak, p.peak);
   }

   // `t` at or after the drop; false with no drop yet.
   inline bool bacf_slice::after(std::uint32_t t) const
   {
      return _dropped && std::int32_t(t - _drop) >= 0;
   }

   inline std::uint32_t bacf_slice::one_lag(std::uint32_t lag) const
   {
      auto const m = std::max(lag, _min_window);
      return (m + ws - 1) / ws * ws;
   }

   inline std::uint32_t bacf_slice::window(
      std::uint32_t lag, std::uint32_t now) const
   {
      if (!_dropped)
         return _window;
      auto const since = now - _drop;
      if (since < lag + ws)
         return one_lag(lag);
      auto const clear = (since - lag - ws) / ws * ws;
      return clear >= _window ? _window : one_lag(lag);
   }

   // One lag of the slice, a lag back and a word more, all after the drop.
   inline bool bacf_slice::ready(std::uint32_t lag, std::uint32_t now) const
   {
      return !_dropped || now - _drop >= one_lag(lag) + lag + ws;
   }
}

#endif
