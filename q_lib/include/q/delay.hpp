/*=============================================================================
   Copyright (c) 2014-2018 Cycfi Research. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(CYCFI_Q_DELAY_JULY_20_2014)
#define CYCFI_Q_DELAY_JULY_20_2014

#include <q/lut.hpp>

namespace cycfi { namespace q
{
   ////////////////////////////////////////////////////////////////////////////
   // delay: a basic class for implementing multi-tapped delays
   ////////////////////////////////////////////////////////////////////////////
   template <
      typename T
    , typename Storage = buffer<T>
    , typename Interpolation = sample_interpolation::none>
   class delay
   {
   public:

      typedef T value_type;
      typedef Storage storage_type;
      typedef Interpolation interpolation_type;

      // tap representation
      struct tap
      {
         tap(T secs, std::uint32_t sps) : tsamples(secs * sps) {}
         T tsamples; // time in samples
      };

      // constructor (max_delay in seconds)
      delay(T max_delay, std::uint32_t sps)
       : lu(std::size_t(std::ceil(max_delay * sps)))
      {}

      // constructor (max_delay in seconds)
      template <typename Taps>
      delay(T max_delay, Taps const& taps, std::uint32_t sps)
       : lu(std::size_t(std::ceil(max_delay * sps)))
       , taps(taps)
      {}

      // copy and assign
      delay(delay const& rhs) = default;
      delay(delay&& rhs) = default;
      delay& operator=(delay const& rhs) = default;
      delay& operator=(delay&& rhs) = default;

      // mix
      template <typename Mixer>
      T operator()(Mixer mixer, T mix = T()) const
      {
         interpolation_type interpolate;
         for (auto tap : taps)
            mix = mixer(interpolate(lu, tap.tsamples), mix);
         return mix;
      }

      // push fresh data to the front of the delay
      friend void operator>>(T val, delay& d)
      {
         val >> d.lu;
      }

      // the taps
      std::vector<tap> taps;

   private:

        lut<T, Storage, Interpolation> lu;
   };

   ////////////////////////////////////////////////////////////////////////////
   // single_delay: a basic class for implementing single delays
   ////////////////////////////////////////////////////////////////////////////
   template <
      typename T
    , typename Storage = buffer<T>
    , typename Interpolation = sample_interpolation::none>
   class single_delay
   {
   public:

      typedef T value_type;
      typedef Storage storage_type;
      typedef Interpolation interpolation_type;

      // constructor (max_delay in seconds)
      single_delay(T max_delay, std::uint32_t sps)
       : lu(std::size_t(std::ceil(max_delay * sps)))
       , tsamples_delay(max_delay * sps)
      {}

      // constructor (max_delay and delay in seconds)
      single_delay(T max_delay, T delay, std::uint32_t sps)
       : lu(std::size_t(std::ceil(max_delay * sps)))
       , tsamples_delay(delay * sps)
      {}

      // constructor (max_delay in samples)
      single_delay(std::size_t max_delay)
       : lu(max_delay)
       , tsamples_delay(max_delay)
      {}

      // constructor (max_delay and delay in samples)
      single_delay(std::size_t max_delay, std::size_t delay)
       : lu(max_delay)
       , tsamples_delay(delay)
      {}

      // copy and assign
      single_delay(single_delay const& rhs) = default;
      single_delay(single_delay&& rhs) = default;
      single_delay& operator=(single_delay const& rhs) = default;
      single_delay& operator=(single_delay&& rhs) = default;

      // get the delayed signal
      T operator()() const
      {
         interpolation_type interpolate;
         return interpolate(lu, tsamples_delay);
      }

      // push a new signal and return the delayed signal
      T operator()(T val)
      {
         T delayed = (*this)();
         val >> *this;
         return delayed;
      }

      // push fresh data to the front of the delay
      friend void operator>>(T val, single_delay& d)
      {
         val >> d.lu;
      }

      // get/set the delay (in seconds)
      void delay(T delay, std::uint32_t sps) { tsamples_delay = delay * sps; }
      T delay(uint32_t sps) const { return tsamples_delay / sps; }

      // get/set the delay (in samples)
      void samples_delay(T samples) { tsamples_delay = samples; }
      T samples_delay() const { return tsamples_delay; }

   private:

      lut<T, Storage, Interpolation> lu;
      T tsamples_delay;
   };
}}

#endif
