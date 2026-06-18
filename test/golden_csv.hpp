/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the Boost Software License, Version 1.0.
   [ https://www.boost.org/LICENSE_1_0.txt ]
=============================================================================*/
#if !defined(Q_TEST_GOLDEN_CSV_HPP)
#define Q_TEST_GOLDEN_CSV_HPP

// Generic golden-CSV comparison with per-column auto-computed tolerances.
//
// A golden is a CSV with three sections:
//   Row 1: column names (header)
//   Row 2: per-column tolerances (written by write_golden_csv, read back by compare)
//   Row 3+: data
//
// Tolerances are computed from the golden data itself:
//   tol = max(acoustic_floor, 0.001 * (max - min))   over non-sentinel values
//
// This makes each test self-calibrating: a test with a stable signal (small
// range) gets a tight tolerance; a variable signal (wide range) gets a
// proportionally looser one — but always at least the acoustic_floor, which
// represents the minimum audibly meaningful difference for that column type.
//
// A column may declare a SENTINEL (an exact "state" value, e.g. 0 for
// "no pitch lock", -999 for "ungated"): rows where both sides hold the
// sentinel match; rows where only one side does are STATE MISMATCHES,
// allowed up to a small fraction (boundary windows flip by a sample or two
// across platforms — that is jitter, not regression).
//
// WAV generation is suppressed when the CI environment variable is set
// (GitHub Actions sets CI=true automatically). Use suppress_wav() to guard
// every wav_writer block.
//
// Workflow: each covered case writes results/golden/<name>.csv. When
// test/golden/<name>.csv exists, it is compared; when absent, the case
// reports how to mint (copy the results file into test/golden/ after a
// listening pass — the git diff of a golden IS the re-blessing review
// artifact).

#include <infra/catch.hpp>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <limits>
#include <numeric>
#include <sstream>
#include <string>
#include <vector>

namespace q_test
{
   struct golden_column
   {
      std::string name;
      double      acoustic_floor = 0.0;   // minimum tolerance, in column units
      bool        has_sentinel   = false;
      double      sentinel       = 0.0;
      // Values below silence_floor are inaudible and excluded from numeric
      // comparison (and from the tolerance range). Default disabled.
      // For a dB column this is a quiet-tail threshold: below it, a steep
      // decay shifting by a fraction of a window across platforms reads as a
      // large dB swing that is musically meaningless. Rows where both sides
      // are below match; rows where only one side is below are state flips.
      double      silence_floor  = -1e30;
   };

   using golden_row = std::vector<double>;

   // Returns true when WAV output should be skipped (CI environment).
   inline bool suppress_wav()
   {
      return std::getenv("CI") != nullptr;
   }

   // Returns true in normal/CI mode. Set DEEP_TEST=1 for full diagnostics.
   inline bool quick_test()
   {
      return std::getenv("DEEP_TEST") == nullptr;
   }

   // Compute per-column tolerances from the data.
   // tol[c] = max(col.acoustic_floor, 0.001 * (max - min)) over non-sentinel values.
   inline std::vector<double> compute_tolerances(
      std::vector<golden_column> const& cols,
      std::vector<golden_row>    const& rows)
   {
      std::vector<double> tols(cols.size());
      for (std::size_t c = 0; c != cols.size(); ++c)
      {
         auto const& col = cols[c];
         double lo =  std::numeric_limits<double>::max();
         double hi = -std::numeric_limits<double>::max();
         for (auto const& r : rows)
         {
            if (c >= r.size()) continue;
            double v = r[c];
            if (col.has_sentinel && v == col.sentinel) continue;
            if (v < col.silence_floor) continue;   // inaudible tail
            lo = std::min(lo, v);
            hi = std::max(hi, v);
         }
         double range_tol = (lo <= hi) ? 0.001 * (hi - lo) : 0.0;
         tols[c] = std::max(col.acoustic_floor, range_tol);
      }
      return tols;
   }

   inline void write_golden_csv(
      std::string               const& path,
      std::vector<golden_column> const& cols,
      std::vector<golden_row>    const& rows)
   {
      auto tols = compute_tolerances(cols, rows);

      // Goldens are grouped into per-test subdirs (golden/<test>/...); create
      // the target directory so the result write does not fail.
      if (auto dir = std::filesystem::path(path).parent_path(); !dir.empty())
         std::filesystem::create_directories(dir);

      std::ofstream out(path);
      // Header row
      for (std::size_t c = 0; c != cols.size(); ++c)
         out << (c ? ", " : "") << cols[c].name;
      out << '\n';
      // Tolerance row
      out << std::fixed << std::setprecision(6);
      for (std::size_t c = 0; c != tols.size(); ++c)
         out << (c ? ", " : "") << tols[c];
      out << '\n';
      // Data rows
      out << std::fixed << std::setprecision(4);
      for (auto const& r : rows)
      {
         for (std::size_t c = 0; c != r.size(); ++c)
            out << (c ? ", " : "") << r[c];
         out << '\n';
      }
   }

   struct golden_data
   {
      std::vector<double>     tolerances;
      std::vector<golden_row> rows;
   };

   inline golden_data read_golden_csv(std::string const& path)
   {
      golden_data gd;
      std::ifstream in(path);
      if (!in) return gd;
      std::string line;

      std::getline(in, line);   // header row (discard)

      auto parse_row = [](std::string const& s) {
         golden_row row;
         std::istringstream ss(s);
         std::string cell;
         while (std::getline(ss, cell, ','))
            row.push_back(std::stod(cell));
         return row;
      };

      if (!std::getline(in, line)) return gd;
      gd.tolerances = parse_row(line);   // tolerance row

      while (std::getline(in, line))
      {
         auto row = parse_row(line);
         if (!row.empty())
            gd.rows.push_back(std::move(row));
      }
      return gd;
   }

   // Compare result rows against golden/<name>.csv.
   // max_state_mismatch is the allowed fraction of rows, per column,
   // where exactly one side sits on the column's sentinel.
   inline void compare_golden_csv(
      std::string                const& name,
      std::vector<golden_column>  const& cols,
      std::vector<golden_row>     const& result,
      float                       max_state_mismatch = 0.02f)
   {
      auto gd = read_golden_csv("golden/" + name + ".csv");
      if (gd.rows.empty())
      {
         WARN("golden/" << name << ".csv missing — to mint: copy "
            "results/golden/" << name << ".csv into test/golden/ "
            "(after a listening pass) and rebuild.");
         return;
      }

      // Boundary windows can shift by 1-2 rows across platforms due to
      // floating-point rounding in sps/window_size. The tail is near-silent,
      // so compare only the overlap and allow up to 3 rows of drift.
      {
         int diff = int(result.size()) - int(gd.rows.size());
         INFO("row count: result=" << result.size()
              << " golden=" << gd.rows.size()
              << " diff=" << diff);
         CHECK(std::abs(diff) <= 3);
      }
      auto n_rows = std::min(result.size(), gd.rows.size());

      std::vector<int> state_mismatches(cols.size(), 0);
      int              numeric_failures = 0;
      std::ostringstream detail;

      for (std::size_t i = 0; i != n_rows; ++i)
      {
         REQUIRE(result[i].size() == cols.size());
         REQUIRE(gd.rows[i].size() == cols.size());
         for (std::size_t c = 0; c != cols.size(); ++c)
         {
            auto const& col = cols[c];
            double a   = result[i][c];
            double b   = gd.rows[i][c];
            double tol = (c < gd.tolerances.size())
                       ? gd.tolerances[c]
                       : col.acoustic_floor;

            if (col.has_sentinel)
            {
               bool sa = (a == col.sentinel);
               bool sb = (b == col.sentinel);
               if (sa && sb) continue;
               if (sa != sb) { ++state_mismatches[c]; continue; }
            }
            // Inaudible tail: both quiet -> match; one quiet, one not ->
            // a boundary flip (counted like a state mismatch), not a
            // numeric regression.
            {
               bool qa = (a < col.silence_floor);
               bool qb = (b < col.silence_floor);
               if (qa && qb) continue;
               if (qa != qb) { ++state_mismatches[c]; continue; }
            }
            if (std::abs(a - b) > tol)
            {
               if (++numeric_failures <= 10)
                  detail << "\n  row " << i << " (t=" << result[i][0]
                         << "s) col '" << col.name << "': got " << a
                         << ", golden " << b << " (tol=" << tol << ')';
            }
         }
      }

      INFO("golden mismatch in '" << name << "':" << detail.str());
      CHECK(numeric_failures == 0);

      for (std::size_t c = 0; c != cols.size(); ++c)
      {
         INFO("column '" << cols[c].name << "' state flips: "
            << state_mismatches[c] << "/" << result.size());
         CHECK(state_mismatches[c]
            <= int(max_state_mismatch * result.size()));
      }
   }

   // Windowed-level helper: compute windowed RMS (dB) of each channel
   // from a flat interleaved float buffer. Returns one golden_row per window:
   //   { time_s, ch0_db, ch1_db, ... }
   // Silent windows (all zero) report -999.0 (sentinel).
   inline std::vector<golden_row> windowed_level_csv(
      std::vector<float> const& audio,
      int                       n_channels,
      float                     sps,
      float                     window_s = 0.010f)
   {
      constexpr double sentinel_db = -999.0;
      int win = std::max(1, int(window_s * sps));
      int n_samples = int(audio.size()) / n_channels;
      int n_windows = n_samples / win;

      std::vector<golden_row> rows;
      rows.reserve(n_windows);

      for (int w = 0; w != n_windows; ++w)
      {
         golden_row row;
         row.push_back(double(w * win) / sps);   // time_s
         for (int ch = 0; ch != n_channels; ++ch)
         {
            double sum2 = 0.0;
            for (int s = 0; s != win; ++s)
            {
               double v = audio[(w * win + s) * n_channels + ch];
               sum2 += v * v;
            }
            double rms = std::sqrt(sum2 / win);
            row.push_back(rms < 1e-10 ? sentinel_db
                                       : 20.0 * std::log10(rms));
         }
         rows.push_back(std::move(row));
      }
      return rows;
   }

   // Build golden_column descriptors for windowed-level CSVs.
   // n_channels output channels get column names ch0_db, ch1_db, ...
   inline std::vector<golden_column> level_columns(int n_channels)
   {
      std::vector<golden_column> cols;
      cols.push_back({"time_s", 0.0});   // exact time index, no floor needed
      for (int ch = 0; ch != n_channels; ++ch)
      {
         std::string n = "ch" + std::to_string(ch) + "_db";
         // floor=0.25 dB (absorbs cross-platform FP jitter at audible levels),
         // sentinel=-999 (digital silence), silence_floor=-35 dB (inaudible
         // decay tail excluded: sub-window timing jitter on a steep decay
         // shows up there as large, meaningless dB swings).
         cols.push_back({n, 0.25, true, -999.0, -35.0});
      }
      return cols;
   }
}

#endif
