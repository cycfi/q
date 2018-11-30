/*=============================================================================
   Copyright (c) 2014-2018 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(CYCFI_Q_AUDIO_FILE_HPP_MARCH_28_2018)
#define CYCFI_Q_AUDIO_FILE_HPP_MARCH_28_2018

#include <cstdint>
#include <cstddef>
#include <string>

extern "C"
{

}

namespace cycfi { namespace q { namespace audio_file
{
   ////////////////////////////////////////////////////////////////////////////
   struct wav_impl;

   ////////////////////////////////////////////////////////////////////////////
   class base
   {
   public:
                     base();
                     ~base();

      explicit       operator bool() const;
      std::size_t    sps() const;
      std::size_t    num_channels() const;

   protected:

      wav_impl*      _wav;
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
   template <typename Buffer>
   inline std::size_t reader::read(Buffer& buffer)
   {
      return read(buffer.data(), buffer.size());
   }

   template <typename Buffer>
   inline std::size_t writer::write(Buffer const& buffer)
   {
      return write(buffer.data(), buffer.size());
   }
}}}

#endif