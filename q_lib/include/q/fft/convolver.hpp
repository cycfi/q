/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(CYCFI_Q_CONVOLVER_NOVEMBER_2_2026)
#define CYCFI_Q_CONVOLVER_NOVEMBER_2_2026

#include <q/fft/fft.hpp>

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <span>
#include <vector>

namespace cycfi::q
{
   ////////////////////////////////////////////////////////////////////////////
   // convolver: convolves a signal with an impulse response (a cabinet, a
   // room, a body), sample by sample, with no latency. The IR is cut into
   // partitions of `block` samples (a power of 2). The first runs as a
   // direct FIR; the rest are convolved in the frequency domain, overlap-
   // save with 2 * block point FFTs, against the spectra of the past input
   // blocks (a frequency-domain delay line). It allocates only at
   // construction.
   //
   // The cost stays nearly flat: the partitions that use input older than
   // the last block are summed a few each sample, so the end of a block is
   // left with one forward FFT, one partition's product and one inverse
   // FFT. A larger block costs more per sample in the FIR and less in the
   // partitions; 32 or 64 suits most IRs.
   ////////////////////////////////////////////////////////////////////////////
   class convolver
   {
   public:

                           convolver(
                              std::span<float const> ir
                            , std::size_t block = 64
                           );

      float                operator()(float s);

      std::size_t          block() const;
      std::size_t          length() const;
      void                 reset();

   private:

      void                 accumulate(std::size_t from, std::size_t to);
      void                 next_block();

      using spectrum = float*;

      std::size_t          _block;
      std::size_t          _length;
      std::size_t          _parts;        // partitions after the first
      std::size_t          _per_sample;   // older partitions summed a sample
      std::size_t          _stride;       // a spectrum's scalars, 2 block + 2
      basic_rfft<float>    _fft;
      std::vector<float>   _head;         // the first partition, reversed
      std::vector<float>   _h;            // the later partitions' spectra
      std::vector<float>   _fdl;          // the past input spectra, a ring
      std::size_t          _newest = 0;   // the ring's newest
      std::vector<float>   _x;            // the last two input blocks
      std::vector<float>   _acc;          // the next block's sum, building
      std::vector<float>   _work;
      std::vector<float>   _y;            // this block's later partitions
      std::size_t          _i = 0;        // the sample in the block
   };

   ////////////////////////////////////////////////////////////////////////////
   // Inlines
   ////////////////////////////////////////////////////////////////////////////
   inline convolver::convolver(std::span<float const> ir, std::size_t block)
    : _block{block}
    , _length{ir.size()}
    , _parts{ir.size() > block ? (ir.size() - 1) / block : 0}
    , _per_sample{_parts > 1 ? (_parts - 2) / block + 1 : 0}
    , _stride{2 * block + 2}
    , _fft{2 * block}
    , _head(block, 0.0f)
    , _h(_parts * _stride, 0.0f)
    , _fdl(_parts > 1 ? (_parts - 1) * _stride : 0, 0.0f)
    , _x(2 * block, 0.0f)
    , _acc(_stride, 0.0f)
    , _work(_stride, 0.0f)
    , _y(block, 0.0f)
   {
      assert(block >= 2 && (block & (block - 1)) == 0 && !ir.empty());

      // The FIR's taps, reversed so they run with the input forwards.
      auto const head = std::min(block, ir.size());
      for (std::size_t t = 0; t != head; ++t)
         _head[block - 1 - t] = ir[t];

      // Partition p's spectrum: its taps, zero padded to 2 block.
      for (std::size_t p = 1; p <= _parts; ++p)
      {
         auto* h = &_h[(p - 1) * _stride];
         auto const from = p * block;
         auto const to = std::min(from + block, ir.size());
         std::copy(ir.begin() + from, ir.begin() + to, h);
         _fft.forward(h);
      }
   }

   inline float convolver::operator()(float s)
   {
      auto const b = _block;
      _x[b + _i] = s;

      // The first partition, direct: the last `block` samples, oldest first.
      auto const* x = &_x[_i + 1];
      auto const* h = _head.data();
      float y0 = 0, y1 = 0;
      for (std::size_t t = 0; t < b; t += 2)
      {
         y0 += h[t] * x[t];
         y1 += h[t + 1] * x[t + 1];
      }
      auto const y = _y[_i] + y0 + y1;

      // A few of the older partitions, toward the next block.
      if (_per_sample)
      {
         auto const from = 2 + _i * _per_sample;
         accumulate(from, std::min(from + _per_sample, _parts + 1));
      }

      if (++_i == b)
         next_block();
      return y;
   }

   // acc += H[p] X[p], p from `from` to `to`, X[p] the input spectrum p - 2
   // blocks older than the newest: the partitions whose input is older
   // than the block being taken.
   inline void convolver::accumulate(std::size_t from, std::size_t to)
   {
      auto const slots = _parts - 1;
      auto const bins = _block + 1;
      auto* acc = _acc.data();
      for (std::size_t p = from; p < to; ++p)
      {
         auto const age = p - 2;
         auto const slot = (_newest + slots - age) % slots;
         auto const* h = &_h[(p - 1) * _stride];
         auto const* xs = &_fdl[slot * _stride];
         for (std::size_t k = 0; k != bins; ++k)
         {
            auto const hr = h[2 * k], hi = h[2 * k + 1];
            auto const xr = xs[2 * k], xi = xs[2 * k + 1];
            acc[2 * k] += hr * xr - hi * xi;
            acc[2 * k + 1] += hr * xi + hi * xr;
         }
      }
   }

   // The block is in: its spectrum, the first later partition's product
   // with it, and the sum back to the time domain for the next block.
   inline void convolver::next_block()
   {
      auto const b = _block;
      _i = 0;
      if (_parts > 0)
      {
         std::copy(_x.begin(), _x.end(), _work.begin());
         _fft.forward(_work.data());

         auto const* h = _h.data();
         auto const* xs = _work.data();
         auto* acc = _acc.data();
         for (std::size_t k = 0; k != b + 1; ++k)
         {
            auto const hr = h[2 * k], hi = h[2 * k + 1];
            auto const xr = xs[2 * k], xi = xs[2 * k + 1];
            acc[2 * k] += hr * xr - hi * xi;
            acc[2 * k + 1] += hr * xi + hi * xr;
         }

         if (_parts > 1)
         {
            _newest = (_newest + 1) % (_parts - 1);
            std::copy(_work.begin(), _work.end(), &_fdl[_newest * _stride]);
         }

         // Overlap-save: the last `block` samples are the linear part.
         _fft.inverse(_acc.data());
         std::copy(_acc.begin() + b, _acc.begin() + 2 * b, _y.begin());
         std::fill(_acc.begin(), _acc.end(), 0.0f);
      }
      std::copy(_x.begin() + b, _x.end(), _x.begin());
   }

   inline std::size_t convolver::block() const
   {
      return _block;
   }

   inline std::size_t convolver::length() const
   {
      return _length;
   }

   inline void convolver::reset()
   {
      std::fill(_fdl.begin(), _fdl.end(), 0.0f);
      std::fill(_x.begin(), _x.end(), 0.0f);
      std::fill(_acc.begin(), _acc.end(), 0.0f);
      std::fill(_y.begin(), _y.end(), 0.0f);
      _newest = 0;
      _i = 0;
   }
}

#endif
