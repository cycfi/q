/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(CYCFI_Q_FORMANT_LEARNER_NOVEMBER_2_2026)
#define CYCFI_Q_FORMANT_LEARNER_NOVEMBER_2_2026

#include <q/fft/lpc_fit.hpp>
#include <q/fft/spectral_learner.hpp>
#include <q/fx/formant_corrector.hpp>

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <span>
#include <vector>

namespace cycfi::q
{
   ////////////////////////////////////////////////////////////////////////////
   // formant_learner: a spectral_learner that fits the formant_table
   // formant_corrector uses: an instrument's fixed spectral envelope, and
   // the same spectral envelope as a pitch shift leaves it. It runs off the
   // audio thread (a worker, or an MCU's low-priority loop) and allocates
   // only at construction.
   //
   // fit() fills a table from what has been learned: for every grid ratio,
   // the spectral envelope as the shift leaves it, fitted with lpc_fit
   // (see note 1). The model's order is given at construction, and fit()
   // takes tables of that order. Response is lpc_fit's: how |A(f)|^2 is
   // computed.
   ////////////////////////////////////////////////////////////////////////////
   template <typename Response = detail::default_lpc_response>
   class basic_formant_learner : public spectral_learner
   {
   public:

      using table_type = formant_table;
      using response_type = Response;

      explicit             basic_formant_learner(
                              float sps
                            , std::size_t order = 32
                           );

      bool                 fit(table_type& table) const;
      std::size_t          order() const;

      static constexpr float        boost_cap_db = 9.0f;    // See note 2.

   private:

      using spectrum_type = std::vector<float>;

      void                 shift(float ratio) const;
      double               level(float ratio, double err) const;

      // Scratch, for fit().
      mutable basic_lpc_fit<Response> _fit;
      mutable spectrum_type _env;
      mutable spectrum_type _shifted;
      mutable spectrum_type _a1;          // |A(f)|^2 of F
      mutable std::vector<float> _k1;      // F's reflection coefficients
      mutable double       _err1 = 1;     // F's prediction error
   };

   using formant_learner = basic_formant_learner<>;

   ////////////////////////////////////////////////////////////////////////////
   // Notes
   //
   // 1. Each grid row is fitted to E(f/r), held at its top value past
   //    Nyquist (going down, the shift leaves nothing there), with the
   //    boost capped (note 2). Its level is scaled so the correction keeps
   //    the learned material's average power, as the shift leaves it.
   //
   // 2. Going down, a full restoration lifts a band the shift has emptied
   //    by 30 dB or more, and the noise there with it. The correction
   //    F / F(f/r) boosts at most this much; cuts are not capped.
   ////////////////////////////////////////////////////////////////////////////

   ////////////////////////////////////////////////////////////////////////////
   // Inlines
   ////////////////////////////////////////////////////////////////////////////
   namespace detail
   {
      // Linear interpolation of v at a fractional index, held at its ends.
      inline float at(std::span<float const> v, float i)
      {
         if (i <= 0)
            return v.front();
         auto const last = float(v.size() - 1);
         if (i >= last)
            return v.back();
         auto const j = std::size_t(i);
         auto const t = i - float(j);
         return v[j] + t * (v[j + 1] - v[j]);
      }
   }

   template <typename Response>
   inline basic_formant_learner<Response>::basic_formant_learner(
      float sps, std::size_t order)
    : spectral_learner{sps}
    , _fit{bins(), order}
    , _env(bins())
    , _shifted(bins())
    , _a1(bins())
    , _k1(order)
   {
   }

   template <typename Response>
   inline std::size_t basic_formant_learner<Response>::order() const
   {
      return _fit.order();
   }

   // The spectral envelope as a shift by `ratio` leaves it: E(f/r), held at
   // its top past Nyquist and no more than the boost cap under E (note 1).
   template <typename Response>
   inline void basic_formant_learner<Response>::shift(float ratio) const
   {
      for (std::size_t b = 0; b != _env.size(); ++b)
      {
         auto const from = float(b) / ratio;
         _shifted[b] = std::max(
            detail::at(_env, from), _env[b] - boost_cap_db);
      }
   }

   // The row's level: the correction's power gain on the learned spectrum
   // as the shift leaves it (empty past Nyquist), undone. The row's model
   // is the last fit.
   template <typename Response>
   inline double basic_formant_learner<Response>::level(float ratio, double err) const
   {
      auto const mean = spectrum();
      auto const last = float(mean.size() - 1);
      auto const gain2 = _err1 / err;
      double in = 0, out = 0;
      for (std::size_t b = 0; b != mean.size(); ++b)
      {
         auto const from = float(b) / ratio;
         if (from > last)
            continue;
         auto const moved = double(detail::at(mean, from));
         in += moved;
         out += moved * gain2 * _fit.response(b)
            / std::max(_a1[b], 1e-20f);
      }
      auto const g = out > 0 ? std::sqrt(in / out) : 1.0;
      return std::sqrt(err) / g;
   }

   template <typename Response>
   inline bool basic_formant_learner<Response>::fit(table_type& table) const
   {
      assert(table.order() == order());
      if (frames() == 0)
         return false;

      spectral_envelope(_env);
      _err1 = _fit(_env, _k1);
      _fit.response(_a1);

      for (std::size_t i = 0; i != table_type::num_rows; ++i)
      {
         auto const st = float(table_type::lo)
            + float(i) / float(table_type::steps);
         auto const ratio = std::exp2(st / 12.0f);
         shift(ratio);
         auto const err = _fit(_shifted, table.k(i));
         table.level(i) = float(level(ratio, err));
      }
      return true;
   }
}

#endif
