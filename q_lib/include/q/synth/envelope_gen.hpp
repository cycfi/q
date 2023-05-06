/*=============================================================================
   Copyright (c) 2014-2023 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(CYCFI_Q_SYNTH_ENVELOPE_GEN_HPP_MAY_6_2023)
#define CYCFI_Q_SYNTH_ENVELOPE_GEN_HPP_MAY_6_2023

#include <q/synth/ramp_gen.hpp>
#include <memory>
#include <list>
#include <type_traits>

namespace cycfi::q
{
   using ramp_base_ptr = std::shared_ptr<ramp_base>;

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

      float          operator()();

      void           start(float prev_level);
      void           reset();

      bool           done() const;
      float          level() const;
      void           level(float level_);
      void           config(duration width, float sps);
      void           config(duration width, float level_, float sps);

   private:

      ramp_base_ptr  _ramp_ptr;
      float          _level;
      float          _offset = 0.0f;
      float          _scale = 0.0f;
   };

   template <typename T>
   inline auto make_envelope_segment(duration width, float level, float sps)
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

      void           start_attack(float velocity = 1.0f);
      void           start_release(float velocity = 1.0f);
      float          operator()();

   private:

      void           reset();

      iterator_type  _curr_segment;
      float          _y = 0.0f;
   };

   ////////////////////////////////////////////////////////////////////////////
   // Inline Implementation
   ////////////////////////////////////////////////////////////////////////////
   template <typename TID>
   inline envelope_segment::envelope_segment(TID, duration width, float level, float sps)
    : _ramp_ptr{std::make_unique<ramp_gen<typename TID::type>>(width, sps)}
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

   inline void envelope_segment::config(duration width, float level_, float sps)
   {
      _ramp_ptr->config(width, sps);
      level(level_);
   }

   inline void envelope_gen::start_attack(float velocity)
   {
      reset();
      _curr_segment = begin();
      if (_curr_segment != end())
         _curr_segment->start(0.0f);
   }

   inline void envelope_gen::start_release(float velocity)
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
      for (auto& s : *this)
         s.reset();
   }
}

#endif

