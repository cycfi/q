/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the Boost Software License, Version 1.0.
   [ https://www.boost.org/LICENSE_1_0.txt ]
=============================================================================*/
#define CATCH_CONFIG_MAIN
#include <infra/catch.hpp>
#include <q/utility/ring_buffer.hpp>

#include <array>
#include <vector>

namespace q = cycfi::q;

TEST_CASE("ring_buffer: capacity rounds up to a power of two")
{
   CHECK(q::ring_buffer<float>(4).size() == 4);
   CHECK(q::ring_buffer<float>(5).size() == 8);
   CHECK(q::ring_buffer<float>(48).size() == 64);
   CHECK(q::ring_buffer<float>(1024).size() == 1024);
}

TEST_CASE("ring_buffer: resizable storage starts zero-initialized")
{
   q::ring_buffer<float> buf(8);
   for (std::size_t i = 0; i != buf.size(); ++i)
      CHECK(buf[i] == 0.0f);
}

TEST_CASE("ring_buffer: push, front, back, operator[]")
{
   q::ring_buffer<float> buf(4);

   for (float v : {10.0f, 20.0f, 30.0f, 40.0f})
      buf.push(v);

   // Index 0 is the newest element, size()-1 the oldest
   CHECK(buf[0] == 40.0f);
   CHECK(buf[1] == 30.0f);
   CHECK(buf[2] == 20.0f);
   CHECK(buf[3] == 10.0f);
   CHECK(buf.front() == 40.0f);
   CHECK(buf.back() == 10.0f);

   // Pushing past capacity overwrites the oldest element
   buf.push(50.0f);
   CHECK(buf.front() == 50.0f);
   CHECK(buf.back() == 20.0f);
}

TEST_CASE("ring_buffer: many pushes wrap consistently")
{
   q::ring_buffer<float> buf(8);

   // 100 pushes into 8 slots: the buffer holds the last 8, newest first
   for (auto i = 0; i != 100; ++i)
      buf.push(float(i));

   for (std::size_t i = 0; i != buf.size(); ++i)
      CHECK(buf[i] == float(99 - i));
}

TEST_CASE("ring_buffer: pop_front removes the newest element")
{
   q::ring_buffer<float> buf(4);

   for (float v : {10.0f, 20.0f, 30.0f})
      buf.push(v);

   CHECK(buf.front() == 30.0f);
   buf.pop_front();
   CHECK(buf.front() == 20.0f);
   buf.pop_front();
   CHECK(buf.front() == 10.0f);
}

TEST_CASE("ring_buffer: clear zeroes the contents")
{
   q::ring_buffer<float> buf(4);

   for (float v : {10.0f, 20.0f, 30.0f, 40.0f})
      buf.push(v);

   buf.clear();
   for (std::size_t i = 0; i != buf.size(); ++i)
      CHECK(buf[i] == 0.0f);
}

TEST_CASE("ring_buffer: fixed std::array storage")
{
   // Fixed-size storage uses the default constructor; the size must be a
   // power of two (enforced by static_assert).
   q::ring_buffer<float, std::array<float, 8>> buf;
   CHECK(buf.size() == 8);

   for (float v : {1.0f, 2.0f, 3.0f})
      buf.push(v);

   CHECK(buf.front() == 3.0f);
   CHECK(buf[1] == 2.0f);
   CHECK(buf[2] == 1.0f);
}

TEST_CASE("ring_buffer: store gives raw access to the storage")
{
   q::ring_buffer<float> buf(4);
   CHECK(buf.store().size() == buf.size());

   buf.push(42.0f);
   auto const& cbuf = buf;
   CHECK(cbuf.store().size() == buf.size());
}
