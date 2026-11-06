/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(CYCFI_Q_FORMANT_CORRECTOR_NOVEMBER_2_2026)
#define CYCFI_Q_FORMANT_CORRECTOR_NOVEMBER_2_2026

#include <q/fx/eq_matcher.hpp>

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace cycfi::q
{
   ////////////////////////////////////////////////////////////////////////////
   // formant_table: an instrument's fixed spectral envelope F (its pickup,
   // electronics and body) as an all-pole model, and the same spectral
   // envelope as a pitch shift by a ratio r leaves it, F(f/r), fitted on a
   // grid of ratios: `steps` a semitone from `lo` to `hi` semitones. Each
   // row is the model's reflection coefficients, as many as its order, and
   // its level. The unshifted row is F itself. formant_learner fills it
   // (set); formant_corrector reads it (row, at).
   //
   // The coefficients are held in 16 bits (Q15; a reflection coefficient
   // is under 1 in magnitude, and a lattice takes the rounding well): about
   // 13 KB at order 32. The order is given at construction, the only place
   // it allocates: 32 by default; 24 on a smaller core, a quarter cheaper
   // for the corrector and still close.
   ////////////////////////////////////////////////////////////////////////////
   class formant_table
   {
   public:

      static constexpr int          lo = -24;
      static constexpr int          hi = 24;
      static constexpr int          steps = 4;
      static constexpr std::size_t  num_rows = (hi - lo) * steps + 1;

      explicit                formant_table(std::size_t order = 32);

      std::size_t             order() const;

      void                    row(
                                 std::size_t i
                               , std::span<float> k
                               , float& level
                              ) const;
      void                    set(
                                 std::size_t i
                               , std::span<float const> k
                               , float level
                              );
      float                   level(std::size_t i) const;
      std::size_t             unshifted() const;
      void                    at(
                                 float ratio
                               , std::span<float> k
                               , float& level
                              ) const;

   private:

      static constexpr float  q15 = 32767.0f;
      static constexpr float  from_q15 = 1.0f / q15;

      std::size_t             _order;
      std::vector<std::int16_t> _k;       // num_rows rows of _order, Q15
      std::vector<float>      _level;
   };

   ////////////////////////////////////////////////////////////////////////////
   // formant_corrector: restores the fixed spectral envelope a pitch
   // shifter moved. After a shift by a ratio r the spectral envelope is
   // F(f/r); the corrector applies F(f) / F(f/r): an eq_matcher from the
   // moved spectral envelope, its source, to F, its target. The source
   // follows the ratio every `block` samples, interpolated between the
   // table's rows. Its tables are of the order given at construction, the
   // only place it allocates.
   //
   // amount blends from 0, no correction (the plain shift), to 1, the full
   // one. With no table, at a ratio of 1, or at an amount of 0, the output
   // is the input itself and the filters rest, at no cost. Near a ratio of
   // 1 the correction is nearly flat, so starting them again from rest
   // makes no seam. The table must outlive its use.
   ////////////////////////////////////////////////////////////////////////////
   class formant_corrector
   {
   public:

      using table_type = formant_table;

      explicit             formant_corrector(std::size_t order = 32);

      float                operator()(float s, float ratio);

      void                 table(table_type const* table);
      void                 amount(float a);
      float                amount() const;
      std::size_t          order() const;
      void                 reset();

   private:

      static constexpr std::size_t block = 32;

      void                 update(float ratio);

      table_type const*    _table = nullptr;
      eq_matcher           _match;
      std::vector<float>   _kr;     // the row at the ratio, scratch
      bool                 _through = true;
      std::size_t          _count = 0;
   };

   ////////////////////////////////////////////////////////////////////////////
   // Inlines
   ////////////////////////////////////////////////////////////////////////////
   inline formant_table::formant_table(std::size_t order)
    : _order{order}
    , _k(num_rows * order, 0)
    , _level(num_rows, 1.0f)
   {
      assert(order > 0);
   }

   inline std::size_t formant_table::order() const
   {
      return _order;
   }

   inline void formant_table::row(
      std::size_t i, std::span<float> k, float& level) const
   {
      assert(i < num_rows && k.size() == _order);
      auto const* r = _k.data() + i * _order;
      for (std::size_t m = 0; m != _order; ++m)
         k[m] = float(r[m]) * from_q15;
      level = _level[i];
   }

   inline void formant_table::set(
      std::size_t i, std::span<float const> k, float level)
   {
      assert(i < num_rows && k.size() == _order);
      auto* r = _k.data() + i * _order;
      for (std::size_t m = 0; m != _order; ++m)
         r[m] = std::int16_t(std::lround(std::clamp(k[m], -1.0f, 1.0f) * q15));
      _level[i] = level;
   }

   inline float formant_table::level(std::size_t i) const
   {
      return _level[i];
   }

   inline std::size_t formant_table::unshifted() const
   {
      return std::size_t(-lo * steps);
   }

   // The row at a ratio, interpolated linearly between the two grid rows
   // around it; ratios beyond the grid take its ends.
   inline void formant_table::at(
      float ratio, std::span<float> k, float& level) const
   {
      assert(k.size() == _order);
      auto const st = std::clamp(
         12.0f * std::log2(ratio), float(lo), float(hi));
      auto const u = (st - float(lo)) * float(steps);
      auto const i = std::min(std::size_t(u), num_rows - 2);
      auto const t = u - float(i);

      auto const* a = _k.data() + i * _order;
      auto const* b = a + _order;
      for (std::size_t m = 0; m != _order; ++m)
      {
         auto const ka = float(a[m]) * from_q15;
         auto const kb = float(b[m]) * from_q15;
         k[m] = ka + t * (kb - ka);
      }
      level = _level[i] + t * (_level[i + 1] - _level[i]);
   }

   inline formant_corrector::formant_corrector(std::size_t order)
    : _match{order}
    , _kr(order, 0.0f)
   {
   }

   inline float formant_corrector::operator()(float s, float ratio)
   {
      if (_count++ % block == 0)
         update(ratio);

      if (_through)
         return s;
      return _match(s);
   }

   inline void formant_corrector::table(table_type const* table)
   {
      assert(!table || table->order() == order());
      _table = table;
      if (table)
      {
         float level;
         table->row(table->unshifted(), _kr, level);
         _match.target(_kr, level);
      }
      _count = 0;
   }

   inline void formant_corrector::amount(float a)
   {
      _match.amount(a);
      _count = 0;
   }

   inline float formant_corrector::amount() const
   {
      return _match.amount();
   }

   inline std::size_t formant_corrector::order() const
   {
      return _match.order();
   }

   inline void formant_corrector::reset()
   {
      _match.reset();
      _count = 0;
   }

   // At a ratio of 1, with no table, or at an amount of 0, the input
   // passes through and the matcher rests.
   inline void formant_corrector::update(float ratio)
   {
      auto const through =
         !_table || ratio == 1.0f || _match.amount() == 0.0f;
      if (through)
      {
         if (!_through)
            _match.reset();
         _through = true;
         return;
      }
      _through = false;

      float level;
      _table->at(ratio, _kr, level);
      _match.source(_kr, level);
   }
}

#endif
