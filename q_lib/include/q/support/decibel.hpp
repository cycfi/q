/*=============================================================================
   Copyright (c) 2014-2023 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(CYCFI_Q_DECIBEL_HPP_FEBRUARY_21_2018)
#define CYCFI_Q_DECIBEL_HPP_FEBRUARY_21_2018

#include <cmath>
#include <q/detail/db_table.hpp>
#include <q/support/unit.hpp>
#include <q/support/concepts.hpp>

namespace cycfi::q
{
   ////////////////////////////////////////////////////////////////////////////
   // decibel is a highly optimized class for dealing with decibels. The
   // class provides fast conversion from linear to decibel and back. The
   // decibel value is non-linear. It operates on the logarithmic domain. The
   // decibel class is perfectly suitable for dynamics processing (e.g.
   // compressors and limiters and envelopes). Q provides fast decibel
   // computations using lookup tables and fast math computations for
   // converting to and from scalars.
   ////////////////////////////////////////////////////////////////////////////
   struct decibel : unit<double, decibel>
   {
      using base_type = unit<double, decibel>;
      using base_type::base_type;
   };

   // Free functions
   double               lin_double(decibel db);
   constexpr float      lin_float(decibel db);
   inline decibel       approx_db(float val);
   decibel              lin_to_db(double val);

   ////////////////////////////////////////////////////////////////////////////
   // Inlines
   ////////////////////////////////////////////////////////////////////////////
   inline double lin_double(decibel db)
   {
      return std::pow(10, db.rep/20);
   }

   constexpr float lin_float(decibel db)
   {
      return detail::db2a(db.rep);
   }

   inline decibel approx_db(float val)
   {
      return decibel{20.0f * faster_log10(val)};
   }

   inline decibel lin_to_db(double val)
   {
      return decibel{20.0f * fast_log10(val)};
   }
}

#endif
