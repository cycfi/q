/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(CYCFI_Q_FM_OPERATOR_HPP_SEPTEMBER_15_2026)
#define CYCFI_Q_FM_OPERATOR_HPP_SEPTEMBER_15_2026

#include <q/support/phase.hpp>
#include <q/synth/concepts.hpp>
#include <q/synth/sin_osc.hpp>
#include <q/synth/fm/dx_envelope_gen.hpp>
#include <cstdint>

namespace cycfi::q
{
   ////////////////////////////////////////////////////////////////////////////
   // basic_fm_operator: one FM operator, an oscillator with its own phase
   // and envelope, both template parameters; fm_operator is the one with a
   // sine and the DX7 shaped envelope. Its pitch is a
   // ratio of the master phase iterator it is given per sample, or a fixed
   // step that ignores the master. Modulation is phase modulation: the
   // caller sets the phase offset for this sample, a property, so the
   // call is the oscillator's, o(master). The gain is the envelope's
   // master volume (amplitude modulation, for one).
   //
   // The operator follows the master's step, not its phase: with any
   // non-integer ratio, its phase runs on its own, restarted by sync().
   // Its output level is in its envelope's levels.
   ////////////////////////////////////////////////////////////////////////////
   template <
      concepts::Oscillator Osc = sin_osc
    , concepts::EnvelopeGenerator Env = dx_envelope_gen
   >
   struct basic_fm_operator
   {
      using envelope_type = Env;

                     basic_fm_operator();

      void           ratio(double r);            // of the master's frequency
      void           fixed(phase step);          // a fixed frequency
      void           envelope(typename Env::config const& cfg, float sps);

      void           attack();
      void           release();
      void           sync();                     // restart the phase
      bool           active() const;

      void           modulation(phase mod)   { _mod = mod; }
      phase          modulation() const      { return _mod; }
      void           gain(float g)           { _env.gain(g); }
      float          gain() const            { return _env.gain(); }

      float          operator()(phase_iterator master);

      Osc            osc;

   private:

      phase_iterator _pi;
      phase          _mod;
      double         _ratio = 1.0;               // 0: fixed
      std::uint32_t  _master = 0;                // the master's step
      Env            _env;
   };

   using fm_operator = basic_fm_operator<sin_osc, dx_envelope_gen>;

   ////////////////////////////////////////////////////////////////////////////
   // Implementation
   ////////////////////////////////////////////////////////////////////////////

   // The envelope's default config, at a placeholder sample rate until
   // envelope() sets the real one
   template <concepts::Oscillator Osc, concepts::EnvelopeGenerator Env>
   inline basic_fm_operator<Osc, Env>::basic_fm_operator()
    : _env{typename Env::config{}, 44100.0f}
   {}

   template <concepts::Oscillator Osc, concepts::EnvelopeGenerator Env>
   inline void basic_fm_operator<Osc, Env>::ratio(double r)
   {
      _ratio = r;
      _master = 0;                     // the next sample sets the step
   }

   template <concepts::Oscillator Osc, concepts::EnvelopeGenerator Env>
   inline void basic_fm_operator<Osc, Env>::fixed(phase step)
   {
      _ratio = 0.0;
      _pi._step = step;
   }

   template <concepts::Oscillator Osc, concepts::EnvelopeGenerator Env>
   inline void basic_fm_operator<Osc, Env>::envelope(
      typename Env::config const& cfg, float sps)
   {
      _env.set(cfg, sps);
   }

   template <concepts::Oscillator Osc, concepts::EnvelopeGenerator Env>
   inline void basic_fm_operator<Osc, Env>::attack()
   {
      _env.attack();
   }

   template <concepts::Oscillator Osc, concepts::EnvelopeGenerator Env>
   inline void basic_fm_operator<Osc, Env>::release()
   {
      _env.release();
   }

   template <concepts::Oscillator Osc, concepts::EnvelopeGenerator Env>
   inline void basic_fm_operator<Osc, Env>::sync()
   {
      _pi._phase = phase{};
   }

   template <concepts::Oscillator Osc, concepts::EnvelopeGenerator Env>
   inline bool basic_fm_operator<Osc, Env>::active() const
   {
      return !_env.in_idle_phase();
   }

   template <concepts::Oscillator Osc, concepts::EnvelopeGenerator Env>
   inline float basic_fm_operator<Osc, Env>::operator()(
      phase_iterator master)
   {
      if (_ratio != 0.0 && master._step.rep != _master)
      {
         _master = master._step.rep;
         _pi._step = phase{std::uint32_t(_master * _ratio), direct_unit};
      }
      auto at = _pi;
      at._phase = at._phase + _mod;
      auto y = osc(at) * _env();
      ++_pi;
      return y;
   }
}

#endif
