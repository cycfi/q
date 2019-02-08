# Q DSP Library

## Introduction

Q, is a C++ cross-platform library for Audio Digital Signal Processing. Aptly
named after the "Q factor", a dimensionless parameter that describes the
quality of a resonant circuit, the Q DSP Library is designed to be simple and
elegant, as the simplicity of its name suggests, and efficient enough to run
on small microcontrollers.

Q leverages the power of modern C++17 and efficient use of functional
programming, especially function composition using fine-grained and reusable
function objects (both stateless and stateful), to simplify complex DSP
programming tasks without sacrificing readability.

Q is the host of some experimental Music related DSP facilities such as
[Virtual Pickups](http://tinyurl.com/y8cqt8jr) (Virtual pickup placement
simulator) and [Bitstream Autocorrelation](http://tinyurl.com/yb49zlld) (An
extremely fast and efficient pitch detection scheme) [the author](#jdeguzman)
has accumulated over the years as part of research and development, and will
continue to evolve to accommodate more facilities necessary for the
fulfillment of various Music related projects. The library is Open Source and
released under the very liberal [MIT license](http://tinyurl.com/p6pekvo).

## Design and Architecture

The Q library comprises two layers:

1. *q_lib*: The core DSP library, which has no dependencies except the standard
   c++ library
2. *q_io*: Audio and MIDI I/O layer, with very minimal dependencies
   ([portaudio](http://www.portaudio.com/) and
   [portmidi](http://portmedia.sourceforge.net/portmidi/)) and very loose
   coupling via thin wrappers that are easy to transplant and port to a host,
   with or without an operating system, such as an audio plugin or direct to
   hardware ADC and DAC.

![Architecture](docs/architecture.png)

By design, communication to and from the application, including but not
limited to, parameter control, is done using MIDI. We will track the
development of the forthcoming (as of January 2019) MIDI 2.0, especially
extended 16-bit and 32-bit resolution and MIDI Capability Inquiry (MIDI-CI)
"Universal System Exclusive" messages.

The full Q library (including q_lib and q_io) is cross-platform and is tested
to work on the three major operating systems: MacOS, Linux and Windows. The Q
core library, q_lib, works on any environment that has a modern C++ compiler
with the standard C++ library (for example, all 32-bit ARM microcontrollers).

## Setup and Installation

Follow the [Setup and Installation guide](docs/setup.md) to get started using
the library.

## Hello, World

Here's a quick "Hello, World" example that highlights the simplicity of the Q
DSP Library: a delay effects processor.

```c++
   // 1: fractional delay
   q::delay _delay{ 350_ms, 44100 };

   // 2: Mix the signal s, and the delayed signal (where s is the incoming sample)
   auto _y = s + _delay();

   // 3: Feed back the result to the delay
   _delay.push(_y * _feedback);
```

Normally, there will be a processing loop that receives the incoming samples,
`s`. You place 1, the delay constructor, `q::delay`, before the processing
loop and 2 and 3 inside inside the loop.

44100 is the desired sampling rate. _feedback is the amount of feedback
desired (anything from 0.0 to less than 1.0, e.g. 0.85). But take note of
`350_ms`. Here, we take advantage of C++ (from c++11) type safe [user-defined
literals](http://tinyurl.com/yafvvb6b), instead of the usual `float` or
`double` which can be unsafe when values from different units (e.g. frequency
vs. duration) are mismatched. The Q DSP library makes abundant use of
user-defined literals for units such as time, frequency and even sound level
(e.g. 24_dB, instead of a unit-less 24 or worse, a non-intuitive, unit-less
15.8 —the gain equivalent of 24_dB). Such constants also make the code very
readable, another objective of this library.

Processors such as `q::delay` are C++ function objects (sometimes called
functors) that can be composed to form more complex processors. For example
if you want to filter the delayed signal with a low-pass with a 1 kHz cutoff
frequency, you apply the `q::lowpass` filter over the result of the delay:

```c++
   q::lowpass _lp{ 1_kHz, 44100 };
```

then insert the filter where it is needed in the processing loop:

```c++
   // 2: Add the signal s, and the delayed, low-pass filtered signal
   auto _y = s + _lp(_delay());
```

## Hello, Universe

Let us move on to a more elaborate example. How about a fully functional,
bandwidth limited square wave synthesizer with ADSR envelope that controls an
amplifier and a resonant filter and control the note-on and note-off using
MIDI? Sounds good? This example is complete and self-contained in one .cpp
file, yet still kept as simple as possible to highlight the ease of use.

The full example can be found here:
[example/square_synth.cpp](example/square_synth.cpp). After building the
program, make sure you have a MIDI keyboard connected before starting the
application. At startup, the app will present you with a list of available
MIDI hardware and will ask you what you want to use.

There are more demo applications in the example directory. After this quick
tutorial, free to explore.

### The Synth

Here's the actual synthesizer with the processing loop:

```c++
   struct my_square_synth : q::audio_stream
   {
      my_square_synth(q::envelope::config env_cfg)
       : audio_stream(0, 2)
       , env(env_cfg, this->sampling_rate())
       , filter(0.5, 0.8)
      {}

      void process(out_channels const& out)
      {
         auto left = out[0];
         auto right = out[1];
         for (auto frame : out.frames())
         {
            // Generate the ADSR envelope
            auto env_ = env();

            // Set the filter frequency
            filter.cutoff(env_);

            // Synthesize the square wave
            auto val = q::square(phase++);

            // Apply the envelope (amplifier and filter) with soft clip
            val = clip(filter(val) * env_);

            // Output
            right[frame] = left[frame] = val;
         }
      }

      q::phase_iterator phase;            // The phase iterator
      q::envelope       env;              // The envelope
      q::reso_filter    filter;           // The resonant filter
      q::soft_clip      clip;             // Soft clip
   };
```

Our synth is a subclass of `q::audio_stream` sets up buffers for the input
and output audio streams and presents those to our processing loop (the
`process` function above). Here, in this example, we setup an audio stream
with no inputs and two (stereo) outputs: `audio_stream(0, 2)`.

### The Oscillator

Behind the scenes, there's a lot going on here, actually. But you will notice
that emphasis is given to making the library very readable, easy to
understand and follow by breaking down complex tasks into smaller manageable
tasks and using function composition at progressively higher levels, while
maintaining simplicity and clarity of intent.

The synthesizer above is composed of smaller building blocks: fine grained
C++ function objects. For example, here's the square wave oscillator
(bandwidth limited using poly_blep). For now, we will skim over details such
as `phase`, `phase_iterator`, and  and this thing called `poly blep`. The
important point, exemplified here, is that we want to keep our building
blocks as simple and minimal as possible. One will notice that our
`square_synth` class does not even have state.

```c++
   struct square_synth
   {
      constexpr float operator()(phase p, phase dt) const
      {
         constexpr auto middle = phase::max() / 2;
         auto r = p < middle ? 1.0f : -1.0f;

         // Correct rising discontinuity
         r += poly_blep(p, dt);

         // Correct falling discontinuity
         r -= poly_blep(p + middle, dt);

         return r;
      }

      constexpr float operator()(phase_iterator i) const
      {
         return (*this)(i._phase, i._incr);
      }
   };

   constexpr auto square = square_synth{};
```

The modern C++ savvy programmer will immediately notice the use of
`constexpr`, applied judiciously all throughout the library. Such modern c++
facilities allow the compiler to generate extremely efficient code, even
those that are generated at compile time. That means, for this example, that
one can build an oscillator at compile time if needed, perhaps with constant
wavetable results stored in read-only memory.

### Processing MIDI

The `midi_processor` takes care of MIDI events. Your application will have
its own MIDI processor that deals with MIDI events that you are interested
in. For this simple example, we simply want to process note-on and note-off
events. On note-on events, our MIDI processor sets `my_square_synth`'s note
frequency and triggers its envelope for attack. On note-off events, our MIDI
processor initiates the envelope's release.

```c++
   struct my_midi_processor : midi::processor
   {
      using midi::processor::operator();

      my_midi_processor(my_square_synth& synth)
       : _synth(synth)
      {}

      void operator()(midi::note_on msg, std::size_t time)
      {
         _key = msg.key();
         auto freq = midi::note_frequency(_key);
         _synth.phase.set(freq, _synth.sampling_rate());
         _synth.env.trigger(float(msg.velocity()) / 128);
      }

      void operator()(midi::note_off msg, std::size_t time)
      {
         if (msg.key() == _key)
            _synth.env.release();
      }

      std::uint8_t      _key;
      my_square_synth&  _synth;
   };
```

### The Main Function

In the main function, we instantiate `my_square_synth` and
`my_midi_processor`. The synth constructor, in case you haven't noticed yet,
requires an envelope configuration (`envelope::config`). Here, we provide our
configuration. Take note that in this example, the envelope parameters are
constant, for the sake of simplicity, but you can definitely have these
controllable by the user by writing your own MIDI processor that deals with
MIDI control change messages.

Again, take note of the abundant use of user-defined literals for units such
as time, frequency.

```c++
   auto env_cfg = q::envelope::config
   {
      100_ms      // attack rate
    , 1_s         // decay rate
    , -12_dB      // sustain level
    , 5_s         // sustain rate
    , 1_s         // release rate
   };

   my_square_synth synth{ env_cfg };
```

Then, we create `my_midi_processor`, giving it a reference to
`my_square_synth`. We'll also need a `midi_input_stream` that receives the
actual incoming MIDI messages from the chosen hardware.

```c++
   q::midi_input_stream stream;
   my_midi_processor proc{ synth };
```

Now we're all set. We start the synth and enter a loop that exits when the
user presses ctrl-c (in which case the running flag becomes false). In the
loop, we give our MIDI processor a chance to process incoming MIDI events as
they arrive from the MIDI stream:

```c++
   synth.start();
   while (running)
      stream.process(proc);
   synth.stop();
```

---

## <a name="jdeguzman"></a>About the Author

Joel got into electronics and programming in the 80s because almost
everything in music, his first love, is becoming electronic and digital.
Since then, he builds his own guitars, effect boxes and synths. He enjoys
playing distortion-laden rock guitar, composes and produces his own music in
his home studio.

Joel de Guzman is the principal architect and engineer at [Cycfi
Research](https://www.cycfi.com/) and a consultant at [Ciere
Consulting](https://ciere.com/). He is a software engineer specializing in
advanced C++ and an advocate of Open Source. He has authored a number of
highly successful Open Source projects such as
[Boost.Spirit](http://tinyurl.com/ydhotlaf),
[Boost.Phoenix](http://tinyurl.com/y6vkeo5t) and
[Boost.Fusion](http://tinyurl.com/ybn5oq9v). These libraries are all part of
the [Boost Libraries](http://tinyurl.com/jubgged), a well respected,
peer-reviewed, Open Source, collaborative development effort.

---

*Copyright (c) 2014-2018 Joel de Guzman. All rights reserved.*
*Distributed under the [MIT License](https://opensource.org/licenses/MIT)*

