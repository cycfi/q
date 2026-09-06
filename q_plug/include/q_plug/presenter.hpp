/*=============================================================================
   Copyright (c) 2026 Joel de Guzman

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(QPLUG_PRESENTER_HPP_SEPTEMBER_6_2026)
#define QPLUG_PRESENTER_HPP_SEPTEMBER_6_2026

#include <elements/base_view.hpp>
#include <cstdint>
#include <memory>

namespace cycfi::elements { class view; }

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
   // Builds the element tree and links it to the controller's parameter
   // models. Owns the view, which exists only while the host has an editor
   // open. A plugin with no presenter at all is headless.
   ////////////////////////////////////////////////////////////////////////////
   class presenter
   {
   public:
                              presenter();
      virtual                 ~presenter();

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

      view_ptr                _view;
      view_sink*              _sink = nullptr;
   };

   using presenter_ptr = std::unique_ptr<presenter>;
}

#endif
