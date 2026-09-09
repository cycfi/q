/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the Boost Software License, Version 1.0.
   [ https://www.boost.org/LICENSE_1_0.txt ]
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

      // A byte port carries MIDI 1.0; a packet port carries Universal MIDI
      // Packets. A system lists the two apart, and a device is one or the
      // other.
      enum protocol_type { midi_1_0, midi_2_0 };

      static device_list         list(protocol_type p = midi_1_0);
      std::uint32_t              id() const;
      std::string                name() const;
      std::size_t                num_inputs() const;
      std::size_t                num_outputs() const;
      protocol_type              protocol() const;

   private:

      struct impl;
      midi_device(impl const& impl)
       : _impl(impl)
      {}

      impl const&                _impl;
   };
}

#endif