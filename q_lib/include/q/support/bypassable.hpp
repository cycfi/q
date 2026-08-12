/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the Boost Software License, Version 1.0.
   [ https://www.boost.org/LICENSE_1_0.txt ]
=============================================================================*/
#if !defined(CYCFI_Q_BYPASSABLE_HPP_AUGUST_2026)
#define CYCFI_Q_BYPASSABLE_HPP_AUGUST_2026

#include <type_traits>
#include <utility>

namespace cycfi::q
{
   namespace detail
   {
      // The stand-in a bypassed member becomes.
      struct bypassed_stage {};
   }

   ////////////////////////////////////////////////////////////////////////////
   // bypassable<Bypass, T> is T when Bypass is false, and an empty stand-in
   // when it is true. It lets a processor drop a stage at COMPILE TIME and
   // pay neither its work nor its storage -- the difference between an
   // `if constexpr` that merely skips the processing and one that also
   // removes the member.
   //
   // The polarity follows the name: BYPASS is true to remove the stage, so a
   // host reads the way its own switch reads.
   //
   //    template <bool bypass_comp = false>
   //    class my_processor
   //    {
   //       ...
   //       bypassable<bypass_comp, compressor> _comp;
   //    };
   //
   //    float my_processor::operator()(float s)
   //    {
   //       if constexpr (!bypass_comp)
   //          s = _comp(s);
   //       return s;
   //    }
   //
   // The point is the WORK, not the bytes: bypassing takes a stage out of the
   // per-sample path entirely.
   //
   // Do NOT count on it shrinking the object. An empty class still occupies a
   // byte, and alignment padding routinely absorbs that byte along with the
   // whole of a small stage -- bypassing an 8-byte stage next to an 8-byte
   // aligned member typically saves nothing. The guarantee is only that a
   // bypassed member never costs MORE than the stage it replaces. Recovering
   // the remainder needs [[no_unique_address]] (which MSVC ignores in favour
   // of its own [[msvc::no_unique_address]]) or an empty base (which MSVC
   // applies to only one base unless told otherwise); neither wart is worth a
   // byte, so this stays a plain member.
   //
   // Reach for this when a stage is genuinely optional for some consumers,
   // not to shave bytes off a class everyone uses in full -- a bypass switch
   // nobody flips costs template instantiations and reader attention for
   // nothing.
   ////////////////////////////////////////////////////////////////////////////
   template <bool Bypass, typename T>
   using bypassable = std::conditional_t<Bypass, detail::bypassed_stage, T>;

   ////////////////////////////////////////////////////////////////////////////
   // make_bypassable<Bypass, T>(args...) builds the stage, or its stand-in
   // when bypassed, discarding the arguments.
   //
   // This exists because stages usually have no default constructor, so they
   // must be built in the member-init list -- where `if constexpr` cannot
   // reach. The factory moves the choice into an expression the init list can
   // hold:
   //
   //    my_processor::my_processor(config const& conf)
   //     : _comp{make_bypassable<bypass_comp, compressor>(
   //          conf.threshold, conf.slope)}
   //    {}
   ////////////////////////////////////////////////////////////////////////////
   template <bool Bypass, typename T, typename... Args>
   inline bypassable<Bypass, T> make_bypassable(Args&&... args)
   {
      if constexpr (Bypass)
         return detail::bypassed_stage{};
      else
         return T{std::forward<Args>(args)...};
   }
}

#endif
