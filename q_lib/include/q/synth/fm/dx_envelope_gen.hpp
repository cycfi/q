/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(CYCFI_Q_DX_ENVELOPE_GEN_HPP_SEPTEMBER_15_2026)
#define CYCFI_Q_DX_ENVELOPE_GEN_HPP_SEPTEMBER_15_2026

#include <q/support/base.hpp>
#include <q/support/decibel.hpp>
#include <q/support/duration.hpp>
#include <algorithm>
#include <cmath>
#include <cstdint>

namespace cycfi::q
{
   ////////////////////////////////////////////////////////////////////////////
   // dx_envelope_gen: a four rate, four level envelope in the log domain.
   // The four-and-four shape is the DX7's, and so is the attack curve
   // (note 1), but neither is peculiar to it: the shape is a useful
   // envelope for anything, in ideal units. The constants below are the
   // DX7's; the patch's own 0..99 units are dx_patcher's business.
   //
   // Levels are decibels, 0 dB full and negative below. A rate is a
   // slope, given as the time a segment would take to travel the
   // envelope's whole range, floor to full: a segment covering less of it
   // takes proportionally less time. attack() runs R1 to L1, R2 to L2, R3
   // to L3 and holds there; release() runs R4 to L4 from wherever it is,
   // then goes idle once silent (or holds at L4 if L4 is audible). Both
   // start from the current level, so a retrigger or an early key-off is
   // click-free. operator() returns the linear gain, times a master
   // volume, `gain`, for anything that scales the output as a whole.
   //
   // Note 1, the attack curve, as measured on a DX7s (Music Synthesizer
   // for Android wiki): a decay falls at its rate, but an attack climbs
   // faster, by its rate times 2 plus the number of doublings (6 dB) it
   // is below full, so it is fast at first and slows toward the top, and
   // from silence it starts at attack_from rather than crawling up from
   // the floor. A plain constant-slope attack in the log domain sounds
   // sluggish; this is why. Levels below floor are silent.
   //
   // T is the type of the level and its steps. dx_envelope_gen is double: a
   // slow rate moves a small fraction of a dB per sample, which float
   // rounds coarsely near the floor.
   ////////////////////////////////////////////////////////////////////////////
   template <typename T>
   struct basic_dx_envelope_gen
   {
      // Silent below floor; an attack from silence starts at attack_from;
      // the range is what a rate's time covers; fastest is the DX7's
      static constexpr decibel floor = dB(-89.93);
      static constexpr decibel attack_from = dB(-49.95);
      static constexpr decibel range = dB(89.93);
      static constexpr duration fastest = duration{0.00556};

      // An envelope that is at once at full level, and silent at release
      struct config
      {
         duration       rate[4] = {fastest, fastest, fastest, fastest};
         decibel        level[4] = {dB(0), dB(0), dB(0), floor};
      };

      enum phase_index : std::uint8_t
      {
         r1, r2, r3, sustain, release_phase, idle
      };

                     basic_dx_envelope_gen();
                     basic_dx_envelope_gen(config const& cfg, float sps);

      void           set(config const& cfg, float sps);

      void           attack();
      void           release();
      void           reset();                      // idle and silent

      void           gain(float g)              { _gain = g; }
      float          gain() const               { return _gain; }

      float          operator()();
      float          level() const              { return float(_db); }
      std::size_t    index() const              { return _i; }
      bool           in_idle_phase() const      { return _i == idle; }
      bool           in_release_phase() const   { return _i == release_phase; }

   private:

      void           start(std::size_t i);
      void           advance();

      T              _target[4];                   // dB
      T              _step[4];                     // dB per sample
      T              _db = T(floor.rep);
      float          _gain = 1.0f;
      bool           _rising = false;
      std::size_t    _i = idle;
   };

   using dx_envelope_gen = basic_dx_envelope_gen<double>;

   ////////////////////////////////////////////////////////////////////////////
   // Implementation
   ////////////////////////////////////////////////////////////////////////////
   template <typename T>
   inline basic_dx_envelope_gen<T>::basic_dx_envelope_gen()
    : basic_dx_envelope_gen{config{}, 44100.0f}
   {}

   template <typename T>
   inline basic_dx_envelope_gen<T>::basic_dx_envelope_gen(
      config const& cfg, float sps)
   {
      set(cfg, sps);
   }

   template <typename T>
   inline void basic_dx_envelope_gen<T>::set(config const& cfg, float sps)
   {
      for (std::size_t i = 0; i != 4; ++i)
      {
         _step[i] = T(range.rep / (as_double(cfg.rate[i]) * sps));
         _target[i] = T(std::max(cfg.level[i].rep, floor.rep));
      }
   }

   template <typename T>
   inline void basic_dx_envelope_gen<T>::attack()
   {
      start(r1);
   }

   template <typename T>
   inline void basic_dx_envelope_gen<T>::release()
   {
      if (!in_idle_phase())
         start(release_phase);
   }

   template <typename T>
   inline void basic_dx_envelope_gen<T>::reset()
   {
      _i = idle;
      _db = T(floor.rep);
   }

   // Enter segment i from the current level. A segment that is already at
   // its target is skipped, so the envelope moves on the next sample.
   template <typename T>
   inline void basic_dx_envelope_gen<T>::start(std::size_t i)
   {
      _i = i;
      if (i == sustain || i == idle)
         return;

      auto target = _target[i == release_phase ? 3 : i];
      _rising = target > _db;
      if (_rising && _db < T(attack_from.rep))
         _db = std::min(T(attack_from.rep), target);
      if (_db == target)
         advance();
   }

   // After R3, hold at L3. After R4, idle once silent, else hold at L4.
   template <typename T>
   inline void basic_dx_envelope_gen<T>::advance()
   {
      if (_i == release_phase)
      {
         if (_db <= T(floor.rep))
            _i = idle;
      }
      else
      {
         start(_i == r3 ? sustain : _i + 1);
      }
   }

   template <typename T>
   inline float basic_dx_envelope_gen<T>::operator()()
   {
      if (_i == idle)
         return 0.0f;

      auto k = _i == release_phase ? 3 : _i;
      if (_i != sustain && _db != _target[k])
      {
         auto target = _target[k];
         if (_rising)
         {
            auto below = std::floor(-_db / T(6.0206));      // doublings
            auto factor = std::max(T(1), T(2) + below);
            _db = std::min(_db + _step[k] * factor, target);
         }
         else
         {
            _db = std::max(_db - _step[k], target);
         }
         if (_db == target)
            advance();
      }
      if (_i == idle || _db <= T(floor.rep))
         return 0.0f;
      return lin_float(dB(float(_db))) * _gain;
   }
}

#endif
