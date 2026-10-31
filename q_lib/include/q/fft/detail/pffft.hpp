/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(CYCFI_Q_FFT_DETAIL_PFFFT_OCTOBER_31_2026)
#define CYCFI_Q_FFT_DETAIL_PFFFT_OCTOBER_31_2026

// PFFFT (https://github.com/cycfi/pffft), float only, compiled header-only
// into cycfi::q::detail::pffft. PFFFT_EXPORT gives every PFFFT function
// internal linkage, so any number of translation units can include this.
// The C and SIMD headers PFFFT uses come first, outside the namespace, and
// PFFFT's own macros are removed again at the end. PFFFT's double build
// cannot share a translation unit with the float one, so double stays on
// the native FFT.

#include <assert.h>
#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(__aarch64__) || defined(__arm64__) || defined(_M_ARM64)
# include <arm_neon.h>
# if !defined(PFFFT_ENABLE_NEON)
#  define PFFFT_ENABLE_NEON
#  define CYCFI_Q_PFFFT_DEFINED_NEON
# endif
#elif defined(__x86_64__) || defined(_M_X64) || defined(__SSE__)
# include <xmmintrin.h>
#endif

#define PFFFT_EXPORT static inline

#if defined(__clang__)
# pragma clang diagnostic push
# pragma clang diagnostic ignored "-Wunknown-warning-option"
# pragma clang diagnostic ignored "-W#pragma-messages"
# pragma clang diagnostic ignored "-Wvla-cxx-extension"
# pragma clang diagnostic ignored "-Wvla-extension"
# pragma clang diagnostic ignored "-Wunused-function"
#elif defined(__GNUC__)
# pragma GCC diagnostic push
# pragma GCC diagnostic ignored "-Wvla"
# pragma GCC diagnostic ignored "-Wunused-function"
#endif

namespace cycfi::q::detail::pffft
{
#include <pffft.c>
#include <pffft_common.c>

   // PFFFT's own alignment test for its transform buffers.
   inline bool is_aligned(void const* p)
   {
      return VALIGNED(p);
   }
}

#if defined(__clang__)
# pragma clang diagnostic pop
#elif defined(__GNUC__)
# pragma GCC diagnostic pop
#endif

#undef PFFFT_EXPORT
#if defined(CYCFI_Q_PFFFT_DEFINED_NEON)
# undef PFFFT_ENABLE_NEON
# undef CYCFI_Q_PFFFT_DEFINED_NEON
#endif
#undef ALWAYS_INLINE
#undef COMPILER_GCC
#undef COMPILER_MSVC
#undef EXTRA_BYTES_FOR_ALIGNMENT_AND_A_POINTER
#undef FUNC_ALIGNED_FREE
#undef FUNC_ALIGNED_MALLOC
#undef FUNC_COS
#undef FUNC_CPLX_FINALIZE
#undef FUNC_CPLX_PREPROCESS
#undef FUNC_DESTROY
#undef FUNC_IS_VALID_SIZE
#undef FUNC_MIN_FFT_SIZE
#undef FUNC_NEAREST_SIZE
#undef FUNC_NEW_SETUP
#undef FUNC_REAL_FINALIZE
#undef FUNC_REAL_FINALIZE_4X4
#undef FUNC_REAL_PREPROCESS
#undef FUNC_REAL_PREPROCESS_4X4
#undef FUNC_SIMD_ARCH
#undef FUNC_SIMD_SIZE
#undef FUNC_SIN
#undef FUNC_TRANSFORM_INTERNAL
#undef FUNC_TRANSFORM_ORDERED
#undef FUNC_TRANSFORM_UNORDRD
#undef FUNC_VALIDATE_SIMD_A
#undef FUNC_VALIDATE_SIMD_EX
#undef FUNC_ZCONVERT_ZP
#undef FUNC_ZCONVOLVE
#undef FUNC_ZCONVOLVE_ACCUMULATE
#undef FUNC_ZCONVOLVE_SCALE
#undef FUNC_ZCONVOLVE_ZP
#undef FUNC_ZREORDER
#undef IFAC_MAX_SIZE
#undef INTERLEAVE2
#undef LD_PS1
#undef MALLOC_V4SF_ALIGNMENT
#undef NEVER_INLINE
#undef PFFFT_ASSERT4
#undef PFFFT_COMMON_ENUMS
#undef PFFFT_DEPRECATED
#undef PFFFT_H
#undef PFFFT_X86_HAVE_FMA
#undef PF_ALTIVEC_FLT_H
#undef PF_FLT_H
#undef PF_FMA_X86_H
#undef PF_NEON_FLT_H
#undef PF_RVV_FLT_H
#undef PF_RVV_HAVE_SHUFFLEVECTOR
#undef PF_RVV_SHUF4
#undef PF_SCAL_FLT_H
#undef PF_SSE1_FLT_H
#undef PF_WASM_FLT_H
#undef RESTRICT
#undef SETUP_STRUCT
#undef SIMD_SZ
#undef SVMADD
#undef SVMSUB
#undef SVMUL
#undef UNINTERLEAVE2
#undef VADD
#undef VALIGNED
#undef VARCH
#undef VCPLXMUL
#undef VCPLXMULCONJ
#undef VLA_ARRAY_ON_STACK
#undef VLOAD_ALIGNED
#undef VLOAD_UNALIGNED
#undef VMADD
#undef VMSUB
#undef VMUL
#undef VREQUIRES_ALIGN
#undef VREV_C
#undef VREV_S
#undef VSUB
#undef VSWAPHL
#undef VTRANSPOSE4
#undef VZERO
#undef assertv4
#undef cc_ref
#undef ch_ref
#undef pffft_transform_internal_nosimd
#undef pffft_zconvert_zp_nosimd
#undef pffft_zconvolve_accumulate_nosimd
#undef pffft_zconvolve_nosimd
#undef pffft_zconvolve_scale_nosimd
#undef pffft_zconvolve_zp_nosimd
#undef pffft_zreorder_nosimd

#endif
