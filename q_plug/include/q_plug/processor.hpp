/*=============================================================================
   Copyright (c) 2019-2026 Joel de Guzman

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(QPLUG_PROCESSOR_HPP_SEPTEMBER_6_2026)
#define QPLUG_PROCESSOR_HPP_SEPTEMBER_6_2026

#include <qplug/base_plugin.hpp>
#include <q/support/audio_stream.hpp>
#include <q/midi/messages.hpp>
#include <q/midi/ump.hpp>
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

      // What the host sent, at its sample offset into the block, on the
      // audio thread and before process is called. A processor that
      // answers notes derives from midi_processor rather than overriding
      // these, and writes Q overloads instead; the host offers a note
      // port only to a processor that says it wants one.
      virtual bool            has_midi_input() const { return false; }
      virtual void            midi(q::midi_1_0::raw_message, std::size_t) {}
      virtual void            midi(q::midi_2_0::packet const&, std::size_t) {}

   private:

      friend class plugin;

      std::uint32_t           _sps = 0;
      std::uint32_t           _max_frames = 0;
   };

   using processor_ptr = std::unique_ptr<processor>;
}

#endif
