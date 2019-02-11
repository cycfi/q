# ![Q-Logo](images/q-logo-small.png) Audio DSP Library

## Values API

Most (not all) value types, except raw sample data types which are `float`
types, and `decibel`, which is special because it operates on the logarithmic
domain, inherit from the `value` template class.

These declarations should be self-explanatory:

```c++
   template <typename T, typename Derived>
   struct value
   {
      using derived_type = Derived;
      using value_type = T;

      constexpr explicit            value(T val = T(0));
      constexpr                     value(value const&) = default;
      constexpr                     value(value&&) = default;

      constexpr value&              operator=(value const&) = default;
      constexpr value&              operator=(value&&) = default;

      constexpr explicit operator   T() const;
      constexpr derived_type        operator+() const;
      constexpr derived_type        operator-() const;

      constexpr derived_type&       operator+=(value rhs);
      constexpr derived_type&       operator-=(value rhs);
      constexpr derived_type&       operator*=(value rhs);
      constexpr derived_type&       operator/=(value rhs);

      constexpr derived_type const& derived() const;
      constexpr derived_type&       derived();

      T rep;
   };

   ////////////////////////////////////////////////////////////////////////////
   template <typename T, typename Derived>
   constexpr bool operator==(value<T, Derived> a, value<T, Derived> b);

   template <typename T, typename Derived>
   constexpr bool operator==(T a, value<T, Derived> b);

   template <typename T, typename Derived>
   constexpr bool operator==(value<T, Derived> a, T b);

   ////////////////////////////////////////////////////////////////////////////
   template <typename T, typename Derived>
   constexpr bool operator!=(value<T, Derived> a, value<T, Derived> b);

   template <typename T, typename Derived>
   constexpr bool operator!=(T a, value<T, Derived> b);

   template <typename T, typename Derived>
   constexpr bool operator!=(value<T, Derived> a, T b);

   ////////////////////////////////////////////////////////////////////////////
   template <typename T, typename Derived>
   constexpr bool operator<(value<T, Derived> a, value<T, Derived> b);

   template <typename T, typename Derived>
   constexpr bool operator<(T a, value<T, Derived> b);

   template <typename T, typename Derived>
   constexpr bool operator<(value<T, Derived> a, T b);

   ////////////////////////////////////////////////////////////////////////////
   template <typename T, typename Derived>
   constexpr bool operator<=(value<T, Derived> a, value<T, Derived> b);

   template <typename T, typename Derived>
   constexpr bool operator<=(T a, value<T, Derived> b);

   template <typename T, typename Derived>
   constexpr bool operator<=(value<T, Derived> a, T b);

   ////////////////////////////////////////////////////////////////////////////
   template <typename T, typename Derived>
   constexpr bool operator>(value<T, Derived> a, value<T, Derived> b);

   template <typename T, typename Derived>
   constexpr bool operator>(T a, value<T, Derived> b);

   template <typename T, typename Derived>
   constexpr bool operator>(value<T, Derived> a, T b);

   ////////////////////////////////////////////////////////////////////////////
   template <typename T, typename Derived>
   constexpr bool operator>=(value<T, Derived> a, value<T, Derived> b);

   template <typename T, typename Derived>
   constexpr bool operator>=(T a, value<T, Derived> b);

   template <typename T, typename Derived>
   constexpr bool operator>=(value<T, Derived> a, T b);

   ////////////////////////////////////////////////////////////////////////////
   template <typename T, typename Derived>
   constexpr Derived operator+(value<T, Derived> a, value<T, Derived> b);

   template <typename T, typename Derived>
   constexpr Derived operator-(value<T, Derived> a, value<T, Derived> b);

   template <typename T, typename Derived>
   constexpr Derived operator*(value<T, Derived> a, value<T, Derived> b);

   template <typename T, typename Derived>
   constexpr Derived operator/(value<T, Derived> a, value<T, Derived> b);

   ////////////////////////////////////////////////////////////////////////////
   template <typename T, typename Derived, typename T2>
   constexpr typename std::enable_if<
      !std::is_same<Derived, T2>::value, Derived
   >::type
   operator+(T2 a, value<T, Derived> b);

   template <typename T, typename Derived, typename T2>
   constexpr typename std::enable_if<
     !std::is_same<Derived, T2>::value, Derived
   >::type
   operator-(T2 a, value<T, Derived> b);

   template <typename T, typename Derived, typename T2>
   constexpr typename std::enable_if<
      !std::is_same<Derived, T2>::value, Derived
   >::type
   operator*(T2 a, value<T, Derived> b);

   template <typename T, typename Derived, typename T2>
   constexpr typename std::enable_if<
      !std::is_same<Derived, T2>::value, Derived
   >::type
   operator/(T2 a, value<T, Derived> b);

   ////////////////////////////////////////////////////////////////////////////
   template <typename T, typename Derived, typename T2>
   constexpr typename std::enable_if<
      !std::is_same<Derived, T2>::value, Derived
   >::type
   operator+(value<T, Derived> a, T2 b);

   template <typename T, typename Derived, typename T2>
   constexpr typename std::enable_if<
      !std::is_same<Derived, T2>::value, Derived
   >::type
   operator-(value<T, Derived> a, T2 b);

   template <typename T, typename Derived, typename T2>
   constexpr typename std::enable_if<
      !std::is_same<Derived, T2>::value, Derived
   >::type
   operator*(value<T, Derived> a, T2 b);
   }

   template <typename T, typename Derived, typename T2>
   constexpr typename std::enable_if<
      !std::is_same<Derived, T2>::value, Derived
   >::type
   operator/(value<T, Derived> a, T2 b);
```

## Values types

### frequency

```c++
   struct frequency : value<double, frequency>
   {
      using base_type = value<double, frequency>;
      using base_type::base_type;

      constexpr                     frequency(double val);
      constexpr                     frequency(duration d);

      constexpr explicit operator   double() const ;
      constexpr explicit operator   float() const;
      constexpr q::period           period() const;
   };
```

### duration

```c++
   struct duration : value<double, duration>
   {
      using base_type = value<double, duration>;
      using base_type::base_type;

      constexpr                     duration(double val);

      constexpr explicit operator   double() const;
      constexpr explicit operator   float() const;
   };
```

### period

```c++
   struct period : duration
   {
      using duration::duration;

      constexpr                     period(duration d);
      constexpr                     period(frequency f);
   };
```

### phase

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

### decibel

```c++
   struct decibel
   {
      decibel() : val(0.0f) {}
      decibel(double val);

      explicit operator double() const;
      explicit operator float() const;
      constexpr decibel operator-() const;

      double val = 0.0f;
   };

   constexpr decibel operator-(decibel a, decibel b);
   constexpr decibel operator+(decibel a, decibel b);
   constexpr decibel operator*(decibel a, decibel b);
   constexpr decibel operator*(decibel a, double b);
   constexpr decibel operator*(decibel a, float b);
   constexpr decibel operator*(double a, decibel b);
   constexpr decibel operator*(float a, decibel b);
   decibel           operator/(decibel a, decibel b);
   decibel           operator/(decibel a, double b);
   decibel           operator/(decibel a, float b);
   constexpr bool    operator==(decibel a, decibel b);
   constexpr bool    operator!=(decibel a, decibel b);
   constexpr bool    operator<(decibel a, decibel b);
   constexpr bool    operator<=(decibel a, decibel b);
   constexpr bool    operator>(decibel a, decibel b);
   constexpr bool    operator>=(decibel a, decibel b);
```
