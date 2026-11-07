/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(CYCFI_Q_DETAIL_BACF_NOTE_NOVEMBER_7_2026)
#define CYCFI_Q_DETAIL_BACF_NOTE_NOVEMBER_7_2026

#include <q/pitch/detail/bacf_bitstream.hpp>

#include <algorithm>
#include <cstdint>

namespace cycfi::q::detail
{
   ////////////////////////////////////////////////////////////////////////////
   // bacf_note: the note since the last drop, on the sample clock. It keeps
   // the drop's time, its hint (the last good lag, 0 for none) and the
   // strongest pulse since, and decides the window a score runs over: one
   // lag of the note (at least `min_window`, whole words) until the full
   // window, a lag and a word are all after the drop, so a score never
   // mixes the note with what came before it.
   //
   // Before any drop the note is long ago (half the clock's wrap): scores
   // run over the full window and no pulse is after it.
   ////////////////////////////////////////////////////////////////////////////
   class bacf_note
   {
   public:

      static constexpr std::uint32_t never = 0x80000000u;

                           bacf_note(
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
      std::uint32_t        _drop = never;
      float                _hint = 0.0f;
      float                _peak = 0.0f;
   };

   ////////////////////////////////////////////////////////////////////////////
   // Inlines
   ////////////////////////////////////////////////////////////////////////////
   inline bacf_note::bacf_note(
      std::uint32_t window
    , std::uint32_t min_window
   )
    : _window{window}
    , _min_window{min_window}
   {}

   inline void bacf_note::drop(std::uint32_t now, float hint)
   {
      _drop = now;
      _hint = hint;
      _peak = 0.0f;
   }

   inline void bacf_note::reset()
   {
      _drop = never;
      _hint = 0.0f;
      _peak = 0.0f;
   }

   inline void bacf_note::observe(bacf_pulse const& p)
   {
      if (after(p.lead))
         _peak = std::max(_peak, p.peak);
   }

   // `t` at or after the drop.
   inline bool bacf_note::after(std::uint32_t t) const
   {
      return std::int32_t(t - _drop) >= 0;
   }

   inline std::uint32_t bacf_note::one_lag(std::uint32_t lag) const
   {
      auto const m = std::max(lag, _min_window);
      return (m + ws - 1) / ws * ws;
   }

   inline std::uint32_t bacf_note::window(
      std::uint32_t lag, std::uint32_t now) const
   {
      auto const since = now - _drop;
      if (since < lag + ws)
         return one_lag(lag);
      auto const clear = (since - lag - ws) / ws * ws;
      return clear >= _window ? _window : one_lag(lag);
   }

   // One lag of the note, a lag back and a word more, all after the drop.
   inline bool bacf_note::ready(std::uint32_t lag, std::uint32_t now) const
   {
      return now - _drop >= one_lag(lag) + lag + ws;
   }
}

#endif
