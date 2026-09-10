/*=============================================================================
   Copyright (c) 2019-2026 Joel de Guzman

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(QPLUG_VA_SYNTH_4_ADSR_CONTROL_SEPTEMBER_11_2026)
#define QPLUG_VA_SYNTH_4_ADSR_CONTROL_SEPTEMBER_11_2026

#include <elements.hpp>
#include <qplug/adsr_shape.hpp>
#include <functional>

namespace elements = cycfi::elements;
namespace qplug = cycfi::qplug;

///////////////////////////////////////////////////////////////////////////////
// An envelope drawn as its own shape, with its corners dragged to set it.
//
// This is what a custom Elements control looks like: derive from tracker,
// which follows the mouse between a press and a release, and override the
// four members that matter. draw puts the shape on the canvas; click picks
// up the corner under the cursor; keep_tracking moves it; cursor lights up
// the corner the mouse is over.
//
// The geometry is qplug::adsr_shape, which knows nothing of drawing or of
// the mouse, so it can be tested on its own. What is left here is the part
// that has to be seen to be judged.
//
// A slider carries one value, so the framework can bind it to one
// parameter. This carries four, so it reports which of them moved and the
// presenter sends that one on. The gesture callback brackets a drag, so
// the host records it as one movement rather than a hundred.
///////////////////////////////////////////////////////////////////////////////
class adsr_control : public elements::tracker<>
{
public:

   using point = elements::point;
   using shape = qplug::adsr_shape;

   // Which value moved, and where it now stands. Called while dragging.
   using on_change_f = std::function<void(shape::handle, float)>;

   // A drag began or ended on a value: the host brackets its automation
   // with these.
   using on_gesture_f = std::function<void(shape::handle, bool begin)>;

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

   shape             values;
   on_change_f       on_change;
   on_gesture_f      on_gesture;

private:

   // The unit square the shape lives in, and the bounds it is drawn in.
   point             to_screen(shape::point p, elements::rect b) const;
   shape::point      from_screen(point p, elements::rect b) const;

   // The area the shape is drawn in, inset so a corner at the edge is
   // still drawn whole, and the corner nearest a point in it.
   elements::rect    drawing(elements::context const& ctx) const;
   shape::handle     pick(elements::context const& ctx, point p) const;

   // Move one corner, and say what changed.
   void              move(shape::handle which, shape::point to);

   shape::handle     _dragging = shape::no_handle;
   shape::handle     _hot = shape::no_handle;
};

#endif
