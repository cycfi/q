## Hello, World

Here's a quick "Hello, World" example that highlights the simplicity of the Q
DSP Library: a delay effects processor.

:point_right: &nbsp; The full example can be found here:
[example/delay.cpp](https://github.com/cycfi/Q/blob/master/example/delay.cpp).

The example loads a pre-recorded wav file and plays it back with processing.
The raw audio source will be played in the left channel while the delayed
signal will be played in the right channel. Pretty much as straightforward as
possible. The audio will be played using the currently selected audio device.

## The DSP Code

```c++
   // 1: fractional delay
   q::delay _delay{ 350_ms, 44100 };

   // 2: Mix the signal s, and the delayed signal (where s is the incoming sample)
   auto _y = s + _delay();

   // 3: Feed back the result to the delay
   _delay.push(_y * _feedback);
```

Normally, there will be a processing loop that receives the incoming samples,
`s`. You place #1, the delay constructor, `q::delay`, before the processing
loop and #2 and #3 inside inside the loop.

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

---

*Copyright (c) 2014-2021 Joel de Guzman. All rights reserved.*
*Distributed under the [MIT License](https://opensource.org/licenses/MIT)*

