/*=============================================================================
   Copyright (c) 2019-2026 Joel de Guzman

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(QPLUG_VA_SYNTH_5_PRESENTER_SEPTEMBER_11_2026)
#define QPLUG_VA_SYNTH_5_PRESENTER_SEPTEMBER_11_2026

#include <qplug/presenter.hpp>
#include "va_synth_controller.hpp"
#include "adsr_control.hpp"
#include <memory>
#include <string>

namespace qplug = cycfi::qplug;
namespace elements = cycfi::elements;

///////////////////////////////////////////////////////////////////////////////
class va_synth_presenter : public qplug::presenter
{
public:
                        va_synth_presenter(va_synth_controller& ctl);

protected:

   void                 on_attach(elements::view& view_) override;

private:

   // The preset bar: a menu of the names the controller knows, and the
   // two buttons that write and remove a user preset.
   std::shared_ptr<elements::element>
                        make_preset_bar(elements::view& view_);

   // The menu is rebuilt rather than edited, because saving a preset can
   // add a name and deleting one takes a name away.
   void                 rebuild_preset_menu();
   void                 load_preset(std::string name);
   void                 open_save_dialog();
   void                 save_preset(std::string name);
   void                 delete_preset();

   // The name in the button, with a leading star when the values no
   // longer are what the preset holds.
   void                 show_preset_name();

   va_synth_controller& _ctl;

   std::shared_ptr<elements::element>      _preset_button;
   elements::basic_button_menu*            _preset_menu = nullptr;
   std::shared_ptr<elements::basic_label>  _preset_label;
};

#endif
