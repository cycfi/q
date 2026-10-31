###############################################################################
#  Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.
#
#  Distributed under the MIT License (https://opensource.org/licenses/MIT)
###############################################################################
# Micro-benchmarks. These are build-only: they measure and print, with no
# pass/fail assertions, so they are NOT registered with add_test (nothing for
# CI to check). Build with the rest of the tree; run by hand to recalibrate
# the figures that quote their numbers.
###############################################################################

set(BENCH_SOURCES
   decibel_bench.cpp
   fast_trig_bench.cpp
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

# The FFT benchmark compares two builds of the same transforms, so it is
# two translation units: fft_bench_native.cpp builds without Q_USE_PFFFT.
add_executable(fft_bench fft_bench.cpp fft_bench_native.cpp)
target_link_libraries(fft_bench libq libqio)
