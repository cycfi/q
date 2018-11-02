/*=============================================================================
   Copyright (c) 2014-2018 Joel de Guzman. All rights reserved.

   Distributed under the Boost Software License, Version 1.0. (See accompanying
   file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#if !defined(CYCFI_Q_AUDIO_FILE_HPP_MARCH_28_2018)
#define CYCFI_Q_AUDIO_FILE_HPP_MARCH_28_2018

#include <dr_wav.h>
#include <cstdint>
#include <cstddef>
#include <string>

namespace cycfi { namespace q { namespace audio_file
{
   ////////////////////////////////////////////////////////////////////////////
   class base
   {
   public:
                     base();
                     ~base();

      explicit       operator bool() const   { return _wav != nullptr; }
      std::size_t    sps() const             { return _wav->sampleRate; }
      std::size_t    num_channels() const    { return _wav->channels; }

   protected:

      drwav*         _wav;
   };

   ////////////////////////////////////////////////////////////////////////////
   class reader : public base
   {
   public:
                     reader(std::string filename)
                      : reader(filename.c_str())
                     {}

                     reader(char const* filename);

      std::size_t    length() const;
      std::size_t    read(float* data, std::uint32_t len);

                     template <typename Buffer>
      std::size_t    read(Buffer& buffer);

   };

   ////////////////////////////////////////////////////////////////////////////
   class writer : public base
   {
   public:
                     writer(
                        std::string filename
                      , std::uint32_t num_channels, std::uint32_t sps)
                      : writer(filename.c_str(), num_channels, sps)
                     {}

                     writer(
                        char const* filename
                      , std::uint32_t num_channels, std::uint32_t sps);

      std::size_t    write(float const* data, std::uint32_t len);

                     template <typename Buffer>
      std::size_t    write(Buffer const& buffer);
   };

   ////////////////////////////////////////////////////////////////////////////
   // Inlines
   ////////////////////////////////////////////////////////////////////////////
   inline std::size_t reader::length() const
   {
      return _wav->totalSampleCount;
   }

   template <typename Buffer>
   inline std::size_t reader::read(Buffer& buffer)
   {
      return read(buffer.data(), buffer.size());
   }

   inline std::size_t reader::read(float* data, std::uint32_t len)
   {
      if (_wav == nullptr)
         return 0;
      return drwav_read_f32(_wav, len, data);
   }

   template <typename Buffer>
   inline std::size_t writer::write(Buffer const& buffer)
   {
      return write(buffer.data(), buffer.size());
   }

   inline std::size_t writer::write(float const* data, std::uint32_t len)
   {
      if (_wav == nullptr)
         return 0;
      return drwav_write(_wav, len, data);
   }
}}}

#endif