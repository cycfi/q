/*=============================================================================
   Copyright (c) 2019-2026 Joel de Guzman

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(QPLUG_CLAP_EVENT_SLICES_HPP_SEPTEMBER_11_2026)
#define QPLUG_CLAP_EVENT_SLICES_HPP_SEPTEMBER_11_2026

#include <clap/clap.h>
#include <cstdint>
#include <utility>

namespace cycfi::qplug
{
   ////////////////////////////////////////////////////////////////////////////
   // split_at_events: a block, cut where its events fall.
   //
   // Every event carries the frame it happens at (clap/events.h: the host
   // delivers them sorted in sample order). Applying them all at the block
   // start instead quantizes every note to the buffer boundary, which at
   // 512 frames is up to 11 milliseconds of slop, heard as loose timing.
   //
   //    split_at_events(in, frames, apply, process);
   //
   // apply(header) is called for each event, process(start, count) for each
   // run of frames between them. An event is applied before the frames it
   // is stamped with, so it is heard from that sample on. Runs are never
   // empty, so a processor is never handed a block of nothing.
   //
   // Events the host stamps past the end of the block, and events that
   // arrive out of order, are applied where they are met rather than
   // dropped or run backwards.
   ////////////////////////////////////////////////////////////////////////////
   template <typename Apply, typename Process>
   inline void split_at_events(
      clap_input_events_t const* in, std::uint32_t frames
    , Apply&& apply, Process&& process)
   {
      auto const count = in? in->size(in) : 0;
      std::uint32_t frame = 0;
      std::uint32_t index = 0;

      while (true)
      {
         // Everything that happens at this frame, or that should have
         // happened before it. At the end of the block that is everything
         // left, including an event the host stamped past the end.
         while (index != count)
         {
            auto const hdr = in->get(in, index);
            if (hdr->time > frame && frame != frames)
               break;
            apply(hdr);
            ++index;
         }

         if (frame == frames)
            break;

         // Up to the next event, or to the end of the block.
         auto next = frames;
         if (index != count)
         {
            auto const time = in->get(in, index)->time;
            if (time < frames)
               next = time;
         }

         process(frame, next - frame);
         frame = next;
      }
   }
}

#endif
