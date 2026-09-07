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

      using channel_config = cycfi::qplug::channel_config;

      // The channel layout: stereo in, stereo out unless overridden.
      virtual channel_config  channels() const { return {2, 2}; }

      // The stream the host gave us, both set before activate is called:
      // the sample rate, and the largest block process will be handed, for
      // a processor that has to size something up front.
      std::uint32_t           sps() const { return _sps; }
      std::uint32_t           max_frames() const { return _max_frames; }

      // Called on the main thread, before and after the audio thread runs.
      // activate is the place to size anything up from max_frames; reset
      // follows it, so whatever depends only on the stream goes there.
      virtual void            activate() {}
      virtual void            deactivate() {}

      // Take up the current stream and clear any state carried over.
      // Called after activate, so a rate change always reaches it, and by
      // the host on the audio thread.
      virtual void            reset() {}

   private:

      friend class plugin;

      std::uint32_t           _sps = 0;
      std::uint32_t           _max_frames = 0;
   };

   using processor_ptr = std::unique_ptr<processor>;
}

#endif
