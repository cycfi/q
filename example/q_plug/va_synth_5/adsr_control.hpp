/*=============================================================================
   Copyright (c) 2019-2026 Joel de Guzman

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(QPLUG_VA_SYNTH_5_ADSR_CONTROL_SEPTEMBER_11_2026)
#define QPLUG_VA_SYNTH_5_ADSR_CONTROL_SEPTEMBER_11_2026

#include <elements.hpp>
#include <functional>
#include <memory>

namespace elements = cycfi::elements;

///////////////////////////////////////////////////////////////////////////////
// An envelope drawn as its own shape, with its corners dragged to set it.
//
// This is what a custom Elements control looks like: derive from tracker,
// which follows the mouse between a press and a release, and override the
// four members that matter. draw puts the shape on the canvas; click picks
// up the corner under the cursor; keep_tracking moves it; cursor lights up
// the corner the mouse is over.
//
// A slider carries one value, with one setter and one on_change, and that
// is what the binder links to a model. This carries four, so it has a
// setter and an on_change per value, named, and a gesture callback each
// that brackets a drag, so the host records one movement rather than a
// hundred. attack_of and the rest hand each value out as a bindable_proxy,
// a control of its own to the binder, so the presenter binds an envelope
// exactly as it binds a slider, four times.
//
// The four values are the control's travel, 0 to 1, not seconds: a
// parameter's own taper turns them into those.
///////////////////////////////////////////////////////////////////////////////
class adsr_control : public elements::tracker<>
{
public:

   using point = elements::point;
   using change_function = std::function<void(float)>;
   using gesture_function = std::function<void(bool begin)>;

   void              attack_value(float v)   { _attack = v; }
   void              decay_value(float v)    { _decay = v; }
   void              sustain_value(float v)  { _sustain = v; }
   void              release_value(float v)  { _release = v; }

   float             attack_value() const    { return _attack; }
   float             decay_value() const     { return _decay; }
   float             sustain_value() const   { return _sustain; }
   float             release_value() const   { return _release; }

   change_function   on_attack_change;
   change_function   on_decay_change;
   change_function   on_sustain_change;
   change_function   on_release_change;

   gesture_function  on_attack_gesture;
   gesture_function  on_decay_gesture;
   gesture_function  on_sustain_gesture;
   gesture_function  on_release_gesture;

   elements::view_limits
                     limits(elements::basic_context const& ctx) const override;
   void              draw(elements::context const& ctx) override;
   bool              click(
                        elements::context const& ctx
                      , elements::mouse_button btn) override;
   void              keep_tracking(
                        elements::context const& ctx
                      , tracker_info& track) override;
   bool              cursor(
                        elements::context const& ctx, point p
                      , elements::cursor_tracking status) override;
   bool              wants_control() const override { return true; }

private:

   // The shape, in a unit square with y upward.
   //
   //          peak
   //           /\__ corner ____ plateau
   //          /                        \
   //         /                          \
   //     start                          finish
   //
   // Attack, decay and release each own a share of the width, so a long
   // attack pushes the peak right without changing the fall after it,
   // and everything at full still fits. The plateau is fixed: a sustain
   // has no length, it lasts as long as the key is down.
   struct unit_point
   {
      float x = 0.0f;
      float y = 0.0f;
   };

   enum handle
   {
      no_handle = -1
    , attack_handle           // the peak
    , decay_handle            // the corner: how far it falls, and to where
    , release_handle          // the end of the tail
   };

   static constexpr float stage_width = 0.28f;
   static constexpr float plateau_width = 0.16f;

   unit_point        start() const;
   unit_point        peak() const;
   unit_point        corner() const;
   unit_point        plateau() const;
   unit_point        finish() const;

   // The corner nearest a point, if one is within reach; the corner
   // taken to a point, which sets the values it is made of.
   handle            nearest(unit_point p, float reach) const;
   void              take(handle which, unit_point to);

   // Between the unit square and the bounds the shape is drawn in.
   point             to_screen(unit_point p, elements::rect b) const;
   unit_point        from_screen(point p, elements::rect b) const;

   // The area the shape is drawn in, inset so a corner at the edge is
   // still drawn whole, and the corner nearest a point in it.
   elements::rect    drawing(elements::context const& ctx) const;
   handle            pick(elements::context const& ctx, point p) const;

   // Move one corner, and say what changed; bracket a drag on one.
   void              move(handle which, unit_point to);
   void              gesture(handle which, bool begin);

   float             _attack = 0.2f;
   float             _decay = 0.3f;
   float             _sustain = 0.6f;      // a level, not a time
   float             _release = 0.3f;

   handle            _dragging = no_handle;
   handle            _hot = no_handle;
};

///////////////////////////////////////////////////////////////////////////////
// Each value of the control as a control of its own, for the binder.
///////////////////////////////////////////////////////////////////////////////
inline auto attack_of(std::shared_ptr<adsr_control> c)
{
   return elements::make_bindable_proxy(c, &adsr_control::attack_value
    , c->on_attack_change, &c->on_attack_gesture);
}

inline auto decay_of(std::shared_ptr<adsr_control> c)
{
   return elements::make_bindable_proxy(c, &adsr_control::decay_value
    , c->on_decay_change, &c->on_decay_gesture);
}

inline auto sustain_of(std::shared_ptr<adsr_control> c)
{
   return elements::make_bindable_proxy(c, &adsr_control::sustain_value
    , c->on_sustain_change, &c->on_sustain_gesture);
}

inline auto release_of(std::shared_ptr<adsr_control> c)
{
   return elements::make_bindable_proxy(c, &adsr_control::release_value
    , c->on_release_change, &c->on_release_gesture);
}

#endif
