/*=============================================================================
   Copyright (c) 2019-2026 Joel de Guzman

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(QPLUG_DELAY_PRESENTER_SEPTEMBER_7_2026)
#define QPLUG_DELAY_PRESENTER_SEPTEMBER_7_2026

#include <qplug/presenter.hpp>
#include "delay_controller.hpp"

namespace qplug = cycfi::qplug;
namespace elements = cycfi::elements;

///////////////////////////////////////////////////////////////////////////////
class delay_presenter : public qplug::presenter
{
public:
                        delay_presenter(delay_controller& ctl);

protected:

   void                 on_attach(elements::view& view_) override;

private:

   delay_controller&    _ctl;
};

#endif
