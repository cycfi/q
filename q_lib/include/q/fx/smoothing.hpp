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
    ///////////////////////////////////////////////////////////////////////////////
    // Ensures float-safe snap
    ///////////////////////////////////////////////////////////////////////////////
    struct snap
    {
        explicit snap(float delta) : _delta(delta) {}

        float operator()(float s, float target)
        {
            _settled = std::abs(s - target) <= _delta || s == _prev;
            _prev = s;
            return _settled? target : s;
        }

        bool settled() const { return _settled; }

        float _delta;
        float _prev = std::numeric_limits<float>::quiet_NaN();
        bool  _settled = false;
    };

    ///////////////////////////////////////////////////////////////////////////////
    // Smoothed value parameter that utilizes a one-pole lowpass
    ///////////////////////////////////////////////////////////////////////////////
    class smoothed_value
    {
    private:
        float               _target = 0.0f;
        bool                _active = false;
        one_pole_lowpass    _lp;
        snap                _snap;

    public:
        smoothed_value(float value, duration smoothing_time, double sps)
            : _target(value), _lp(1.0f / (2_pi * as_float(smoothing_time)), sps), _snap(1e-4f)
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
                _lp = _target = target;
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
                return _lp();

           _snap(_lp(_target), _target);

            // Snap to value when close enough
            if (_snap.settled())
            {
                _lp = _target;
                _active = false;
            }

            return _lp();
        }

        float value() const { return _lp(); }
        float target() const { return _target; }
        bool active() const { return _active; }
    };
}

#endif