/*=============================================================================
   Copyright (c) 2019-2026 Joel de Guzman

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
// The geometry behind a draggable ADSR control: where the corners of the
// envelope sit for a set of values, and what a corner dragged to a point
// means. No drawing and no mouse here, so it can be tested.
#define CATCH_CONFIG_MAIN
#include <infra/catch.hpp>
#include <qplug/adsr_shape.hpp>

namespace qplug = cycfi::qplug;
using qplug::adsr_shape;

TEST_CASE("The corners run left to right, in the order they are heard")
{
   adsr_shape s{0.5f, 0.5f, 0.5f, 0.5f};
   CHECK(s.start().x == Approx(0.0f));
   CHECK(s.peak().x > s.start().x);
   CHECK(s.corner().x > s.peak().x);
   CHECK(s.plateau().x > s.corner().x);
   CHECK(s.finish().x > s.plateau().x);
   CHECK(s.finish().x <= 1.0f);
}

TEST_CASE("The envelope starts and ends at nothing, and peaks at one")
{
   adsr_shape s{0.3f, 0.3f, 0.6f, 0.3f};
   CHECK(s.start().y == Approx(0.0f));
   CHECK(s.peak().y == Approx(1.0f));
   CHECK(s.finish().y == Approx(0.0f));
}

TEST_CASE("The sustain is the height of the plateau")
{
   adsr_shape s{0.3f, 0.3f, 0.6f, 0.3f};
   CHECK(s.corner().y == Approx(0.6f));
   CHECK(s.plateau().y == Approx(0.6f));
}

TEST_CASE("A longer attack moves the peak to the right")
{
   adsr_shape slow{0.9f, 0.3f, 0.5f, 0.3f};
   adsr_shape fast{0.1f, 0.3f, 0.5f, 0.3f};
   CHECK(slow.peak().x > fast.peak().x);
}

TEST_CASE("A longer decay widens the fall, not the attack")
{
   adsr_shape a{0.3f, 0.2f, 0.5f, 0.3f};
   adsr_shape b{0.3f, 0.8f, 0.5f, 0.3f};
   CHECK(a.peak().x == Approx(b.peak().x));
   CHECK(b.corner().x - b.peak().x > a.corner().x - a.peak().x);
}

TEST_CASE("A longer release widens the tail")
{
   adsr_shape a{0.3f, 0.3f, 0.5f, 0.2f};
   adsr_shape b{0.3f, 0.3f, 0.5f, 0.8f};
   CHECK(b.finish().x - b.plateau().x > a.finish().x - a.plateau().x);
}

TEST_CASE("Everything at full still fits the drawing")
{
   adsr_shape s{1.0f, 1.0f, 1.0f, 1.0f};
   CHECK(s.finish().x == Approx(1.0f));
   CHECK(s.finish().x <= 1.0f);
}

////////////////////////////////////////////////////////////////////////////
// Dragging: a corner taken to a point, and what that means for the values.
////////////////////////////////////////////////////////////////////////////
TEST_CASE("Dragging the peak sets the attack, and nothing else")
{
   adsr_shape s{0.3f, 0.4f, 0.5f, 0.6f};
   auto moved = s;
   moved.drag(adsr_shape::attack_handle, {0.2f, 0.5f});

   CHECK(moved.attack != Approx(s.attack));
   CHECK(moved.decay == Approx(s.decay));
   CHECK(moved.sustain == Approx(s.sustain));
   CHECK(moved.release == Approx(s.release));
}

TEST_CASE("The peak dragged to where it already is changes nothing")
{
   adsr_shape s{0.3f, 0.4f, 0.5f, 0.6f};
   auto moved = s;
   moved.drag(adsr_shape::attack_handle, s.peak());
   CHECK(moved.attack == Approx(s.attack).margin(1e-4));
}

TEST_CASE("Dragging the corner sets the decay and the sustain together")
{
   // It is one corner, and it carries both: how far the fall reaches and
   // how far down it goes.
   adsr_shape s{0.3f, 0.4f, 0.5f, 0.6f};
   auto moved = s;
   moved.drag(adsr_shape::decay_handle, {s.corner().x + 0.1f, 0.25f});

   CHECK(moved.decay > s.decay);
   CHECK(moved.sustain == Approx(0.25f).margin(0.01));
   CHECK(moved.attack == Approx(s.attack));
   CHECK(moved.release == Approx(s.release));
}

TEST_CASE("Dragging the end sets the release, and nothing else")
{
   adsr_shape s{0.3f, 0.4f, 0.5f, 0.6f};
   auto moved = s;
   moved.drag(adsr_shape::release_handle, {s.finish().x - 0.1f, 0.0f});

   CHECK(moved.release < s.release);
   CHECK(moved.attack == Approx(s.attack));
   CHECK(moved.decay == Approx(s.decay));
   CHECK(moved.sustain == Approx(s.sustain));
}

TEST_CASE("A corner dragged out of the drawing stays in it")
{
   adsr_shape s{0.5f, 0.5f, 0.5f, 0.5f};

   auto low = s;
   low.drag(adsr_shape::attack_handle, {-5.0f, 0.5f});
   CHECK(low.attack == Approx(0.0f));

   auto high = s;
   high.drag(adsr_shape::attack_handle, {5.0f, 0.5f});
   CHECK(high.attack == Approx(1.0f));

   auto deep = s;
   deep.drag(adsr_shape::decay_handle, {s.corner().x, -5.0f});
   CHECK(deep.sustain == Approx(0.0f));

   auto tall = s;
   tall.drag(adsr_shape::decay_handle, {s.corner().x, 5.0f});
   CHECK(tall.sustain == Approx(1.0f));
}

TEST_CASE("A point picks the corner nearest it, and only if it is near")
{
   adsr_shape s{0.3f, 0.4f, 0.5f, 0.6f};
   CHECK(s.nearest(s.peak(), 0.05f) == adsr_shape::attack_handle);
   CHECK(s.nearest(s.corner(), 0.05f) == adsr_shape::decay_handle);
   CHECK(s.nearest(s.finish(), 0.05f) == adsr_shape::release_handle);
   CHECK(s.nearest({0.5f, 0.5f}, 0.01f) == adsr_shape::no_handle);
}

TEST_CASE("Values survive the round trip through the drawing")
{
   for (float v = 0.0f; v <= 1.0f; v += 0.1f)
   {
      adsr_shape s{v, 1.0f - v, v, 1.0f - v};
      auto moved = s;
      moved.drag(adsr_shape::attack_handle, s.peak());
      moved.drag(adsr_shape::decay_handle, s.corner());
      moved.drag(adsr_shape::release_handle, s.finish());

      CHECK(moved.attack == Approx(s.attack).margin(1e-4));
      CHECK(moved.decay == Approx(s.decay).margin(1e-4));
      CHECK(moved.sustain == Approx(s.sustain).margin(1e-4));
      CHECK(moved.release == Approx(s.release).margin(1e-4));
   }
}
