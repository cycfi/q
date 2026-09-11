/*=============================================================================
   Copyright (c) 2019-2026 Joel de Guzman

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#include <qplug/top_bar.hpp>
#include <qplug/base_plugin.hpp>
#include <elements.hpp>
#include <infra/string_view.hpp>

namespace cycfi::qplug
{
   using namespace cycfi::elements;

   namespace
   {
      // Past this many presets the menu scrolls rather than grows, at a
      // height that is a share of the editor's own.
      constexpr std::size_t unscrolled_max = 12;
      constexpr float menu_height_share = 0.6f;

      // The menu is wider than the button it drops from, so a long name
      // and its tag have room between them.
      constexpr float button_width = 250.0f;
      constexpr float menu_width = 340.0f;

      void refresh(presenter& p)
      {
         if (auto v = p.view())
            v->refresh();
      }

      // The name in the bar is read from the controller as it is drawn,
      // so any refresh shows the right one: a preset loaded, saved,
      // deleted, or edited from the panel or from the host.
      struct preset_label : basic_label
      {
         preset_label(controller& ctl)
          : basic_label("")
          , _ctl(ctl)
         {}

         void draw(context const& ctx) override
         {
            auto const& name = _ctl.preset_name();
            set_text(_ctl.preset_edited()? "*" + name : name);
            basic_label::draw(ctx);
         }

         controller& _ctl;
      };

      // One line of the preset menu: the name, and at the right whose
      // it is.
      auto preset_item(std::string const& name, bool factory)
      {
         auto tag = label(factory? "factory" : "user")
            .font_color(factory? colors::gold : colors::aquamarine)
            .relative_font_size(0.8);
         return basic_menu_item(
            hmargin({20, 30},
               htile(
                  align_left(label(name)),
                  margin_left(24, align_right(align_middle(std::move(tag))))
               )));
      }

      // The list is made each time the menu opens, so it is never stale:
      // a preset saved or deleted a moment ago is there, or gone.
      void populate(presenter& p, basic_button_menu& btn)
      {
         auto& ctl = p.ctl();
         vtile_composite list;
         for (auto const& name : ctl.preset_names())
         {
            auto item = share(preset_item(name, ctl.is_factory_preset(name)));
            item->on_click =
               [&p, name]()
               {
                  p.ctl().load_preset(name);
                  refresh(p);
               };
            list.push_back(item);
         }

         if (list.size() > unscrolled_max)
         {
            auto const height = p.size().y * menu_height_share;
            btn.menu(layer(
               hmin_size(menu_width, vsize(height, vscroller(list)))
             , panel{}));
         }
         else
         {
            btn.menu(layer(hmin_size(menu_width, list), panel{}));
         }
      }

      void open_save_dialog(presenter& p)
      {
         auto v = p.view();
         if (!v)
            return;

         auto field = input_box("Name");
         field.second->set_text(p.ctl().preset_name());

         auto on_ok =
            [&p, input = field.second]()
            {
               auto name = to_utf8(input->get_text());
               if (name.empty() || !p.ctl().save_preset(name))
                  return;
               p.ctl().preset_name(std::move(name));
               refresh(p);
            };

         auto dialog =
            dialog2(*v
             , hsize(320,
                  margin({20, 20, 20, 20},
                     vtile(
                        align_left(label("Save the preset as:")),
                        margin_top(10, std::move(field.first))
                     )))
             , on_ok
             , []() {}
             , "Save"
            );

         open_popup(std::move(dialog), *v);
      }

      void open_about(presenter& p)
      {
         auto v = p.view();
         if (!v)
            return;

         auto const& i = info();
         auto text = std::string(i.name) + "\nVersion " + i.version
            + "\n" + i.vendor;
         open_popup(message_box1(*v, std::move(text), icons::info, []() {})
          , *v);
      }
   }

   element_ptr make_main_menu(presenter& p)
   {
      auto btn = momentary_button<basic_button_menu>(
         icon_button_styler{icons::menu, 1.2f});
      btn.position(menu_position::bottom_right);

      auto save_as = menu_item("Save Preset As...");
      auto remove = menu_item("Delete Preset");
      auto zoom_in = menu_item("Zoom In"
       , shortcut_key{key_code::equal, mod_action});
      auto zoom_out = menu_item("Zoom Out"
       , shortcut_key{key_code::minus, mod_action});
      auto actual = menu_item("Actual Size"
       , shortcut_key{key_code::_0, mod_action});
      auto about = menu_item("About " + std::string(info().name) + "...");

      save_as.on_click = [&p]() { open_save_dialog(p); };

      // A factory preset is not the user's to remove.
      remove.is_enabled =
         [&p]()
         {
            auto const& name = p.ctl().preset_name();
            return !name.empty() && !p.ctl().is_factory_preset(name);
         };
      remove.on_click =
         [&p]()
         {
            if (p.ctl().delete_preset(p.ctl().preset_name()))
               p.ctl().preset_name("");
            refresh(p);
         };

      zoom_in.on_click = [&p]() { p.zoom_in(); };
      zoom_out.on_click = [&p]() { p.zoom_out(); };
      actual.on_click = [&p]() { p.actual_size(); };
      about.on_click = [&p]() { open_about(p); };

      btn.menu(
         layer(
            vtile(
               std::move(save_as),
               std::move(remove),
               menu_item_spacer(),
               std::move(zoom_in),
               std::move(zoom_out),
               std::move(actual),
               menu_item_spacer(),
               std::move(about)
            ),
            panel{}
         ));

      return share(std::move(btn));
   }

   element_ptr make_preset_menu(presenter& p)
   {
      auto v = p.view();
      assert(v && "make the bar from on_attach, when the view exists");

      auto name = share(preset_label{p.ctl()});
      auto btn = make_selection_menu_button(name);
      btn.position(menu_position::bottom_right);
      btn.on_open_menu = [&p](basic_button_menu& b) { populate(p, b); };
      auto menu = share(std::move(btn));

      // The controller marks the preset edited whenever a parameter
      // moves, from the panel or from the host; the models are what
      // announce that on the main thread, so each is watched and the
      // name redrawn.
      auto const count = int(p.ctl().parameters().size());
      for (int i = 0; i != count; ++i)
         v->bindings().observe(p.ctl().model(i)
          , [&p, weak = std::weak_ptr<element>(menu)](double)
            {
               if (auto e = weak.lock())
                  if (auto v = p.view())
                     v->refresh(*e);
            });

      return menu;
   }

   element_ptr make_zoom_buttons(presenter& p)
   {
      auto out = icon_button(icons::zoom_out, 1.2f);
      auto in = icon_button(icons::zoom_in, 1.2f);
      out.on_click = [&p](bool) { p.zoom_out(); };
      in.on_click = [&p](bool) { p.zoom_in(); };
      return share(htile(std::move(out), hspace(4), std::move(in)));
   }

   element_ptr make_top_bar(presenter& p, element_ptr logo)
   {
      auto right = logo?
         share(htile(
            hold(make_zoom_buttons(p)), hspace(12), align_middle(hold(logo))))
       : make_zoom_buttons(p);

      return share(
         vmargin({6, 6},
            htile(
               align_middle(hsize(36, hold(make_main_menu(p)))),
               hspace(8),
               align_middle(hsize(button_width, hold(make_preset_menu(p)))),
               align_right(align_middle(hold(right)))
            )));
   }
}
