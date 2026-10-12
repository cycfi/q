/*=============================================================================
   Copyright (c) 2019-2026 Joel de Guzman

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(Q_PLUG_LOOPER_PRESENTER_OCTOBER_12_2026)
#define Q_PLUG_LOOPER_PRESENTER_OCTOBER_12_2026

#include <q_plug/presenter.hpp>
#include "looper_controller.hpp"

namespace q_plug = cycfi::q_plug;
namespace elements = cycfi::elements;

///////////////////////////////////////////////////////////////////////////////
class looper_presenter : public q_plug::presenter
{
public:
                        looper_presenter(looper_controller& ctl);

protected:

   void                 on_attach(elements::view& view_) override;

private:

   looper_controller&   _ctl;
};

#endif
