/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the Boost Software License, Version 1.0.
   [ https://www.boost.org/LICENSE_1_0.txt ]
=============================================================================*/
#if !defined(CYCFI_Q_UTILITY_KEYBOARD_HPP_JUNE_2026)
#define CYCFI_Q_UTILITY_KEYBOARD_HPP_JUNE_2026

// Non-blocking keyboard query and single-character read, cross-platform.
//
// On Unix, kbhit() uses a zero-timeout select() against stdin. Raw terminal
// mode (termios) must be set up by the caller before use so that getch()
// returns immediately without waiting for a newline.
//
// On Windows, delegates directly to <conio.h> _kbhit() / _getch().

#ifdef _WIN32
# include <conio.h>

namespace cycfi::q
{
   inline bool kbhit() { return ::_kbhit() != 0; }
   inline int  getch() { return ::_getch(); }
}

#else
# include <sys/select.h>
# include <unistd.h>

namespace cycfi::q
{
   inline bool kbhit()
   {
      timeval tv{0, 0};
      fd_set fds;
      FD_ZERO(&fds);
      FD_SET(STDIN_FILENO, &fds);
      return select(STDIN_FILENO + 1, &fds, nullptr, nullptr, &tv) > 0;
   }

   inline int getch()
   {
      char c;
      return read(STDIN_FILENO, &c, 1) > 0 ? static_cast<int>(c) : -1;
   }
}

#endif
#endif
