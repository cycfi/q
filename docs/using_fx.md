---
title: Using FX
image: "/assets/images/q-logo-small.png"
---

## Biquad filters and delay

Let's take a look into applying some fx in real time on a test audio file. In
particular we're going to look into some biquad highpass and lowpass filters
as well as a delay effect.
This examples are complete and self-contained in one .cpp
file, kept as simple as possible to highlight the ease-of-use.

Make sure you have a working sound system and the volume is up :)

Take note that the demo is a console application. The Q library does not have
a GUI, for good reason! We want to keep it as simple as possible. The GUI is
taken cared of by other libraries (e.g.
[Elements](https://github.com/cycfi/elements)).

There are more demo applications in the example directory. After this quick
tutorial, free to explore.

### The FX processor

Here's the filter processor:

```c++
   struct filter_processor : q::port_audio_stream
   {
      filter_processor(
         q::wav_memory& wav
       , q::frequency hpfFreq
       , q::frequency lpfFreq
      )
       : port_audio_stream(0, 2, wav.sps())
       , _wav(wav)
       , _hpf(hpfFreq, wav.sps())
       , _lpf(lpfFreq, wav.sps())
      {}

      void process(out_channels const& out)
      {
         auto left = out[0];
         auto right = out[1];
         for (auto frame : out.frames())
         {
            // Get the next input sample
            auto s = _wav()[0];

            // Cascaded highpass and lowpass processing
            _y = _hpf(s),
            _y = _lpf(_y);

            // Output
            left[frame] = _y;
            right[frame] = _y;
         }
      }

      q::wav_memory&    _wav;
      q::highpass      _hpf;
      q::lowpass       _lpf;
      float             _y = 0.0f;
   };
```

As seen in other examples too, this is a subclass of `q::port_audio_stream`. 
We setup an audio stream with the default settings and provide the output 
audio stream to our processing loop (the `process` function seen above). 

```c++
   filter_processor(
      q::wav_memory& wav
    , q::frequency hpfFreq
    , q::frequency lpfFreq
   )
    : port_audio_stream(0, 2, wav.sps())
    , _wav(wav)
    , _hpf(hpfFreq, wav.sps())
    , _lpf(lpfFreq, wav.sps())
   {}
```

Moreover, we provide the class with the data of a wave file loaded from the 
hard disk as well as settings for the filters, namely cutoff frequencies. 
The sampling frequency is of course defined by the wave file. 
The filters are initialized:

```c++
   _hpf(hpfFreq, wav.sps())
```

```c++
   _lpf(lpfFreq, wav.sps())
```

Now let's take a closer look at the very "flesh" of our example, our process function:

```c++
   void process(out_channels const& out)
   {
      auto left = out[0];
      auto right = out[1];
      for (auto frame : out.frames())
      {
         // Get the next input sample
         auto s = _wav()[0];

         // Cascaded highpass and lowpass processing
         _y = _hpf(s),
         _y = _lpf(_y);

         // Output
         left[frame] = _y;
         right[frame] = _y;
      }
   }
```

The first two lines give us access to the audio output stream:

```c++
   auto left = out[0];
   auto right = out[1];
```

Then we loop through all the samples in the wave file, fetching one-by-one:

```c++
   auto s = _wav()[0];
```

And apply our filters in cascade to the every sample:

```c++
   _y = _hpf(s),
   _y = _lpf(_y);
```

And finally send the processed sample off to the output stream:

```c++
   left[frame] = _y;
   right[frame] = _y;
```

### The Main Function

In the main function, we make sure to load our test audio file.

```c++
   q::wav_memory     wav{ "audio_files/Low E.wav" };
```

The we initialize our audio stream by providing with the wave file and some 
drastic cutoff frequencies to make the result of the processing very apparent.

```c++
   filter_processor   proc{ wav, 1_kHz, 2_kHz };
```

Now we're ready to start the audio stream and wait for the audio file to be played to the end.

```c++
   if (proc.is_valid())
   {
      proc.start();
      q::sleep(q::duration(wav.length()) / wav.sps());
      proc.stop();
   }
```

### Delay effects

Similarly, in the delay.cpp example also found in the example folder, 
we have our delay processor:


```c++
   struct delay_processor : q::port_audio_stream
   {
      delay_processor(
         q::wav_memory& wav
       , q::duration delay
       , float feedback
      )
       : port_audio_stream(0, 2, wav.sps())
       , _wav(wav)
       , _delay(delay, wav.sps())
       , _feedback(feedback)
      {}

      void process(out_channels const& out)
      {
         auto left = out[0];
         auto right = out[1];
         for (auto frame : out.frames())
         {
            // Get the next input sample
            auto s = _wav()[0];

            // Mix the signal and the delayed signal
            _y = s + _delay();

            // Feed back the result to the delay
            _delay.push(_y * _feedback);

            // Output
            left[frame] = s;
            right[frame] = _y;
         }
      }

      q::wav_memory&    _wav;
      q::delay          _delay;
      float             _feedback;
      float             _y = 0.0f;
   };
```

The only difference here is that we initialize a delay effect and provide a value (between 0 to 1) to define how much feedback the delay will have:

```c++
   _delay(delay, wav.sps())
```

```c++
   _feedback(feedback)
```

In our process loop we mix the current sample with the sum of delayed samples and make sure to push the new sample to the delay stack with the defined feedback gain.

```c++
   _y = s + _delay();
   _delay.push(_y * _feedback);
```
In the main function we make sure to initiliaze our delay processor with the test audio file and delay and feedback parameters.

```c++
   delay_processor   proc{ wav, 350_ms, 0.85f };
```   



---

*Copyright (c) 2014-2021 Joel de Guzman. All rights reserved.*
*Distributed under the [MIT License](https://opensource.org/licenses/MIT)*

