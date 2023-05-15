/*=============================================================================
   Copyright (c) 2014-2023 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(CYCFI_Q_OSC_ENVELOPE_GEN_HPP_MAY_6_2023)
#define CYCFI_Q_OSC_ENVELOPE_GEN_HPP_MAY_6_2023

#include <q/support/base.hpp>
#include <q/synth/concepts.hpp>
#include <q/synth/exponential_gen.hpp>
#include <q/synth/linear_gen.hpp>

#include <memory>
#include <list>
#include <type_traits>

namespace cycfi::q
{
   ////////////////////////////////////////////////////////////////////////////
   // Ramp holder abstract base class. This is provided so we can hold
   // references (pointers, smart pointers, etc.) to ramp generators in std
   // containers.
   ////////////////////////////////////////////////////////////////////////////
   struct ramp_holder_base
   {
      virtual        ~ramp_holder_base() = default;
      virtual float  operator()(float offset, float scale) = 0;
      virtual bool   done() const = 0;
      virtual void   reset() = 0;

      virtual void   config(duration width, float sps) = 0;
   };

   using ramp_base_ptr = std::shared_ptr<ramp_holder_base>;

   ////////////////////////////////////////////////////////////////////////////
   // Ramp holders are generic components used to compose segments of an
   // envelope. Multiple ramp segments with distinct shape characteristics
   // may be used to construct ADSR envelopes, AD envelopes, etc. The common
   // feature of a ramp generator is the ability to specify the ramp's width.
   // Available ramp shape forms include exponential, linear, blackman, hold,
   // and hann, both upward and downward variants of each.
   ////////////////////////////////////////////////////////////////////////////
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

   ////////////////////////////////////////////////////////////////////////////
   // envelope_gen
   ////////////////////////////////////////////////////////////////////////////
   struct envelope_gen : std::list<envelope_segment>
   {
      using base_type = std::list<envelope_segment>;
      using iterator_type = base_type::iterator;

      using base_type::base_type;

      void           attack();
      void           release();
      float          operator()();
      float          current() const;
      void           reset();

   private:


      iterator_type  _curr_segment;
      float          _y = 0.0f;
   };

   ////////////////////////////////////////////////////////////////////////////
   // exp_envelope_gen
   ////////////////////////////////////////////////////////////////////////////
   struct exp_envelope_gen : envelope_gen
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

                     exp_envelope_gen(config const& config, float sps);
                     exp_envelope_gen(float sps);

      void           attack_rate(duration rate, float sps);
      void           decay_rate(duration rate, float sps);
      void           sustain_level(decibel level);
      void           sustain_rate(duration rate, float sps);
      void           release_rate(duration rate, float sps);
   };

   ////////////////////////////////////////////////////////////////////////////
   // Inline Implementation
   ////////////////////////////////////////////////////////////////////////////
   inline void ramp_holder_base::config(duration width, float sps)
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
      Base::config(width, sps);
      _end = std::ceil(as_float(width) * sps);

      Base::reset();
      reset();
   }

   template <concepts::Ramp Base>
   inline void ramp_holder<Base>::reset()
   {
      Base::reset();
      _time = 0;
   }

   template <typename TID>
   inline envelope_segment::envelope_segment(TID, duration width, float level, float sps)
    : _ramp_ptr{std::make_shared<ramp_holder<typename TID::type>>(width, sps)}
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

   inline void envelope_gen::attack()
   {
      reset();
      _curr_segment = begin();
      if (_curr_segment != end())
         _curr_segment->start(0.0f);
   }

   inline void envelope_gen::release()
   {
      auto i = end();
      if (i != begin())
      {
         _curr_segment = --i;
         _curr_segment->start(_y);
      }
   }

   inline float envelope_gen::operator()()
   {
      if (_curr_segment != end())
      {
         _y = (*_curr_segment)();
         if (_curr_segment->done())
         {
            auto prev_segment = _curr_segment;
            ++_curr_segment;
            if (_curr_segment != end())
               _curr_segment->start(prev_segment->level());
         }
         return _y;
      }
      return 0.0f;
   }

   inline void envelope_gen::reset()
   {
      _curr_segment = end();
      for (auto& s : *this)
         s.reset();
   }

   inline float envelope_gen::current() const
   {
      return _y;
   }

   inline exp_envelope_gen::exp_envelope_gen(config const& config_, float sps)
    : envelope_gen{
         make_envelope_segment<exp_upward_ramp_gen>(
            config_.attack_rate, 1.0f, sps)                             // Attack
       , make_envelope_segment<exp_downward_ramp_gen>(
            config_.decay_rate, as_float(config_.sustain_level), sps)   // Decay
       , make_envelope_segment<lin_downward_ramp_gen>(
            config_.sustain_rate, 0.0f, sps)                            // Sustain
       , make_envelope_segment<exp_downward_ramp_gen>(
            config_.release_rate, 0.0f, sps)                            // Release
      }
   {
      reset();
   }

   inline exp_envelope_gen::exp_envelope_gen(float sps)
    : exp_envelope_gen(config{}, sps)
   {
   }

   inline void exp_envelope_gen::attack_rate(duration rate, float sps)
   {
      front().config(rate, sps);
   }

   inline void exp_envelope_gen::decay_rate(duration rate, float sps)
   {
      auto i = begin();
      (++i)->config(rate, sps);
   }

   inline void exp_envelope_gen::sustain_level(decibel level)
   {
      auto i = begin();
      (++++i)->level(as_float(level));
   }

   inline void exp_envelope_gen::sustain_rate(duration rate, float sps)
   {
      auto i = begin();
      (++++i)->config(rate, sps);
   }

   inline void exp_envelope_gen::release_rate(duration rate, float sps)
   {
      back().config(rate, sps);
   }
}

#endif

