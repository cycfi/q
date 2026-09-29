/*=============================================================================
   Copyright (c) 2019-2026 Joel de Guzman

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(Q_PLUG_GAIN_PRESENTER_SEPTEMBER_6_2026)
#define Q_PLUG_GAIN_PRESENTER_SEPTEMBER_6_2026

#include <q_plug/presenter.hpp>
#include "gain_controller.hpp"

namespace q_plug = cycfi::q_plug;
namespace elements = cycfi::elements;

///////////////////////////////////////////////////////////////////////////////
class gain_presenter : public q_plug::presenter
{
public:
                        gain_presenter(gain_controller& ctl);

protected:

   void                 on_attach(elements::view& view_) override;

private:

   gain_controller&     _ctl;
};

#endif
