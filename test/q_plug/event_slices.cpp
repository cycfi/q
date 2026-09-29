/*=============================================================================
   Copyright (c) 2019-2026 Joel de Guzman

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
// Cutting a block at the events inside it. Cases are named for the clauses
// of clap/events.h they come from.
#define CATCH_CONFIG_MAIN
#include <infra/catch.hpp>
#include <qplug/clap/event_slices.hpp>

#include <cstdint>
#include <string>
#include <vector>

namespace qplug = cycfi::qplug;

namespace
{
   // A host's event list: headers in the order the host would deliver them.
   struct event_list
   {
      event_list(std::vector<std::uint32_t> const& times)
      {
         for (auto t : times)
         {
            clap_event_header_t hdr{};
            hdr.size = sizeof(hdr);
            hdr.time = t;
            hdr.space_id = CLAP_CORE_EVENT_SPACE_ID;
            hdr.type = CLAP_EVENT_MIDI;
            _events.push_back(hdr);
         }

         _list.ctx = this;
         _list.size = [](clap_input_events_t const* l) -> std::uint32_t
         {
            return std::uint32_t(
               static_cast<event_list const*>(l->ctx)->_events.size());
         };
         _list.get = [](clap_input_events_t const* l, std::uint32_t i)
         {
            return &static_cast<event_list const*>(l->ctx)->_events[i];
         };
      }

      clap_input_events_t const* operator&() const { return &_list; }

      std::vector<clap_event_header_t>  _events;
      clap_input_events_t              _list;
   };

   // What happened, in order: "apply@t" for each event, "run[a,b)" for each
   // run of frames.
   struct trace
   {
      std::vector<std::string> _steps;

      auto apply()
      {
         return [this](clap_event_header_t const* hdr)
         {
            _steps.push_back("apply@" + std::to_string(hdr->time));
         };
      }

      auto process()
      {
         return [this](std::uint32_t start, std::uint32_t count)
         {
            _steps.push_back(
               "run[" + std::to_string(start) + ","
                      + std::to_string(start + count) + ")");
         };
      }
   };

   std::vector<std::string> run(
      std::vector<std::uint32_t> const& times, std::uint32_t frames)
   {
      event_list list{times};
      trace t;
      qplug::split_at_events(&list, frames, t.apply(), t.process());
      return t._steps;
   }
}

TEST_CASE("A block with no events is processed whole")
{
   CHECK(run({}, 512) == std::vector<std::string>{"run[0,512)"});
}

TEST_CASE("An event at the block start precedes every frame")
{
   CHECK(run({0}, 512)
      == std::vector<std::string>{"apply@0", "run[0,512)"});
}

TEST_CASE("An event inside the block cuts it there")
{
   // The event is applied before the frame it is stamped with, so it is
   // heard from that sample on, not from the next block.
   CHECK(run({128}, 512)
      == std::vector<std::string>{
         "run[0,128)", "apply@128", "run[128,512)"});
}

TEST_CASE("Every event cuts the block, in sample order")
{
   CHECK(run({64, 128, 400}, 512)
      == std::vector<std::string>{
         "run[0,64)", "apply@64", "run[64,128)", "apply@128"
       , "run[128,400)", "apply@400", "run[400,512)"});
}

TEST_CASE("Events at one sample are applied together, without an empty run")
{
   CHECK(run({128, 128, 128}, 512)
      == std::vector<std::string>{
         "run[0,128)", "apply@128", "apply@128", "apply@128"
       , "run[128,512)"});
}

TEST_CASE("An event at the block end is applied after the last frame")
{
   // Out of range by the clause, since a block of 512 has no frame 512,
   // but a host may still stamp one there. It must not make an empty run,
   // and it must not be dropped: the next block has to see its effect.
   CHECK(run({512}, 512)
      == std::vector<std::string>{"run[0,512)", "apply@512"});
}

TEST_CASE("An event past the block end is applied, not dropped")
{
   CHECK(run({9000}, 512)
      == std::vector<std::string>{"run[0,512)", "apply@9000"});
}

TEST_CASE("An event out of order does not run the block backwards")
{
   // The clause says the host delivers these sorted. One that arrives
   // late is applied at once rather than rewinding the block.
   CHECK(run({256, 64}, 512)
      == std::vector<std::string>{
         "run[0,256)", "apply@256", "apply@64", "run[256,512)"});
}

TEST_CASE("A block of no frames still applies its events")
{
   CHECK(run({0, 0}, 0)
      == std::vector<std::string>{"apply@0", "apply@0"});
}

TEST_CASE("No event list is a block processed whole")
{
   trace t;
   qplug::split_at_events(nullptr, 256, t.apply(), t.process());
   CHECK(t._steps == std::vector<std::string>{"run[0,256)"});
}
