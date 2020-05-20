/*=============================================================================
   Copyright (c) 2014-2020 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#include <q_io/audio_device.hpp>
#include <q_io/midi_device.hpp>
#include <iostream>
#include <string>

namespace q = cycfi::q;

int main()
{
   std::cout << "================================================================================" << std::endl;
   std::cout << "Available Audio Devices: " << std::endl;
   for (auto const& device : q::audio_device::list())
   {
      std::cout
         << "id: " << device.id() << std:: endl
         << "name: \"" << device.name() << '"' << std:: endl
         << "number of input channels: " << device.input_channels() << std:: endl
         << "number of output channels: " << device.output_channels() << std:: endl
         ;
   }

   std::cout << "================================================================================" << std::endl;
   std::cout << "Available MIDI Devices: " << std::endl;
   for (auto const& device : q::midi_device::list())
   {
      std::cout
         << "id: " << device.id() << std:: endl
         << "name: \"" << device.name() << '"' << std:: endl
         << "number of inputs: " << device.num_inputs() << std:: endl
         << "number of outputs: " << device.num_outputs() << std:: endl
         ;
   }
   return 0;
}

