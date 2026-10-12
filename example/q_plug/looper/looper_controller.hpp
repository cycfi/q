/*=============================================================================
   Copyright (c) 2019-2026 Joel de Guzman

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(Q_PLUG_LOOPER_CONTROLLER_OCTOBER_12_2026)
#define Q_PLUG_LOOPER_CONTROLLER_OCTOBER_12_2026

#include <q_plug/controller.hpp>
#include <q/utility/handoff.hpp>
#include <atomic>
#include <filesystem>
#include <string>
#include <vector>

namespace q_plug = cycfi::q_plug;
namespace q = cycfi::q;

///////////////////////////////////////////////////////////////////////////////
// A sample, loaded whole: its frames interleaved, at the rate it was
// recorded at.
///////////////////////////////////////////////////////////////////////////////
struct sample_buffer
{
   std::size_t          length() const { return data.size() / channels; }

   std::vector<float>   data;
   std::size_t          channels = 1;
   float                sps = 0.0f;
};

///////////////////////////////////////////////////////////////////////////////
// The controller declares the parameters and holds the sample. A sample is
// loaded on the main thread and sent through the handoff; the processor
// takes the current one at the top of each block.
///////////////////////////////////////////////////////////////////////////////
class looper_controller : public q_plug::controller
{
public:

   using decibel = cycfi::q::decibel;
   using path = std::filesystem::path;

   enum { play_id, volume_id, loop_id };

   // The volume runs from silence, the 24 bit floor, up to +10 dB.
   static constexpr decibel silence = cycfi::q::dB(-144.0);
   static constexpr decibel max_volume = cycfi::q::dB(10.0);

   parameter_list       parameters() const override;

   bool                 playing() const;
   bool                 looping() const;
   decibel              volume() const;
   parameter const&     volume_param() const;

   // Main thread: Play off, as an edit that reaches the host.
   void                 stop();

   // Main thread. False if the file is not a WAV file Q can read.
   bool                 load(path const& file);
   std::string const&   sample_name() const { return _sample_name; }

   // Back to the start: set on the main thread, taken once on the audio
   // thread.
   void                 rewind();
   bool                 take_rewind();

   q::handoff<sample_buffer> sample;

private:

   std::string          _sample_name;
   std::atomic<bool>    _rewind = false;
};

///////////////////////////////////////////////////////////////////////////////
// Inline implementation
///////////////////////////////////////////////////////////////////////////////
inline bool looper_controller::playing() const
{
   return get_parameter<bool>(play_id);
}

inline bool looper_controller::looping() const
{
   return get_parameter<bool>(loop_id);
}

inline looper_controller::decibel looper_controller::volume() const
{
   return get_parameter<decibel>(volume_id);
}

inline looper_controller::parameter const&
looper_controller::volume_param() const
{
   return parameters()[volume_id];
}

inline void looper_controller::stop()
{
   begin_edit(play_id);
   edit_parameter(play_id, false);
   end_edit(play_id);
}

inline void looper_controller::rewind()
{
   _rewind.store(true, std::memory_order_relaxed);
}

inline bool looper_controller::take_rewind()
{
   return _rewind.exchange(false, std::memory_order_relaxed);
}

#endif
