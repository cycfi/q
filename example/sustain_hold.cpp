/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the Boost Software License, Version 1.0.
   [ https://www.boost.org/LICENSE_1_0.txt ]
=============================================================================*/
#include <q/support/literals.hpp>
#include <q/fx/clip.hpp>
#include <q/fx/dynamic.hpp>
#include <q/fx/envelope.hpp>
#include <q/synth/grain.hpp>
#include <q/utility/best_lag.hpp>
#include <q/utility/fractional_ring_buffer.hpp>
#include <q_io/audio_stream.hpp>
#include <q_io/audio_file.hpp>

#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstdio>
#include <iostream>
#include <vector>

#include <csignal>
#include <sys/select.h>
#include <termios.h>
#include <unistd.h>

///////////////////////////////////////////////////////////////////////////////
// Interactive monophonic sustain hold using q::grain and q::best_lag.
//
//    SPACE : engage the hold / release it
//    q     : quit
//
// While idle, the input plays through dry and is recorded into a ring
// buffer. On engage:
//
//  * The correlator (best_lag) measures the period: the lag at which the
//    recent signal best matches itself, with sub-sample precision.
//
//  * The freshest few periods are re-emitted as Hann-windowed grains
//    spanning multiple periods, overlap-added at half-width hops. Hop
//    and width are whole multiples of the period, so overlapping grains
//    are phase-aligned: no comb filtering.
//
//  * The anchor ping-pongs through a small recent region as a tape loop
//    on the period grid: consecutive grains read contiguous material --
//    the forward leg is literally the original recording playing back --
//    so the variation is the string's own recorded evolution, nothing
//    synthetic. Each anchor position is RMS-matched (level-normalized)
//    so the held level stays constant.
//
//  * The dry signal hands over through a half-Hann down-ramp that is the
//    exact complement of the grains' overlap-add ramp-in: no click.
//
// A gentle compressor (-12 dB threshold, 1:8, 2x makeup) evens the
// level so the held sustain stays clearly audible against the dry note.
//
// On release, the sustain eases out over the release time and the dry
// path returns. The hold can be re-engaged any number of times. The
// program exits when the input is exhausted and the hold is idle, or on
// q / Ctrl-C; every exit path fades the output before stopping the
// stream, so termination never clicks.
///////////////////////////////////////////////////////////////////////////////

namespace q = cycfi::q;
using namespace q::literals;

struct sustain_hold : q::audio_stream
{
   static constexpr auto cycles = 16u;       // grain width, in periods
   static constexpr auto onset_level = 0.1f; // attack detection threshold
   static constexpr auto region_limit = 32;  // max loop region, in periods
   static constexpr auto attack_margin = 4;  // keep-out after attack, periods
   static constexpr auto max_hold_secs = 120.0;
   static constexpr auto comp_makeup = 2.0f;

   enum class state : int { idle, held, releasing };

   using buffer_type = q::fractional_ring_buffer<
      float, std::vector<float>, double, q::sample_interpolation::hermite>;
   using base_buffer = q::ring_buffer<float, std::vector<float>>;

   sustain_hold(
      float* wav
    , std::size_t len
    , float sps
    , q::duration release
   )
    : audio_stream(0, 2, sps)
    , _wav(wav)
    , _len(len)
    , _sps(sps)
    , _buff(std::size_t(max_hold_secs * sps) + 8192)
    , _grains{q::grain<>{sps}, q::grain<>{sps}}
    , _env(500_ms, sps)
    , _min_lag(std::size_t(sps / 400.0f))    // guitar range: 400 Hz ...
    , _max_lag(std::size_t(sps / 60.0f))     // ... down to 60 Hz
    , _dry_ramp(q::duration{0.01}, sps)      // reconfigured at engage
    , _release(release, sps)
    , _release_len(std::size_t(as_double(release) * sps))
    , _master_step(1.0f / (0.05f * sps))     // 50ms master fade on quit
    , _comp(-12_dB, 1.0f / 8)                // threshold -12dB, ratio 1:8
    , _comp_env(10_ms, 250_ms, sps)          // AR follower: no gain chatter
   {}

   // Called from the main (keyboard) thread
   void toggle()
   {
      _toggle.fetch_add(1, std::memory_order_relaxed);
   }

   // Begin the brief master fade-out; called before stopping the stream
   void quit()
   {
      _quit.store(true, std::memory_order_relaxed);
   }

   bool finished() const
   {
      return _file_done.load(std::memory_order_relaxed)
         && _state.load(std::memory_order_relaxed) == state::idle;
   }

   void process(out_channels const& out)
   {
      auto left = out[0];
      auto right = out[1];
      for (auto frame : out.frames)
      {
         if (_toggle.load(std::memory_order_relaxed) > 0)
         {
            _toggle.fetch_sub(1, std::memory_order_relaxed);
            handle_toggle();
         }

         auto s = _t < _len ? _wav[_t] : 0.0f;
         _buff.push(s);

         if (_t == _len + std::size_t(_sps))    // input done (+1s grace)
            _file_done.store(true, std::memory_order_relaxed);

         // Track the envelope to remember where the last attack was:
         // the capture region must never reach into it.
         auto e = _env(std::abs(s));
         if (_prev_env <= onset_level && e > onset_level)
         {
            _attack_pos = _t;
            _dry_target = 1.0f;     // a new note: let the dry through
         }
         _prev_env = e;

         // Re-emit grains at half-width hops while the hold is active;
         // the anchor ping-pongs through the recent region on the
         // period grid.
         if (_state != state::idle && double(_t) >= _next)
         {
            auto delay = (_next - _anchor) + double(_k) * _lag;
            _g ^= 1;
            _grains[_g].spawn(float(delay), _width);
            _gains[_g] = _norm[_k];

            auto hop_periods = int(cycles / 2);
            _k += _dir * hop_periods;
            if (_k >= _k_max) { _k = _k_max; _dir = -1; }
            if (_k <= 0)      { _k = 0; _dir = +1; }
            _next += _hop;
         }

         // Dry path: complementary hand-over on engage. After a release
         // the dry STAYS muted until a new attack arrives -- returning
         // it immediately would replay the old note's decayed remainder
         // as a ghost tail (lifted further by the compressor makeup).
         float dry_gain;
         if (_state == state::held)
         {
            dry_gain =
               _t < _hold_pos + std::size_t(_hop) ? _dry_ramp() : 0.0f;
            _dry_g = dry_gain;
         }
         else
         {
            _dry_g += 0.0002f * (_dry_target - _dry_g);
            dry_gain = _dry_g;
         }

         // Sustain gain: unity while held, release ramp on release,
         // ZERO when idle -- grains spawned late in the release outlive
         // the state flip; without this they pop back at full gain (an
         // audible snip right after the release ends).
         float sus_gain;
         if (_state == state::held)
            sus_gain = 1.0f;
         else if (_state == state::releasing)
         {
            if (_rel_count)
            {
               --_rel_count;
               sus_gain = _release();
            }
            else
            {
               sus_gain = 0.0f;
               _state = state::idle;
            }
         }
         else
            sus_gain = 0.0f;
         // Master fade on quit: no exit path cuts the audio dead
         if (_quit.load(std::memory_order_relaxed) && _master > 0.0f)
            _master = std::max(0.0f, _master - _master_step);

         auto sustain =
            _gains[0] * _grains[0](_buff) + _gains[1] * _grains[1](_buff);
         auto mix = (s * dry_gain) + (0.9f * sus_gain * sustain);

         // Compress + makeup: keeps the held sustain clearly audible.
         // The AR follower smooths the side-chain (an instant-attack peak
         // follower ripples at the waveform rate on low notes, modulating
         // the gain into audible distortion); the soft clip catches the
         // brief attack overshoots an AR side-chain lets through.
         auto env_db = q::lin_to_db(_comp_env(std::abs(mix)));
         mix = _clip(mix * q::lin_float(_comp(env_db)) * comp_makeup);

         left[frame] = _master * mix;
         right[frame] = _master * mix;
         ++_t;
      }
   }

   void handle_toggle()
   {
      if (_state == state::idle)
      {
         // Need enough recorded material to correlate and capture
         if (_t > std::size_t(_sps))
            engage_hold();
         else
            _event.store(3, std::memory_order_release);
      }
      else if (_state == state::held)
      {
         _release.reset();
         _rel_count = _release_len;
         _state = state::releasing;
         _event.store(2, std::memory_order_release);
      }
      // Toggles while releasing are ignored
   }

   void engage_hold()
   {
      // Analysis uses the base ring buffer's integer indexing: the
      // hermite-interpolating index operator is for the audio path; in
      // the analysis inner loops it would blow the callback deadline
      // (an audible click on engage).
      auto const& raw = static_cast<base_buffer const&>(_buff);

      auto r = q::best_lag(raw, 2048, _min_lag, _max_lag);
      _lag = r.lag;
      _width = std::size_t(cycles * r.lag + 0.5f);
      _hop = double(cycles) * r.lag / 2.0;     // = (cycles/2) periods
      _anchor = double(_t) - double(_width);
      _next = double(_t);
      _hold_pos = _t;
      _dry_ramp.config(q::duration{_hop / _sps}, _sps);

      // How far back the loop region may reach: up to the attack
      // keep-out zone, at most region_limit periods.
      auto avail = (double(_t - _attack_pos) - double(_width)) / r.lag
         - attack_margin;
      _k_max = std::clamp(int(avail), 0, region_limit);
      _k = 0;
      _dir = 1;

      // Level-normalize each anchor position: without this, traversing
      // into earlier (louder) material sweeps loudness.
      _norm.resize(_k_max + 1);
      auto rms = [&](int k) {
         double acc = 0;
         auto base = std::size_t(k * _lag);
         for (std::size_t j = 0; j != _width; ++j)
         {
            double v = raw[base + j];
            acc += v * v;
         }
         return std::sqrt(acc / _width);
      };
      auto ref = rms(0);
      for (int k = 0; k <= _k_max; ++k)
      {
         auto rk = rms(k);
         _norm[k] = rk > 0 ? std::clamp(float(ref / rk), 0.25f, 4.0f) : 1.0f;
      }

      _dry_target = 0.0f;     // dry returns only on the next attack
      _state = state::held;

      _rep_period = r.lag;
      _rep_similarity = r.similarity;
      _rep_region = _k_max;
      _event.store(1, std::memory_order_release);
   }

   // Main thread: print pending reports (stdout must stay out of the
   // audio callback -- a blocking write there causes underrun clicks)
   void poll_events()
   {
      switch (_event.exchange(0, std::memory_order_acquire))
      {
         case 1:
            std::cout
               << "[hold] period " << _rep_period << " samples ("
               << _sps / _rep_period << " Hz), similarity "
               << _rep_similarity << ", loop region " << _rep_region
               << " periods" << std::endl;
            break;
         case 2:
            std::cout << "[release]" << std::endl;
            break;
         case 3:
            std::cout << "[not enough material yet]" << std::endl;
            break;
      }
   }

   float*                        _wav;
   std::size_t                   _len;
   float                         _sps;
   buffer_type                   _buff;
   q::grain<>                    _grains[2];
   q::peak_envelope_follower     _env;
   std::size_t                   _min_lag, _max_lag;
   std::atomic<int>              _toggle{0};
   std::atomic<int>              _event{0};
   float                         _rep_period = 0;
   float                         _rep_similarity = 0;
   int                           _rep_region = 0;
   std::atomic<bool>             _quit{false};
   std::atomic<bool>             _file_done{false};
   std::atomic<state>            _state{state::idle};
   std::size_t                   _t = 0;
   float                         _prev_env = 0;
   std::size_t                   _hold_pos = 0;
   std::size_t                   _attack_pos = 0;
   float                         _lag = 0;
   std::size_t                   _width = 0;
   double                        _hop = 0;
   double                        _anchor = 0;
   double                        _next = 0;
   int                           _k = 0;
   int                           _k_max = 0;
   int                           _dir = 1;
   unsigned                      _g = 0;
   float                         _gains[2] = {1.0f, 1.0f};
   std::vector<float>            _norm;
   float                         _dry_g = 1.0f;
   float                         _dry_target = 1.0f;
   q::hann_downward_ramp_gen     _dry_ramp;
   q::hann_downward_ramp_gen     _release;
   std::size_t                   _release_len;
   std::size_t                   _rel_count = 0;
   float                         _master = 1.0f;
   float                         _master_step;
   q::compressor                 _comp;
   q::ar_envelope_follower       _comp_env;
   q::soft_clip                  _clip;
};

std::atomic<bool> sigint_flag{false};

int main()
{
   q::wav_reader wav{"audio_files/Low E.wav"};
   if (!wav)
      return -1;

   std::vector<float> in(wav.length());
   wav.read(in);

   sustain_hold proc{
      in.data(), in.size(), wav.sps()
    , 2_s     // release time
   };

   if (!proc.is_valid())
      return -1;

   // Raw (non-canonical) terminal input: keys act immediately
   termios saved;
   tcgetattr(STDIN_FILENO, &saved);
   termios raw = saved;
   raw.c_lflag &= ~(ICANON | ECHO);
   raw.c_cc[VMIN] = 1;
   raw.c_cc[VTIME] = 0;
   tcsetattr(STDIN_FILENO, TCSANOW, &raw);

   // sigaction without SA_RESTART: Ctrl-C must interrupt select() --
   // std::signal on macOS sets SA_RESTART, which silently restarts the
   // select and the quit flag is never seen (the "cannot end it" bug).
   struct sigaction sa = {};
   sa.sa_handler = [](int) { sigint_flag = true; };
   sa.sa_flags = 0;
   sigaction(SIGINT, &sa, nullptr);

   std::cout
      << "SPACE: hold / release    q: quit\n"
      << "TIP:   the hold sounds best engaged late, near the end of the\n"
      << "       note's decay, where the timbre has settled"
      << std::endl;

   proc.start();
   while (!sigint_flag && !proc.finished())
   {
      fd_set fds;
      FD_ZERO(&fds);
      FD_SET(STDIN_FILENO, &fds);
      timeval tv{0, 100000};      // 100ms tick
      auto n = select(STDIN_FILENO+1, &fds, nullptr, nullptr, &tv);
      proc.poll_events();
      if (n > 0)
      {
         char c;
         if (read(STDIN_FILENO, &c, 1) <= 0)
            break;
         if (c == ' ')
            proc.toggle();
         else if (c == 'q' || c == 'Q')
            break;
      }
   }

   proc.quit();                   // fade the master ...
   usleep(150000);                // ... let the fade complete
   proc.stop();

   tcsetattr(STDIN_FILENO, TCSANOW, &saved);
   return 0;
}
