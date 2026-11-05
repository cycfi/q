/*=============================================================================
   Copyright (c) 2019-2026 Joel de Guzman

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(Q_PLUG_BENDER_BEND_VIEW_NOVEMBER_5_2026)
#define Q_PLUG_BENDER_BEND_VIEW_NOVEMBER_5_2026

#include <elements.hpp>
#include <memory>
#include <vector>
#include "bender_controller.hpp"

namespace elements = cycfi::elements;

///////////////////////////////////////////////////////////////////////////////
// The bend range as a curve editor, over a fixed frame of -12 to +12
// semitones, the bar's travel across:
//
//    0: the bottom, at the dive depth      2: the middle's top, +middle
//    1: the middle's bottom, -middle       3: the top, at the pull-up
//
// Points 0 and 3 move up and down only; 1 and 2 mirror each other, and sit
// where the bend range puts the middle's ends. Under the editor, the
// frame: its grid, the middle shaded, and a trace of the bend after the
// range over the last few seconds, with a dot where the bar is now.
///////////////////////////////////////////////////////////////////////////////
namespace bend
{
   constexpr float frame = 12.0f;         // semitones either way
   constexpr float inset = 8.0f;          // the curve editor's, too

   enum { bottom, middle_lo, middle_hi, top };

   // A semitone value as a height in the unit square, and back.
   constexpr float height(float st) { return (st + frame) / (2 * frame); }
   constexpr float semitones(float y) { return y * 2 * frame - frame; }

   // A depth parameter as its point's height, for the binder.
   struct depth
   {
      double         position(double v) const;
      double         value(double pos) const;

      bool           up;
   };

   // The editor, its points placed and moved by the bend range.
   std::shared_ptr<elements::basic_curve_editor>
   make_editor(bender_controller const& ctl);

   // Put the middle's ends where the bend range has them.
   void place(elements::basic_curve_editor& e, q::midi::bend_range r);

   ////////////////////////////////////////////////////////////////////////////
   // The frame under the editor, and the trace.
   ////////////////////////////////////////////////////////////////////////////
   class scope : public elements::element
   {
   public:
                              scope(bender_controller& ctl);

      elements::view_limits   limits(elements::basic_context const& ctx)
                                 const override;
      void                    draw(elements::context const& ctx) override;

      // Take what the audio thread has sent. Main thread.
      void                    poll();

      static constexpr double trace_time = 4.0;  // seconds

   private:

      bender_controller&      _ctl;
      std::vector<float>      _trace;
      std::size_t             _head = 0;
      float                   _now = 0.0f;
   };

   ////////////////////////////////////////////////////////////////////////////
   // How much of the band the formant learner has seen, as a bar.
   ////////////////////////////////////////////////////////////////////////////
   class coverage : public elements::element
   {
   public:
                              coverage(bender_controller const& ctl);

      elements::view_limits   limits(elements::basic_context const& ctx)
                                 const override;
      void                    draw(elements::context const& ctx) override;

   private:

      bender_controller const& _ctl;
   };
}

#endif
