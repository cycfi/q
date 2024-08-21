/*=============================================================================
   Copyright (c) 2016-2023 Cycfi Research. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(CYCFI_Q_AUDIO_STREAM_OCTOBER_3_2018)
#define CYCFI_Q_AUDIO_STREAM_OCTOBER_3_2018

#include <q/support/multi_buffer.hpp>

namespace cycfi::q
{
   ////////////////////////////////////////////////////////////////////////////
   class audio_stream_base : non_copyable
   {
   public:

      using in_channels = multi_buffer<float const>;
      using out_channels = multi_buffer<float>;

                              audio_stream_base() {}
      virtual                 ~audio_stream_base() = default;

      virtual void            process(in_channels const& in) {}
      virtual void            process(out_channels const& out) {}
      virtual void            process(in_channels const& in, out_channels const& out) {}
   };
}

#endif
