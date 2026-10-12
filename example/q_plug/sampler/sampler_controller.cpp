/*=============================================================================
   Copyright (c) 2019-2026 Joel de Guzman

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#include "sampler_controller.hpp"
#include <q_io/audio_file.hpp>
#include <memory>

using parameter_list = sampler_controller::parameter_list;
using namespace cycfi::q::literals;

// Play is the transport, so a preset leaves it out.
parameter_list sampler_controller::parameters() const
{
   static parameter params[] =
   {
      parameter{ 1, "Play", false }.dont_save(),
      parameter{ 2, "Volume", 0_dB }.range(silence.rep, max_volume.rep)
   };

   return { params };
}

bool sampler_controller::load(path const& file)
{
   q::wav_reader wav{file.string()};
   if (!wav || wav.num_channels() == 0)
      return false;

   auto s = std::make_unique<sample_buffer>();
   s->data.resize(wav.length());
   s->data.resize(wav.read(s->data));
   s->channels = wav.num_channels();
   s->sps = wav.sps();

   sample.send(std::move(s));
   _sample_name = file.filename().string();
   return true;
}
