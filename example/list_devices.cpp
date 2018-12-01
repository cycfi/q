/*=============================================================================
   Copyright (c) 2014-2018 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#include <q_io/audio_device.hpp>
#include <iostream>

namespace q = cycfi::q;

int main()
{
   std::cout << "Available Audio Devices: " << std::endl;
   for (auto const& device : q::audio_device::list())
   {
      std::cout
         << "id: " << device.id() << std:: endl
         << "name: \"" << device.name() << '"' << std:: endl
         << "number of channels: " << device.num_channels() << std:: endl
         << "direction: " <<
            (device.direction() == q::audio_device::input?
            "input" : "output") << std:: endl
         ;

   }
   return 0;
}

