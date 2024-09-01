/*=============================================================================
   Copyright (c) 2014-2024 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(CYCFI_Q_SLEEP_FEBRUARY_21_2018)
#define CYCFI_Q_SLEEP_FEBRUARY_21_2018

#if !defined(Q_DONT_USE_THREADS)
#include <chrono>
#include <thread>
#endif

namespace cycfi::q
{
#if !defined(Q_DONT_USE_THREADS)
   inline void sleep(duration t)
   {
      std::this_thread::sleep_for(std::chrono::duration<double>(as_double(t)));
   }
#endif
}

#endif
