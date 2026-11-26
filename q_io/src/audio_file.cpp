/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#include <q_io/audio_file.hpp>
#include <cassert>
#include <vector>

#define DR_WAV_IMPLEMENTATION
#include <dr_wav.h>

#define DR_FLAC_IMPLEMENTATION
#define DR_FLAC_NO_OGG
#include <dr_flac.h>

namespace cycfi::q
{
   // A WAV or a FLAC stream; exactly one is open. Counts are in samples
   // (frames times channels), as dr_wav's are; FLAC counts in frames.
   struct wav_impl
   {
      drwav*         wav = nullptr;
      drflac*        flac = nullptr;
   };

   wav_base::wav_base()
    : _wav(nullptr)
   {}

   wav_base::~wav_base()
   {
      if (_wav)
      {
         if (_wav->wav)
            drwav_close(_wav->wav);
         if (_wav->flac)
            drflac_close(_wav->flac);
         delete _wav;
      }
   }

   wav_base::operator bool() const
   {
      return _wav;
   }

   float wav_base::sps() const
   {
      if (!_wav)
         return 0;
      return _wav->wav? _wav->wav->sampleRate : _wav->flac->sampleRate;
   }

   std::size_t wav_base::num_channels() const
   {
      if (!_wav)
         return 0;
      return _wav->wav? _wav->wav->channels : _wav->flac->channels;
   }

   // The file's contents decide the format: WAV first, then FLAC.
   wav_reader::wav_reader(char const* filename)
   {
      if (auto wav = drwav_open_file(filename))
         _wav = new wav_impl{wav, nullptr};
      else if (auto flac = drflac_open_file(filename, nullptr))
         _wav = new wav_impl{nullptr, flac};
   }

   std::uint64_t wav_reader::length() const
   {
      if (!_wav)
         return 0;
      if (_wav->wav)
         return _wav->wav->totalSampleCount;
      return _wav->flac->totalPCMFrameCount * _wav->flac->channels;
   }

   std::size_t wav_reader::read(float* data, std::uint32_t len)
   {
      if (!_wav)
         return 0;
      if (_wav->wav)
         return drwav_read_f32(_wav->wav, len, data);

      auto ch = _wav->flac->channels;
      return drflac_read_pcm_frames_f32(_wav->flac, len / ch, data) * ch;
   }

   bool wav_reader::restart()
   {
      if (!_wav)
         return false;
      if (_wav->wav)
         return drwav_seek_to_first_sample(_wav->wav);
      return drflac_seek_to_pcm_frame(_wav->flac, 0);
   }

   std::uint64_t wav_reader::position()
   {
      if (!_wav)
         return 0;
      if (auto w = _wav->wav)
         return w->totalSampleCount - (w->bytesRemaining / w->bytesPerSample);
      return _wav->flac->currentPCMFrame * _wav->flac->channels;
   }

   bool wav_reader::seek(std::uint64_t target)
   {
      if (!_wav)
         return false;
      if (_wav->wav)
         return drwav_seek_to_sample(_wav->wav, target);
      return drflac_seek_to_pcm_frame(
         _wav->flac, target / _wav->flac->channels);
   }

   wav_writer::wav_writer(
      char const* filename
    , std::uint32_t num_channels, float sps)
   {
      drwav_data_format format;
      format.container = drwav_container_riff;
      format.format = DR_WAVE_FORMAT_IEEE_FLOAT;
      format.channels = num_channels;
      format.sampleRate = sps;
      format.bitsPerSample = 32;
      if (auto wav = drwav_open_file_write(filename, &format))
         _wav = new wav_impl{wav, nullptr};
   }

   std::size_t wav_writer::write(float const* data, std::uint32_t len)
   {
      if (_wav)
         return drwav_write(_wav->wav, len, data);
      return 0;
   }
}
