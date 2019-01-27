# Q DSP Library

## Introduction

Q, is a C++ library for Audio Digital Signal Processing. Aptly named after
the "Q factor", a dimensionless parameter that describes the quality of a
resonant circuit, the Q DSP Library is designed to be simple and elegant, as
the simplicity of its name suggests, and efficient enough to run on small
microcontrollers.

Q leverages the power of modern C++17 and efficient use of functional
programming, especially function composition using fine-grained and reusable
function objects (both stateless and stateful), to simplify complex DSP
programming tasks without sacrificing readability.

## Hello World

Here's our quick "Hello World" sample application that highlights the
simplicity of the Q DSP Library: a bandwidth limited square wave synthesizer
with ADSR envelope that controls an amplifier and resonant filter.

```c++
struct square_synth : q::audio_stream
{
   square_synth(q::envelope::config env_cfg)
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
         auto cutoff = env();       // Generate the ADSR envelope
         filter.cutoff(cutoff);     // Set the filter frequency

         // Synthesize the square wave
         auto val = q::square(phase++);

         // Apply the envelope (amplifier and filter) with soft clip
         val = clip(filter(val) * env());

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

Behind the scenes, there's a lot going on here, actually. But you will notice
that emphasis is given on making the library very readable, easy to
understand and follow by breaking down complex tasks into smaller manageable
tasks and using function composition at progressively high-levels, while
maintaining simplicity and clarity of intent.

The synthesizer above is composed of smaller building blocks: fine grained
C++ function objects. For example, here's the square wave oscillator
(bandwidth limited using poly_blep):

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
one can build an oscillator at compile time if needed, perhaps with wavetable
results stored in read-only memory.



