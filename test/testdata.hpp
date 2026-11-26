/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(Q_TEST_TESTDATA_HPP)
#define Q_TEST_TESTDATA_HPP

// Test data sets hosted on R2 (test/testdata/sets.txt). A test registered
// with q_testdata(<test> <set>) finds its set through the environment
// variable Q_TESTDATA_<SET>, which ctest sets after the fixture has fetched
// it. A test whose set is missing, offline or run by hand outside ctest,
// skips: it says why and exits with skip_code, which ctest reports as
// skipped, never as passed.

#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <string>
#include <string_view>

namespace q_test
{
   constexpr int skip_code = 77;

   // The directory of a fetched set, or an empty path when it is missing.
   inline std::filesystem::path testdata(std::string_view set)
   {
      std::string var = "Q_TESTDATA_";
      for (char c : set)
         var += char(std::toupper(static_cast<unsigned char>(c)));

      char const* dir = std::getenv(var.c_str());
      if (!dir || !*dir)
         return {};

      std::filesystem::path p{dir};
      std::error_code ec;
      if (!std::filesystem::exists(p / ".complete", ec))
         return {};
      return p;
   }

   // The set's directory; or, when it is missing, a note and an exit with
   // skip_code. Call it first thing in main.
   inline std::filesystem::path require_testdata(std::string_view set)
   {
      auto p = testdata(set);
      if (p.empty())
      {
         std::printf(
            "skipped: test data set %.*s is not available (fetched by the "
            "ctest fixture testdata_%.*s; see its output)\n"
          , int(set.size()), set.data(), int(set.size()), set.data());
         std::exit(skip_code);
      }
      return p;
   }
}

#endif
