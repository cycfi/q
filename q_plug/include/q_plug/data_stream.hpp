/*=============================================================================
   Copyright (c) 2026 Joel de Guzman

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(QPLUG_DATA_STREAM_HPP_SEPTEMBER_6_2026)
#define QPLUG_DATA_STREAM_HPP_SEPTEMBER_6_2026

#include <cstdint>

namespace cycfi::qplug
{
   ////////////////////////////////////////////////////////////////////////////
   // Byte streams for state save and load
   ////////////////////////////////////////////////////////////////////////////
   struct ostream
   {
      virtual                 ~ostream() = default;
      virtual std::int64_t    write(void const* data, std::int64_t size) = 0;
   };

   struct istream
   {
      virtual                 ~istream() = default;
      virtual std::int64_t    read(void* data, std::int64_t size) = 0;
   };
}

#endif
