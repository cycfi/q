/*=============================================================================
   Copyright (c) 2014-2024 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(CYCFI_Q_AUDIO_FINGERPRINTER_HPP_AUGUST_30_2024)
#define CYCFI_Q_AUDIO_FINGERPRINTER_HPP_AUGUST_30_2024

#include <q/fft/fft.hpp>
#include <q/synth/hamming_gen.hpp>
#include <array>
#include <vector>
#include <unordered_map>
#include <algorithm>
#include <string>

namespace cycfi::q
{
   namespace detail
   {
      /////////////////////////////////////////////////////////////////////////
      // Function to extract spectral peaks from the magnitude spectrum
      /////////////////////////////////////////////////////////////////////////
      template <std::size_t N, std::floating_point T>
      inline std::unordered_map<std::size_t, T>
      extract_features(T* mag_spectrum)
      {
         std::unordered_map<std::size_t, float> features;
         constexpr float threshold = 0.001f;

         // Iterate through the magnitude spectrum to find peaks
         for (size_t i = 1; i < N-1; ++i)
         {
            auto prev = mag_spectrum[i-1];
            auto curr = mag_spectrum[i];
            auto next = mag_spectrum[i+1];

            if (curr > threshold && curr > prev && curr > next)
               features[i] = mag_spectrum[i];
         }

         return features;
      }

      /////////////////////////////////////////////////////////////////////////
      // Function to generate a fingerprint based on the extracted features
      /////////////////////////////////////////////////////////////////////////
      template <typename Map>
      inline std::string generate_fingerprint(Map const& features)
      {
         // Generate a simple hash based on the features
         std::hash<float> hasher;
         std::hash<std::size_t> index_hasher;
         size_t hash = 0;
         for (const auto& feature : features)
         {
            auto mix = 0x9e3779b9 + (hash << 6) + (hash >> 2);
            hash ^= hasher(feature.second) + mix;
            hash ^= index_hasher(feature.first) + mix;
         }

         return std::to_string(hash);
      }
   }

   ////////////////////////////////////////////////////////////////////////////
   // Function to process the audio data and generate a fingerprint
   ////////////////////////////////////////////////////////////////////////////
   inline std::string finger_printer(std::vector<float> const& audio, float sps)
   {
      constexpr std::size_t window_size = 1024;
      constexpr std::size_t overlap_size = window_size / 2;
      constexpr std::size_t step_size = window_size - overlap_size;
      constexpr std::size_t mag_spectrum_size = (window_size / 2) + 1;

      using detail::extract_features;
      using detail::generate_fingerprint;

      std::string result;

      // Create a Hamming window generator
      hamming_gen hamming_window(duration(window_size/sps), sps);

      // Iterate through the audio data with a step size of 512 samples
      for (std::size_t i = 0; i < audio.size(); i += step_size)
      {
         // Calculate the number of samples to copy
         std::size_t frame_size = std::min(window_size, audio.size()-i);

         // Initialize the frame with the required samples
         std::array<float, window_size> frame;
         std::copy(audio.begin()+i, audio.begin()+i+frame_size, frame.begin());

         // Pad the remaining part of the frame with zeros if necessary
         std::fill(frame.begin()+frame_size, frame.end(), 0.0f);

         // Apply the Hamming window to each sample in the frame
         for (std::size_t j = 0; j < frame.size(); ++j)
            frame[j] *= hamming_window();

         // Compute the magnitude spectrum of the windowed frame
         magspec<window_size>(frame.data());

         // Extract features from the magnitude spectrum
         auto features = extract_features<mag_spectrum_size>(frame.data());

         // Generate a fingerprint from the extracted features
         auto fingerprint = generate_fingerprint(features);

         // Append the generated fingerprint
         result += fingerprint;

         // Reset the Hamming window generator
         hamming_window.reset();
      }
      return result;
   }
}

#endif
