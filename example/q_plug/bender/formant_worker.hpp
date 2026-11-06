/*=============================================================================
   Copyright (c) 2019-2026 Joel de Guzman

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(Q_PLUG_BENDER_FORMANT_WORKER_NOVEMBER_5_2026)
#define Q_PLUG_BENDER_FORMANT_WORKER_NOVEMBER_5_2026

#include <q/fft/formant_learner.hpp>
#include <q/fx/formant_corrector.hpp>
#include <q/utility/handoff.hpp>
#include <q/utility/ring_allocator.hpp>
#include <atomic>
#include <cstddef>
#include <memory>
#include <mutex>
#include <optional>
#include <span>
#include <thread>

namespace q = cycfi::q;

///////////////////////////////////////////////////////////////////////////////
// The formant learner, on a thread of its own. The audio thread passes its
// dry input on in blocks (feed), and takes the latest table at the top of
// each block (table). The worker learns from the blocks and fits a new
// table every few seconds, unless frozen.
//
// start and stop are main thread, with the audio thread stopped: the
// learner is made for the sample rate there, and takes what was learned
// before it. learn, the learner's state to save, and a state to restore
// are main thread too; feed, freeze and table are the audio thread's. A
// restored state is fitted and its table sent at once, and learning goes
// on from it.
///////////////////////////////////////////////////////////////////////////////
class formant_worker
{
public:

   using table_type = q::formant_table;
   using state_type = q::spectral_learner::state;

   static constexpr std::size_t block_size = 256;

                        formant_worker();
                        ~formant_worker();

   // Main thread.
   void                 start(float sps);
   void                 stop();
   void                 learn();
   float                coverage() const;
   std::optional<state_type> saved() const;
   void                 restore(state_type s);

   // Audio thread.
   void                 feed(std::span<float const> in);
   void                 freeze(bool f);
   table_type const*    table();

private:

   void                 run();
   void                 take(float const* p, std::size_t n);
   void                 fit();
   void                 snapshot();

   // Audio thread to worker: the input, in blocks of block_size.
   q::ring_allocator    _input{1 << 18};
   float*               _block = nullptr;
   std::size_t          _filled = 0;

   // Worker to audio thread.
   q::handoff<table_type> _tables;

   std::unique_ptr<q::formant_learner> _learner;
   std::thread          _thread;
   std::atomic<bool>    _running = false;
   std::atomic<bool>    _learn = false;
   std::atomic<bool>    _frozen = false;
   std::atomic<float>   _coverage = 0.0f;
   std::size_t          _blocks = 0;

   // What was learned, as of the last fit, for saving; and a state
   // restored, for the worker to take up.
   mutable std::mutex   _mutex;
   std::optional<state_type> _saved;
   std::optional<state_type> _restored;
};

#endif
