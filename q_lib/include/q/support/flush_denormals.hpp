/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(CYCFI_Q_FLUSH_DENORMALS_HPP_OCTOBER_28_2026)
#define CYCFI_Q_FLUSH_DENORMALS_HPP_OCTOBER_28_2026

#include <cstdint>

#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
# include <xmmintrin.h>
# define CYCFI_Q_FLUSH_DENORMALS_X86
#elif defined(_M_ARM64)
# include <intrin.h>
# define CYCFI_Q_FLUSH_DENORMALS_ARM64_MSVC
#elif defined(__aarch64__)
# define CYCFI_Q_FLUSH_DENORMALS_ARM64
#elif defined(__arm__) && defined(__ARM_FP)
# define CYCFI_Q_FLUSH_DENORMALS_ARM32
#endif

namespace cycfi::q
{
   ////////////////////////////////////////////////////////////////////////////
   // flush_denormals flushes subnormal numbers to zero on this thread for
   // its life, then puts the thread's floating-point mode back as it found
   // it. Subnormals are what a decaying filter, follower or ramp leaves
   // below about 1e-38; arithmetic on them is slow on many processors, and
   // they are far below anything audible. Put one at the top of an audio
   // callback:
   //
   //    void process(...)
   //    {
   //       q::flush_denormals guard;
   //       ...
   //    }
   //
   // x86 sets MXCSR's FTZ and DAZ; ARM sets FZ in FPCR (64-bit) or FPSCR
   // (32-bit, Cortex-M included). Elsewhere it does nothing. On Cortex-M an
   // interrupt starts with the mode in FPDSCR, so a guard in the handler
   // is still needed, or FZ set in FPDSCR once at startup.
   ////////////////////////////////////////////////////////////////////////////
   class flush_denormals
   {
   public:
                              flush_denormals();
                              ~flush_denormals();

                              flush_denormals(flush_denormals const&) = delete;
      flush_denormals&        operator=(flush_denormals const&) = delete;

   private:

      std::uint64_t           _saved = 0;
   };

   ////////////////////////////////////////////////////////////////////////////
   // Implementation
   ////////////////////////////////////////////////////////////////////////////
   namespace detail
   {
#if defined(CYCFI_Q_FLUSH_DENORMALS_X86)
      constexpr std::uint64_t flush_bits = 0x8040;    // FTZ | DAZ
      inline std::uint64_t fp_mode() { return _mm_getcsr(); }
      inline void fp_mode(std::uint64_t m) { _mm_setcsr(unsigned(m)); }

#elif defined(CYCFI_Q_FLUSH_DENORMALS_ARM64_MSVC)
# if !defined(ARM64_FPCR)
#  define ARM64_FPCR ARM64_SYSREG(3, 3, 4, 4, 0)
# endif
      constexpr std::uint64_t flush_bits = 1u << 24;  // FPCR.FZ
      inline std::uint64_t fp_mode() { return _ReadStatusReg(ARM64_FPCR); }
      inline void fp_mode(std::uint64_t m) { _WriteStatusReg(ARM64_FPCR, m); }

#elif defined(CYCFI_Q_FLUSH_DENORMALS_ARM64)
      constexpr std::uint64_t flush_bits = 1u << 24;  // FPCR.FZ
      inline std::uint64_t fp_mode()
      {
         std::uint64_t m;
         asm volatile("mrs %0, fpcr" : "=r"(m));
         return m;
      }
      inline void fp_mode(std::uint64_t m)
      {
         asm volatile("msr fpcr, %0" : : "r"(m));
      }

#elif defined(CYCFI_Q_FLUSH_DENORMALS_ARM32)
      constexpr std::uint64_t flush_bits = 1u << 24;  // FPSCR.FZ
      inline std::uint64_t fp_mode()
      {
         std::uint32_t m;
         asm volatile("vmrs %0, fpscr" : "=r"(m));
         return m;
      }
      inline void fp_mode(std::uint64_t m)
      {
         asm volatile("vmsr fpscr, %0" : : "r"(std::uint32_t(m)));
      }

#else
      constexpr std::uint64_t flush_bits = 0;
      inline std::uint64_t fp_mode() { return 0; }
      inline void fp_mode(std::uint64_t) {}
#endif
   }

   inline flush_denormals::flush_denormals()
    : _saved{detail::fp_mode()}
   {
      detail::fp_mode(_saved | detail::flush_bits);
   }

   inline flush_denormals::~flush_denormals()
   {
      detail::fp_mode(_saved);
   }
}

#endif
