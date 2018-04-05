/*=============================================================================
   Copyright (c) 2014-2018 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(CYCFI_Q_AUDIO_FILE_HPP_MARCH_28_2018)
#define CYCFI_Q_AUDIO_FILE_HPP_MARCH_28_2018

#include <sndfile.h>
#include <cstdint>
#include <cstddef>
#include <string>

namespace cycfi { namespace q { namespace audio_file
{
   ////////////////////////////////////////////////////////////////////////////
   enum format
   {
      wav   = SF_FORMAT_WAV,     // Microsoft WAV format
      aiff  = SF_FORMAT_AIFF,    // Apple/SGI AIFF format
      mat4  = SF_FORMAT_MAT4,    // Matlab (tm) V4.2 / GNU Octave 2.0
      mat5  = SF_FORMAT_MAT5,    // Matlab (tm) V5.0 / GNU Octave 2.1
      flac  = SF_FORMAT_FLAC     // FLAC lossless file format
   };

   ////////////////////////////////////////////////////////////////////////////
   enum data_format
   {
      _16_bits = SF_FORMAT_PCM_16,
      _24_bits = SF_FORMAT_PCM_24,
      _32_bits = SF_FORMAT_PCM_32
   };

   ////////////////////////////////////////////////////////////////////////////
   class base
   {
   public:
                     base();
                     ~base();

      explicit       operator bool() const   { return _file != nullptr; }
      std::size_t    sps() const             { return _sfinfo.samplerate; }
      std::size_t    num_channels() const    { return _sfinfo.channels; }

   protected:

      SF_INFO        _sfinfo;
      SNDFILE*       _file;
   };

   ////////////////////////////////////////////////////////////////////////////
   class reader : public base
   {
   public:
                     reader(std::string filename)
                      : reader(filename.c_str())
                     {}

                     reader(char const* filename);

      std::size_t    length() const          { return _sfinfo.frames; }

      template <typename Buffer>
      std::size_t    read(Buffer& buffer)
      {
         return read(buffer.data(), buffer.size());
      }

      std::size_t    read(float* data, std::uint32_t len)
      {
         if (_file == nullptr)
            return 0;
         return sf_read_float(_file, data, len);
      }

      std::size_t    read(double* data, std::uint32_t len)
      {
         if (_file == nullptr)
            return 0;
         return sf_read_double(_file, data, len);
      }
   };

   ////////////////////////////////////////////////////////////////////////////
   class writer : public base
   {
   public:
                     writer(
                        std::string filename
                      , format format_, data_format data_format_
                      , std::uint32_t num_channels, std::uint32_t sps)
                      : writer(filename.c_str(), format_
                        , data_format_, num_channels, sps)
                     {}

                     writer(
                        char const* filename
                      , format format_, data_format data_format_
                      , std::uint32_t num_channels, std::uint32_t sps);

      template <typename Buffer>
      std::size_t    write(Buffer const& buffer)
      {
         return write(buffer.data(), buffer.size());
      }

      std::size_t    write(float const* data, std::uint32_t len)
      {
         if (_file == nullptr)
            return 0;
         return sf_write_float(_file, data, len);
      }

      std::size_t    write(double const* data, std::uint32_t len)
      {
         if (_file == nullptr)
            return 0;
         return sf_write_double(_file, data, len);
      }
   };
}}}

#endif