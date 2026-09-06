/*=============================================================================
   Copyright (c) 2026 Joel de Guzman

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(QPLUG_PROCESSOR_HPP_SEPTEMBER_6_2026)
#define QPLUG_PROCESSOR_HPP_SEPTEMBER_6_2026

#include <qplug/base_plugin.hpp>
#include <q/support/audio_stream.hpp>
#include <cstdint>
#include <memory>

namespace cycfi::qplug
{
   ////////////////////////////////////////////////////////////////////////////
   // The processor
   ////////////////////////////////////////////////////////////////////////////
   class processor : public q::audio_stream_base
   {
   public:

      // The channel layouts this processor can run in. Stereo in, stereo
      // out unless overridden. process() gets whichever the host chose.
      virtual channel_config_list
                              channel_configs() const;
      channel_config          channels() const;
      bool                    set_channels(channel_config config);

      // Called on the main thread, before and after the audio thread runs.
      virtual void            activate(std::uint32_t sps
                               , std::uint32_t max_frames) {}
      virtual void            deactivate() {}

      // Called on the audio thread.
      virtual void            reset() {}

   private:

      channel_config          _channels = {0, 0};   // 0: not chosen yet
   };

   ////////////////////////////////////////////////////////////////////////////
   // Inline implementation
   ////////////////////////////////////////////////////////////////////////////
   inline channel_config_list processor::channel_configs() const
   {
      static channel_config const stereo[] = {{2, 2}};
      return {stereo, stereo + 1};
   }

   inline channel_config processor::channels() const
   {
      if (_channels.inputs == 0 && _channels.outputs == 0)
         return *channel_configs().begin();
      return _channels;
   }

   inline bool processor::set_channels(channel_config config)
   {
      for (auto const& c : channel_configs())
      {
         if (c.inputs == config.inputs && c.outputs == config.outputs)
         {
            _channels = config;
            return true;
         }
      }
      return false;
   }

   using processor_ptr = std::unique_ptr<processor>;
}

#endif
