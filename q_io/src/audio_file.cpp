/*=============================================================================
   Copyright (c) 2014-2019 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#include <q_io/audio_file.hpp>
#include <cassert>
#include <vector>

#define DR_WAV_IMPLEMENTATION
#include <dr_wav.h>

namespace cycfi::q
{
   struct wav_impl : drwav {};

   wav_base::wav_base()
    : _wav(nullptr)
   {}

   wav_base::~wav_base()
   {
      if (_wav)
         drwav_close(_wav);
   }

   wav_base::operator bool() const
   {
      return _wav;
   }

   std::size_t wav_base::sps() const
   {
      if (_wav)
         return _wav->sampleRate;
      return 0;
}

   std::size_t wav_base::num_channels() const
   {
      if (_wav)
         return _wav->channels;
      return 0;
}

   wav_reader::wav_reader(char const* filename)
   {
      _wav = static_cast<wav_impl*>(drwav_open_file(filename));
   }

   std::size_t wav_reader::length() const
   {
      if (_wav)
         return _wav->totalSampleCount;
      return 0;
   }

   std::size_t wav_reader::read(float* data, std::uint32_t len)
   {
      if (_wav)
         return drwav_read_f32(_wav, len, data);
      return 0;
   }

   wav_writer::wav_writer(
      char const* filename
    , std::uint32_t num_channels, std::uint32_t sps)
   {
	   drwav_data_format format;
      format.container = drwav_container_riff;
      format.format = DR_WAVE_FORMAT_IEEE_FLOAT;
      format.channels = num_channels;
      format.sampleRate = sps;
      format.bitsPerSample = 32;
      _wav = static_cast<wav_impl*>(drwav_open_file_write(filename, &format));
   }

   std::size_t wav_writer::write(float const* data, std::uint32_t len)
   {
      if (_wav)
         return drwav_write(_wav, len, data);
      return 0;
   }
}

