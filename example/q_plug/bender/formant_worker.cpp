/*=============================================================================
   Copyright (c) 2019-2026 Joel de Guzman

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#include "formant_worker.hpp"
#include <algorithm>
#include <chrono>

namespace
{
   // A new table about every 3 seconds at 44.1 kHz: 512 blocks of 256.
   constexpr std::size_t fit_every = 512;

   // How often the worker looks for input.
   constexpr auto nap = std::chrono::milliseconds{20};
}

formant_worker::formant_worker() = default;

formant_worker::~formant_worker()
{
   stop();
}

// A new learner takes up what the last one learned, if nothing newer
// was restored, so a restart, at any sample rate, keeps the learning.
void formant_worker::start(float sps)
{
   stop();
   {
      std::lock_guard<std::mutex> lock{_mutex};
      if (!_restored)
         _restored = _saved;
   }
   _learner = std::make_unique<q::formant_learner>(sps);
   _learner->freeze(_frozen);
   _blocks = 0;
   _running = true;
   _thread = std::thread{[this]{ run(); }};
}

void formant_worker::stop()
{
   _running = false;
   if (_thread.joinable())
      _thread.join();
}

void formant_worker::learn()
{
   _learn = true;
}

float formant_worker::coverage() const
{
   return _coverage;
}

// A state restored and not yet taken up is still what there is to save.
std::optional<formant_worker::state_type> formant_worker::saved() const
{
   std::lock_guard<std::mutex> lock{_mutex};
   return _restored ? _restored : _saved;
}

void formant_worker::restore(state_type s)
{
   std::lock_guard<std::mutex> lock{_mutex};
   _restored = std::move(s);
}

// Fill blocks of block_size from the input and publish each one full. With
// the ring full, the input is dropped: the learner loses a little, and the
// audio thread never waits.
void formant_worker::feed(std::span<float const> in)
{
   while (!in.empty())
   {
      if (!_block)
      {
         auto p = _input.allocate(block_size * sizeof(float));
         if (!p)
            return;
         _block = static_cast<float*>(p);
         _filled = 0;
      }

      auto const n = std::min(in.size(), block_size - _filled);
      std::copy_n(in.begin(), n, _block + _filled);
      _filled += n;
      in = in.subspan(n);

      if (_filled == block_size)
      {
         _input.publish();
         _block = nullptr;
      }
   }
}

void formant_worker::freeze(bool f)
{
   _frozen = f;
}

formant_worker::table_type const* formant_worker::table()
{
   return _tables.receive();
}

void formant_worker::run()
{
   bool frozen = _frozen;
   while (_running)
   {
      if (_learn.exchange(false))
      {
         _learner->learn();
         _blocks = 0;
         _coverage = 0.0f;
         snapshot();
      }

      if (frozen != _frozen)
      {
         frozen = _frozen;
         _learner->freeze(frozen);
      }

      std::optional<state_type> restored;
      {
         std::lock_guard<std::mutex> lock{_mutex};
         restored.swap(_restored);
      }
      // Its coverage before its table, so whoever sees the table sees
      // what it covers.
      if (restored && _learner->restore(*restored))
      {
         _blocks = 0;
         _coverage = _learner->coverage();
         fit();
      }

      _input.consume(
         [this](void* p, std::size_t size)
         {
            take(static_cast<float const*>(p), size / sizeof(float));
         }
      );
      _coverage = _learner->coverage();
      std::this_thread::sleep_for(nap);
   }

   // Stopped: what was learned since the last fit is kept too.
   snapshot();
}

void formant_worker::take(float const* p, std::size_t n)
{
   (*_learner)(std::span<float const>{p, n});
   if (!_learner->frozen() && ++_blocks % fit_every == 0)
      fit();
}

void formant_worker::fit()
{
   auto table = std::make_unique<table_type>(_learner->order());
   if (!_learner->fit(*table))
      return;
   _tables.send(std::move(table));
   snapshot();
}

// The learner's state, for saving. Copied here, on the worker, at each
// fit, not while the main thread waits.
void formant_worker::snapshot()
{
   state_type s;
   _learner->save(s);
   std::lock_guard<std::mutex> lock{_mutex};
   _saved = std::move(s);
}
