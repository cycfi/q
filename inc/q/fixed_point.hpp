#include <type_traits>
#include <cstdint>

namespace cycfi { namespace q
{
   template <
      typename T
    , std::size_t frac = sizeof(T) * 4
    , typename T2 = decltype(promote(T()))
   >
   struct fixed_point
   {
      static_assert(std::is_arithmetic<T>::value,
         "Error: T must be an arithmetic type"
      );

      constexpr fixed_point() = default;
      constexpr fixed_point(fixed_point const&) = default;

      template <typename U>
      constexpr fixed_point(
         U val, typename std::enable_if<std::is_arithmetic<U>::value>::type* = 0);

      template <typename U, std::size_t frac_, typename U2>
      constexpr fixed_point(fixed_point<U, frac_, U2> rhs);

      template <typename U>
      constexpr explicit operator U() const;

      constexpr T rep() const;

      constexpr fixed_point&        operator=(fixed_point const&) = default;

      template <typename U>
      constexpr fixed_point&        operator=(U x);

      constexpr fixed_point&        operator+=(fixed_point x);
      constexpr fixed_point&        operator-=(fixed_point x);
      constexpr fixed_point&        operator*=(fixed_point x);
      constexpr fixed_point&        operator/=(fixed_point x);
      constexpr fixed_point         operator-() const;

      template <typename U>
      typename std::enable_if<std::is_arithmetic<U>::value, fixed_point&>::type
      constexpr operator*=(U x);

      template <typename U>
      typename std::enable_if<std::is_arithmetic<U>::value, fixed_point&>::type
      constexpr operator/=(U x);

   private:

      struct private_ {};
      fixed_point(int _rep, private_) : _rep(_rep) {}

      T _rep = 0;
   };

   template <typename T, std::size_t frac, typename T2>
   constexpr fixed_point<T, frac, T2> operator+(fixed_point<T, frac, T2> x, fixed_point<T, frac, T2> y);

   template <typename T, std::size_t frac, typename T2>
   constexpr fixed_point<T, frac, T2> operator-(fixed_point<T, frac, T2> x, fixed_point<T, frac, T2> y);

   template <typename T, std::size_t frac, typename T2>
   constexpr fixed_point<T, frac, T2> operator*(fixed_point<T, frac, T2> x, fixed_point<T, frac, T2> y);

   template <typename T, std::size_t frac, typename T2>
   constexpr fixed_point<T, frac, T2> operator/(fixed_point<T, frac, T2> x, fixed_point<T, frac, T2> y);

   template <typename T, std::size_t frac, typename T2>
   constexpr bool operator==(fixed_point<T, frac, T2> x, fixed_point<T, frac, T2> y);

   template <typename T, std::size_t frac, typename T2>
   constexpr bool operator!=(fixed_point<T, frac, T2> x, fixed_point<T, frac, T2> y);

   template <typename T, std::size_t frac, typename T2>
   constexpr bool operator>(fixed_point<T, frac, T2> x, fixed_point<T, frac, T2> y);

   template <typename T, std::size_t frac, typename T2>
   constexpr bool operator<(fixed_point<T, frac, T2> x, fixed_point<T, frac, T2> y);

   template <typename T, std::size_t frac, typename T2>
   constexpr bool operator>=(fixed_point<T, frac, T2> x, fixed_point<T, frac, T2> y);

   template <typename T, std::size_t frac, typename T2>
   constexpr bool operator<=(fixed_point<T, frac, T2> x, fixed_point<T, frac, T2> y);

   ////////////////////////////////////////////////////////////////////////////
   // Implementation
   ////////////////////////////////////////////////////////////////////////////
   template <typename T, std::size_t frac, typename T2>
   template <typename U>
   constexpr fixed_point<T, frac, T2>::fixed_point(
      U val, typename std::enable_if<std::is_arithmetic<U>::value>::type*)
    : _rep(val * static_pow2<U, frac>::val)
   {}

// $$$ implement me $$$
//   template <typename T, std::size_t frac, typename T2>
//   template <typename U, std::size_t frac_, typename U2>
//   constexpr fixed_point<T, frac, T2>::fixed_point(fixed_point<U, frac, U2> rhs)
//    : _rep(rhs._rep)
//   {
//      if (frac > U::frac)
//         _rep <<= frac - U::frac;
//      else if (frac < U::frac)
//         _rep >>= U::frac - frac;
//   }

   template <typename T, std::size_t frac, typename T2>
   template <typename U>
   constexpr fixed_point<T, frac, T2>::operator U() const
   {
      return static_cast<U>(_rep) / static_pow2<U, frac>::val;
   }

   template <typename T, std::size_t frac, typename T2>
   constexpr T fixed_point<T, frac, T2>::rep() const
   {
      return _rep;
   }

   template <typename T, std::size_t frac, typename T2>
   template <typename U>
   constexpr fixed_point<T, frac, T2>& fixed_point<T, frac, T2>::operator=(U x)
   {
      _rep = x * static_pow2<U, frac>::val;
      return *this;
   }

