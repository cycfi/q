---
title: Primitive Data Types
---

## Primitive Data Types

These are the primitive data types used by the Q library:

- Sample data types which are `float` types.
- The date type `decibel` that operates on the logarithmic domain.
- Other primitives that inherit from the `value` template class.

-------------------------------------------------------------------------------
## value&lt;T&gt;

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

## Values types

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
phase::min()   // Get the minimum phase representing 0 degrees
phase::max()   // Get the maximum phase representing 360 degrees (2π)
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


