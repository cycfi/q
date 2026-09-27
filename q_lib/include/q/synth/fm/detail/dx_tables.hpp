/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the Boost Software License, Version 1.0.
   [ https://www.boost.org/LICENSE_1_0.txt ]
=============================================================================*/
#if !defined(CYCFI_Q_DX_TABLES_HPP_SEPTEMBER_15_2026)
#define CYCFI_Q_DX_TABLES_HPP_SEPTEMBER_15_2026

#include <cstdint>

namespace cycfi::q::detail
{
   ////////////////////////////////////////////////////////////////////////////
   // DX7 parameter tables, where a measured table is faster or more
   // faithful than a formula. Measured on the reference emulation at every
   // step (September 2026, see the KB: q/fm-synthesis), except the output
   // levels below 20, from the published DX7s measurements.
   ////////////////////////////////////////////////////////////////////////////

   // Output level 0..19 to its internal units; 20..99 is 28 + level
   constexpr int dx_output_level_low[20] =
   {
         0, 5, 9, 13, 17, 20, 23, 25, 27, 29,
         31, 33, 35, 37, 39, 41, 42, 43, 45, 46
   };

   // Velocity 1, 4, 7, .., 127 to dB at sensitivity 7 (43 entries). The
   // first entry is the reference's floor.
   constexpr float dx_velocity_db[43] =
   {
         -76.16f, -50.09f, -46.67f, -41.01f, -38.75f, -35.00f,
         -33.11f, -29.73f,
         -28.60f, -25.96f, -24.84f, -22.58f, -21.45f, -19.94f,
         -18.81f, -17.31f,
         -16.18f, -14.67f, -13.92f, -12.79f, -12.04f, -10.54f,
         -9.78f, -7.90f,
         -7.53f, -6.02f, -5.27f, -4.52f, -3.76f, -2.63f, -2.26f, -1.13f,
         -0.38f, 0.38f, 0.75f, 1.51f, 1.88f, 2.63f, 3.01f, 3.76f,
         4.14f, 4.89f, 5.27f
   };

   // LFO speed 0..99 to Hz
   constexpr float dx_lfo_hz[100] =
   {
         0.106f, 0.130f, 0.322f, 0.452f, 0.645f, 0.776f, 0.967f, 1.163f,
         1.291f, 1.484f, 1.613f, 1.810f, 1.937f, 2.131f, 2.322f, 2.453f,
         2.645f, 2.776f, 2.969f, 3.098f, 3.294f, 3.482f, 3.616f, 3.804f,
         3.939f, 4.134f, 4.325f, 4.454f, 4.651f, 4.777f, 4.974f, 5.094f,
         5.292f, 5.492f, 5.615f, 5.804f, 5.941f, 6.144f, 6.264f, 6.456f,
         6.645f, 6.782f, 6.973f, 7.096f, 7.286f, 7.491f, 7.620f, 7.807f,
         7.943f, 8.139f, 8.259f, 8.453f, 8.650f, 8.787f, 8.973f, 9.103f,
         9.285f, 9.425f, 9.609f, 9.816f, 9.927f, 10.123f, 10.280f, 10.453f,
         11.625f, 11.762f, 12.980f, 14.130f, 14.366f, 15.593f,
         16.910f, 17.191f,
         18.463f, 19.831f, 20.070f, 21.525f, 21.733f, 23.249f,
         24.800f, 25.065f,
         26.601f, 28.076f, 28.494f, 30.017f, 31.712f, 32.227f,
         33.742f, 35.456f,
         35.825f, 37.625f, 39.465f, 39.830f, 41.728f, 42.102f,
         44.040f, 45.797f,
         46.373f, 48.325f, 50.203f, 50.896f
   };

   // Pitch envelope level 0..99 to its pitch, in 1/32 octave
   constexpr std::int8_t dx_pitch_eg_32nds[100] =
   {
         -128, -116, -104, -95, -85, -76, -68, -61, -56, -52, -49, -46,
         -43, -41, -39, -37, -35, -33, -32, -31, -30, -29, -28, -27,
         -26, -25, -24, -23, -22, -21, -20, -19, -18, -17, -16, -15,
         -14, -13, -12, -11, -10, -9, -8, -7, -6, -5, -4, -3,
         -2, -1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,
         10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21,
         22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33,
         34, 35, 38, 40, 43, 46, 49, 53, 58, 65, 73, 82,
         92, 103, 115, 127
   };

   // Pitch envelope rate 0..99 to its speed, semitones per second
   constexpr float dx_pitch_rate[100] =
   {
         0.563f, 1.127f, 1.690f, 1.690f, 2.254f, 2.254f, 2.817f, 2.817f,
         3.384f, 3.384f, 3.948f, 3.948f, 4.512f, 4.512f, 5.076f, 5.076f,
         5.645f, 5.645f, 6.205f, 6.205f, 6.767f, 6.767f, 7.334f, 7.334f,
         7.912f, 7.912f, 8.467f, 9.034f, 9.034f, 9.595f, 10.180f, 10.180f,
         10.729f, 11.306f, 11.881f, 12.452f, 13.022f, 13.585f,
         14.144f, 14.697f,
         15.245f, 15.856f, 17.011f, 17.543f, 18.677f, 19.294f,
         20.414f, 21.031f,
         21.518f, 22.134f, 23.223f, 23.834f, 24.900f, 26.126f,
         26.749f, 27.772f,
         29.008f, 30.001f, 30.619f, 31.843f, 33.081f, 34.018f,
         35.248f, 36.485f,
         37.725f, 38.565f, 39.812f, 41.038f, 42.282f, 43.540f,
         44.859f, 46.728f,
         48.585f, 50.464f, 52.351f, 53.471f, 55.962f, 58.458f,
         60.958f, 63.466f,
         65.538f, 68.623f, 71.750f, 74.886f, 78.110f, 81.869f,
         83.841f, 87.662f,
         91.442f, 95.130f, 98.939f, 103.428f, 107.930f, 113.053f,
         118.898f, 121.136f,
         134.363f, 141.425f, 148.563f, 149.267f
   };

   // Pitch modulation sensitivity 0..7 to semitones at full depth
   constexpr float dx_pm_sens[8] =
   {
         0.000f, 0.470f, 0.937f, 1.545f, 2.571f, 4.301f, 7.142f, 11.914f
   };

   // Amplitude modulation sensitivity 0..3: the cut at the LFO's bottom is
   // a (e^(k depth) - 1) dB, depth 0..99
   struct dx_am_law
   {
      float a, k;
   };

   constexpr dx_am_law dx_am_sens[4] =
   {
      {0.0f, 0.0f},
      {1.06770f, 0.011637f},
      {1.05589f, 0.019320f},
      {1.04982f, 0.045060f}
   };

   // Exponential keyboard scaling at depth 99, in output level units, by
   // distance in groups of 3 notes, 0..17
   constexpr float dx_ex_scaling[18] =
   {
         0.0f, 0.0f, 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f,
         8.0f, 10.0f, 13.0f, 15.0f, 18.0f, 22.0f, 26.0f, 32.0f, 38.0f
   };
}

#endif
