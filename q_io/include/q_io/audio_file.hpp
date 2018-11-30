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
   class wav_base
   {
   public:
                     wav_base();
                     ~wav_base();

      explicit       operator bool() const;
      std::size_t    sps() const;
      std::size_t    num_channels() const;

   protected:

      wav_impl*      _wav;
   };

   ////////////////////////////////////////////////////////////////////////////
   class wav_reader : public wav_base
   {
   public:
                     wav_reader(std::string filename)
                      : wav_reader(filename.c_str())
                     {}

                     wav_reader(char const* filename);

      std::size_t    length() const;
      std::size_t    read(float* data, std::uint32_t len);

                     template <typename Buffer>
      std::size_t    read(Buffer& buffer);

   };

   ////////////////////////////////////////////////////////////////////////////
   class wav_writer : public wav_base
   {
   public:
                     wav_writer(
                        std::string filename
                      , std::uint32_t num_channels, std::uint32_t sps)
                      : wav_writer(filename.c_str(), num_channels, sps)
                     {}

                     wav_writer(
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
   inline std::size_t wav_reader::read(Buffer& buffer)
   {
      return read(buffer.data(), buffer.size());
   }

   template <typename Buffer>
   inline std::size_t wav_writer::write(Buffer const& buffer)
   {
      return write(buffer.data(), buffer.size());
   }
}}}

#endif