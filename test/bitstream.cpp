/*=============================================================================
   Copyright (c) 2014-2018 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <infra/doctest.hpp>
#include <q/support/literals.hpp>
#include <q/utility/bitstream.hpp>

namespace q = cycfi::q;

TEST_CASE("Test_bitstream_32")
{
   q::bitstream<std::uint32_t> bs{ 100 };

   CHECK(bs.size() == 128);   // nearest power 2

   bs.set(3, true);
   CHECK(bs.data()[0] == 8);
   CHECK(bs.get(3));

   bs.set(8, 32, true);
   CHECK(bs.data()[0] == 0xFFFFFF08);
   CHECK(bs.data()[1] == 0x000000FF);
   CHECK(bs.data()[2] == 0x00000000);
   CHECK(bs.data()[3] == 0x00000000);

   bs.set(35, 25, true);
   CHECK(bs.data()[0] == 0xFFFFFF08);
   CHECK(bs.data()[1] == 0x0FFFFFFF);
   CHECK(bs.data()[2] == 0x00000000);
   CHECK(bs.data()[3] == 0x00000000);
   CHECK(bs.get(35));
   CHECK(bs.get(35+24));
   CHECK(!bs.get(35+25));

   bs.set(65, 60, true);
   CHECK(bs.data()[0] == 0xFFFFFF08);
   CHECK(bs.data()[1] == 0x0FFFFFFF);
   CHECK(bs.data()[2] == 0xFFFFFFFE);
   CHECK(bs.data()[3] == 0x1FFFFFFF);
   CHECK(bs.get(65));
   CHECK(bs.get(65+59));
   CHECK(!bs.get(65+60));

   bs.shift_half();
   CHECK(bs.data()[0] == 0xFFFFFFFE);  // Moved bs.data()[2]
   CHECK(bs.data()[1] == 0x1FFFFFFF);  // Moved bs.data()[3]

   bs.clear();
   CHECK(bs.data()[0] == 0x00000000);
   CHECK(bs.data()[1] == 0x00000000);
   CHECK(bs.data()[2] == 0x00000000);
   CHECK(bs.data()[3] == 0x00000000);

   bs.set(1, 126, true);
   CHECK(bs.data()[0] == 0xFFFFFFFE);
   CHECK(bs.data()[1] == 0xFFFFFFFF);
   CHECK(bs.data()[2] == 0xFFFFFFFF);
   CHECK(bs.data()[3] == 0x7FFFFFFF);
   CHECK(!bs.get(0));
   CHECK(bs.get(1));
   CHECK(bs.get(126));
   CHECK(!bs.get(127));
}

TEST_CASE("Test_bitstream_64")
{
   q::bitstream<std::uint64_t> bs{ 100 };

   CHECK(bs.size() == 128);   // nearest power 2

   bs.set(3, true);
   CHECK(bs.data()[0] == 8);
   CHECK(bs.get(3));

   bs.set(8, 32, true);
   CHECK(bs.data()[0] == 0x000000FFFFFFFF08);
   CHECK(bs.data()[1] == 0x0000000000000000);

   bs.set(35, 25, true);
   CHECK(bs.data()[0] == 0x0FFFFFFFFFFFFF08);
   CHECK(bs.data()[1] == 0x0000000000000000);
   CHECK(bs.get(35));
   CHECK(bs.get(35+24));

   bs.set(65, 60, true);
   CHECK(bs.data()[0] == 0x0FFFFFFFFFFFFF08);
   CHECK(bs.data()[1] == 0x1FFFFFFFFFFFFFFE);
   CHECK(bs.get(65));
   CHECK(bs.get(65+59));

   bs.shift_half();
   CHECK(bs.data()[0] == 0x1FFFFFFFFFFFFFFE);   // Moved from bs.data()[1]

   bs.clear();
   CHECK(bs.data()[0] == 0x0000000000000000);
   CHECK(bs.data()[1] == 0x0000000000000000);

   bs.set(1, 126, true);
   CHECK(bs.data()[0] == 0xFFFFFFFFFFFFFFFE);
   CHECK(bs.data()[1] == 0x7FFFFFFFFFFFFFFF);
   CHECK(!bs.get(0));
   CHECK(bs.get(1));
   CHECK(bs.get(126));
   CHECK(!bs.get(127));
}