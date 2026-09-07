/*=============================================================================
   Copyright (c) 2019-2026 Joel de Guzman

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(QPLUG_PARAMETER_HPP_OCTOBER_17_2016)
#define QPLUG_PARAMETER_HPP_OCTOBER_17_2016

#include <q/support/midi_messages.hpp>
#include <q/support/decibel.hpp>
#include <q/support/duration.hpp>
#include <q/support/frequency.hpp>
#include <q/support/literals.hpp>

#include <type_traits>
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>

namespace cycfi::qplug
{
   using namespace q::literals;

   ////////////////////////////////////////////////////////////////////////////
   // A parameter, as the host sees it. The id is the parameter's identity
   // for the life of the plugin: never reuse one, never change one. Values
   // are plain, in the parameter's own units (decibels are decibels). The
   // kind decides how the host treats and displays the value; the curve
   // only shapes the GUI control's travel.
   ////////////////////////////////////////////////////////////////////////////
   struct parameter
   {
      enum type
      {
         bool_, int_, double_, note, frequency, decibel, duration, enum_
      };
      using id_type = std::uint32_t;

      template <typename T
       , typename std::enable_if<
            std::is_floating_point<T>::value>::type* = nullptr
      >
      constexpr parameter(id_type id, char const* name, T init)
       : _id(id)
       , _name(name)
       , _type(double_)
       , _init(init)
       , _min(std::min(0.0, double(init)))
       , _max(std::max(1.0, double(init)))
      {}

      template <typename T
       , typename std::enable_if<std::is_integral<T>::value>::type* = nullptr
      >
      constexpr parameter(id_type id, char const* name, T init)
       : _id(id)
       , _name(name)
       , _type(std::is_same<T, bool>::value ? bool_ : int_)
       , _init(init)
       , _min(std::min<double>(0.0, init))
       , _max(std::max<double>(1.0, init))
       , _step(1.0)
      {}

      constexpr parameter(id_type id, char const* name, q::midi_1_0::note init)
       : _id(id)
       , _name(name)
       , _type(note)
       , _init(double(init))
       , _min(double(q::midi_1_0::note::A0))
       , _max(double(q::midi_1_0::note::G9))
       , _step(1.0)
      {}

      constexpr parameter(id_type id, char const* name, q::frequency init)
       : _id(id)
       , _name(name)
       , _type(frequency)
       , _init(init.rep)
       , _min((15_Hz).rep)
       , _max((20_kHz).rep)
       , _unit("Hz")
      {}

      constexpr parameter(id_type id, char const* name, q::duration init)
       : _id(id)
       , _name(name)
       , _type(duration)
       , _init(init.rep)
       , _min(0.0)
       , _max(1.0)
       , _unit("s")
      {}

      constexpr parameter(id_type id, char const* name, q::decibel init)
       : _id(id)
       , _name(name)
       , _type(decibel)
       , _init(init.rep)
       , _min(-60.0)
       , _max(12.0)
       , _unit("dB")
      {}

      // An enumeration: names is null terminated, the value is the index.
      constexpr parameter(id_type id, char const* name, int init
       , char const* const* names)
       : _id(id)
       , _name(name)
       , _type(enum_)
       , _init(init)
       , _min(0.0)
       , _max(double(count(names) - 1))
       , _step(1.0)
       , _names(names)
      {}

      template <typename T>
      constexpr parameter range(T min, T max) const
      {
         parameter r = *this;
         r._min = double(min);
         r._max = double(max);
         r._init = std::clamp(_init, double(min), double(max));
         return r;
      }

      template <typename T>
      constexpr parameter range(T min, T max, T step) const
      {
         parameter r = range(min, max);
         r._step = double(step);
         return r;
      }

      template <typename T>
      constexpr parameter step(T step_) const
      {
         parameter r = *this;
         r._step = double(step_);
         return r;
      }

      constexpr parameter curve(double curve_) const
      {
         parameter r = *this;
         r._curve = curve_;
         return r;
      }

      constexpr parameter unit(char const* unit_) const
      {
         parameter r = *this;
         r._unit = unit_;
         return r;
      }

      constexpr parameter module(char const* module_) const
      {
         parameter r = *this;
         r._module = module_;
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

      constexpr parameter hidden() const
      {
         parameter r = *this;
         r._hidden = true;
         return r;
      }

      constexpr parameter bypass() const
      {
         parameter r = *this;
         r._bypass = true;
         return r;
      }

      constexpr parameter periodic() const
      {
         parameter r = *this;
         r._periodic = true;
         return r;
      }

      constexpr bool stepped() const
      {
         return _type == bool_ || _type == int_ || _type == note
            || _type == enum_;
      }

      // The GUI control's travel, 0 to 1, from a value and back. The curve
      // is the exponent: 1 is linear, more bunches values toward the low
      // end, as for a frequency.
      double position(double value) const
      {
         auto span = _max - _min;
         auto norm = span > 0
            ? (std::clamp(value, _min, _max) - _min) / span : 0.0;
         return _curve == 1.0 ? norm : std::pow(norm, 1.0 / _curve);
      }

      double value(double position_) const
      {
         auto pos = std::clamp(position_, 0.0, 1.0);
         auto norm = _curve == 1.0 ? pos : std::pow(pos, _curve);
         return _min + norm * (_max - _min);
      }

      // Text for a value, and a value from text, in the parameter's units.
      // Returns false if the buffer is too small or the text unreadable.
      bool to_text(double val, char* text, std::size_t size) const
      {
         int n = 0;
         switch (_type)
         {
            case bool_:
               n = std::snprintf(text, size, "%s", val > 0.5 ? "On" : "Off");
               break;
            case int_:
               n = std::snprintf(text, size, "%d%s%s", int(std::round(val))
                , *_unit ? " " : "", _unit);
               break;
            case enum_:
               {
                  auto i = std::clamp(int(std::round(val)), 0, int(_max));
                  n = std::snprintf(text, size, "%s", _names[i]);
               }
               break;
            case note:
               n = std::snprintf(text, size, "%s"
                , q::midi_1_0::note_name(std::uint8_t(std::round(val))));
               break;
            case frequency:
               n = val >= 1000.0
                  ? std::snprintf(text, size, "%.2f kHz", val / 1000.0)
                  : std::snprintf(text, size, "%.1f Hz", val);
               break;
            case decibel:
               n = std::snprintf(text, size, "%.1f dB", val);
               break;
            case duration:
               n = val < 1.0
                  ? std::snprintf(text, size, "%.1f ms", val * 1000.0)
                  : std::snprintf(text, size, "%.3f s", val);
               break;
            case double_:
               n = std::snprintf(text, size, "%.3f", val);
               if (n > 0 && std::size_t(n) < size)
               {
                  // Trim the trailing zeros: 0.950 reads as 0.95, 1.000
                  // as 1, which is what a label wants.
                  while (n > 1 && text[n-1] == '0')
                     text[--n] = 0;
                  if (n > 1 && text[n-1] == '.')
                     text[--n] = 0;
                  n += std::snprintf(text + n, size - n, "%s%s"
                   , *_unit ? " " : "", _unit);
               }
               break;
         }
         return n > 0 && std::size_t(n) < size;
      }

      bool from_text(char const* text, double& val) const
      {
         switch (_type)
         {
            case bool_:
               val = (std::strncmp(text, "On", 2) == 0
                  || std::strncmp(text, "on", 2) == 0
                  || std::atof(text) > 0.5) ? 1.0 : 0.0;
               return true;
            case enum_:
               for (int i = 0; _names[i]; ++i)
                  if (std::strcmp(text, _names[i]) == 0)
                  {
                     val = i;
                     return true;
                  }
               return false;
            case note:
               for (int key = int(_min); key <= int(_max); ++key)
               {
                  auto name = q::midi_1_0::note_name(std::uint8_t(key));
                  if (std::strcmp(text, name) == 0)
                  {
                     val = key;
                     return true;
                  }
               }
               return false;
            default:
               {
                  char* end = nullptr;
                  val = std::strtod(text, &end);
                  if (end == text)
                     return false;
                  while (*end == ' ') ++end;
                  if (_type == frequency && (*end == 'k' || *end == 'K'))
                     val *= 1000.0;
                  if (_type == duration && *end == 'm')
                     val /= 1000.0;
                  val = std::clamp(val, _min, _max);
                  return true;
               }
         }
      }

      id_type              _id;
      char const*          _name;
      type                 _type;
      double               _init;
      double               _min = 0.0;
      double               _max = 1.0;
      double               _step = 0.001;
      double               _curve = 1.0;
      char const*          _unit = "";
      char const*          _module = "";
      char const* const*   _names = nullptr;
      bool                 _can_automate = true;
      bool                 _save_in_preset = true;
      bool                 _hidden = false;
      bool                 _bypass = false;
      bool                 _periodic = false;

   private:

      static constexpr int count(char const* const* names)
      {
         int n = 0;
         while (names[n]) ++n;
         return n;
      }
   };

   ////////////////////////////////////////////////////////////////////////////
   // A parameter's value is a plain double, as CLAP carries it; its kind
   // says what that double means. These traits carry it back to the type
   // the plugin thinks in, so a controller can hand out a q::decibel or a
   // q::frequency rather than a naked number.
   ////////////////////////////////////////////////////////////////////////////
   template <typename T>
   struct parameter_traits;

   template <>
   struct parameter_traits<bool>
   {
      static constexpr bool matches(parameter::type t)
      {
         return t == parameter::bool_;
      }

      static constexpr bool get(double val) { return val > 0.5; }
      static constexpr double set(bool val) { return val ? 1.0 : 0.0; }
   };

   template <>
   struct parameter_traits<int>
   {
      // An enumeration is an index, so it reads as an int too.
      static constexpr bool matches(parameter::type t)
      {
         return t == parameter::int_ || t == parameter::enum_;
      }

      static constexpr int get(double val)
      {
         return int(val < 0 ? val - 0.5 : val + 0.5);
      }

      static constexpr double set(int val) { return val; }
   };

   template <>
   struct parameter_traits<double>
   {
      static constexpr bool matches(parameter::type t)
      {
         return t == parameter::double_;
      }

      static constexpr double get(double val) { return val; }
      static constexpr double set(double val) { return val; }
   };

   template <>
   struct parameter_traits<q::decibel>
   {
      static constexpr bool matches(parameter::type t)
      {
         return t == parameter::decibel;
      }

      static constexpr q::decibel get(double val) { return q::dB(val); }
      static constexpr double set(q::decibel val) { return val.rep; }
   };

   template <>
   struct parameter_traits<q::duration>
   {
      static constexpr bool matches(parameter::type t)
      {
         return t == parameter::duration;
      }

      static constexpr q::duration get(double val)
      {
         return q::duration{val};
      }

      static constexpr double set(q::duration val) { return val.rep; }
   };

   template <>
   struct parameter_traits<q::frequency>
   {
      static constexpr bool matches(parameter::type t)
      {
         return t == parameter::frequency;
      }

      static constexpr q::frequency get(double val)
      {
         return q::frequency{val};
      }

      static constexpr double set(q::frequency val) { return val.rep; }
   };

   template <>
   struct parameter_traits<q::midi_1_0::note>
   {
      static constexpr bool matches(parameter::type t)
      {
         return t == parameter::note;
      }

      static q::midi_1_0::note get(double val)
      {
         return q::midi_1_0::note(std::uint8_t(std::round(val)));
      }

      static constexpr double set(q::midi_1_0::note val)
      {
         return double(val);
      }
   };
}

#endif
