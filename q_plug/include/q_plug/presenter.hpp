/*=============================================================================
   Copyright (c) 2026 Joel de Guzman

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(QPLUG_PRESENTER_HPP_SEPTEMBER_6_2026)
#define QPLUG_PRESENTER_HPP_SEPTEMBER_6_2026

#include <memory>

namespace cycfi::qplug
{
   ////////////////////////////////////////////////////////////////////////////
   // The presenter
   //
   // Builds the element tree and links it to the controller's parameters.
   // The view it presents into is owned here and is null while the host has
   // no editor open. GUI support arrives with the next milestone.
   ////////////////////////////////////////////////////////////////////////////
   class presenter
   {
   public:

      virtual                 ~presenter() = default;
   };

   using presenter_ptr = std::unique_ptr<presenter>;
}

#endif
