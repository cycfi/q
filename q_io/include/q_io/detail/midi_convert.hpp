/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the Boost Software License, Version 1.0.
   [ https://www.boost.org/LICENSE_1_0.txt ]
=============================================================================*/
#if !defined(CYCFI_Q_MIDI_CONVERT_SEPTEMBER_9_2026)
#define CYCFI_Q_MIDI_CONVERT_SEPTEMBER_9_2026

#include <q/support/midi_messages.hpp>
#include <cstdint>
#include <span>

namespace cycfi::q::detail
{
   ////////////////////////////////////////////////////////////////////////////
   // to_raw_message: pack the bytes a device gave us into the 24 bit word
   // q::midi_1_0 reads, status first.
   //
   // Returns false for anything that does not fit in that word: an empty
   // span, or a message longer than three bytes, which in practice means a
   // sysex. Truncating one would produce a message the sender never sent.
   ////////////////////////////////////////////////////////////////////////////
   inline bool to_raw_message(
      std::span<std::uint8_t const> bytes, midi_1_0::raw_message& out)
   {
      if (bytes.empty() || bytes.size() > 3)
         return false;

      std::uint32_t data = bytes[0];
      if (bytes.size() > 1)
         data |= std::uint32_t(bytes[1]) << 8;
      if (bytes.size() > 2)
         data |= std::uint32_t(bytes[2]) << 16;

      out.data = data;
      return true;
   }
}

#endif
