/*=============================================================================
   Copyright (c) 2019-2026 Joel de Guzman

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
// Binding a button to a parameter. A button is the one control neither of
// the binder's own paths reaches, so bind wires it itself, through
// bind_button. Driven here with a binder of its own: what the presenter
// hands it is a view's binder, which a unit test has no window for.
#define CATCH_CONFIG_MAIN
#include <infra/catch.hpp>

#include <qplug/plugin.hpp>
#include <qplug/presenter.hpp>
#include <elements.hpp>

#include <memory>
#include <string>
#include <vector>

using namespace cycfi;
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
   // What bind asks of a control to tell the paths apart. Named here
   // because a requires expression on a concrete type is checked where it
   // is written, and one that fails is an error rather than a false.
   template <typename T>
   concept clickable = requires (T& c) { c.on_click; };

   template <typename T>
   concept changeable = requires (T& c) { c.on_change; };

   // A button, as far as the binder is concerned: a value it can set and
   // a click it can hook. Elements' own buttons are the same two members.
   struct fake_button : element
   {
      void  value(bool v)  { _value = v; }
      bool  value() const  { return _value; }

      std::function<void(bool)>  on_click;
      bool _value = false;
   };

   // Every call the wiring makes on the controller, in order, so a test
   // can check that a click is one edit and that it is bracketed.
   struct edit_log : qplug::edit_sink
   {
      void begin_edit(int index) override
      {
         calls.push_back("begin " + std::to_string(index));
      }

      void edit_parameter(int index, double value) override
      {
         calls.push_back("edit " + std::to_string(index)
          + " " + std::to_string(value));
      }

      void end_edit(int index) override
      {
         calls.push_back("end " + std::to_string(index));
      }

      std::vector<std::string> calls;
   };

   // Power is plain on and off. Shape is an enumeration of four, to bind
   // a button to something whose travel is not the value.
   char const* const shapes[] = {"Sine", "Triangle", "Saw", "Square", nullptr};

   struct test_controller : qplug::controller
   {
      enum : int { power_id, shape_id, num_params };

      test_controller(qplug::edit_sink& sink) { init(sink); }

      parameter_list parameters() const override
      {
         static qplug::parameter const params[] =
         {
            qplug::parameter{1, "Power", true},
            qplug::parameter{2, "Shape", 0, shapes}
         };
         return {params, params + num_params};
      }
   };

   struct fixture
   {
      edit_log                      log;
      test_controller               ctl{log};
      model_binder                  bindings{[](element&) {}};
      std::shared_ptr<fake_button>  button = std::make_shared<fake_button>();

      void bind(int index)
      {
         qplug::detail::bind_button(bindings, ctl, index, button
          , ctl.parameters()[index]);
      }
   };
}

TEST_CASE("Bound, the button starts where the parameter stands")
{
   // Power's init is on, so a fresh controller binds to a button already
   // on, and one turned off before the editor opened binds to one off.
   {
      fixture f;
      f.bind(test_controller::power_id);
      CHECK(f.button->value() == true);
   }
   {
      fixture f;
      f.ctl.set_parameter(test_controller::power_id, 0.0);
      f.ctl.update_models();

      f.bind(test_controller::power_id);
      CHECK(f.button->value() == false);
   }
}

TEST_CASE("The model turns the button on and off")
{
   fixture f;
   f.bind(test_controller::power_id);

   f.ctl.model(test_controller::power_id) = 1.0;
   CHECK(f.button->value() == true);

   f.ctl.model(test_controller::power_id) = 0.0;
   CHECK(f.button->value() == false);
}

TEST_CASE("A click is one edit, bracketed")
{
   fixture f;
   f.bind(test_controller::power_id);
   f.log.calls.clear();

   f.button->on_click(true);

   CHECK(f.log.calls == std::vector<std::string>{
      "begin 0", "edit 0 1.000000", "end 0"});
}

TEST_CASE("Clicked off, the edit carries the off value")
{
   fixture f;
   f.bind(test_controller::power_id);
   f.log.calls.clear();

   f.button->on_click(false);

   CHECK(f.log.calls == std::vector<std::string>{
      "begin 0", "edit 0 0.000000", "end 0"});
   CHECK(f.ctl.get_parameter(test_controller::power_id) == 0.0);
}

TEST_CASE("The parameter maps the travel, both ways")
{
   // Shape runs 0 to 3, so the halves of the travel are 1.5 and either
   // end of it: on is the last shape, and anything past the middle shows
   // as on.
   fixture f;
   f.bind(test_controller::shape_id);
   f.log.calls.clear();

   f.button->on_click(true);
   CHECK(f.log.calls == std::vector<std::string>{
      "begin 1", "edit 1 3.000000", "end 1"});
   CHECK(f.button->value() == true);

   f.ctl.model(test_controller::shape_id) = 1.0;
   CHECK(f.button->value() == false);

   f.ctl.model(test_controller::shape_id) = 2.0;
   CHECK(f.button->value() == true);
}

TEST_CASE("A real Elements toggle is wired the same")
{
   edit_log log;
   test_controller ctl{log};
   model_binder bindings{[](element&) {}};

   auto toggle = share(toggle_icon_button(icons::power, 1.2f));
   qplug::detail::bind_button(bindings, ctl, test_controller::power_id
    , toggle, ctl.parameters()[test_controller::power_id]);

   ctl.model(test_controller::power_id) = 1.0;
   CHECK(toggle->value() == true);

   log.calls.clear();
   toggle->on_click(false);       // what the toggle does on mouse up
   CHECK(log.calls == std::vector<std::string>{
      "begin 0", "edit 0 0.000000", "end 0"});
}

TEST_CASE("Only a button takes the button path")
{
   // bind picks the path on on_click. A tracker carries on_change and is
   // followed instead; if one ever grew an on_click, it would silently
   // change hands.
   using toggle_type = decltype(toggle_icon_button(icons::power, 1.2f));
   using slider_type =
      decltype(slider(basic_thumb<25>(), basic_track<5, true>()));

   static_assert(clickable<toggle_type>);
   static_assert(!clickable<slider_type>);
   static_assert(changeable<slider_type>);
}
