/*=============================================================================
   Copyright (c) 2019-2026 Joel de Guzman

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(Q_PLUG_BENDER_CONTROLLER_NOVEMBER_5_2026)
#define Q_PLUG_BENDER_CONTROLLER_NOVEMBER_5_2026

#include <q_plug/controller.hpp>
#include <q/midi/bend_range.hpp>
#include <q/utility/ring_allocator.hpp>
#include "formant_worker.hpp"

namespace q_plug = cycfi::q_plug;
namespace q = cycfi::q;

///////////////////////////////////////////////////////////////////////////////
// The controller declares the parameters and holds what the threads share:
// the formant worker, and the scope, the bend in semitones a few hundred
// times a second, from the audio thread to the editor. The learned table
// rides in the plugin's state, so a session comes back with its
// instrument's formants.
///////////////////////////////////////////////////////////////////////////////
class bender_controller : public q_plug::controller
{
public:

   enum
   {
      dive_id, pull_up_id, middle_id,
      formants_id, amount_id, freeze_id,
      bend_id
   };

   parameter_list       parameters() const override;

   q::midi::bend_range  range() const;
   bool                 formants() const;
   float                amount() const;      // 0 to 1
   bool                 frozen() const;
   float                wheel() const;       // -1 to 1, 0 at rest

   formant_worker       worker;

   // The scope: one float per scope_period, the bend in semitones.
   static constexpr double scope_period = 0.005;    // seconds
   q::ring_allocator    scope{1 << 14};

protected:

   void                 save_extra(json& j) const override;
   void                 load_extra(json const& j
                         , std::uint32_t version) override;
};

///////////////////////////////////////////////////////////////////////////////
// Inline implementation
///////////////////////////////////////////////////////////////////////////////
inline q::midi::bend_range bender_controller::range() const
{
   return {
      float(get_parameter<double>(dive_id))
    , float(get_parameter<double>(pull_up_id))
    , float(get_parameter<double>(middle_id))
   };
}

inline bool bender_controller::formants() const
{
   return get_parameter<bool>(formants_id);
}

inline float bender_controller::amount() const
{
   return float(get_parameter<double>(amount_id) / 100.0);
}

inline bool bender_controller::frozen() const
{
   return get_parameter<bool>(freeze_id);
}

inline float bender_controller::wheel() const
{
   return float(get_parameter<double>(bend_id));
}

#endif
