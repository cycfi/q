/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the Boost Software License, Version 1.0.
   [ https://www.boost.org/LICENSE_1_0.txt ]
=============================================================================*/
#if !defined(CYCFI_Q_FM_ROUTING_HPP_SEPTEMBER_16_2026)
#define CYCFI_Q_FM_ROUTING_HPP_SEPTEMBER_16_2026

#include <cstddef>
#include <cstdint>

namespace cycfi::q
{
   // A bit per operator in a mask, and a mask per operator in the table
   // every voice carries, so the count is kept to what a synth needs; one
   // wanting more runs more voices.
   constexpr std::size_t fm_max_operators = 16;

   namespace detail
   {
      // What fm_algorithm reads: a bit per operator, numbered from 0 as
      // the operator tuple is, so gathering an operator's modulators is
      // a walk over set bits. fm_routing builds it; see there.
      struct fm_routing_table
      {
         using mask = std::uint16_t;

         mask           mod[fm_max_operators] = {};   // modulators of each
         mask           carriers = 0;
         std::uint8_t   fb_src = 0;    // feedback from this operator
         std::uint8_t   fb_dst = 0;    // into this one
      };
   }

   ////////////////////////////////////////////////////////////////////////////
   // An operator by number, from 1 as the charts number them: fm_ops::op<1>
   // and up.
   // `a >> b` is a modulates b, chaining to the right as a stack does, and
   // `|` puts paths side by side:
   //
   //    op<2> >> op<1>                       a pair
   //    op<6> >> op<5> >> op<4> >> op<3>     a stack
   //    op<4> >> op<3> | op<5> >> op<3>      two modulating one
   //    op<6> >> op<4> | op<6> >> op<5>      one modulating two
   //    op<2> >> op<1> | op<3>               and that one by itself
   //
   // `>>` binds tighter than `|`, so a chart reads without parentheses.
   // A path is a routing already: it knows the operators it mentions, so
   //
   //    (op<2> >> op<1> | op<6> >> op<5> >> op<4> >> op<3>)
   //       .feedback(op<6>)
   //
   // is the whole of a six operator algorithm. An operator that only
   // sounds is named on its own, so that it is one of them.
   ////////////////////////////////////////////////////////////////////////////
   struct fm_op
   {
      std::size_t    n;
   };

   struct fm_routing;

   struct fm_path
   {
      using mask = detail::fm_routing_table::mask;

      // A routing of the operators the path mentions, feeding one of them
      // back into its own modulation, or into another's
      constexpr fm_routing    feedback(fm_op op) const;
      constexpr fm_routing    feedback(fm_op src, fm_op dst) const;

      mask           mod[fm_max_operators] = {};   // modulators of each
      std::size_t    tail = 0;      // what a further >> modulates from
      std::size_t    top = 0;       // the highest operator mentioned
      bool           ok = true;     // every edge in range and ordered
   };

   // The operators of an expression live in their own namespace, being
   // names as short as any: `using namespace fm_ops;` to write one.
   namespace fm_ops
   {
                              template <std::size_t N>
      constexpr fm_op         op{N};
   }

   constexpr fm_path operator>>(fm_op from, fm_op to);
   constexpr fm_path operator>>(fm_path path, fm_op to);
   constexpr fm_path operator|(fm_path a, fm_path b);

   // An operator on its own: one of them, modulating nothing
   constexpr fm_path operator|(fm_path path, fm_op op);
   constexpr fm_path operator|(fm_op op, fm_path path);
   constexpr fm_path operator|(fm_op a, fm_op b);

   ////////////////////////////////////////////////////////////////////////////
   // fm_routing: what modulates what, said the way the patch chart says it.
   // A modulator must have a higher number than what it modulates, so one
   // pass down the operators has each one's modulators ready; valid()
   // checks that, and that every operator is one the routing has.
   //
   // A routing is for a number of operators and takes the paths among
   // them; the carriers follow, being those that modulate nothing, so
   // there is nothing to keep in step by hand:
   //
   //    auto r = (op<2> >> op<1> | op<6> >> op<5>).feedback(op<6>);
   //
   // Feedback is one path, from an operator's own output back into what
   // it modulates, itself included. It is constexpr throughout, so a
   // routing costs nothing at run time.
   ////////////////////////////////////////////////////////////////////////////
   struct fm_routing
   {
      using table = detail::fm_routing_table;
      using mask = table::mask;

      constexpr               fm_routing(
                                 std::size_t size = fm_max_operators);

      // A path is a routing of the operators it mentions
      constexpr               fm_routing(fm_path const& path);

      // The paths among the operators
      constexpr fm_routing&   operator()(fm_path const& path);

      // The feedback path, from an operator's output into its own
      // modulation, or into another's
      constexpr fm_routing&   feedback(fm_op op);
      constexpr fm_routing&   feedback(fm_op src, fm_op dst);

      constexpr mask          carriers() const;
      constexpr mask          modulators() const;
      constexpr std::size_t   size() const   { return _size; }
      constexpr bool          valid() const  { return _ok; }

      // The low level table, its carriers filled in
      constexpr table         get() const;

   private:

      table          _t;
      std::size_t    _size;         // how many operators it is for
      bool           _ok = true;    // every path was in range and ordered
   };

   ////////////////////////////////////////////////////////////////////////////
   // Implementation
   ////////////////////////////////////////////////////////////////////////////
   namespace detail
   {
      // `from` modulates `to`, both numbered from 1
      constexpr void fm_add_edge(
         fm_path& path, std::size_t from, std::size_t to)
      {
         if (from > fm_max_operators || to == 0 || from <= to)
         {
            path.ok = false;        // out of range, or a modulator below
         }
         else
         {
            path.mod[to - 1] |= fm_path::mask(1 << (from - 1));
            path.top = from > path.top ? from : path.top;
         }
         path.tail = to;
      }
   }

   constexpr fm_path operator>>(fm_op from, fm_op to)
   {
      fm_path path;
      detail::fm_add_edge(path, from.n, to.n);
      return path;
   }

   // A stack: what the last edge modulated now modulates in turn
   constexpr fm_path operator>>(fm_path path, fm_op to)
   {
      detail::fm_add_edge(path, path.tail, to.n);
      return path;
   }

   constexpr fm_path operator|(fm_path a, fm_path b)
   {
      for (std::size_t i = 0; i != fm_max_operators; ++i)
         a.mod[i] |= b.mod[i];
      a.tail = b.tail;
      a.top = b.top > a.top ? b.top : a.top;
      a.ok = a.ok && b.ok;
      return a;
   }

   namespace detail
   {
      // An operator that only sounds: no edge, but one of the operators
      constexpr fm_path fm_single(fm_op op)
      {
         fm_path path;
         if (op.n == 0 || op.n > fm_max_operators)
            path.ok = false;
         else
            path.top = op.n;
         path.tail = op.n;
         return path;
      }
   }

   constexpr fm_path operator|(fm_path path, fm_op op)
   {
      return path | detail::fm_single(op);
   }

   constexpr fm_path operator|(fm_op op, fm_path path)
   {
      return detail::fm_single(op) | path;
   }

   constexpr fm_path operator|(fm_op a, fm_op b)
   {
      return detail::fm_single(a) | detail::fm_single(b);
   }

   constexpr fm_routing fm_path::feedback(fm_op op) const
   {
      return feedback(op, op);
   }

   constexpr fm_routing::fm_routing(std::size_t size)
    : _size{size <= fm_max_operators ? size : fm_max_operators}
    , _ok{size <= fm_max_operators}
   {}

   constexpr fm_routing::fm_routing(fm_path const& path)
    : fm_routing{path.top}
   {
      (*this)(path);
   }

   constexpr fm_routing& fm_routing::operator()(fm_path const& path)
   {
      _ok = _ok && path.ok;
      for (std::size_t i = 0; i != fm_max_operators; ++i)
      {
         if (i >= _size && path.mod[i])
            _ok = false;            // an operator this routing has not got
         _t.mod[i] |= path.mod[i];
      }
      if (modulators() >> _size)
         _ok = false;               // a modulator it has not got
      return *this;
   }

   constexpr fm_routing& fm_routing::feedback(fm_op op)
   {
      return feedback(op, op);
   }

   constexpr fm_routing& fm_routing::feedback(fm_op src, fm_op dst)
   {
      if (src.n == 0 || dst.n == 0 || src.n > _size || dst.n > _size)
      {
         _ok = false;
      }
      else
      {
         _t.fb_src = std::uint8_t(src.n - 1);
         _t.fb_dst = std::uint8_t(dst.n - 1);
      }
      return *this;
   }

   // A path's feedback: a routing of the operators it mentions, the
   // feedback among them
   constexpr fm_routing fm_path::feedback(fm_op src, fm_op dst) const
   {
      auto size = top;
      size = src.n > size ? src.n : size;
      size = dst.n > size ? dst.n : size;
      return fm_routing{size}(*this).feedback(src, dst);
   }

   // Every operator that modulates another
   constexpr fm_routing::mask fm_routing::modulators() const
   {
      mask m = 0;
      for (auto each : _t.mod)
         m |= each;
      return m;
   }

   // The rest sound: an operator modulating nothing is a carrier
   constexpr fm_routing::mask fm_routing::carriers() const
   {
      return mask(~modulators() & ((1u << _size) - 1));
   }

   constexpr fm_routing::table fm_routing::get() const
   {
      table t = _t;
      t.carriers = carriers();
      return t;
   }
}

#endif
