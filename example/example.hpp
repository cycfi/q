/*=============================================================================
   Copyright (c) 2014-2021 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#include <q_io/midi_device.hpp>
#include <q_io/audio_device.hpp>
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
   std::cout << "Available MIDI Devices (ID : \"Name\" inputs/outputs): " << std::endl;
   for (auto const& device : q::midi_device::list())
   {
      std::cout <<
         device.id()
         << " : \"" << device.name() << "\" "
         << device.num_inputs() << '/' << device.num_outputs()
         << std:: endl
         ;
   }
   std::cout << "================================================================================" << std::endl;
   std::cout << "Choose MIDI Device ID: ";
   int id;
   std::cin.clear();
   std::cin >> id;
   return id;
}

int get_audio_device()
{
   std::cout << "================================================================================" << std::endl;
   std::cout << "Available Audio Devices (ID : \"Name\" inputs/outputs): " << std::endl;
   for (auto const& device : q::audio_device::list())
   {
      std::cout <<
         device.id()
         << " : \"" << device.name() << "\" "
         << device.input_channels() << '/' << device.output_channels()
         << std:: endl
         ;
   }
   std::cout << "================================================================================" << std::endl;
   std::cout << "Choose Audio Device ID: ";
   int id;
   std::cin.clear();
   std::cin >> id;
   return id;
}



