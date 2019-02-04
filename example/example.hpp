/*=============================================================================
   Copyright (c) 2014-2018 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#include <q_io/midi_device.hpp>
#include <iostream>
#include <csignal>
#include <cstdlib>
#include <cstdio>
#include <string>

namespace q = cycfi::q;

bool running = true;
void signal_handler(int sig)
{
   // Catch exceptions
   switch (sig)
   {
      case SIGINT:
      case SIGTERM:
         std::cout << "================================================================================" << std::endl;
         std::cout << "Goodbye!" << std::endl;
         std::cout << "================================================================================" << std::endl;
         running = false;
         break;

      default:
         break;
   }
}

int get_midi_device()
{
   signal(SIGINT, signal_handler);
   signal(SIGTERM, signal_handler);

   std::cout << "================================================================================" << std::endl;
   std::cout << "Available MIDI Devices: " << std::endl;
   for (auto const& device : q::midi_device::list())
   {
      std::cout
         << "ID: " << device.id() << std:: endl
         << "name: \"" << device.name() << '"' << std:: endl
         << "number of inputs: " << device.num_inputs() << std:: endl
         << "number of outputs: " << device.num_outputs() << std:: endl
         ;
   }
   std::cout << "================================================================================" << std::endl;
   std::cout << "Choose MIDI Device ID: ";
   return std::cin.get() - '0';
}