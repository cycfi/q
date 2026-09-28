/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the Boost Software License, Version 1.0.
   [ https://www.boost.org/LICENSE_1_0.txt ]
=============================================================================*/
#if !defined(CYCFI_Q_SYNC_HPP_SEPTEMBER_28_2026)
#define CYCFI_Q_SYNC_HPP_SEPTEMBER_28_2026

#include <q/support/phase.hpp>

namespace cycfi::q
{
   ////////////////////////////////////////////////////////////////////////////
   // hard_sync: one oscillator restarted by another, the sync input a
   // voltage controlled oscillator has. The slave runs at its own frequency
   // until the master completes a cycle, at which point it starts again, so
   // the pitch heard is the master's and the slave's frequency becomes a
   // timbre control. Sweeping it is the sound the Prophet's OSC A SYNC
   // switch is there for.
   //
   // Both phases stay the caller's. Advance them as usual, then call this
   // with the pair: it sees the master wrap and moves the slave.
   //
   // The restart lands between samples, and this places it there: the master
   // is that fraction of a step past zero, so the slave starts the same
   // fraction into its own step. Without that the restart would quantize to
   // the sample grid and the pitch would waver.
   //
   // What it does not do yet is correct the discontinuity the restart makes.
   // The slave's own waveform corrections assume a full jump at the end of
   // its own cycle, and a sync restart cuts the wave at an arbitrary value,
   // so the difference aliases. Measuring that needs an oversampled
   // rendering to compare against: a synced oscillator is periodic at the
   // master's rate, so its aliases land on the master's own harmonics and no
   // inharmonic test can see them. The voice will want the residual BLEP,
   // and it has the previous sample to hand, which is what the correction
   // needs.
   ////////////////////////////////////////////////////////////////////////////
   struct hard_sync
   {
      bool           operator()(
                        phase_iterator& slave, phase_iterator const& master
                     );

      float          fraction() const  { return _fraction; }

   private:

      phase          _last;                  // the master's previous phase
      float          _fraction = 0.0f;       // where in the sample it landed
   };

   ////////////////////////////////////////////////////////////////////////////
   // Implementation
   ////////////////////////////////////////////////////////////////////////////
   inline bool hard_sync::operator()(
      phase_iterator& slave, phase_iterator const& master)
   {
      auto now = master._phase;
      auto wrapped = now.rep < _last.rep;    // the unsigned phase rolled over
      _last = now;

      if (!wrapped)
         return false;

      // How far past zero the master already is, as a fraction of its step:
      // the slave starts the same fraction into its own.
      _fraction = master._step.rep?
         float(double(now.rep) / master._step.rep) :
         0.0f
         ;
      slave._phase = phase{
         std::uint32_t(_fraction * slave._step.rep), direct_unit};
      return true;
   }
}

#endif
