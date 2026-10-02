/*=============================================================================
   Copyright (c) 2019-2026 Joel de Guzman

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(Q_PLUG_DEXTER_CONTROLLER_OCTOBER_1_2026)
#define Q_PLUG_DEXTER_CONTROLLER_OCTOBER_1_2026

#include <q_plug/controller.hpp>
#include <q/synth/fm/dx_patcher.hpp>
#include <cstdint>
#include <span>
#include <string>

namespace q_plug = cycfi::q_plug;
namespace q = cycfi::q;

///////////////////////////////////////////////////////////////////////////////
// The controller declares the plugin's parameters: every field of a DX7
// voice, 145 of them, then the plugin's own: the volume, how the keys play
// (poly or mono, and the portamento), and a switch per operator.
//
// The voice's values are the numbers a DX7's panel shows, so a patch maps
// one to one: patch() reads the parameters back as a q::dx_patch, and
// state_of() writes a dx_patch as a preset. The operator switches are
// for auditioning: a session keeps them, a preset does not, and loading
// one turns every operator on.
///////////////////////////////////////////////////////////////////////////////
class dexter_controller : public q_plug::controller
{
public:

   // The index of each parameter of the voice and of the plugin
   enum
   {
      algorithm_id, feedback_id
    , pitch_rate_id, pitch_level_id = pitch_rate_id + 4
    , lfo_speed_id = pitch_level_id + 4, lfo_delay_id
    , lfo_pitch_depth_id, lfo_amp_depth_id, lfo_sync_id, lfo_wave_id
    , pitch_mod_sens_id, osc_sync_id, transpose_id
    , volume_id, mono_id, porta_mode_id, glide_id
    , op_base
   };

   // An operator's parameters, at op_index(op, field)
   enum op_field
   {
      level, fixed, coarse, fine, detune
    , rate, env_level = rate + 4
    , break_point = env_level + 4, left_depth, right_depth
    , left_curve, right_curve
    , rate_scaling, amp_mod_sens, velocity_sens
    , on
    , op_size
   };

   static constexpr int num_operators = 6;
   static constexpr int num_params = op_base + num_operators * op_size;

   // op is 0 for OP1
   static constexpr int op_index(int op, int field)
   {
      return op_base + op * op_size + field;
   }

   parameter_list       parameters() const override;

   q::dx_patch          patch() const;
   q::decibel           volume() const;
   bool                 mono() const;
   bool                 porta_follow() const;
   int                  glide() const;          // 0..99
   bool                 enabled(int op) const;

   // A voice as a preset: the state the controller would save if its
   // parameters held this voice, the plugin's own at their defaults.
   json                 state_of(q::dx_patch const& p) const;

   // The voices of a DX7 .syx file, a cartridge or one voice, added as
   // user presets named after the voices; a name already taken gets the
   // file's name after it. The names added, in the file's order: none if
   // the bytes are not a DX7 dump.
   using bytes = std::span<std::uint8_t const>;
   name_list            import_voices(bytes syx, std::string const& file);
};

#endif
