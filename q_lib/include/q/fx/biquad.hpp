/*=============================================================================
   Copyright (c) 2014-2020 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(CYCFI_Q_BIQUAD_HPP_FEBRUARY_8_2018)
#define CYCFI_Q_BIQUAD_HPP_FEBRUARY_8_2018

#include <q/support/base.hpp>
#include <cmath>

namespace cycfi::q
{
   ////////////////////////////////////////////////////////////////////////////
   // biquad class. Based on Audio-EQ Cookbook by Robert Bristow-Johnson.
   // https://www.w3.org/2011/audio/audio-eq-cookbook.html
   ////////////////////////////////////////////////////////////////////////////
   struct biquad
   {
      biquad(biquad const&) = default;

      biquad(float a0, float a1, float a2, float a3, float a4)
       : a0(a0), a1(a1), a2(a2), a3(a3), a4(a4)
       , x1(0), x2(0), y1(0), y2(0)
      {}

      float operator()(float s)
      {
         // compute result
         auto r = a0 * s + a1 * x1 + a2 * x2 - a3 * y1 - a4 * y2;

         // shift x1 to x2, s to x1
         x2 = x1;
         x1 = s;

         // shift y1 to y2, r to y1
         y2 = y1;
         y1 = r;

         return r;
      }

      void config(float a0_, float a1_, float a2_, float a3_, float a4_)
      {
         a0 = a0_;
         a1 = a1_;
         a2 = a2_;
         a3 = a3_;
         a4 = a4_;
      }

      // Coefficients
      float a0, a1, a2, a3, a4;

      // Sample delays
      float x1, x2, y1, y2;
   };

   ////////////////////////////////////////////////////////////////////////////
   // bw: utility type to distinguish bandwidth from q which is just a double
   ////////////////////////////////////////////////////////////////////////////
   struct bw
   {
      double val; // in octaves
   };

   namespace detail
   {
      /////////////////////////////////////////////////////////////////////////
      struct config_biquad
      {
         config_biquad(frequency f, std::uint32_t sps)
          : omega(2_pi * double(f) / sps)
          , sin(std::sin(omega))
          , cos(std::cos(omega))
         {}

         config_biquad(frequency f, std::uint32_t sps, bw _bw)
          : config_biquad(f, sps)
         {
            alpha = sin * std::sinh(std::log(2.0) / 2.0 * _bw.val * omega / sin);
         }

         config_biquad(frequency f, std::uint32_t sps, double q)
          : config_biquad(f, sps)
         {
            alpha = sin / (2.0 * q);
         }

         biquad make()
         {
            return biquad(b0 / a0, b1 / a0, b2 / a0, a1 / a0, a2 / a0);
         }

         void config(biquad& bq)
         {
            bq.config(b0 / a0, b1 / a0, b2 / a0, a1 / a0, a2 / a0);
         }

         double omega, sin, cos, alpha;
         double a0, a1, a2, b0, b1, b2;
      };

      /////////////////////////////////////////////////////////////////////////
      struct config_biquad_a : config_biquad
      {
         config_biquad_a(double db_gain, frequency f, std::uint32_t sps, bw _bw)
          : config_biquad(f, sps, _bw)
          , a(std::pow(10.0, db_gain / 40.0))
          , beta(std::sqrt(a + a))
         {}

         config_biquad_a(double db_gain, frequency f, std::uint32_t sps, double q)
          : config_biquad(f, sps, q)
          , a(std::pow(10.0, db_gain / 40.0))
          , beta(std::sqrt(a + a))
         {}

         double a;
         double beta;
      };

      /////////////////////////////////////////////////////////////////////////
      struct config_lowpass : config_biquad
      {
         config_lowpass(frequency f, std::uint32_t sps, double q)
          : config_biquad(f, sps, q)
         {
            init();
         }

         void init()
         {
            b0 = (1.0 - cos) / 2.0;
            b1 = 1.0 - cos;
            b2 = (1.0 - cos) / 2.0;
            a0 = 1.0 + alpha;
            a1 = -2.0 * cos;
            a2 = 1.0 - alpha;
         }
      };

      /////////////////////////////////////////////////////////////////////////
      struct config_highpass : config_biquad
      {
         config_highpass(frequency f, std::uint32_t sps, double q)
          : config_biquad(f, sps, q)
         {
            init();
         }

         void init()
         {
            b0 = (1.0 + cos) / 2.0;
            b1 = -(1.0 + cos);
            b2 = (1.0 + cos) / 2.0;
            a0 = 1.0 + alpha;
            a1 = -2.0 * cos;
            a2 = 1.0 - alpha;
         }
      };

      /////////////////////////////////////////////////////////////////////////
      struct config_bandpass_csg : config_biquad
      {
         config_bandpass_csg(frequency f, std::uint32_t sps, bw _bw)
          : config_biquad(f, sps, _bw)
         {
            init();
         }

         config_bandpass_csg(frequency f, std::uint32_t sps, double q)
          : config_biquad(f, sps, q)
         {
            init();
         }

         void init()
         {
            b0 = sin / 2.0;
            b1 = 0.0;
            b2 = -sin / 2;
            a0 = 1.0 + alpha;
            a1 = -2.0 * cos;
            a2 = 1.0 - alpha;
         }
      };

      /////////////////////////////////////////////////////////////////////////
      struct config_bandpass_cpg : config_biquad
      {
         config_bandpass_cpg(frequency f, std::uint32_t sps, bw _bw)
          : config_biquad(f, sps, _bw)
         {
            init();
         }

         config_bandpass_cpg(frequency f, std::uint32_t sps, double q)
          : config_biquad(f, sps, q)
         {
            init();
         }

         void init()
         {
            b0 = alpha;
            b1 = 0.0;
            b2 = -alpha;
            a0 = 1.0 + alpha;
            a1 = -2.0 * cos;
            a2 = 1.0 - alpha;
         }
      };

      /////////////////////////////////////////////////////////////////////////
      struct config_notch : config_biquad
      {
         config_notch(frequency f, std::uint32_t sps, bw _bw)
          : config_biquad(f, sps, _bw)
         {
            init();
         }

         config_notch(frequency f, std::uint32_t sps, double q)
          : config_biquad(f, sps, q)
         {
            init();
         }

         void init()
         {
            b0 = 1.0;
            b1 = -2.0 * cos;
            b2 = 1.0;
            a0 = 1.0 + alpha;
            a1 = -2.0 * cos;
            a2 = 1.0 - alpha;
         }
      };

      /////////////////////////////////////////////////////////////////////////
      struct config_allpass : config_biquad
      {
         config_allpass(frequency f, std::uint32_t sps, double q)
          : config_biquad(f, sps, q)
         {
            init();
         }

         void init()
         {
            b0 = 1.0 - alpha;
            b1 = -2.0 * cos;
            b2 = 1.0 + alpha;
            a0 = 1.0 + alpha;
            a1 = -2.0 * cos;
            a2 = 1.0 - alpha;
         }
      };

      /////////////////////////////////////////////////////////////////////////
      struct config_peaking : config_biquad_a
      {
         config_peaking(double db_gain, frequency f, std::uint32_t sps, bw _bw)
          : config_biquad_a(db_gain, f, sps, _bw)
         {
            init();
         }

         config_peaking(double db_gain, frequency f, std::uint32_t sps, double q)
          : config_biquad_a(db_gain, f, sps, q)
         {
            init();
         }

         void init()
         {
            b0 = 1.0 + alpha * a;
            b1 = -2.0 * cos;
            b2 = 1.0 - alpha * a;
            a0 = 1.0 + alpha / a;
            a1 = -2.0 * cos;
            a2 = 1.0 - alpha / a;
         }
      };

      /////////////////////////////////////////////////////////////////////////
      struct config_lowshelf : config_biquad_a
      {
         config_lowshelf(double db_gain, frequency f, std::uint32_t sps, double q)
          : config_biquad_a(db_gain, f, sps, q)
         {
            init();
         }

         void init()
         {
            b0 = a * ((a + 1.0) -(a - 1.0) * cos + beta * sin);
            b1 = 2.0 * a * ((a - 1.0) - (a + 1.0) * cos);
            b2 = a * ((a + 1.0) - (a - 1.0) * cos - beta * sin);
            a0 = (a + 1.0) + (a - 1.0) * cos + beta * sin;
            a1 = -2.0 * ((a - 1.0) + (a + 1.0) * cos);
            a2 = (a + 1.0) + (a - 1.0) * cos - beta * sin;
         }
      };

      /////////////////////////////////////////////////////////////////////////
      struct config_highshelf : config_biquad_a
      {
         config_highshelf(double db_gain, frequency f, std::uint32_t sps, double q)
          : config_biquad_a(db_gain, f, sps, q)
         {
            init();
         }

         void init()
         {
            b0 = a * ((a + 1.0) + (a - 1.0) * cos + beta * sin);
            b1 = -2.0 * a * ((a - 1.0) + (a + 1.0) * cos);
            b2 = a * ((a + 1.0) + (a - 1.0) * cos - beta * sin);
            a0 = (a + 1.0) - (a - 1.0) * cos + beta * sin;
            a1 = 2.0 * ((a - 1.0) - (a + 1.0) * cos);
            a2 = (a + 1.0) - (a - 1.0) * cos - beta * sin;
         }
      };
   }

   ////////////////////////////////////////////////////////////////////////////
   // Low pass filter
   ////////////////////////////////////////////////////////////////////////////
   struct lowpass : biquad
   {
      lowpass(frequency f, std::uint32_t sps, double q = 0.707)
       : biquad(detail::config_lowpass(f, sps, q).make())
      {}

      void config(frequency f, std::uint32_t sps, double q = 0.707)
      {
         detail::config_lowpass(f, sps, q).config(*this);
      }
   };

   ////////////////////////////////////////////////////////////////////////////
   // High pass filter
   ////////////////////////////////////////////////////////////////////////////
   struct highpass : biquad
   {
      highpass(frequency f, std::uint32_t sps, double q = 0.707)
       : biquad(detail::config_highpass(f, sps, q).make())
      {}

      void config(frequency f, std::uint32_t sps, double q = 0.707)
      {
         detail::config_highpass(f, sps, q).config(*this);
      }
   };

   ////////////////////////////////////////////////////////////////////////////
   // Band pass filter; constant skirt gain, peak gain = Q
   ////////////////////////////////////////////////////////////////////////////
   struct bandpass_csg : biquad
   {
      bandpass_csg(frequency f, std::uint32_t sps, bw _bw)
       : biquad(detail::config_bandpass_csg(f, sps, _bw).make())
      {}

      bandpass_csg(frequency f, std::uint32_t sps, double q = 0.707)
       : biquad(detail::config_bandpass_csg(f, sps, q).make())
      {}

      void config(frequency f, std::uint32_t sps, bw _bw)
      {
         detail::config_bandpass_csg(f, sps, _bw).config(*this);
      }

      void config(frequency f, std::uint32_t sps, double q = 0.707)
      {
         detail::config_bandpass_csg(f, sps, q).config(*this);
      }
   };

   ////////////////////////////////////////////////////////////////////////////
   // Band pass filter; constant 0 dB peak gain
   ////////////////////////////////////////////////////////////////////////////
   struct bandpass_cpg : biquad
   {
      bandpass_cpg(frequency f, std::uint32_t sps, bw _bw)
       : biquad(detail::config_bandpass_cpg(f, sps, _bw).make())
      {}

      bandpass_cpg(frequency f, std::uint32_t sps, double q = 0.707)
       : biquad(detail::config_bandpass_cpg(f, sps, q).make())
      {}

      void config(frequency f, std::uint32_t sps, bw _bw)
      {
         detail::config_bandpass_cpg(f, sps, _bw).config(*this);
      }

      void config(frequency f, std::uint32_t sps, double q = 0.707)
      {
         detail::config_bandpass_cpg(f, sps, q).config(*this);
      }
   };

   ////////////////////////////////////////////////////////////////////////////
   // All pass filter
   ////////////////////////////////////////////////////////////////////////////
   struct allpass : biquad
   {
      allpass(frequency f, std::uint32_t sps, double q = 0.707)
       : biquad(detail::config_allpass(f, sps, q).make())
      {}

      void config(frequency f, std::uint32_t sps, double q = 0.707)
      {
         detail::config_allpass(f, sps, q).config(*this);
      }
   };

   ////////////////////////////////////////////////////////////////////////////
   // Notch filter
   ////////////////////////////////////////////////////////////////////////////
   struct notch : biquad
   {
      notch(frequency f, std::uint32_t sps, bw _bw)
       : biquad(detail::config_notch(f, sps, _bw).make())
      {}

      notch(frequency f, std::uint32_t sps, double q = 0.707)
       : biquad(detail::config_notch(f, sps, q).make())
      {}

      void config(frequency f, std::uint32_t sps, bw _bw)
      {
         detail::config_notch(f, sps, _bw).config(*this);
      }

      void config(frequency f, std::uint32_t sps, double q = 0.707)
      {
         detail::config_notch(f, sps, q).config(*this);
      }
   };

   ////////////////////////////////////////////////////////////////////////////
   // Peaking band EQ filter
   ////////////////////////////////////////////////////////////////////////////
   struct peaking : biquad
   {
      peaking(double db_gain, frequency f, std::uint32_t sps, bw _bw)
       : biquad(detail::config_peaking(db_gain, f, sps, _bw).make())
      {}

      peaking(double db_gain, frequency f, std::uint32_t sps, double q = 0.707)
       : biquad(detail::config_peaking(db_gain, f, sps, q).make())
      {}

      void config(double db_gain, frequency f, std::uint32_t sps, bw _bw)
      {
         detail::config_peaking(db_gain, f, sps, _bw).config(*this);
      }

      void config(double db_gain, frequency f, std::uint32_t sps, double q = 0.707)
      {
         detail::config_peaking(db_gain, f, sps, q).config(*this);
      }
   };

   ////////////////////////////////////////////////////////////////////////////
   // Low shelf filter
   ////////////////////////////////////////////////////////////////////////////
   struct lowshelf : biquad
   {
      lowshelf(double db_gain, frequency f, std::uint32_t sps, double q = 0.707)
       : biquad(detail::config_lowshelf(db_gain, f, sps, q).make())
      {}

      void config(double db_gain, frequency f, std::uint32_t sps, double q = 0.707)
      {
         detail::config_lowshelf(db_gain, f, sps, q).config(*this);
      }
   };

   ////////////////////////////////////////////////////////////////////////////
   // High shelf filter
   ////////////////////////////////////////////////////////////////////////////
   struct highshelf : biquad
   {
      highshelf(double db_gain, frequency f, std::uint32_t sps, double q = 0.707)
       : biquad(detail::config_highshelf(db_gain, f, sps, q).make())
      {}

      void config(double db_gain, frequency f, std::uint32_t sps, double q = 0.707)
      {
         detail::config_highshelf(db_gain, f, sps, q).config(*this);
      }
   };
}

#endif
