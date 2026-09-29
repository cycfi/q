/*=============================================================================
   Copyright (c) 2019-2026 Joel de Guzman

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
// Sizes across the CLAP boundary: logical units on the plugin's side, the
// window API's own on the host's, which on Windows are physical pixels.
#define CATCH_CONFIG_MAIN
#include <infra/catch.hpp>
#include <qplug/clap/gui_scale.hpp>

#include <limits>

using namespace cycfi::qplug;

TEST_CASE("At a scale of one a whole size crosses unchanged")
{
   CHECK(to_host(920.0f, 1.0f) == 920);
   CHECK(to_host(568.0f, 1.0f) == 568);
   CHECK(from_host(920, 1.0f) == 920.0f);
}

TEST_CASE("At 125% the host is given physical pixels")
{
   // A display at 120 DPI, 120 over 96: an editor of 920 by 568 needs a
   // window of 1150 by 710 pixels to show all of it.
   CHECK(to_host(920.0f, 1.25f) == 1150);
   CHECK(to_host(568.0f, 1.25f) == 710);
   CHECK(from_host(1150, 1.25f) == 920.0f);
   CHECK(from_host(710, 1.25f) == 568.0f);
}

TEST_CASE("A whole size comes back from the host as it went")
{
   for (float scale : {1.0f, 1.25f, 1.5f, 1.75f, 2.0f})
      for (float size : {100.0f, 568.0f, 920.0f})
         CHECK(from_host(to_host(size, scale), scale) == size);
}

TEST_CASE("A size between pixels goes to the nearest one")
{
   // A zoomed editor is rarely whole. At 150%, 100.3 and 100.4 are 150.45
   // and 150.6 pixels: one rounds down, the other up.
   CHECK(to_host(100.3f, 1.5f) == 150);
   CHECK(to_host(100.4f, 1.5f) == 151);
}

TEST_CASE("An unbounded limit is held to what a host can take")
{
   auto const huge = std::numeric_limits<float>::max();
   CHECK(to_host(huge, 1.0f) == 32768);
   CHECK(to_host(huge, 1.25f) == 32768);   // overflows to infinity first
   CHECK(to_host(1e30f, 2.0f) == 32768);
}
