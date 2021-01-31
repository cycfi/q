/*=============================================================================
   Copyright (c) 2014-2021 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(CYCFI_Q_MIDI_DEVICE_HPP_DECEMBER_10_2018)
#define CYCFI_Q_MIDI_DEVICE_HPP_DECEMBER_10_2018

#include <vector>
#include <cstdint>
#include <string>

namespace cycfi::q
{
   ////////////////////////////////////////////////////////////////////////////
   class midi_device
   {
   public:

      using device_list = std::vector<midi_device>;

      static device_list         list();
      std::uint32_t              id() const;
      std::string                name() const;
      std::size_t                num_inputs() const;
      std::size_t                num_outputs() const;

   private:

      struct impl;
      midi_device(impl const& impl)
       : _impl(impl)
      {}

      impl const&                _impl;
   };
}

#endif