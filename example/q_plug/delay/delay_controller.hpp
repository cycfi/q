/*=============================================================================
   Copyright (c) 2019-2026 Joel de Guzman

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(QPLUG_DELAY_CONTROLLER_SEPTEMBER_7_2026)
#define QPLUG_DELAY_CONTROLLER_SEPTEMBER_7_2026

#include <qplug/controller.hpp>

namespace qplug = cycfi::qplug;

///////////////////////////////////////////////////////////////////////////////
// The controller declares the plugin's parameters; the base holds them.
///////////////////////////////////////////////////////////////////////////////
class delay_controller : public qplug::controller
{
public:

   using duration = cycfi::q::duration;

   enum { delay_id, feedback_id };

   // The delay line is sized for the longest delay the parameter allows.
   static constexpr duration max_delay = cycfi::q::duration{1.0};

   parameter_list       parameters() const override;

   duration             delay() const;
   double               feedback() const;      // percent
};

///////////////////////////////////////////////////////////////////////////////
// Inline implementation
///////////////////////////////////////////////////////////////////////////////
inline delay_controller::duration delay_controller::delay() const
{
   return get_parameter<duration>(delay_id);
}

inline double delay_controller::feedback() const
{
   return get_parameter<double>(feedback_id);
}

#endif
