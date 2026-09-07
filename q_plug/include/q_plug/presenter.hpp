/*=============================================================================
   Copyright (c) 2019-2026 Joel de Guzman

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(QPLUG_PRESENTER_HPP_SEPTEMBER_6_2026)
#define QPLUG_PRESENTER_HPP_SEPTEMBER_6_2026

#include <qplug/controller.hpp>
#include <elements/view.hpp>
#include <cstdint>
#include <memory>
#include <vector>

namespace cycfi::qplug
{
   ////////////////////////////////////////////////////////////////////////////
   // Where the presenter's requests to the host go. The plugin is the sink.
   ////////////////////////////////////////////////////////////////////////////
   struct view_sink
   {
      virtual                 ~view_sink() = default;
      virtual bool            request_resize(elements::extent size) = 0;
   };

   ////////////////////////////////////////////////////////////////////////////
   // The presenter
   //
   // Builds the element tree and binds its controls to the controller's
   // parameters. Owns the view, which exists only while the host has an
   // editor open. A plugin with no presenter at all is headless.
   ////////////////////////////////////////////////////////////////////////////
   class presenter
   {
   public:
                              presenter(controller& ctl);
      virtual                 ~presenter();

      // Bind a control to a parameter. What the user does to the control
      // reaches the host as an edit, bracketed as one gesture; what the
      // host does to the parameter reaches the control, through the
      // view's bindings, which leave with the view. The mapping carries
      // the control's travel, 0 to 1, to the parameter's value and back: a
      // db_scale, or the parameter itself for a linear one. Any number of
      // controls may share a parameter. Call it from on_attach.
                              template <typename Control, typename Mapping>
      void                    bind(int index, std::shared_ptr<Control> control
                               , Mapping mapping);

      // The view's life: made at the given size and its content built,
      // put into the host's parent view, resized, taken down. The
      // content's own limits bound the size, then and on every resize.
      // All main thread.
      bool                    create(elements::extent size);
      bool                    attach(void* parent, elements::extent size);
      void                    detach();
      void                    show(bool show);

      // The live view's size and limits; zero size and no limits without
      // a view.
      elements::extent        size() const;
      elements::view_limits   limits() const;
      bool                    resize(elements::extent size);

      // Ask the host for a new size, as from a size menu in the UI. The
      // host answers with set_size, or not at all.
      bool                    request_resize(elements::extent size);

      void                    sink(view_sink& s) { _sink = &s; }
      elements::view*         view() const { return _view.get(); }

   protected:

      // Called with the freshly made view; build the content here.
      virtual void            on_attach(elements::view& view_) {}
      virtual void            on_detach() {}

   private:

      using deleter = void(*)(elements::view*);
      using view_ptr = std::unique_ptr<elements::view, deleter>;

      // Which parameter a control edits, for the gesture dispatch. The
      // element is held weakly: controls come and go with the editor.
      struct gesture
      {
         int                              index;
         std::weak_ptr<elements::element> element;
      };

      // Makes the view and builds its content. parent is null where the
      // view can be made before the host gives us one.
      bool                    build(void* parent, elements::extent size_);

      controller&             _ctl;
      view_ptr                _view;
      view_sink*              _sink = nullptr;
      std::vector<gesture>    _gestures;
   };

   using presenter_ptr = std::unique_ptr<presenter>;

   ////////////////////////////////////////////////////////////////////////////
   // Inline implementation
   ////////////////////////////////////////////////////////////////////////////
   template <typename Control, typename Mapping>
   inline void presenter::bind(int index, std::shared_ptr<Control> control
    , Mapping mapping)
   {
      _gestures.push_back({index, control});
      _view->bindings().follow(_ctl.model(index), control
       , [mapping](double value) { return mapping.position(value); }
       , [this, index, mapping](double pos)
         {
            _ctl.edit_parameter(index, mapping.value(pos));
         });
   }
}

#endif
