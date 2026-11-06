/*=============================================================================
   Copyright (c) 2019-2026 Joel de Guzman

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(Q_PLUG_BENDER_PRESENTER_NOVEMBER_5_2026)
#define Q_PLUG_BENDER_PRESENTER_NOVEMBER_5_2026

#include <q_plug/presenter.hpp>
#include <functional>
#include "bender_controller.hpp"

namespace q_plug = cycfi::q_plug;
namespace elements = cycfi::elements;

///////////////////////////////////////////////////////////////////////////////
// Two panels under the header. Bend: the bend range as a curve editor,
// over the trace of the bend, and the bend now in its title. Formants: bypass, the amount, Learn and
// Freeze, and how much the learner has seen. A timer, while the editor is
// open, takes the trace from the audio thread and redraws what moves.
///////////////////////////////////////////////////////////////////////////////
class bender_presenter : public q_plug::presenter
{
public:
                        bender_presenter(bender_controller& ctl);

protected:

   void                 on_attach(elements::view& view_) override;
   void                 on_detach() override;

private:

   bender_controller&   _ctl;
   bool                 _attached = false;
   std::function<void()> _tick;
};

#endif
