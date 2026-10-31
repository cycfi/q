/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(CYCFI_Q_STFT_OCTOBER_31_2026)
#define CYCFI_Q_STFT_OCTOBER_31_2026

#include <q/fft/fft.hpp>
#include <q/fft/stft_window.hpp>

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <span>
#include <vector>

namespace cycfi::q
{
   ////////////////////////////////////////////////////////////////////////////
   // stft: a short-time Fourier transform of n samples (the window's size),
   // run one sample at a time. Every `hop` samples, the latest n samples are
   // windowed with the analysis window and transformed with rfft, `f` is
   // called with the n/2+1 bins (rfft's layout, n+2 floats), and the frame
   // comes back through the inverse, the synthesis window and overlap-add.
   // Each call takes one input sample and gives one output sample,
   // `latency()` samples behind; with `f` leaving the bins alone, the output
   // is the input delayed.
   //
   // Allocates at construction only. `f` is called inside the call that
   // completes a hop, so a frame's cost lands on one sample in `hop`.
   ////////////////////////////////////////////////////////////////////////////
   class stft
   {
   public:

      using bins_type = std::span<float>;

                           stft(stft_window const& w, std::size_t hop);

                           template <typename F>
      float                operator()(float s, F&& f);

      std::size_t          size() const;
      std::size_t          latency() const;
      std::size_t          hop() const;
      void                 reset();

   private:

                           template <typename F>
      void                 process(F&& f);

      stft_window          _window;
      std::size_t          _hop;
      std::size_t          _count = 0;
      rfft                 _fft;
      std::vector<float>   _in;
      std::vector<float>   _out;
      std::vector<float>   _ready;
      std::vector<float>   _frame;
   };

   ////////////////////////////////////////////////////////////////////////////
   // Inlines
   ////////////////////////////////////////////////////////////////////////////
   inline stft::stft(stft_window const& w, std::size_t hop)
    : _window{w}
    , _hop{hop}
    , _fft{w.size()}
    , _in(w.size())
    , _out(w.size())
    , _ready(w.size())
    , _frame(w.size() + 2)
   {
      assert(hop > 0 && hop <= w.latency);
   }

   template <typename F>
   inline float stft::operator()(float s, F&& f)
   {
      _in[_in.size() - _hop + _count] = s;
      float y = _ready[_count];
      if (++_count == _hop)
      {
         _count = 0;
         process(f);
      }
      return y;
   }

   template <typename F>
   inline void stft::process(F&& f)
   {
      auto const n = _in.size();
      for (std::size_t i = 0; i != n; ++i)
         _frame[i] = _in[i] * _window.analysis[i];

      _fft.forward(_frame.data());
      f(bins_type{_frame});
      _fft.inverse(_frame.data());

      // Only the newest `latency` samples of a frame carry synthesis. The
      // oldest `hop` of those are now complete: they go out over the next
      // hop, and the rest move down to meet the next frame.
      auto const start = n - _window.latency;
      for (auto i = start; i != n; ++i)
         _out[i] += _frame[i] * _window.synthesis[i];

      auto out = _out.begin();
      std::copy(out + start, out + start + _hop, _ready.begin());
      std::copy(out + start + _hop, _out.end(), out + start);
      std::fill(_out.end() - _hop, _out.end(), 0.0f);

      std::copy(_in.begin() + _hop, _in.end(), _in.begin());
   }

   inline std::size_t stft::size() const
   {
      return _in.size();
   }

   inline std::size_t stft::latency() const
   {
      return _window.latency;
   }

   inline std::size_t stft::hop() const
   {
      return _hop;
   }

   inline void stft::reset()
   {
      _count = 0;
      std::fill(_in.begin(), _in.end(), 0.0f);
      std::fill(_out.begin(), _out.end(), 0.0f);
      std::fill(_ready.begin(), _ready.end(), 0.0f);
   }
}

#endif
