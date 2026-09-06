/*=============================================================================
   Copyright (c) 2026 Joel de Guzman

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(QPLUG_GAIN_CONTROLLER_SEPTEMBER_6_2026)
#define QPLUG_GAIN_CONTROLLER_SEPTEMBER_6_2026

#include <qplug/controller.hpp>
#include <elements/model.hpp>
#include <atomic>

namespace qplug = cycfi::qplug;
namespace elements = cycfi::elements;
using parameter = cycfi::qplug::parameter;

///////////////////////////////////////////////////////////////////////////////
// Volume, in decibels, is held twice, on purpose. The atomic is what the
// audio thread reads and what host automation writes. The model is the
// main-thread mirror the presenter links to; update_models brings it in
// step.
///////////////////////////////////////////////////////////////////////////////
class gain_controller : public qplug::controller
{
public:

   using volume_model = elements::value_model<double>;

   parameter_list       parameters() const override;
   double               get_parameter(int id) const override;
   void                 set_parameter(int id, double value) override;
   void                 update_models() override;
   void                 edit_parameter(int id, double value) override;

   double               volume() const;      // decibels
   volume_model&        volume_model_() { return _volume_model; }
   parameter const&     volume_param() const { return parameters()[0]; }

private:

   std::atomic<double>  _volume{ 0.0 };
   std::atomic<bool>    _dirty{ false };
   volume_model         _volume_model{ 0.0 };
};

///////////////////////////////////////////////////////////////////////////////
// Inline implementation
///////////////////////////////////////////////////////////////////////////////
inline double gain_controller::volume() const
{
   return _volume.load(std::memory_order_relaxed);
}

#endif