   template <typename T, std::size_t frac, typename T2>
   constexpr fixed_point<T, frac, T2>& fixed_point<T, frac, T2>::operator+=(fixed_point<T, frac, T2> x)
   {
      _rep += x._rep;
      return *this;
   }

   template <typename T, std::size_t frac, typename T2>
   constexpr fixed_point<T, frac, T2>& fixed_point<T, frac, T2>::operator-=(fixed_point<T, frac, T2> x)
   {
      _rep -= x._rep;
      return *this;
   }

   template <typename T, std::size_t frac, typename T2>
   constexpr fixed_point<T, frac, T2>& fixed_point<T, frac, T2>::operator*=(fixed_point<T, frac, T2> x)
   {
      _rep = (T2(_rep) * x._rep) >> frac;
      return *this;
   }

   template <typename T, std::size_t frac, typename T2>
   constexpr fixed_point<T, frac, T2>& fixed_point<T, frac, T2>::operator/=(fixed_point<T, frac, T2> x)
   {
      _rep = (T2(_rep) << frac) / x._rep;
      return *this;
   }

   template <typename T, std::size_t frac, typename T2>
   template <typename U>
   constexpr typename std::enable_if<std::is_arithmetic<U>::value, fixed_point<T, frac, T2>&>::type
   fixed_point<T, frac, T2>::operator*=(U x)
   {
      _rep *= x;
      return *this;
   }

   template <typename T, std::size_t frac, typename T2>
   template <typename U>
   constexpr typename std::enable_if<std::is_arithmetic<U>::value, fixed_point<T, frac, T2>&>::type
   fixed_point<T, frac, T2>::operator/=(U x)
   {
      _rep /= x;
      return *this;
   }

   template <typename T, std::size_t frac, typename T2>
   constexpr fixed_point<T, frac, T2> fixed_point<T, frac, T2>::operator-() const
   {
      return fixed_point<T, frac, T2>{-_rep, private_{}};
   }

   template <typename T, std::size_t frac, typename T2>
   constexpr fixed_point<T, frac, T2> operator+(fixed_point<T, frac, T2> x, fixed_point<T, frac, T2> y)
   {
      return x += y;
   }

   template <typename T, std::size_t frac, typename T2>
   constexpr fixed_point<T, frac, T2> operator-(fixed_point<T, frac, T2> x, fixed_point<T, frac, T2> y)
   {
      return x -= y;
   }

   template <typename T, std::size_t frac, typename T2>
   constexpr fixed_point<T, frac, T2> operator*(fixed_point<T, frac, T2> x, fixed_point<T, frac, T2> y)
   {
      return x *= y;
   }

   template <typename T, std::size_t frac, typename T2>
   constexpr fixed_point<T, frac, T2> operator/(fixed_point<T, frac, T2> x, fixed_point<T, frac, T2> y)
   {
      return x /= y;
   }

   template <typename T, std::size_t frac, typename T2, typename U>
   constexpr typename std::enable_if<std::is_arithmetic<U>::value, fixed_point<T, frac, T2>>::type
   operator*(fixed_point<T, frac, T2> x, U y)
   {
      return x *= y;
   }

   template <typename T, std::size_t frac, typename T2, typename U>
   constexpr typename std::enable_if<std::is_arithmetic<U>::value, fixed_point<T, frac, T2>>::type
   operator*(U x, fixed_point<T, frac, T2> y)
   {
      return y *= x;
   }

   template <typename T, std::size_t frac, typename T2, typename U>
   constexpr typename std::enable_if<std::is_arithmetic<U>::value, fixed_point<T, frac, T2>>::type
   operator/(fixed_point<T, frac, T2> x, U y)
   {
      return x /= y;
   }

   template <typename T, std::size_t frac, typename T2>
   constexpr bool operator==(fixed_point<T, frac, T2> x, fixed_point<T, frac, T2> y)
   {
      return x._rep == y.rep();
   }

   template <typename T, std::size_t frac, typename T2>
   constexpr bool operator!=(fixed_point<T, frac, T2> x, fixed_point<T, frac, T2> y)
   {
      return x.rep() != y.rep();
   }

   template <typename T, std::size_t frac, typename T2>
   constexpr bool operator>(fixed_point<T, frac, T2> x, fixed_point<T, frac, T2> y)
   {
      return x.rep() > y.rep();
   }

   template <typename T, std::size_t frac, typename T2>
   constexpr bool operator<(fixed_point<T, frac, T2> x, fixed_point<T, frac, T2> y)
   {
      return x.rep() < y.rep();
   }

   template <typename T, std::size_t frac, typename T2>
   constexpr bool operator>=(fixed_point<T, frac, T2> x, fixed_point<T, frac, T2> y)
   {
      return x.rep() >= y.rep();
   }

   template <typename T, std::size_t frac, typename T2>
   constexpr bool operator<=(fixed_point<T, frac, T2> x, fixed_point<T, frac, T2> y)
   {
      return x.rep() <= y.rep();
   }
}}
