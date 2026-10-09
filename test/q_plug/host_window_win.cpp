/*=============================================================================
   Copyright (c) 2019-2026 Joel de Guzman

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#include "host_window.hpp"
#include <windows.h>
#include <chrono>

namespace q_plug_test
{
   char const* window_api()
   {
      return CLAP_WINDOW_API_WIN32;
   }

   clap_window_t open_window(int width, int height)
   {
      static bool registered = []
      {
         WNDCLASSW wc = {};
         wc.lpfnWndProc = DefWindowProcW;
         wc.hInstance = GetModuleHandleW(nullptr);
         wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
         wc.lpszClassName = L"QPlugTestHostWindow";
         return RegisterClassW(&wc) != 0;
      }();
      (void) registered;

      RECT r{0, 0, width, height};
      AdjustWindowRect(&r, WS_OVERLAPPEDWINDOW, false);
      HWND hwnd = CreateWindowW(
         L"QPlugTestHostWindow", L"QPlug test host", WS_OVERLAPPEDWINDOW,
         CW_USEDEFAULT, CW_USEDEFAULT, r.right - r.left, r.bottom - r.top,
         nullptr, nullptr, GetModuleHandleW(nullptr), nullptr
      );
      ShowWindow(hwnd, SW_SHOWNOACTIVATE);

      clap_window_t w{};
      w.api = CLAP_WINDOW_API_WIN32;
      w.win32 = hwnd;
      return w;
   }

   void close_window(clap_window_t const& w)
   {
      DestroyWindow(static_cast<HWND>(w.win32));
   }

   void run_events(int ms)
   {
      using clock = std::chrono::steady_clock;
      auto const end = clock::now() + std::chrono::milliseconds(ms);
      while (clock::now() < end)
      {
         MSG msg;
         while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE))
         {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
         }
         Sleep(10);
      }
   }
}
