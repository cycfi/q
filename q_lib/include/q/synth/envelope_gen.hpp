/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the Boost Software License, Version 1.0.
   [ https://www.boost.org/LICENSE_1_0.txt ]
=============================================================================*/
#if !defined(CYCFI_Q_OSC_ENVELOPE_GEN_HPP_MAY_6_2023)
#define CYCFI_Q_OSC_ENVELOPE_GEN_HPP_MAY_6_2023

#include <q/support/base.hpp>
#include <q/synth/concepts.hpp>
#include <q/synth/exponential_gen.hpp>
#include <q/synth/linear_gen.hpp>

#include <memory>
#include <vector>
#include <type_traits>

namespace cycfi::q
{
   namespace detail
   {
      /////////////////////////////////////////////////////////////////////////
      // Ramp holder abstract base class. This is provided so we can hold
      // references (pointers, smart pointers, etc.) to ramp generators in
      // std containers.
      /////////////////////////////////////////////////////////////////////////
      struct ramp_holder_base
      {
         virtual        ~ramp_holder_base() = default;
         virtual float  operator()(float offset, float scale) = 0;
         virtual bool   done() const = 0;
         virtual void   reset() = 0;

         virtual void   config(duration width, float sps) = 0;
      };

      using ramp_base_ptr = std::shared_ptr<ramp_holder_base>;

      /////////////////////////////////////////////////////////////////////////
      // constant_holder: holds its level instead of ramping toward another.
      // It has no width and never finishes on its own, so an envelope waits
      // at it until it is released. This is the sustain of a classic ADSR,
      // as against Q's own, which runs down over a sustain rate.
      /////////////////////////////////////////////////////////////////////////
      struct constant_holder : ramp_holder_base
      {
         // The segment enters with offset and scale spanning the level it
         // came from and the level it holds, so their sum is the higher of
         // the two: it takes over exactly where the decay left off.
         float          operator()(float offset, float scale) override
                        {
                           return offset + scale;
                        }

         bool           done() const override      { return false; }
         void           reset() override           {}
         void           config(duration, float) override {}
      };

      /////////////////////////////////////////////////////////////////////////
      // Ramp holders are generic components used to compose segments of an
      // envelope. Multiple ramp segments with distinct shape characteristics
      // may be used to construct ADSR envelopes, AD envelopes, etc. The
      // common feature of a ramp generator is the ability to specify the
      // ramp's width. Available ramp shape forms include exponential,
      // linear, blackman, hold, and hann, both upward and downward variants
      // of each.
      /////////////////////////////////////////////////////////////////////////
      template <concepts::Ramp Base>
      struct ramp_holder : ramp_holder_base, Base
      {
                        ramp_holder(duration width, float sps);

         virtual float  operator()(float offset, float scale) override;
         virtual bool   done() const override;
         virtual void   config(duration width, float sps) override;

         virtual void   reset() override;

      private:

         std::size_t    _time = 0;
         std::size_t    _end = 0;
      };
   }

   ////////////////////////////////////////////////////////////////////////////
   // envelope_segment
   ////////////////////////////////////////////////////////////////////////////
   struct envelope_segment
   {
                        template <typename TID>
                        envelope_segment(TID
                         , duration width
                         , float level
                         , float sps
                        );

                        // From a holder of its own, for a segment whose
                        // shape is not a ramp. See make_constant_segment.
                        envelope_segment(
                           detail::ramp_base_ptr ptr, float level)
                         : _ramp_ptr{std::move(ptr)}
                         , _level{level}
                        {}

                        envelope_segment(envelope_segment const&) = default;
      envelope_segment& operator=(envelope_segment const&) = default;

      float             operator()();

      void              start(float prev_level);
      void              reset();

      bool              done() const;
      float             level() const;

      void              level(float level);
      void              config(duration width, float sps);
      void              config(float level, duration width, float sps);

   private:

      using ramp_base_ptr = detail::ramp_base_ptr;

      ramp_base_ptr     _ramp_ptr;
      float             _level;
      float             _offset = 0.0f;
      float             _scale = 0.0f;
   };

   template <typename T>
   inline envelope_segment make_envelope_segment(duration width, float level, float sps)
   {
      return envelope_segment{std::type_identity<T>{}, width, level, sps};
   }

   // A segment that holds `level` until the envelope is released.
   inline envelope_segment make_constant_segment(float level)
   {
      return envelope_segment{
         std::make_shared<detail::constant_holder>(), level};
   }

   ////////////////////////////////////////////////////////////////////////////
   // envelope_gen
   ////////////////////////////////////////////////////////////////////////////
   struct envelope_gen : std::vector<envelope_segment>
   {
      using base_type = std::vector<envelope_segment>;

                     template <typename ...T>
                     envelope_gen(T&& ...arg);

      void           attack();
      void           release();
      float          operator()();
      void           reset();

      float          current() const;
      bool           in_idle_phase() const;
      bool           in_attack_phase() const;
      bool           in_release_phase() const;
      std::size_t    index() const;

   private:

      std::size_t    _i;
      float          _y = 0.0f;
   };

   ////////////////////////////////////////////////////////////////////////////
   // adsr_envelope_gen
   ////////////////////////////////////////////////////////////////////////////
   struct adsr_envelope_gen : envelope_gen
   {
      struct config
      {
         // Default settings

         duration    attack_rate    = 30_ms;
         duration    decay_rate     = 70_ms;
         decibel     sustain_level  = -6_dB;
         duration    sustain_rate   = 50_s;
         duration    release_rate   = 100_ms;
      };

      // Any config will do; `config` above is just one default kind. What
      // the sustain does is read from the shape of the one it is given: a
      // config with a sustain_rate runs the sustain down over that time,
      // one without holds the sustain level until the note is released.
                     template <concepts::ADSRConfig Config>
                     adsr_envelope_gen(Config const& config, float sps);

      void           attack_rate(duration rate, float sps);
      void           decay_rate(duration rate, float sps);
      void           sustain_level(decibel level);
      void           sustain_level(float level);      // linear, 0 to 1
      void           sustain_rate(duration rate, float sps);
      void           release_rate(duration rate, float sps);
   };

   ////////////////////////////////////////////////////////////////////////////
   // Inline Implementation
   ////////////////////////////////////////////////////////////////////////////
   namespace detail
   {
      inline void detail::ramp_holder_base::config(duration width, float sps)
      {
         this->config(width, sps);
      }

      template <concepts::Ramp Base>
      inline ramp_holder<Base>::ramp_holder(duration width, float sps)
      : Base{width, sps}
      , _end(std::ceil(as_float(width) * sps))
      {
      }

      template <concepts::Ramp Base>
      inline float ramp_holder<Base>::operator()(float offset, float scale)
      {
         ++_time;
         return offset + (Base::operator()() * scale);
      }

      template <concepts::Ramp Base>
      inline bool ramp_holder<Base>::done() const
      {
         return _time >= _end;
      }

      template <concepts::Ramp Base>
      inline void ramp_holder<Base>::config(
         duration width, float sps)
      {
         // A width changed while the ramp is running must not restart it.
         // The generators change only their rate and keep the level they
         // are at, so the ramp carries on from where it is instead of
         // jumping back to the start of the segment, which is heard as a
         // click: a synth's panel moves these under the player's hand.
         //
         // How far through the segment it is is kept as a fraction of the
         // new width, so a ramp shortened while it runs still finishes
         // rather than ending the moment it is shortened.
         auto const end = std::size_t(std::ceil(as_float(width) * sps));
         if (_end != 0)
            _time = (_time * end) / _end;
         _end = end;

         Base::config(width, sps);
      }

      template <concepts::Ramp Base>
      inline void ramp_holder<Base>::reset()
      {
         Base::reset();
         _time = 0;
      }
   }

   template <typename TID>
   inline envelope_segment::envelope_segment(TID, duration width, float level, float sps)
    : _ramp_ptr{std::make_shared<detail::ramp_holder<typename TID::type>>(width, sps)}
    , _level{level}
   {
   }

   inline float envelope_segment::operator()()
   {
      return (*_ramp_ptr)(_offset, _scale);
   }

   inline void envelope_segment::start(float prev_level)
   {
      _offset = std::min(_level, prev_level);
      _scale = std::abs(_level-prev_level);
   }

   inline void envelope_segment::reset()
   {
      _ramp_ptr->reset();
   }

   inline bool envelope_segment::done() const
   {
      return _ramp_ptr->done();
   }

   inline float envelope_segment::level() const
   {
      return _level;
   }

   inline void envelope_segment::level(float level_)
   {
      _level = level_;
   }

   inline void envelope_segment::config(duration width, float sps)
   {
      _ramp_ptr->config(width, sps);
   }

   inline void envelope_segment::config(float level_, duration width, float sps)
   {
      _ramp_ptr->config(width, sps);
      level(level_);
   }

   template <typename ...T>
   inline envelope_gen::envelope_gen(T&& ...arg)
    : base_type{std::forward<T>(arg)...}
   {
      reset();
   }

   inline void envelope_gen::attack()
   {
      if (empty())
         return;

      // Retrigger from any phase, not just idle. Reset every segment's ramp so
      // the envelope runs through attack -> decay -> ... afresh, and start the
      // attack from the current output level so a retrigger mid-note (e.g. a
      // fast arpeggio) is click-free instead of being silently ignored.
      for (auto& s : *this)
         s.reset();
      _i = 0;
      (*this)[_i].start(_y);
   }

   inline void envelope_gen::release()
   {
      if (!in_release_phase())
      {
         _i = size();
         if (_i)
         {
            --_i;
            (*this)[_i].start(_y);
         }
      }
   }

   inline float envelope_gen::operator()()
   {
      if (in_idle_phase())
         return 0.0f;

      _y = (*this)[_i]();
      if ((*this)[_i].done())
      {
         auto prev_i = _i;
         ++_i;
         if (!in_idle_phase())
            (*this)[_i].start((*this)[prev_i].level());
      }
      return _y;
   }

   inline void envelope_gen::reset()
   {
      _i = size();
      for (auto& s : *this)
         s.reset();
   }

   inline float envelope_gen::current() const
   {
      return _y;
   }

   inline bool envelope_gen::in_idle_phase() const
   {
      return _i == size();
   }

   inline bool envelope_gen::in_attack_phase() const
   {
      return size() && _i == 0;
   }

   inline bool envelope_gen::in_release_phase() const
   {
      return (size() >= 2) && (_i == (size()-1));
   }

   inline std::size_t envelope_gen::index() const
   {
      return _i;
   }

   namespace detail
   {
      // The sustain, chosen by what the config carries. A sustain_rate is
      // a time to run down over; without one, the sustain holds.
      template <concepts::ADSRConfig Config>
      inline envelope_segment make_sustain_segment(
         Config const& config_, float sps)
      {
         if constexpr (requires { config_.sustain_rate; })
            return make_envelope_segment<lin_downward_ramp_gen>(
               config_.sustain_rate, 0.0f, sps);
         else
            return make_constant_segment(lin_float(config_.sustain_level));
      }
   }

   template <concepts::ADSRConfig Config>
   inline adsr_envelope_gen::adsr_envelope_gen(
      Config const& config_, float sps)
    : envelope_gen{
         make_envelope_segment<exp_upward_ramp_gen>(
            config_.attack_rate, 1.0f, sps)                             // Attack
       , make_envelope_segment<exp_downward_ramp_gen>(
            config_.decay_rate, lin_float(config_.sustain_level), sps)  // Decay
       , detail::make_sustain_segment(config_, sps)              // Sustain
       , make_envelope_segment<exp_downward_ramp_gen>(
            config_.release_rate, 0.0f, sps)                            // Release
      }
   {
   }

   inline void adsr_envelope_gen::attack_rate(duration rate, float sps)
   {
      (*this)[0].config(rate, sps);
   }

   inline void adsr_envelope_gen::decay_rate(duration rate, float sps)
   {
      (*this)[1].config(rate, sps);
   }

   // The decay is what descends to the sustain, so the level belongs to
   // both: the decay's target and the level the sustain holds are the
   // same thing. Moving only the sustain would leave the decay landing
   // where the sustain used to be, and the sustain unable to hold below
   // it.
   inline void adsr_envelope_gen::sustain_level(decibel level)
   {
      sustain_level(lin_float(level));
   }

   // The linear form is the one the work is done in. A caller that holds
   // the level as a fraction already, a modulation amount for instance,
   // uses it directly rather than paying for a conversion each way.
   inline void adsr_envelope_gen::sustain_level(float level)
   {
      (*this)[1].level(level);
      (*this)[2].level(level);

      // A segment works out where it runs from and to when it is entered,
      // so a level changed afterwards would not reach a note that is
      // already sitting in the sustain. Enter it again at the new level:
      // a hand on the control expects the sound to follow it.
      if (index() == 2)
         (*this)[2].start(level);
   }

   inline void adsr_envelope_gen::sustain_rate(duration rate, float sps)
   {
      (*this)[2].config(rate, sps);
   }

   inline void adsr_envelope_gen::release_rate(duration rate, float sps)
   {
      (*this)[3].config(rate, sps);
   }
}

#endif

