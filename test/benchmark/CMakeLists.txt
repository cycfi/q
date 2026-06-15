###############################################################################
#  Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.
#
#  Distributed under the Boost Software License, Version 1.0.
#  [ https://www.boost.org/LICENSE_1_0.txt ]
###############################################################################
# Micro-benchmarks. These are build-only: they measure and print, with no
# pass/fail assertions, so they are NOT registered with add_test (nothing for
# CI to check). Build with the rest of the tree; run by hand to recalibrate
# the figures that quote their numbers.
###############################################################################

set(BENCH_SOURCES
   decibel_bench.cpp
   interpolation_bench.cpp
   log2_bench.cpp
   sin_bench.cpp
   soft_clip_bench.cpp
)

foreach(benchsourcefile ${BENCH_SOURCES})
   string(REPLACE ".cpp" "" benchname ${benchsourcefile})
   add_executable(${benchname} ${benchsourcefile})
   target_link_libraries(${benchname} libq libqio)
endforeach(benchsourcefile ${BENCH_SOURCES})
