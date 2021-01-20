---
title: Fundamentals
---

# Fundamentals

-------------------------------------------------------------------------------

## Table of Contents
* [File Structure](#file-structure)
* [Namespace](#namespace)
* [Functors](#functors)
* [Data Types](#data-types)
* [value](#value)
* [value subclasses](#value-types)
    * [frequency](#frequency)
    * [period](#period)
    * [phase](#phase)
    * [duration](#duration)
* [decibel](#decibel)
* [Literals](#literals)
* [Notes](#notes)

-------------------------------------------------------------------------------
## File Structure

The library is organized with this simplified directory structure:

* docs: Where this documentation resides.
* example: Self-contained and easy to understand c++ programs that
   demonstrate various features of the library.
* q_io:
   * external: 3rd party libraries used by the `q_io` module.
   * include: q_io header files.
   * src: q_io source files.
* q_lib:
   * include: Header-only core q_lib DSP library.
* test: Contains a comprehensive set of c++ files for testing the library.

-------------------------------------------------------------------------------
## Namespace

All entities in the Q library are placed in namespace `cycfi::q`. Everywhere
in this documentation, we will be using a namespace alias to make the code
less verbose:

```c++
namespace q = cycfi::q;
```

-------------------------------------------------------------------------------
## Functors

In the world of electronic music, there are *processors* and *synthesizers*,
the definitions of which are somewhat overlapping and differ only on one
specific point: that processors take in one or more input value(s) and
produces one or more output value(s), whereas a synthesizer does not take in
any inputs at all.

In the Q world, both *processors* and *synthesizers* are just *functors* —C++
function objects, which are basic building blocks that can be composed to
form more complex functions. A functor can have zero or more input values and
produces one or more output values (typically just one, but in certain cases,
two or more output values may be returned in the form of C++ tuples).

Syntactically, you use these just like any other function. So, for instance,
for a single input functor:

```c++
float r = f(s);
```
where `s` is the input value, and `f(s)` returns a result and stores it in
the variable `r`. Typical audio processor *functors* in the Q DSP library
work on 32-bit `float` input samples with the normal -1.0 to 1.0 range.

-------------------------------------------------------------------------------
## Data Types

Values are not restricted to sampled signals, however. For an example, signal
envelopes are best represented in the decibel domain, and so dynamic-range
processors such as compressors and expanders take `decibel` as inputs and
return `decibel` results. For example:

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
* `duration` : A time span (seconds, milliseconds, etc.)
* `period` : The inverse of frequency
* `phase`: Fixed point 1.31 format where 31 bits are fractional. `phase`
  represents 0 to 2π phase values suitable for oscillators.
* `decibel`: Ratio of one value to another on a logarithmic scale (dB)

The Q DSP library is typeful and typesafe. You can not mismatch values of
different types such as `frequency` and `decibel`, for example. Such
potentially disastrous mistakes can happen if all values are just raw
floating point types.

Values do not have implicit conversion to raw types, however, except for
`decibel`, which is special because it operates on the logarithmic domain,
comparison and arithmetic with raw types are possible. For example:

```c++
   auto harmonic = 440_Hz * 4; // 440_Hz is a frequency literal (see below)
```

-------------------------------------------------------------------------------
## value

Type safe representation of a scalar value. `value` is a template
parameterized by the underlying type, `T` and the derived class `Derived`.

```c++
template <typename T, typename Derived>
struct value;
```

### Expressions

#### Notation

| `v`          | Scalar value.            |
| `a`, `b`     | Instance of `value<T>`   |

#### Constructors and assignment

```c++
// Default constructor [1].
value<T>{}

// Constructor [2].
value<T>{ v }

// Copy constructor. [3]
value<T>{ a }

// Assignment [4]
a = b
```

#### Semantics
1. Default construct a `value<T>` with initial value `{ 0 }`
2. Construct a `value<T>` given initial value `v`.
3. Copy construct a `value<T>`, `a`.
4. Assign `b`, to `a`.

#### Comparison

```c++
a == b      // Equality
a == v      // Equality with a scalar
v == b      // Equality with a scalar

a != b      // Non-equality
a != v      // Non-equality with a scalar
v != b      // Non-equality with a scalar

a < b       // Less than
a < v       // Less than with a scalar
v < b       // Less than with a scalar

a <= b      // Less than equal
a <= v      // Less than equal with a scalar
v <= b      // Less than equal with a scalar

a > b       // Greater than
a > v       // Greater than with a scalar
v > b       // Greater than with a scalar

a >= b      // Greater than equal
a >= v      // Greater than equal with a scalar
v >= b      // Greater than equal with a scalar
```

#### Arithmetic
```c++
+a          // Positive
-a          // Negative

a += b      // Add assign
a -= b      // Subtract assign
a *= b      // Multiply assign
a /= b      // Divide assign

a + b       // Addition
a + v       // Addition with a scalar
v + b       // Addition with a scalar

a - b       // Subtraction
a - v       // Subtraction with a scalar
v - b       // Subtraction with a scalar

a * b       // Multiplication
a * v       // Multiplication with a scalar
v * b       // Multiplication with a scalar

a / b       // Division
a / v       // Division with a scalar
v / b       // Division with a scalar
```

## value subclasses

-------------------------------------------------------------------------------
### frequency

Type safe representation of frequency in Hertz.

```c++
struct frequency : value<double, frequency>
{
   constexpr                     frequency(double val);
   constexpr                     frequency(duration d);

   constexpr explicit operator   double() const ;
   constexpr explicit operator   float() const;
   constexpr q::period           period() const;
};
```

### Expressions

In addition to valid expressions for `value<T>`, `frequency` allows these
expressions.

#### Notation

| `d`       | Instance of `duration` (see below.)  |
| `f`       | Instance of `frequency`              |

#### Construction

```c++
// Construct a phase given the period (duration)
phase{ d }
```

#### Conversions

```c++
float(f)       // Convert frequency to a scalar (float)
double(f)      // Convert frequency to a scalar (double)
```

#### Misc

```c++
f.period()     // get the period (1/f)
```

-------------------------------------------------------------------------------
### duration

Type safe representation of duration.

```c++
struct duration : value<double, duration>
{
   constexpr                     duration(double val);

   constexpr explicit operator   double() const;
   constexpr explicit operator   float() const;
};
```

### Expressions

In addition to valid expressions for `value<T>`, `duration` allows these
expressions.

#### Notation

| `d`    | Instance of `duration`            |

#### Conversions

```c++
float(d)       // Convert duration to a scalar (float)
double(d)      // Convert duration to a scalar (double)
```

-------------------------------------------------------------------------------
### period

Type safe representation of period (reciprocal of frequency).

```c++
struct period : duration
{
   using duration::duration;

   constexpr                     period(duration d);
   constexpr                     period(frequency f);
};
```

### Expressions

In addition to valid expressions for `value<T>`, `period` allows these
expressions.

#### Notation

| `d`       | Instance of `duration`      |
| `f`       | Instance of `frequency`     |
| `p`       | Instance of `period`        |

#### Construction

```c++
// Construct a phase given a duration
phase{ d }

// Construct a phase given a frequency
phase{ f }
```

#### Conversions

```c++
float(p)       // Convert period to a scalar (float)
double(p)      // Convert period to a scalar (double)
```

-------------------------------------------------------------------------------
### phase

phase: The synthesizers use fixed point 1.31 format computations where 31
bits are fractional. phase represents phase values that run from 0 to
4294967295 (0 to 2π) suitable for oscillators.

The turn, also cycle, full circle, revolution, and rotation, is a complete
circular movement or measure (as to return to the same point) with circle or
ellipse. A turn is abbreviated τ, cyc, rev, or rot depending on the
application. The symbol τ can also be used as a mathematical constant to
represent 2π radians.

[https://en.wikipedia.org/wiki/Angular_unit](https://en.wikipedia.org/wiki/Angular_unit)

```c++
struct phase : value<std::uint32_t, phase>
{
   using base_type = value<std::uint32_t, phase>;
   using base_type::base_type;

   constexpr static auto one_cyc = int_max<std::uint32_t>();
   constexpr static auto bits = sizeof(std::uint32_t) * 8;

   constexpr explicit            phase(value_type val = 0);
   constexpr explicit            phase(float frac);
   constexpr explicit            phase(double frac);
   constexpr                     phase(frequency freq, std::uint32_t sps);

   constexpr explicit operator   float() const;
   constexpr explicit operator   double() const;

   constexpr static phase        min();
   constexpr static phase        max();
};
```

### Expressions

In addition to valid expressions for `value<T>`, `phase` allows these
expressions.

#### Notation

| `f`          | A `double` or `float`    |
| `freq`       | Instance of `frequency`  |
| `sps`        | Scalar value             |
| `p`          | Instance of `phase`  |

#### Construction

```c++
// Construct a phase given the a fractional number from 0.0 to 1.0 (0 to 2π)
phase{ f }

// Construct a phase given the frequency and samples/second (`sps`)
phase{ freq, sps }
```

#### Conversions

```c++
float(p)       // Convert a phase to a scalar (float)
double(p)      // Convert a phase to a scalar (double)
```

#### Min and Max

```c++
phase::begin() // Get the minimum phase representing 0 degrees
phase::end()   // Get the maximum phase representing 360 degrees (2π)
```

-------------------------------------------------------------------------------
## decibel

Decibel is unique. It does not inherit from `value<T>` because it is
non-linear and operates on the logarithmic domain. The `decibel` class is
perfectly suitable for dynamics processing (e.g. compressors and limiters and
envelopes). Q provides fast decibel computations using lookup tables for
converting to and from scalars.

```c++
struct decibel
{
   decibel();
   decibel(double val);

   explicit operator double() const;
   explicit operator float() const;
   constexpr decibel operator+() const;
   constexpr decibel operator-() const;

   double val = 0.0f;
};
```

### Expressions

#### Notation

| `v`          | Scalar value.            |
| `a`, `b`     | Instance of `decibel`    |

#### Constructors and assignment

```c++
// Default constructor [1].
decibel{}

// Constructor [2].
decibel{ v }

// Copy constructor. [3]
decibel{ a }

// Assignment [4]
a = b
```

#### Semantics
1. Default construct a `decibel` with initial value `{ 0 }`
2. Construct a `decibel` given initial value `v`.
3. Copy construct a `decibel`, `a`.
4. Assign `b`, to `a`.

#### Comparison

```c++
a == b      // Equality
a != b      // Non-equality
a < b       // Less than
a <= b      // Less than equal
a > b       // Greater than
a >= b      // Greater than equal
```

#### Arithmetic
```c++
+a          // Positive
-a          // Negative

a += b      // Add assign
a -= b      // Subtract assign
a *= b      // Multiply assign
a /= b      // Divide assign

a + b       // Addition
a - b       // Subtraction

a * b       // Multiplication
a * v       // Multiplication with a scalar
v * b       // Multiplication with a scalar

a / b       // Division
a / v       // Division with a scalar
```

#### Conversions
```c++
float(a)    // Convert a decibel to a scalar (float)
double(a)   // Convert a decibel to a scalar (double)
```

-------------------------------------------------------------------------------
## Literals

To augment the wealth of value types, the Q DSP library makes abundant use of
[C++ user-defined literals][1].

We take advantage of C++ (from c++11) type safe [user-defined literals][1],
instead of the usual `int`, `float` or `double` which can be unsafe when
values from different units (e.g. frequency vs. duration) are mismatched. The
Q DSP library makes abundant use of user-defined literals for units such as
time, frequency and decibels (e.g. 24_dB, instead of a unit-less 24 or worse,
a non-intuitive, unit-less 15.8 —the gain equivalent of 24_dB). Such
constants also make the code very readable, another objective of this
library.

Q Literals are placed in the namespace `q::literals`. The namespace is
sparse enough to be hoisted into your namespace using `using namespace`:

### Example Expressions

#### Frequencies

```c++
82.4069_Hz
440_Hz
1.5_KHz
1.5_kHz
1_KHz
1_kHz
0.5_MHz
3_MHz
```

#### Durations

```c++
10.3_s
1_s
20.5_ms
1_ms
10.5_us
500_us
```

#### Decibels

```c++
-3.5_dB
10_dB
```

#### Pi

```c++
2_pi
0.5_pi
```

To use these literals, include the `literals.hpp` header:

```c++
#include <q/support/literals.hpp>
```

then use the `literals` namespace somewhere in a scope where you need it:

```c++
using namespace q::literals;
```

[1]: http://tinyurl.com/yafvvb6b

-------------------------------------------------------------------------------
## Notes

There is also a complete set of tables for notes from `A[0]` (27.5Hz) to
`Ab[9]` (13289.75Hz). For example, to get the frequencies for each of the
open strings in a 6-string guitar:

```c++
// 6 string guitar frequencies:
constexpr auto low_e   = E[2];
constexpr auto a       = A[2];
constexpr auto d       = D[3];
constexpr auto g       = G[3];
constexpr auto b       = B[3];
constexpr auto high_e  = E[4];
```

To use these literals, include the `notes.hpp` header:

```c++
#include <q/support/notes.hpp>
```

then use the `notes` namespace somewhere in a scope where you need it:

```c++
using namespace q::notes;
```


