/*=============================================================================
   Copyright (c) 2019 Joel de Guzman

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(QPLUG_PARAMETER_HPP_OCTOBER_17_2016)
#define QPLUG_PARAMETER_HPP_OCTOBER_17_2016

#include <type_traits>
#include <algorithm>
#include <q/support/midi_messages.hpp>

#include <ostream>
#include <iomanip>

namespace cycfi::qplug
{
   using namespace q::literals;

   struct parameter
   {
      enum type { bool_, int_, double_, note, frequency };

      template <typename T
       , typename std::enable_if<std::is_floating_point<T>::value>::type* = nullptr
      >
      constexpr parameter(char const* name, T init)
       : _name(name)
       , _type(double_)
       , _init(init)
       , _min(std::min(0.0, init))
       , _max(std::max(1.0, init))
      {}

      template <typename T
       , typename std::enable_if<std::is_integral<T>::value>::type* = nullptr
      >
      constexpr parameter(char const* name, T init)
       : _name(name)
       , _type(std::is_same<T, bool>::value ? bool_ : int_)
       , _init(init)
       , _min(std::min<double>(0.0, init))
       , _max(std::max<double>(1.0, init))
       , _step(1.0)
      {}

      constexpr parameter(char const* name, q::midi_1_0::note init)
       : _name(name)
       , _type(note)
       , _init(double(init))
       , _min(double(q::midi_1_0::note::A0))
       , _max(double(q::midi_1_0::note::G9))
       , _step(1.0)
      {}

      constexpr parameter(char const* name, q::frequency init)
       : _name(name)
       , _type(frequency)
       , _init(init.rep)
       , _min((15_Hz).rep)
       , _max((20_kHz).rep)
       , _unit("Hz")
      {}

      template <typename T>
      constexpr parameter range(T min, T max) const
      {
         parameter r = *this;
         r._min = double(min);
         r._max = double(max);
         (void) std::clamp(_init, double(min), double(max));
         return r;
      }

      template <typename T>
      constexpr parameter range(T min, T max, T step) const
      {
         parameter r = *this;
         r._min = min;
         r._max = max;
         r._step = step;
         std::clamp(_init, min, max);
         return r;
      }

      template <typename T>
      constexpr parameter step(T step_) const
      {
         parameter r = *this;
         r._step = step_;
         return r;
      }

      constexpr parameter curve(double curve_) const
      {
         parameter r = *this;
         r._curve = curve_;
         return r;
      }

      constexpr parameter dont_automate() const
      {
         parameter r = *this;
         r._can_automate = false;
         return r;
      }

      constexpr parameter dont_save_in_preset() const
      {
         parameter r = *this;
         r._save_in_preset = false;
         return r;
      }

      constexpr parameter unit(char const* unit_) const
      {
         parameter r = *this;
         r._unit = unit_;
         return r;
      }

      void print(std::ostream& out, double val) const
      {
         switch (_type)
         {
            case bool_:
               out << ((val > 0.5)? "true" : "false");
               break;
            case int_:
               out << std::round(val);
               break;
            case frequency:
            case double_:
               out << val;
               break;
            case note:
               auto midi_note = std::uint8_t(val);
               out << '"' << q::midi_1_0::note_name(midi_note) << '"';
               break;
         }
      }

      char const*    _name;
      type           _type;
      double         _init;
      double         _min = 0.0;
      double         _max = 1.0;
      double         _step = 0.001;
      double         _curve = 1.0;
      char const*    _unit = "";
      bool           _can_automate = true;
      bool           _save_in_preset = true;
   };
}

#endif