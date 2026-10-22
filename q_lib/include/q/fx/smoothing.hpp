/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman, Nikos Parastatidis. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(CYCFI_Q_EXP_SMOOTHING_OCTOBER_22_2026)
#define CYCFI_Q_EXP_SMOOTHING_OCTOBER_22_2026

#include <q/support/base.hpp>
#include <q/support/literals.hpp>
#include <q/fx/lowpass.hpp>

namespace cycfi::q
{
    class smoothed_value
    {
    private:
        float _value = 0.0f, _target = 0.0f;
        bool _active = false;
        one_pole_lowpass _lp;

    public:
        smoothed_value(float value, duration smoothing_time, double sps)
            : _value(value), _target(value), _lp(1.0f / (2_pi * as_float(smoothing_time)), sps)
        {
            _lp = value;
        }

        void config(duration smoothing_time, double sps)
        {
            _lp.cutoff(1.0f / (2_pi * as_float(smoothing_time)), sps);
        }

        void operator()(float target, bool snap = false)
        {
            if (snap)
            {
                _lp = _value = _target = target;
                _active = false;
            }
            else 
            {
                _target = target;
                _active = true;
            }
        }

        float operator()()
        {
            if (!_active)
                return _value;

            _value = _lp(_target);

            // Snap to value when close enough
            if (std::abs(_value - _target) < 0.00001f)
            {
                _value = _target;
                _active = false;
            }

            return _value;
        }

        float value() const { return _value; }
        float target() const { return _target; }
        bool active() const { return _active; }
    };
}

#endif