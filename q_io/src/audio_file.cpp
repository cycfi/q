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
   struct wav_impl : drwav {};

   base::base()
    : _wav(nullptr)
   {}

   base::~base()
   {
      drwav_close(_wav);
   }

   base::operator bool() const
   {
      return _wav != nullptr;
   }

   std::size_t base::sps() const
   {
      return _wav->sampleRate;
   }

   std::size_t base::num_channels() const
   {
      return _wav->channels;
   }

   reader::reader(char const* filename)
   {
      _wav = static_cast<wav_impl*>(drwav_open_file(filename));
   }

   std::size_t reader::length() const
   {
      return _wav->totalSampleCount;
   }

   std::size_t reader::read(float* data, std::uint32_t len)
   {
      if (_wav == nullptr)
         return 0;
      return drwav_read_f32(_wav, len, data);
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
      _wav = static_cast<wav_impl*>(drwav_open_file_write(filename, &format));
   }

   std::size_t writer::write(float const* data, std::uint32_t len)
   {
      if (_wav == nullptr)
         return 0;
      return drwav_write(_wav, len, data);
   }
}}}

