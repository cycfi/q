# ![Q-Logo](images/q-logo-small.png) Audio DSP Library

## File Structure

The library is organized with this simplified directory structure:

* docs
* example
* q_io
   * external
   * include
   * src
* q_lib
   * include
* test

`docs` is where this documentation resides. `example` contains
self-contained and easy to understand c++ files that demonstrate various
features of the library. `q_io` is the Audio and MIDI I/O layer which
contains `external` —3rd party libraries used by the `q_io` module,
`include` —Header files and `src` —Source files. `q_lib` is the
header-only core DSP library in the `include` sub-directory. Finally, the
`test` directory contains a bunch of c++ files for testing the library.

## Q-Functions

In the electronics music world, there's the notion of *processors* and
*synthesizers*, the definitions of which are somewhat overlapping and differs
only on one specific point: that processors take in one or more input
value(s) and produces one or more output value(s), whereas a synthesizer does
not take in any inputs at all. Here' we'll just call both *processors* and
*synthesizers*, *q-functions*.

*q-functions* are simply function objects, the basic building blocks that can
be composed to form more complex functions. A function object can have zero
or more input values and produces one or more output values (typically just
one, but in certain cases, more than one output values may be returned in the
form of C+++ tuples).

Syntactically, you call these just like any other function like so for a
single input function:

```c++
   float r = f(s);
```
where s is the input value, and `f(s)` returns a result and stores it in the
variable `r`. Typical *q-functions* work on 32-bit `float` input samples with
the typical -1.0 to 1.0 range of, where 1.0 peaks at 0dB.

## Scalar types

Scalar values are not restricted to sampled signals, however. For an example,
signal envelopes are best represented in the decibel domain, and so
dynamic-range processors such as compressors and expanders take `decibel` as
inputs and return `decibel` results. For example:

```c++
   decibel gain = comp(env);
```

Another example, oscillators work on phase-angle inputs and return output
samples:

```c++
   float out = sin(phase++);
```

The Q DSP library has a rich set of such types:

* `float`: Typical sample data type -1.0 to 1.0 (or beyond for some
  computational headroom).
* `frequency`: Cycles per second (Hz).
* `phase`: Fixed point 1.31 format where 31 bits are fractional. `phase`
  represents 0 to 2π phase values suitable for oscillators.
* `decibel`: Ratio of one value to another on a logarithmic scale (dB)

The Q DSP library is typeful and typesafe. You can not mismatch scalar values
of different types such as `frequency` and `decibel`, for example. Such
mismatch mistakes can potentially be disastrous.

## Literals

To augment the wealth of scalar types, the Q DSP library makes abundant use
of [C++ user-defined
literals](https://en.cppreference.com/w/cpp/language/user_literal). Here are
some examples:

```c++
   auto c4 = 261.6256_Hz;
   auto threshold = -36_dB;
   auto coef = 1.0 - (2_pi * 1_kHz / sps);
   auto attack = 20_ms;
```

There are also tables for notes:

```c++
   // 6 string guitar frequencies:
   constexpr auto low_e   = E[2];
   constexpr auto a       = A[2];
   constexpr auto d       = D[3];
   constexpr auto g       = G[3];
   constexpr auto b       = B[3];
   constexpr auto high_e  = E[4];
```

