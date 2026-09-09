/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the Boost Software License, Version 1.0.
   [ https://www.boost.org/LICENSE_1_0.txt ]
=============================================================================*/
#define CATCH_CONFIG_MAIN
#include <infra/catch.hpp>
#include <q/midi/parameters.hpp>

#include <cstdint>
#include <vector>

namespace q = cycfi::q;
namespace midi = q::midi_1_0;

namespace
{
   // Records what came out of the decoder: the parameters it assembled, and
   // the messages it passed through untouched.
   struct recorder : midi::processor
   {
      using midi::processor::operator();

      struct parameter
      {
         bool           registered;
         std::uint8_t   channel;
         std::uint16_t  number;
         std::uint16_t  value;
      };

      void operator()(midi::rpn msg, std::size_t)
      {
         _params.push_back(
            {true, msg.channel(), msg.number(), msg.value()});
      }

      void operator()(midi::nrpn msg, std::size_t)
      {
         _params.push_back(
            {false, msg.channel(), msg.number(), msg.value()});
      }

      void operator()(midi::control_change msg, std::size_t)
      {
         _ccs.push_back(std::uint8_t(msg.controller()));
      }

      void operator()(midi::note_on, std::size_t)
      {
         ++_notes;
      }

      std::vector<parameter>     _params;
      std::vector<std::uint8_t>  _ccs;
      int                        _notes = 0;
   };

   // The four controllers that select a parameter, and the two that set it.
   constexpr std::uint8_t rpn_msb = 101;
   constexpr std::uint8_t rpn_lsb = 100;
   constexpr std::uint8_t nrpn_msb = 99;
   constexpr std::uint8_t nrpn_lsb = 98;
   constexpr std::uint8_t data_msb = 6;
   constexpr std::uint8_t data_lsb = 38;
   constexpr std::uint8_t data_inc = 96;
   constexpr std::uint8_t data_dec = 97;

   struct fixture
   {
      void cc(std::uint8_t channel, std::uint8_t ctrl, std::uint8_t value)
      {
         send(midi::control_change{channel, midi::cc::controller(ctrl), value});
      }

      template <typename Message>
      void send(Message const& msg)
      {
         midi::raw_message raw{
            std::uint32_t(msg.data[0])
          | (std::uint32_t(msg.data[1]) << 8)
          | (std::uint32_t(msg.data[2]) << 16)};
         midi::dispatch(raw, _time++, _chain);
      }

      // Select an RPN, the usual way: number first, most significant half
      // of it first too.
      void select_rpn(std::uint8_t channel, std::uint16_t number)
      {
         cc(channel, rpn_msb, (number >> 7) & 0x7F);
         cc(channel, rpn_lsb, number & 0x7F);
      }

      void select_nrpn(std::uint8_t channel, std::uint16_t number)
      {
         cc(channel, nrpn_msb, (number >> 7) & 0x7F);
         cc(channel, nrpn_lsb, number & 0x7F);
      }

      recorder                                  _rec;
      midi::rpn_reader<recorder&>      _chain{_rec};
      std::size_t                               _time = 0;
   };
}

TEST_CASE("A parameter is not reported until it has a value")
{
   // Selecting says which parameter the next data entry addresses. On its
   // own it changes nothing, so nothing is emitted.
   fixture f;
   f.select_rpn(0, 0);

   CHECK(f._rec._params.empty());
}

TEST_CASE("The selecting controllers are absorbed")
{
   // A processor that watches control changes must not see the four
   // controllers that spell out a parameter, or it would act on the halves
   // of a number as though they were knobs.
   fixture f;
   f.select_rpn(0, 0);
   f.cc(0, data_msb, 2);

   CHECK(f._rec._ccs.empty());
}

TEST_CASE("A most significant data entry sets the parameter")
{
   // RPN 0 is pitch bend sensitivity, and 2 semitones is the usual default.
   fixture f;
   f.select_rpn(0, 0);
   f.cc(0, data_msb, 2);

   REQUIRE(f._rec._params.size() == 1);
   auto const& p = f._rec._params.front();
   CHECK(p.registered);
   CHECK(p.channel == 0);
   CHECK(p.number == 0);

   // The value is 14 bits, and a data entry of 2 sets the top seven.
   CHECK(p.value == (2 << 7));
}

TEST_CASE("The least significant half refines the value")
{
   fixture f;
   f.select_rpn(0, 0);
   f.cc(0, data_msb, 2);
   f.cc(0, data_lsb, 50);

   REQUIRE(f._rec._params.size() == 2);
   CHECK(f._rec._params[0].value == (2 << 7));
   CHECK(f._rec._params[1].value == ((2 << 7) | 50));
   CHECK(f._rec._params[1].number == 0);
}

TEST_CASE("A number spanning both halves is assembled")
{
   fixture f;
   f.select_rpn(0, 0x2045);        // msb 64, lsb 69
   f.cc(0, data_msb, 1);

   REQUIRE(f._rec._params.size() == 1);
   CHECK(f._rec._params.front().number == 0x2045);
}

TEST_CASE("An unregistered parameter is reported as its own kind")
{
   fixture f;
   f.select_nrpn(3, 1000);
   f.cc(3, data_msb, 64);

   REQUIRE(f._rec._params.size() == 1);
   auto const& p = f._rec._params.front();
   CHECK_FALSE(p.registered);
   CHECK(p.channel == 3);
   CHECK(p.number == 1000);
   CHECK(p.value == (64 << 7));
}

TEST_CASE("The selection persists across data entries")
{
   // A controller sweeping a parameter sends the number once and then a run
   // of data entries.
   fixture f;
   f.select_rpn(0, 5);
   for (std::uint8_t v = 0; v != 4; ++v)
      f.cc(0, data_msb, v);

   REQUIRE(f._rec._params.size() == 4);
   for (std::size_t i = 0; i != 4; ++i)
   {
      CHECK(f._rec._params[i].number == 5);
      CHECK(f._rec._params[i].value == (i << 7));
   }
}

TEST_CASE("The null parameter number ends the selection")
{
   // 127/127 is the spec's way of saying stop: a stray data entry after it
   // must not land on the parameter that was selected before.
   fixture f;
   f.select_rpn(0, 0);
   f.cc(0, data_msb, 2);
   f.select_rpn(0, 0x3FFF);      // both halves 127
   f.cc(0, data_msb, 99);

   CHECK(f._rec._params.size() == 1);
}

TEST_CASE("A data entry with nothing selected is ignored")
{
   fixture f;
   f.cc(0, data_msb, 64);
   f.cc(0, data_lsb, 64);

   CHECK(f._rec._params.empty());

   // Ignored, not passed through: a data entry is still a parameter
   // controller, whether or not it lands anywhere.
   CHECK(f._rec._ccs.empty());
}

TEST_CASE("Channels keep their own selection")
{
   fixture f;
   f.select_rpn(0, 1);
   f.select_nrpn(1, 500);
   f.cc(1, data_msb, 10);
   f.cc(0, data_msb, 20);

   REQUIRE(f._rec._params.size() == 2);
   CHECK_FALSE(f._rec._params[0].registered);
   CHECK(f._rec._params[0].channel == 1);
   CHECK(f._rec._params[0].number == 500);

   CHECK(f._rec._params[1].registered);
   CHECK(f._rec._params[1].channel == 0);
   CHECK(f._rec._params[1].number == 1);
}

TEST_CASE("Selecting again replaces what was selected")
{
   fixture f;
   f.select_rpn(0, 1);
   f.select_nrpn(0, 2);
   f.cc(0, data_msb, 7);

   REQUIRE(f._rec._params.size() == 1);
   CHECK_FALSE(f._rec._params.front().registered);
   CHECK(f._rec._params.front().number == 2);
}

TEST_CASE("Increment and decrement step the value")
{
   fixture f;
   f.select_rpn(0, 0);
   f.cc(0, data_msb, 2);         // value 256
   f.cc(0, data_inc, 0);
   f.cc(0, data_inc, 0);
   f.cc(0, data_dec, 0);

   REQUIRE(f._rec._params.size() == 4);
   CHECK(f._rec._params[1].value == (2 << 7) + 1);
   CHECK(f._rec._params[2].value == (2 << 7) + 2);
   CHECK(f._rec._params[3].value == (2 << 7) + 1);
}

TEST_CASE("Stepping stops at the ends of the range")
{
   fixture f;
   f.select_rpn(0, 0);
   f.cc(0, data_msb, 0);
   f.cc(0, data_dec, 0);         // already at zero

   REQUIRE(f._rec._params.size() == 2);
   CHECK(f._rec._params[1].value == 0);

   f.cc(0, data_msb, 127);
   f.cc(0, data_lsb, 127);       // 16383, the top
   f.cc(0, data_inc, 0);

   CHECK(f._rec._params.back().value == 16383);
}

TEST_CASE("Everything else passes through untouched")
{
   fixture f;
   f.select_rpn(0, 0);
   f.cc(0, 7, 100);              // channel volume, mid-selection
   f.send(midi::note_on{0, 60, 100});
   f.cc(0, data_msb, 2);

   // The volume reached the processor, and the note did too.
   REQUIRE(f._rec._ccs.size() == 1);
   CHECK(f._rec._ccs.front() == 7);
   CHECK(f._rec._notes == 1);

   // And neither disturbed the parameter waiting to be set.
   REQUIRE(f._rec._params.size() == 1);
   CHECK(f._rec._params.front().number == 0);
}
