/*=============================================================================
   Copyright (c) 2026 Joel de Guzman

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(QPLUG_GAIN_PRESENTER_SEPTEMBER_6_2026)
#define QPLUG_GAIN_PRESENTER_SEPTEMBER_6_2026

#include <qplug/presenter.hpp>
#include "gain_controller.hpp"

namespace qplug = cycfi::qplug;

///////////////////////////////////////////////////////////////////////////////
class gain_presenter : public qplug::presenter
{
public:
                        gain_presenter(gain_controller& ctl);

protected:

   void                 on_attach(elements::view& view_) override;

private:

   gain_controller&     _ctl;
};

#endif
