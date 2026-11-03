/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(CYCFI_Q_DECIBEL_HPP_FEBRUARY_21_2018)
#define CYCFI_Q_DECIBEL_HPP_FEBRUARY_21_2018

#include <cmath>
#include <type_traits>
#include <q/detail/db_table.hpp>
#include <q/support/unit.hpp>
#include <q/support/basic_concepts.hpp>

namespace cycfi::q
{
   ////////////////////////////////////////////////////////////////////////////
   // decibel is a highly optimized class for dealing with decibels. The
   // class provides fast conversion from linear to decibel and back. The
   // decibel value is non-linear. It operates on the logarithmic domain. The
   // decibel class is perfectly suitable for dynamics processing (e.g.
   // compressors and limiters and envelopes). Q provides fast decibel
   // computations using lookup tables for converting to and from scalars
   // (see detail/db_table.hpp), exact to about 1e-5 dB over the whole
   // float range, and a coarser bit-trick approximation, approx_db.
   //
   // T is the value's type. decibel is double; basic_decibel<float> keeps a
   // level in float for a single-precision FPU. The two mix as one unit, and
   // either converts to the other. dB, lin_to_db and approx_db make a
   // decibel unless given the type, as in lin_to_db<float>(s).
   ////////////////////////////////////////////////////////////////////////////
   struct decibel_unit;

   template <typename T>
   struct basic_decibel : unit<T, basic_decibel<T>>
   {
      using base_type = unit<T, basic_decibel<T>>;
      using base_type::base_type;
      using unit_type = decibel_unit;

      template <typename U>
      constexpr basic_decibel(basic_decibel<U> db)
       : base_type(T(db.rep), direct_unit)
      {}

      // Please use dB(s) or lin_to_db(s) instead of decibel(s).
      //
      basic_decibel(T) = delete;
      //
      // This constructor was used in previous versions of the library to
      // convert linear to decibels, which can be confusing. This version
      // ought to have corrected this nonintuitive semantics, but this is a
      // disruptive change that will alter the semantics of all existing code
      // without warning. In order to avoid further confusion, we will mark
      // this constructor as deleted for the time being, making it a hard
      // error to alert users upgrading to this library version.
   };

   using decibel = basic_decibel<double>;

   // Free functions
   template <typename T>
   double               lin_double(basic_decibel<T> db);

   template <typename T>
   constexpr float      lin_float(basic_decibel<T> db);

   template <typename T = double>
   constexpr basic_decibel<T> dB(std::type_identity_t<T> val);

   template <typename T = double>
   basic_decibel<T>     approx_db(float val);

   template <typename T = double>
   basic_decibel<T>     lin_to_db(float val);

   ////////////////////////////////////////////////////////////////////////////
   // Inlines
   ////////////////////////////////////////////////////////////////////////////
   template <typename T>
   inline double lin_double(basic_decibel<T> db)
   {
      return std::pow(10, db.rep/20);
   }

   template <typename T>
   constexpr float lin_float(basic_decibel<T> db)
   {
      return detail::db2a(db.rep);
   }

   template <typename T>
   constexpr basic_decibel<T> dB(std::type_identity_t<T> val)
   {
      return basic_decibel<T>{val, direct_unit};
   }

   template <typename T>
   inline basic_decibel<T> approx_db(float val)
   {
      return basic_decibel<T>{20.0f * faster_log10(val), direct_unit};
   }

   template <typename T>
   inline basic_decibel<T> lin_to_db(float val)
   {
      return basic_decibel<T>{detail::a2db(val), direct_unit};
   }
}

#endif
