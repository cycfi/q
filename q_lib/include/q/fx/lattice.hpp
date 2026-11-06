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
      auto const* kp = k.data();
      float f = s;
      float older = bs[0];
      bs[0] = s;

      // Four stages at a time, each group's coefficients and states read
      // before any is written: the same operations in the same order as
      // one stage at a time, with nothing for the compiler to reload.
      std::size_t m = 1;
      for (; m + 3 < n; m += 4)
      {
         auto const k0 = kp[m - 1], k1 = kp[m], k2 = kp[m + 1], k3 = kp[m + 2];
         auto const b0 = older;
         auto const b1 = bs[m], b2 = bs[m + 1], b3 = bs[m + 2];
         older = bs[m + 3];
         auto const f1 = f + k0 * b0;
         auto const f2 = f1 + k1 * b1;
         auto const f3 = f2 + k2 * b2;
         auto const f4 = f3 + k3 * b3;
         bs[m] = b0 + k0 * f;
         bs[m + 1] = b1 + k1 * f1;
         bs[m + 2] = b2 + k2 * f2;
         bs[m + 3] = b3 + k3 * f3;
         f = f4;
      }
      for (; m < n; ++m)
      {
         auto const b = older;            // b[m-1], one sample earlier
         older = bs[m];
         auto const next = f + kp[m - 1] * b;
         bs[m] = b + kp[m - 1] * f;
         f = next;
      }
      return f + kp[n - 1] * older;       // the last stage keeps no b
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
      auto const* kp = k.data();
      float f = s - kp[n - 1] * bs[n - 1]; // the last stage keeps no b

      // Four stages at a time, as in lattice_fir.
      std::size_t m = n - 1;
      for (; m >= 4; m -= 4)
      {
         auto const k0 = kp[m - 1], k1 = kp[m - 2], k2 = kp[m - 3], k3 = kp[m - 4];
         auto const b0 = bs[m - 1], b1 = bs[m - 2], b2 = bs[m - 3], b3 = bs[m - 4];
         auto const f0 = f - k0 * b0;
         auto const f1 = f0 - k1 * b1;
         auto const f2 = f1 - k2 * b2;
         auto const f3 = f2 - k3 * b3;
         bs[m] = b0 + k0 * f0;
         bs[m - 1] = b1 + k1 * f1;
         bs[m - 2] = b2 + k2 * f2;
         bs[m - 3] = b3 + k3 * f3;
         f = f3;
      }
      for (; m >= 1; --m)
      {
         f -= kp[m - 1] * bs[m - 1];
         bs[m] = bs[m - 1] + kp[m - 1] * f;
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
