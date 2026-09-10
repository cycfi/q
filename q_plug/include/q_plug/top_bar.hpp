/*=============================================================================
   Copyright (c) 2019-2026 Joel de Guzman

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(QPLUG_TOP_BAR_HPP_SEPTEMBER_12_2026)
#define QPLUG_TOP_BAR_HPP_SEPTEMBER_12_2026

#include <qplug/presenter.hpp>
#include <elements/element/element.hpp>

namespace cycfi::qplug
{
   ////////////////////////////////////////////////////////////////////////////
   // The bar along the top of an editor, the same in every plugin: a main
   // menu at the left, the preset the plugin is on beside it, and at the
   // right the zoom buttons and, if given, a logo. Everything on it works
   // through the controller and the presenter, so a plugin has nothing to
   // wire: it calls make_top_bar from on_attach and puts the result at
   // the top of its content.
   //
   // A plugin that wants a bar of its own composes the same pieces, or
   // builds them beside its own: make_main_menu, make_preset_menu and
   // make_zoom_buttons are what make_top_bar is made of.
   ////////////////////////////////////////////////////////////////////////////

   // The main menu: saving and deleting a preset, zoom, and about.
   elements::element_ptr   make_main_menu(presenter& p);

   // The preset the plugin is on, starred when edited, and a menu of every
   // preset it knows, factory and user told apart. A long list scrolls.
   elements::element_ptr   make_preset_menu(presenter& p);

   // Zoom out and zoom in, a tenth at a time, as the keyboard does.
   elements::element_ptr   make_zoom_buttons(presenter& p);

   // The standard bar.
   elements::element_ptr   make_top_bar(
                              presenter& p
                            , elements::element_ptr logo = nullptr);
}

#endif
