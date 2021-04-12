---
title: Hello, Universe
image: "/assets/images/q_synth.jpg"
---

## Hello, Universe

Let us move on to a more elaborate example. How about a fully functional,
bandwidth limited square wave synthesizer with ADSR envelope that controls an
amplifier and a resonant filter and control the note-on and note-off using
MIDI? Sounds good? This example is complete and self-contained in one .cpp
file, kept as simple as possible to highlight the ease-of-use.

:point_right: &nbsp; The full example can be found here:
[example/square_synth.cpp](https://github.com/cycfi/Q/blob/master/example/square_synth.cpp).

Here's a short video clip:

{% include vimeoPlayer.html id=419775584 width=732 height=462 %}

After building the program, make sure you have a MIDI keyboard connected
before starting the application. At startup, the app will present you with a
list of available MIDI hardware and will ask you what you want to use:

```
================================================================================
Available MIDI Devices (ID : "Name" inputs/outputs):
0 : "Code 61 USB MIDI" 1/0
1 : "Code 61 MIDI DIN" 1/0
2 : "Code 61 Mackie/HUI" 1/0
3 : "Code 61 Editor" 1/0
4 : "ZOOM R16_R24" 1/0
5 : "Code 61 USB MIDI" 0/1
6 : "Code 61 MIDI DIN" 0/1
7 : "Code 61 Mackie/HUI" 0/1
8 : "Code 61 Editor" 0/1
9 : "ZOOM R16_R24" 0/1
================================================================================
Choose MIDI Device ID: 0
```

And then a list of audio devices to choose from:

```
================================================================================
Available Audio Devices (ID : "Name" inputs/outputs):
0 : "Built-in Microphone" 2/0
1 : "Built-in Output" 0/2
2 : "HDMI" 0/2
3 : "ZOOM R16_R24 Driver" 8/2
================================================================================
Choose Audio Device ID: 3
```

Take note that the demo is a console application. The Q library does not have
a GUI, for good reason! We want to keep it as simple as possible. The GUI is
taken cared of by other libraries (e.g.
[Elements](https://github.com/cycfi/elements)).

After choosing the MIDI and Audio driver, the synth is playable. The synth is
monophonic and responds to velocity only, for simplicity.

There are more demo applications in the example directory. After this quick
tutorial, free to explore.

### The Synth

Here's the actual synthesizer with the processing loop:

```c++
   struct my_square_synth : q::port_audio_stream
   {
      my_square_synth(q::envelope::config env_cfg, int device_id)
       : port_audio_stream(q::audio_device::get(device_id), 0, 2)
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

Our synth, a subclass of `q::port_audio_stream`, sets up buffers for the
input and output audio streams and presents those to our processing loop (the
`process` function above). In this example, we setup an audio stream with the
selected device, no inputs and two (stereo) outputs:

```c++
port_audio_stream(q::audio_device::get(device_id), 0, 2)
```

### The Oscillator

Behind the scenes, there's a lot going on here, actually. But you will notice
that emphasis is given to making the library very readable, easy to
understand and follow by breaking down complex tasks into smaller manageable
tasks and using function composition at progressively higher levels, while
maintaining simplicity and clarity of intent.

The synthesizer above is composed of smaller building blocks: fine grained
C++ function objects. For example, here's the square wave oscillator
(bandwidth limited using poly_blep).

:point_right: &nbsp; For now, we will skim over details such as the
`envelope`, `phase`, and `phase_iterator`, and  and this thing called `poly
blep`. The important point, exemplified here, is that we want to keep our
building blocks as simple and minimal as possible. We will cover that in
greater detail later.

The astute reader may notice that our `square_synth` class does not even
have state!

```c++
   struct square_synth
   {
      constexpr float operator()(phase p, phase dt) const
      {
         constexpr auto middle = phase::middle();
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

Yeah, that's the complete oscillator. That's all there is to it! :wink:

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
as duration (e.g. 100_ms) and level (e.g. -12_dB).

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

*Copyright (c) 2014-2021 Joel de Guzman. All rights reserved.*
*Distributed under the [MIT License](https://opensource.org/licenses/MIT)*

