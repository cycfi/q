/*=============================================================================
   Copyright (c) 2014-2020 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(CYCFI_Q_AUDIO_FILE_HPP_MARCH_28_2018)
#define CYCFI_Q_AUDIO_FILE_HPP_MARCH_28_2018

#include <infra/iterator_range.hpp>
#include <cstdint>
#include <cstddef>
#include <string>
#include <vector>

namespace cycfi::q
{
   ////////////////////////////////////////////////////////////////////////////
   struct wav_impl;

   ////////////////////////////////////////////////////////////////////////////
   class wav_base
   {
   public:

      wav_base();
      wav_base(wav_base const&) = delete;
      ~wav_base();

      wav_base&      operator=(wav_base const&) = delete;
      explicit       operator bool() const;
      std::size_t    sps() const;
      std::size_t    num_channels() const;

   protected:

      wav_impl*      _wav;
      bool           first_read;
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
      std::size_t    position();
      std::size_t    read(float* data, std::uint32_t len);
      bool           restart();
      bool           seek(std::uint64_t target);

                     template <typename Buffer>
      std::size_t    read(Buffer& buffer);
   };

   ////////////////////////////////////////////////////////////////////////////
   class wav_memory : private wav_reader
   {
   public:

      using range = iterator_range<float const*>;
      using storage = std::vector<float>;
      using iterator = std::vector<float>::const_iterator;

      wav_memory(std::string filename, std::size_t buff_size = 1024)
       : wav_memory(filename.c_str())
      {}

      wav_memory(char const* filename, std::size_t buff_size = 1024);

      using wav_reader::operator bool;
      using wav_reader::length;
      using wav_reader::position;
      using wav_reader::sps;
      using wav_reader::num_channels;
      using wav_reader::restart;
      using wav_reader::seek;

      range const    operator()();

   private:

      storage        _buff;
      iterator       _pos;
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

   inline wav_memory::wav_memory(char const* filename, std::size_t buff_size)
    : wav_reader(filename)
    , _buff(*this? buff_size * num_channels() : num_channels())
   {
      if (!(*this))
         std::fill(_buff.begin(), _buff.end(), 0.0f);
      first_read = false;
   }

   inline iterator_range<float const*> const wav_memory::operator()()
   {
      if (*this)
      {
         if (!first_read || (_pos + num_channels()) >= _buff.end())
         {
            if (!first_read) first_read = true;
            auto read_len = read(_buff.data(), _buff.size());
            if (read_len == 0)
            {
               _buff.resize(num_channels());
               std::fill(_buff.begin(), _buff.end(), 0.0f);
            }
            else if (read_len != _buff.size())
            {
               std::fill(_buff.begin()+read_len, _buff.end(), 0.0f);
            }
            _pos = _buff.begin();
         }
         else
         {
            _pos += num_channels();
         }
         float const* p = &*_pos;
         iterator_range<float const*> r{ p, p + num_channels() };
         return r;
      }
      else
      {
         return { &*_buff.begin(), &*_buff.end() };
      }
   }
}

#endif
