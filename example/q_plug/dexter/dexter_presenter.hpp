/*=============================================================================
   Copyright (c) 2019-2026 Joel de Guzman

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(Q_PLUG_DEXTER_PRESENTER_OCTOBER_1_2026)
#define Q_PLUG_DEXTER_PRESENTER_OCTOBER_1_2026

#include <q_plug/presenter.hpp>
#include "dexter_controller.hpp"
#include <elements/element/button.hpp>
#include <elements/element/image.hpp>
#include <elements/element/label.hpp>
#include <memory>
#include <vector>

namespace q_plug = cycfi::q_plug;
namespace elements = cycfi::elements;

namespace cycfi::elements { class basic_curve_editor; }

///////////////////////////////////////////////////////////////////////////////
// The editor: the standard header, then the algorithm, the operator being
// edited, the pitch envelope, the LFO and the voice's own settings.
//
// One operator is edited at a time. Each has its page in three decks (its
// envelope, its keyboard scaling, its sliders), all bound once, and a
// click on its thumbnail brings those pages up.
///////////////////////////////////////////////////////////////////////////////
class dexter_presenter : public q_plug::presenter
{
public:
                        dexter_presenter(dexter_controller& ctl);

protected:

   void                 on_attach(elements::view& view_) override;
   element_ptr          logo() override;

private:

   using deck_ptr = std::shared_ptr<elements::deck_composite>;
   using thumb_ptr = std::shared_ptr<elements::basic_latching_button>;
   using label_ptr = std::shared_ptr<elements::basic_label>;

   element_ptr          make_algorithm();
   element_ptr          make_algorithm_menu();
   element_ptr          make_operator();
   element_ptr          make_thumb(int op);
   element_ptr          make_envelope_page(int op);
   element_ptr          make_scaling_page(int op);
   element_ptr          make_slider_page(int op);
   element_ptr          make_pitch_envelope();
   element_ptr          make_lfo();
   element_ptr          make_voice();

   void                 select(int op);
   void                 watch();
   bool                 import(elements::drop_info const& info);

   // A control and a label bound to a parameter
                        template <typename Control>
   void                 link(int index, std::shared_ptr<Control> control);
   void                 link_envelope(
                           std::shared_ptr<elements::basic_curve_editor> e
                         , int rate, int level);
   element_ptr          readout(int index);
   element_ptr          choices(int index, char const* const names[]
                         , int count);
   element_ptr          stepper(int index);

   dexter_controller&       _ctl;
   int                  _selected = 0;
   elements::image_ptr  _algorithms;      // all 32, eight by four
   elements::image_ptr  _waves;           // the six LFO waves, a row each
   label_ptr            _title;           // the operator panel's
   element_ptr          _chart;
   element_ptr          _algorithm;       // the menu's button
   std::vector<thumb_ptr>
                        _thumbs;
   deck_ptr             _envelopes;
   deck_ptr             _scalings;
   deck_ptr             _sliders;
};

#endif
