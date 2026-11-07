/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(CYCFI_Q_DETAIL_BACF_PROVISIONAL_NOVEMBER_7_2026)
#define CYCFI_Q_DETAIL_BACF_PROVISIONAL_NOVEMBER_7_2026

#include <q/pitch/detail/bacf_slice.hpp>

#include <cmath>
#include <cstdint>

namespace cycfi::q::detail
{
   ////////////////////////////////////////////////////////////////////////////
   // bacf_provisional: a note's first estimate of its lag, before any
   // lock: the latest pulse, if strong (60% of the peak since the drop),
   // to the nearest like pulse after the drop. It stands at once, unless
   // the slice's hint is long (over `long_hint` samples: a chord's), when
   // it stands only if the hint is it or a clean multiple of it (2 to 4,
   // within 2%), or once it repeats over one lag of the slice (85%).
   //
   // estimate() runs at an unlocked edge; vet(), at the end of each sample
   // while an estimate waits, takes the one-lag periodicity at a lag.
   ////////////////////////////////////////////////////////////////////////////
   class bacf_provisional
   {
   public:

      static constexpr float        strong      = 0.6f;   // of the peak
      static constexpr int          fractions   = 4;
      static constexpr float        agree       = 0.02f;
      static constexpr float        repeat      = 0.85f;  // periodicity

                           bacf_provisional(
                              std::uint32_t min_lag
                            , std::uint32_t max_lag
                            , float long_hint
                           );

                           template <typename Pulses>
      void                 estimate(
                              Pulses const& pulses, bacf_slice const& slice
                           );

                           template <typename Periodicity>
      void                 vet(
                              bacf_slice const& slice, std::uint32_t now
                            , Periodicity const& periodicity
                           );

      bool                 estimated() const    { return _estimate != 0; }
      bool                 pending() const;
      float                value() const;
      void                 drop();
      void                 reset()              { drop(); }

   private:

      std::uint32_t        _min_lag;
      std::uint32_t        _max_lag;
      float                _long_hint;
      std::uint32_t        _estimate = 0;
      bool                 _vetted = false;
   };

   ////////////////////////////////////////////////////////////////////////////
   // Inlines
   ////////////////////////////////////////////////////////////////////////////
   inline bacf_provisional::bacf_provisional(
      std::uint32_t min_lag
    , std::uint32_t max_lag
    , float long_hint
   )
    : _min_lag{min_lag}
    , _max_lag{max_lag}
    , _long_hint{long_hint}
   {}

   // An estimate that does not stand yet.
   inline bool bacf_provisional::pending() const
   {
      return _estimate && !_vetted;
   }

   // The estimate in samples, once it stands; 0 until then.
   inline float bacf_provisional::value() const
   {
      return _vetted ? float(_estimate) : 0.0f;
   }

   inline void bacf_provisional::drop()
   {
      _estimate = 0;
      _vetted = false;
   }

   template <typename Pulses>
   inline void bacf_provisional::estimate(
      Pulses const& pulses, bacf_slice const& slice)
   {
      auto const& now = pulses[0];
      if (!slice.after(now.lead) || now.peak < strong * slice.peak())
         return;

      for (std::size_t j = 1; j != pulses.size(); ++j)
      {
         auto const& p = pulses[j];
         if (!slice.after(p.lead))
            break;
         auto const d = now.lead - p.lead;
         if (d > _max_lag)
            break;
         if (d >= _min_lag && similar(p, now))
         {
            _estimate = d;
            return;
         }
      }
   }

   template <typename Periodicity>
   inline void bacf_provisional::vet(
      bacf_slice const& slice, std::uint32_t now
    , Periodicity const& periodicity)
   {
      auto const hint = slice.hint();
      if (hint <= _long_hint)
      {
         _vetted = true;
         return;
      }

      for (int k = 1; k <= fractions; ++k)
      {
         if (std::abs(float(_estimate) * float(k) / hint - 1.0f) <= agree)
         {
            _vetted = true;
            return;
         }
      }

      if (slice.ready(_estimate, now))
         _vetted = periodicity(_estimate) >= repeat;
   }
}

#endif
