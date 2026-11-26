/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(CYCFI_Q_WAVEGUIDE_JUNCTION_HPP_NOVEMBER_9_2026)
#define CYCFI_Q_WAVEGUIDE_JUNCTION_HPP_NOVEMBER_9_2026

#include <cstddef>
#include <span>
#include <vector>

namespace cycfi::q
{
   ////////////////////////////////////////////////////////////////////////////
   // waveguide_junction: where waveguides meet at one point, such as six
   // strings and a body at a bridge. Each port has a wave impedance; the
   // junction moves as the impedance-weighted sum of what arrives,
   //
   //    v = 2 sum(r[i] in[i]) / sum(r[i])
   //
   // and sends back into each port out[i] = v - in[i]: lossless, so energy
   // only moves between the ports. A port much stiffer than the rest makes
   // the others reflect nearly inverted (a rigid bridge); equal ports pass a
   // wave straight through. The ins are each waveguide's incoming(), the
   // outs what it is given back; it returns the junction's motion.
   ////////////////////////////////////////////////////////////////////////////
   class waveguide_junction
   {
   public:

      explicit       waveguide_junction(std::size_t ports)
                      : _r(ports, 1.0f)
                     {}

      void           impedance(std::size_t port, float r);
      float          operator()(std::span<float const> in,
                        std::span<float> out) const;
      std::size_t    size() const { return _r.size(); }

   private:

      std::vector<float> _r;
   };

   ////////////////////////////////////////////////////////////////////////////
   // Inlines
   ////////////////////////////////////////////////////////////////////////////
   inline void waveguide_junction::impedance(std::size_t port, float r)
   {
      _r[port] = r;
   }

   inline float waveguide_junction::operator()(
      std::span<float const> in, std::span<float> out) const
   {
      float sum = 0, total = 0;
      for (std::size_t i = 0; i != _r.size(); ++i)
      {
         sum += _r[i] * in[i];
         total += _r[i];
      }
      auto const v = 2 * sum / total;
      for (std::size_t i = 0; i != _r.size(); ++i)
         out[i] = v - in[i];
      return v;
   }
}

#endif
