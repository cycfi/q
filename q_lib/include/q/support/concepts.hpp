/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the Boost Software License, Version 1.0.
   [ https://www.boost.org/LICENSE_1_0.txt ]
=============================================================================*/
#if !defined(CYCFI_Q_CONCEPTS_HPP_MAY_12_2023)
#define CYCFI_Q_CONCEPTS_HPP_MAY_12_2023

// This header used to define Arithmetic itself, under the same include guard
// basic_concepts.hpp uses. Whichever was included first silently emptied the
// other, so a translation unit that reached this one first lost
// IndexableContainer. Forwarding keeps both spellings working with a single
// definition.
#include <q/support/basic_concepts.hpp>

#endif
