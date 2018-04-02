/*=============================================================================
   Copyright (c) 2014-2018 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#include <q_aux/audio_file.hpp>
#include <cassert>
#include <vector>

namespace cycfi { namespace q { namespace audio_file
{
   base::base()
    : _file(nullptr)
   {
      memset(&_sfinfo, 0, sizeof(_sfinfo));
   }

   base::~base()
   {
      sf_close(_file);
   }

   reader::reader(char const* filename)
   {
      _file = sf_open(filename, SFM_READ, &_sfinfo);
   }

   writer::writer(
      char const* filename
      , format format_, data_format data_format_
      , std::uint32_t num_channels, std::uint32_t sps)
   {
	   _sfinfo.samplerate = sps;
	   _sfinfo.channels = num_channels;
	   _sfinfo.format = int(format_) | int(data_format_);
      _file = sf_open(filename, SFM_WRITE, &_sfinfo);
   }
}}}

