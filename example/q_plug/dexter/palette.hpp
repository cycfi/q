/*=============================================================================
   Copyright (c) 2019-2026 Joel de Guzman

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(Q_PLUG_DEXTER_PALETTE_OCTOBER_1_2026)
#define Q_PLUG_DEXTER_PALETTE_OCTOBER_1_2026

#include <elements.hpp>

///////////////////////////////////////////////////////////////////////////////
// The colors the drawn controls share: Anna's blue, and the panel's.
///////////////////////////////////////////////////////////////////////////////
namespace palette
{
   using cycfi::elements::rgba;

   constexpr auto background = rgba(35, 35, 37, 255);
   constexpr auto line = rgba(100, 180, 230, 255);
   constexpr auto fill = rgba(100, 180, 230, 40);
   constexpr auto grid = rgba(80, 80, 84, 255);

   // A modulator's envelope, and a carrier's box in the chart
   constexpr auto muted = rgba(154, 164, 184, 255);
   constexpr auto muted_fill = rgba(154, 164, 184, 30);
   constexpr auto carrier = rgba(18, 49, 95, 255);
   constexpr auto selected = rgba(61, 155, 255, 255);

   // A button that is on: a saturated blue, which Elements draws a
   // little translucent
   constexpr auto button_on = rgba(30, 144, 255, 255);
}

#endif
