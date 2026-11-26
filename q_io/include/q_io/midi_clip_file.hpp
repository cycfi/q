/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(CYCFI_Q_IO_MIDI_CLIP_FILE_HPP_NOVEMBER_8_2026)
#define CYCFI_Q_IO_MIDI_CLIP_FILE_HPP_NOVEMBER_8_2026

#include <q/midi/clip_reader.hpp>
#include <q_io/midi_file.hpp>
#include <string>

namespace cycfi::q
{
   ////////////////////////////////////////////////////////////////////////////
   // midi_clip_file: a MIDI Clip File (M2-116) read from disk.
   //
   // This is `{midi_2_0::clip_reader}` with the bytes taken care of, as
   // midi_file is for a standard MIDI file: it reads the file in, and is the
   // reader from then on. A file that is missing or malformed leaves it
   // false.
   //
   //    q::midi_clip_file clip{"take.midi2"};
   //    if (clip)
   //       clip(sps, my_processor);
   //
   ////////////////////////////////////////////////////////////////////////////
   class midi_clip_file
    : private detail::midi_file_data, public midi_2_0::clip_reader
   {
   public:

      midi_clip_file(std::string const& path)
       : detail::midi_file_data{path}
       , midi_2_0::clip_reader{bytes}
      {}

      midi_clip_file(midi_clip_file const&) = delete;
      midi_clip_file& operator=(midi_clip_file const&) = delete;

      std::size_t    size() const   { return bytes.size(); }
   };
}

#endif
