/*=============================================================================
   Copyright (c) 2026 Joel de Guzman

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#include "gain_processor.hpp"

gain_processor::gain_processor(gain_controller& ctl)
 : _ctl(ctl)
{}

// Mono and stereo, one channel in to one out either way.
qplug::channel_config_list gain_processor::channel_configs() const
{
   static qplug::channel_config const configs[] = {{1, 1}, {2, 2}};
   return {configs, configs + 2};
}

void gain_processor::process(in_channels const& in, out_channels const& out)
{
   auto g = float(_ctl.volume());
   auto frames = in.frames.size();

   for (std::size_t ch = 0; ch != in.size(); ++ch)
   {
      auto i = in[ch].begin();
      auto o = out[ch].begin();
      for (std::size_t f = 0; f != frames; ++f)
         o[f] = i[f] * g;
   }
}
