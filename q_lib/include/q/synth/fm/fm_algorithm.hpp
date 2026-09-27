/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the Boost Software License, Version 1.0.
   [ https://www.boost.org/LICENSE_1_0.txt ]
=============================================================================*/
#if !defined(CYCFI_Q_FM_ALGORITHM_HPP_SEPTEMBER_15_2026)
#define CYCFI_Q_FM_ALGORITHM_HPP_SEPTEMBER_15_2026

#include <q/support/base.hpp>
#include <q/support/phase.hpp>
#include <q/synth/fm/fm_routing.hpp>
#include <bit>
#include <cstdint>
#include <tuple>

namespace cycfi::q
{
   ////////////////////////////////////////////////////////////////////////////
   // fm_algorithm: runs a set of operators through an fm_routing, which
   // says what modulates what and, from that, which of them sound (see
   // fm_routing, where the DX7's 32 algorithms are). The operators are a
   // tuple, so each can be a different oscillator, and one pass from the
   // last down to 0 has each one's modulators ready.
   //
   // Modulation is phase modulation. A modulator's output, its waveform
   // times its envelope gain, is scaled by `index`, the peak phase
   // deviation in radians of a full level modulator, and offsets the phase
   // of what it modulates. Feedback goes the other way, from the last two
   // outputs of its source, averaged (the usual cure for self-modulation
   // instability), scaled by `feedback` in radians, into its destination;
   // the source can be the destination itself. The carriers are summed as
   // they are.
   ////////////////////////////////////////////////////////////////////////////
   struct fm_algorithm
   {
      struct config
      {
         fm_routing     route;                  // nothing modulated: all sound
         float          index = 1.0f;           // radians per unit of modulator
         float          feedback = 0.0f;        // radians per unit of source
      };

                     fm_algorithm();
                     fm_algorithm(config const& cfg);

      void           set(config const& cfg);
      void           sync();                       // clear the feedback

      // Run the operators at the master pitch, setting each one's
      // modulation, and return the sum of the carriers.
                     template <typename Op, typename... RestOps>
      float          operator()(
                        std::tuple<Op, RestOps...>& op
                      , phase_iterator master
                     );

      bool           is_carrier(std::size_t i) const;
      std::size_t    num_carriers() const;

      // A phase deviation in radians as a phase offset, wrapping past a
      // cycle either way.
      static constexpr phase deviation(float radians);

   private:

                     template <std::size_t I, typename Tuple, std::size_t N>
      void           run(
                        Tuple& op, phase_iterator master
                      , float (&out)[N], float& sum
                     );

      using routing_table = fm_routing::table;

      routing_table  _r;
      float          _index;
      float          _fb;           // feedback gain, halved for the average
      float          _y1 = 0.0f;    // the feedback source's last two outputs
      float          _y2 = 0.0f;
   };

   ////////////////////////////////////////////////////////////////////////////
   // Implementation
   ////////////////////////////////////////////////////////////////////////////
   inline fm_algorithm::fm_algorithm()
    : fm_algorithm{config{}}
   {}

   inline fm_algorithm::fm_algorithm(config const& cfg)
   {
      set(cfg);
   }

   inline void fm_algorithm::set(config const& cfg)
   {
      _r = cfg.route.get();
      _index = cfg.index;
      _fb = cfg.feedback / 2;
   }

   inline void fm_algorithm::sync()
   {
      _y1 = _y2 = 0.0f;
   }

   inline constexpr phase fm_algorithm::deviation(float radians)
   {
      // Radians as a fraction of a cycle, scaled to the phase's fixed
      // point range. The deviation may span several cycles, so go through
      // a 64-bit integer and let the conversion to 32 bits wrap.
      constexpr float k = phase::scale<float> / (2 * pi);
      return phase{std::uint32_t(std::int64_t(radians * k)), direct_unit};
   }

   // Operator I, after those above it
   template <std::size_t I, typename Tuple, std::size_t N>
   inline void fm_algorithm::run(
      Tuple& op, phase_iterator master, float (&out)[N], float& sum)
   {
      auto mod = 0.0f;
      for (auto m = _r.mod[I]; m; m &= m - 1)
         mod += out[std::countr_zero(m)];
      auto radians = mod * _index;
      if (I == _r.fb_dst)
         radians += (_y1 + _y2) * _fb;

      auto& o = std::get<I>(op);
      o.modulation(deviation(radians));
      out[I] = o(master);
      if (_r.carriers & (1 << I))
         sum += out[I];
      if constexpr (I > 0)
         run<I - 1>(op, master, out, sum);
   }

   template <typename Op, typename... RestOps>
   inline float fm_algorithm::operator()(
      std::tuple<Op, RestOps...>& op, phase_iterator master)
   {
      constexpr auto n = 1 + sizeof...(RestOps);
      static_assert(n <= fm_max_operators, "too many operators");
      float out[n];
      float sum = 0.0f;
      run<n - 1>(op, master, out, sum);

      _y2 = _y1;
      _y1 = _r.fb_src < n ? out[_r.fb_src] : 0.0f;
      return sum;
   }

   inline bool fm_algorithm::is_carrier(std::size_t i) const
   {
      return _r.carriers & (1 << i);
   }

   inline std::size_t fm_algorithm::num_carriers() const
   {
      return std::popcount(_r.carriers);
   }
}

#endif
