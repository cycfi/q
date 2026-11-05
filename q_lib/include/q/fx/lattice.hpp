/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(CYCFI_Q_LATTICE_NOVEMBER_2_2026)
#define CYCFI_Q_LATTICE_NOVEMBER_2_2026

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <span>
#include <vector>

namespace cycfi::q
{
   ////////////////////////////////////////////////////////////////////////////
   // lattice_fir and lattice_iir: an all-pole model's two filters, in
   // lattice form, given its reflection coefficients k (levinson), as many
   // as the order given at construction, the only place either allocates.
   // lattice_fir is A(z), the inverse filter that flattens (whitens) a
   // signal with the model's spectral envelope; lattice_iir is 1/A(z), the
   // filter that colours a flat signal with it. With the same k at the same
   // sample, one undoes the other.
   //
   // The k may change at any sample: a lattice with every |k| < 1 stays
   // stable, so coefficients can be interpolated from one set to another.
   // Stage m:
   //
   //    f[m] = f[m-1] + k[m] b[m-1] (one sample earlier)
   //    b[m] = b[m-1] (one sample earlier) + k[m] f[m-1]
   ////////////////////////////////////////////////////////////////////////////
   class lattice_fir
   {
   public:

      explicit             lattice_fir(std::size_t order);

      float                operator()(float s, std::span<float const> k);
      std::size_t          order() const;
      void                 reset();

   private:

      std::vector<float>   _b;      // b[0] .. b[N-1], one sample earlier
   };

   class lattice_iir
   {
   public:

      explicit             lattice_iir(std::size_t order);

      float                operator()(float s, std::span<float const> k);
      std::size_t          order() const;
      void                 reset();

   private:

      std::vector<float>   _b;      // b[0] .. b[N-1], one sample earlier
   };

   ////////////////////////////////////////////////////////////////////////////
   // Inlines
   ////////////////////////////////////////////////////////////////////////////
   inline lattice_fir::lattice_fir(std::size_t order)
    : _b(order, 0.0f)
   {
      assert(order > 0);
   }

   inline float lattice_fir::operator()(float s, std::span<float const> k)
   {
      auto const n = _b.size();
      assert(k.size() == n);
      auto* bs = _b.data();
      float f = s;
      float older = bs[0];
      bs[0] = s;
      for (std::size_t m = 1; m <= n; ++m)
      {
         auto const b = older;            // b[m-1], one sample earlier
         if (m < n)
            older = bs[m];
         auto const next = f + k[m - 1] * b;
         if (m < n)
            bs[m] = b + k[m - 1] * f;
         f = next;
      }
      return f;
   }

   inline std::size_t lattice_fir::order() const
   {
      return _b.size();
   }

   inline void lattice_fir::reset()
   {
      std::fill(_b.begin(), _b.end(), 0.0f);
   }

   inline lattice_iir::lattice_iir(std::size_t order)
    : _b(order, 0.0f)
   {
      assert(order > 0);
   }

   // The stages run backwards: f[m-1] = f[m] - k[m] b[m-1].
   inline float lattice_iir::operator()(float s, std::span<float const> k)
   {
      auto const n = _b.size();
      assert(k.size() == n);
      auto* bs = _b.data();
      float f = s;
      for (std::size_t m = n; m >= 1; --m)
      {
         f -= k[m - 1] * bs[m - 1];
         if (m < n)
            bs[m] = bs[m - 1] + k[m - 1] * f;
      }
      bs[0] = f;
      return f;
   }

   inline std::size_t lattice_iir::order() const
   {
      return _b.size();
   }

   inline void lattice_iir::reset()
   {
      std::fill(_b.begin(), _b.end(), 0.0f);
   }
}

#endif
