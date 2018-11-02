/*=============================================================================
   Copyright (c) 2014-2018 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#include <q_io/audio_file.hpp>
#include <cassert>
#include <vector>

#define DR_WAV_IMPLEMENTATION
#include <dr_wav.h>

namespace cycfi { namespace q { namespace audio_file
{
   base::base()
    : _wav(nullptr)
   {}

   base::~base()
   {
      drwav_close(_wav);
   }

   reader::reader(char const* filename)
   {
      _wav = drwav_open_file(filename);
   }

   writer::writer(
      char const* filename
      , std::uint32_t num_channels, std::uint32_t sps)
   {
	   drwav_data_format format;
      format.container = drwav_container_riff;
      format.format = DR_WAVE_FORMAT_IEEE_FLOAT;
      format.channels = num_channels;
      format.sampleRate = sps;
      format.bitsPerSample = 32;
      _wav = drwav_open_file_write(filename, &format);
   }
}}}

