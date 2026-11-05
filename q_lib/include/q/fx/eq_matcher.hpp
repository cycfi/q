/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(CYCFI_Q_EQ_MATCHER_NOVEMBER_2_2026)
#define CYCFI_Q_EQ_MATCHER_NOVEMBER_2_2026

#include <q/fx/lattice.hpp>

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <span>
#include <vector>

namespace cycfi::q
{
   ////////////////////////////////////////////////////////////////////////////
   // eq_matcher: gives a sound with the spectral envelope A the spectral
   // envelope B instead, applying B(f) / A(f). Both are all-pole models
   // (lpc_fit): A's reflection coefficients and level, the source, and
   // B's, the target. A lattice_fir with A flattens the source's spectral
   // envelope, and a lattice_iir with B colours it, scaled by the ratio of
   // their levels. No latency.
   //
   // amount blends from 0, no change, to 1, the full match: the flattening
   // filter's coefficients from B's to A's, and the level ratio with them,
   // in dB. At an amount of 0 the output is the input itself and the
   // filters rest, at no cost; starting them again from rest makes no seam
   // while the match is nearly flat. The source and the target may change
   // at any sample: a lattice with every coefficient under 1 in magnitude
   // stays stable. Their order is given at construction, the only place it
   // allocates.
   ////////////////////////////////////////////////////////////////////////////
   class eq_matcher
   {
   public:

      explicit             eq_matcher(std::size_t order = 32);

      float                operator()(float s);

      void                 source(std::span<float const> k, float level);
      void                 target(std::span<float const> k, float level);
      void                 amount(float a);
      float                amount() const;
      std::size_t          order() const;
      void                 reset();

   private:

      void                 update();

      lattice_fir          _flatten;
      lattice_iir          _colour;
      std::vector<float>   _ka;     // the source's
      std::vector<float>   _kb;     // the target's, the colouring filter's
      std::vector<float>   _k;      // the flattening filter's
      float                _level_a = 1.0f;
      float                _level_b = 1.0f;
      float                _amount = 1.0f;
      float                _gain = 1.0f;
   };

   ////////////////////////////////////////////////////////////////////////////
   // Inlines
   ////////////////////////////////////////////////////////////////////////////
   inline eq_matcher::eq_matcher(std::size_t order)
    : _flatten{order}
    , _colour{order}
    , _ka(order, 0.0f)
    , _kb(order, 0.0f)
    , _k(order, 0.0f)
   {
   }

   inline float eq_matcher::operator()(float s)
   {
      if (_amount == 0.0f)
         return s;
      return _gain * _colour(_flatten(s, _k), _kb);
   }

   inline void eq_matcher::source(std::span<float const> k, float level)
   {
      assert(k.size() == _ka.size());
      std::copy(k.begin(), k.end(), _ka.begin());
      _level_a = level;
      update();
   }

   inline void eq_matcher::target(std::span<float const> k, float level)
   {
      assert(k.size() == _kb.size());
      std::copy(k.begin(), k.end(), _kb.begin());
      _level_b = level;
      update();
   }

   inline void eq_matcher::amount(float a)
   {
      a = std::clamp(a, 0.0f, 1.0f);
      if (a == 0.0f && _amount != 0.0f)
         reset();
      _amount = a;
      update();
   }

   inline float eq_matcher::amount() const
   {
      return _amount;
   }

   inline std::size_t eq_matcher::order() const
   {
      return _k.size();
   }

   inline void eq_matcher::reset()
   {
      _flatten.reset();
      _colour.reset();
   }

   inline void eq_matcher::update()
   {
      for (std::size_t m = 0; m != _k.size(); ++m)
         _k[m] = _kb[m] + _amount * (_ka[m] - _kb[m]);
      _gain = std::pow(_level_b / _level_a, _amount);
   }
}

#endif
