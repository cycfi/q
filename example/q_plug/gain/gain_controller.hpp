/*=============================================================================
   Copyright (c) 2026 Joel de Guzman

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(QPLUG_GAIN_CONTROLLER_SEPTEMBER_6_2026)
#define QPLUG_GAIN_CONTROLLER_SEPTEMBER_6_2026

#include <qplug/controller.hpp>

namespace qplug = cycfi::qplug;

///////////////////////////////////////////////////////////////////////////////
class gain_controller : public qplug::controller
{
public:

   parameter_list       parameters() const override;
   double               get_parameter(int id) const override;
   void                 set_parameter(int id, double value) override;

   double               volume() const { return _volume; }

private:

   double               _volume = 1.0;
};

#endif
