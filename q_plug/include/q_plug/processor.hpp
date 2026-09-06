/*=============================================================================
   Copyright (c) 2026 Joel de Guzman

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(QPLUG_PROCESSOR_HPP_SEPTEMBER_6_2026)
#define QPLUG_PROCESSOR_HPP_SEPTEMBER_6_2026

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

      virtual std::uint32_t   inputs() const { return 2; }
      virtual std::uint32_t   outputs() const { return 2; }

      // Called on the main thread, before and after the audio thread runs.
      virtual void            activate(std::uint32_t sps
                               , std::uint32_t max_frames) {}
      virtual void            deactivate() {}

      // Called on the audio thread.
      virtual void            reset() {}
   };

   using processor_ptr = std::unique_ptr<processor>;
}

#endif
