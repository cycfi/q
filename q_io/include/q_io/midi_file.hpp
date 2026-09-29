/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(CYCFI_Q_IO_MIDI_FILE_HPP_SEPTEMBER_28_2026)
#define CYCFI_Q_IO_MIDI_FILE_HPP_SEPTEMBER_28_2026

#include <q/midi/file_reader.hpp>
#include <cstdint>
#include <fstream>
#include <string>
#include <vector>

namespace cycfi::q
{
   namespace detail
   {
      // The file's bytes, held so the reader has something to look at.
      struct midi_file_data
      {
         midi_file_data(std::string const& path)
         {
            std::ifstream in{path, std::ios::binary};
            if (!in)
               return;
            bytes.assign(
               std::istreambuf_iterator<char>{in},
               std::istreambuf_iterator<char>{}
            );
         }

         std::vector<std::uint8_t> bytes;
      };
   }

   ////////////////////////////////////////////////////////////////////////////
   // midi_file: a standard MIDI file read from disk.
   //
   // This is `{midi::file_reader}` with the bytes taken care of: it reads
   // the file in, and is the reader from then on. A file that is missing or
   // malformed leaves it false.
   //
   //    q::midi_file file{"riff.mid"};
   //    if (file)
   //       file(sps, my_processor);
   //
   ////////////////////////////////////////////////////////////////////////////
   class midi_file : private detail::midi_file_data, public midi::file_reader
   {
   public:

      midi_file(std::string const& path)
       : detail::midi_file_data{path}
       , midi::file_reader{bytes}
      {}

      midi_file(midi_file const&) = delete;
      midi_file& operator=(midi_file const&) = delete;

      std::size_t    size() const   { return bytes.size(); }
   };
}

#endif
