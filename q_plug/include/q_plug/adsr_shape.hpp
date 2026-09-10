/*=============================================================================
   Copyright (c) 2019-2026 Joel de Guzman

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(QPLUG_ADSR_SHAPE_HPP_SEPTEMBER_11_2026)
#define QPLUG_ADSR_SHAPE_HPP_SEPTEMBER_11_2026

#include <algorithm>
#include <cmath>

namespace cycfi::qplug
{
   ////////////////////////////////////////////////////////////////////////////
   // adsr_shape: an envelope as a drawing, and a drawing as an envelope.
   //
   // The four values are the control's travel, 0 to 1 each, not seconds or
   // decibels: a parameter's own taper turns them into those. The corners
   // come back as points in a unit square, x to the right and y upward,
   // which a control scales into its bounds.
   //
   //          peak
   //           /\__ corner ____ plateau
   //          /                        \
   //         /                          \
   //     start                          finish
   //
   // Attack, decay and release each own a share of the width, so a long
   // attack pushes the peak right without changing the fall after it, and
   // everything at full still fits. The plateau is fixed: a sustain has
   // no length, it lasts as long as the key is down.
   ////////////////////////////////////////////////////////////////////////////
   struct adsr_shape
   {
      struct point
      {
         float x = 0.0f;
         float y = 0.0f;
      };

      enum handle
      {
         no_handle = -1
       , attack_handle           // the peak
       , decay_handle            // the corner: how far it falls, and to where
       , release_handle          // the end of the tail
      };

      // The width each stage may take up, and the fixed plateau between
      // the fall and the tail. They come to one when all are at full.
      static constexpr float stage_width = 0.28f;
      static constexpr float plateau_width = 0.16f;

      float          attack = 0.2f;
      float          decay = 0.3f;
      float          sustain = 0.6f;      // a level, not a time
      float          release = 0.3f;

      point          start() const;
      point          peak() const;
      point          corner() const;
      point          plateau() const;
      point          finish() const;

      // The corner nearest p, if one is within reach; no_handle if none.
      handle         nearest(point p, float reach) const;

      // Take a corner to a point. Each corner carries the values it is
      // made of and no others, so a drag says one thing at a time, except
      // the corner of the fall, which is where the decay and the sustain
      // meet and so carries both.
      void           drag(handle which, point to);
   };

   ////////////////////////////////////////////////////////////////////////////
   // Inline Implementation
   ////////////////////////////////////////////////////////////////////////////
   namespace detail
   {
      inline float clamp01(float v)
      {
         return std::clamp(v, 0.0f, 1.0f);
      }

      inline float distance(adsr_shape::point a, adsr_shape::point b)
      {
         auto const dx = a.x - b.x;
         auto const dy = a.y - b.y;
         return std::sqrt((dx * dx) + (dy * dy));
      }
   }

   inline adsr_shape::point adsr_shape::start() const
   {
      return {0.0f, 0.0f};
   }

   inline adsr_shape::point adsr_shape::peak() const
   {
      return {attack * stage_width, 1.0f};
   }

   inline adsr_shape::point adsr_shape::corner() const
   {
      return {peak().x + (decay * stage_width), sustain};
   }

   inline adsr_shape::point adsr_shape::plateau() const
   {
      return {corner().x + plateau_width, sustain};
   }

   inline adsr_shape::point adsr_shape::finish() const
   {
      return {plateau().x + (release * stage_width), 0.0f};
   }

   inline adsr_shape::handle
   adsr_shape::nearest(point p, float reach) const
   {
      struct { handle which; point at; } const corners[] =
      {
         {attack_handle, peak()}
       , {decay_handle, corner()}
       , {release_handle, finish()}
      };

      auto best = no_handle;
      auto best_distance = reach;
      for (auto const& c : corners)
      {
         auto const d = detail::distance(p, c.at);
         if (d <= best_distance)
         {
            best = c.which;
            best_distance = d;
         }
      }
      return best;
   }

   inline void adsr_shape::drag(handle which, point to)
   {
      switch (which)
      {
         case attack_handle:
            attack = detail::clamp01(to.x / stage_width);
            break;

         case decay_handle:
            decay = detail::clamp01((to.x - peak().x) / stage_width);
            sustain = detail::clamp01(to.y);
            break;

         case release_handle:
            release = detail::clamp01((to.x - plateau().x) / stage_width);
            break;

         default:
            break;
      }
   }
}

#endif
