/*=============================================================================
   Copyright (c) 2019-2026 Joel de Guzman

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
// Elements' bindable_proxy: one value of a control that carries several,
// as a control of its own. Tested here because Elements has no test
// suite of its own and this is where it is first needed.
#define CATCH_CONFIG_MAIN
#include <infra/catch.hpp>
#include <elements/model.hpp>
#include <elements/element/element.hpp>

#include <qplug/plugin.hpp>

#include <functional>
#include <memory>

using namespace cycfi::elements;

// Linking qplug for its host layer brings in the plugin, which expects
// these of an implementation. No plugin is ever made here.
namespace cycfi::qplug
{
   controller_ptr make_controller() { return nullptr; }
   processor_ptr make_processor(controller&) { return nullptr; }
   presenter_ptr make_presenter(controller&) { return nullptr; }
   plugin_info const& info() { static plugin_info const i{}; return i; }
}

namespace
{
   // A control with two values: the shape of the thing the proxy is for.
   // The callbacks are per value, as the proxy expects them.
   struct two_values : element
   {
      void  low(float v)   { _low = v; }
      float low() const    { return _low; }
      void  high(float v)  { _high = v; }
      float high() const   { return _high; }

      std::function<void(float)>  on_low_change;
      std::function<void(bool)>   on_low_gesture;
      std::function<void(float)>  on_high_change;

      float _low = 0.0f;
      float _high = 0.0f;
   };

   auto low_of(std::shared_ptr<two_values> c)
   {
      return make_bindable_proxy(
         c, &two_values::low, c->on_low_change, &c->on_low_gesture);
   }

   // No gesture in this one: it may be left out.
   auto high_of(std::shared_ptr<two_values> c)
   {
      return make_bindable_proxy(c, &two_values::high, c->on_high_change);
   }

}

TEST_CASE("A value set on the proxy reaches the control")
{
   auto control = std::make_shared<two_values>();
   auto low = low_of(control);

   low->value(0.25f);
   CHECK(control->low() == 0.25f);
   CHECK(control->high() == 0.0f);
}

TEST_CASE("The proxy's on_change is the control's own")
{
   // A forwarder, not a copy: assigning through the proxy lands on the
   // control, which is what the binder does when it links it.
   auto control = std::make_shared<two_values>();
   auto low = low_of(control);

   CHECK(&low->on_change == &control->on_low_change);
   float got = -1.0f;
   low->on_change = [&](float v) { got = v; };
   control->on_low_change(0.4f);
   CHECK(got == 0.4f);
}

TEST_CASE("A gesture given is reachable, one left out is null")
{
   auto control = std::make_shared<two_values>();
   auto low = low_of(control);
   auto high = high_of(control);

   REQUIRE(low->on_gesture != nullptr);
   CHECK(low->on_gesture == &control->on_low_gesture);
   CHECK(high->on_gesture == nullptr);

   int begins = 0, ends = 0;
   *low->on_gesture = [&](bool begin) { (begin? begins : ends)++; };
   control->on_low_gesture(true);
   control->on_low_gesture(false);
   CHECK(begins == 1);
   CHECK(ends == 1);
}

TEST_CASE("The proxy names the control as what to refresh")
{
   auto control = std::make_shared<two_values>();
   auto low = low_of(control);

   CHECK(low->refresh_target() == control.get());
}

TEST_CASE("A proxy outliving its control sets nothing and names nothing")
{
   auto control = std::make_shared<two_values>();
   auto low = low_of(control);
   control.reset();

   CHECK_NOTHROW(low->value(0.5f));
   CHECK(low->refresh_target() == nullptr);
}

TEST_CASE("Bound, the model drives the control and refreshes it once")
{
   auto control = std::make_shared<two_values>();
   auto low = low_of(control);

   int refreshed = 0;
   element* what = nullptr;
   model_binder binder{[&](element& e) { ++refreshed; what = &e; }};

   value_model<float> model = 0.2f;
   binder.bind(model, low);
   CHECK(control->low() == 0.2f);

   model = 0.6f;
   CHECK(control->low() == 0.6f);
   CHECK(refreshed == 1);
   CHECK(what == control.get());
}

TEST_CASE("Bound, what the user does to the control reaches the model")
{
   auto control = std::make_shared<two_values>();
   auto low = low_of(control);
   auto high = high_of(control);

   model_binder binder{[](element&) {}};
   value_model<float> low_model = 0.0f;
   value_model<float> high_model = 0.0f;
   binder.bind(low_model, low);
   binder.bind(high_model, high);

   // What the control would do as its low corner is dragged.
   control->on_low_change(0.3f);
   CHECK(low_model.get() == 0.3f);
   CHECK(high_model.get() == 0.0f);
}

TEST_CASE("Two proxies on one control are two controls to the binder")
{
   auto control = std::make_shared<two_values>();
   auto low = low_of(control);
   auto high = high_of(control);

   model_binder binder{[](element&) {}};
   value_model<float> low_model = 0.1f;
   value_model<float> high_model = 0.9f;
   binder.bind(low_model, low);
   binder.bind(high_model, high);

   CHECK(control->low() == 0.1f);
   CHECK(control->high() == 0.9f);

   low_model = 0.2f;
   CHECK(control->low() == 0.2f);
   CHECK(control->high() == 0.9f);
}
