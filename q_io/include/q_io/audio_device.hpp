/*=============================================================================
   Copyright (c) 2014-2020 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(CYCFI_Q_AUDIO_DEVICE_HPP_DECEMBER_1_2018)
#define CYCFI_Q_AUDIO_DEVICE_HPP_DECEMBER_1_2018

#include <vector>
#include <cstdint>
#include <string>

namespace cycfi::q
{
   ////////////////////////////////////////////////////////////////////////////
   class audio_device
   {
   public:

      enum io_dir { input, output };
      using device_list = std::vector<audio_device>;

      static device_list         list();
      static audio_device        get(int device_id);

      std::uint32_t              id() const;
      std::string                name() const;
      std::size_t                input_channels() const;
      std::size_t                output_channels() const;
      std::size_t                default_sample_rate() const;

   private:

      struct impl;
                                 audio_device(impl const& impl)
                                  : _impl(impl)
                                 {}

      impl const&                _impl;
   };
}

#endif