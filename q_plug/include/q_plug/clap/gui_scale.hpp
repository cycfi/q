/*=============================================================================
   Copyright (c) 2019-2026 Joel de Guzman

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(QPLUG_CLAP_GUI_SCALE_HPP_SEPTEMBER_12_2026)
#define QPLUG_CLAP_GUI_SCALE_HPP_SEPTEMBER_12_2026

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace cycfi::qplug
{
   ////////////////////////////////////////////////////////////////////////////
   // Sizes across the CLAP boundary.
   //
   // A plugin thinks in logical units; the host speaks the window API's own
   // (clap/ext/gui.h): logical points for Cocoa, physical pixels for Win32.
   // `scale` is physical pixels per logical unit: 1 where the API is
   // logical, the display's DPI over 96 on Windows.
   //
   // To the host a size is rounded to the nearest whole pixel, and a limit
   // Elements gives as unbounded is held to what a host's integers can
   // take. From the host a size is divided back out; the presenter takes it
   // within a pixel of its limits.
   ////////////////////////////////////////////////////////////////////////////
   constexpr float host_size_max = 1 << 15;

   inline std::uint32_t to_host(float logical, float scale)
   {
      auto const physical = logical * scale;
      if (!(physical < host_size_max))    // infinity and NaN too
         return std::uint32_t(host_size_max);
      return std::uint32_t(std::lround(std::max(physical, 0.0f)));
   }

   inline float from_host(std::uint32_t physical, float scale)
   {
      return physical / scale;
   }
}

#endif
