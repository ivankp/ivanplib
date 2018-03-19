#ifndef IVANP_MAYBE_HH
#define IVANP_MAYBE_HH

#include "ivanp/meta.hh"
#include "ivanp/boolean.hh"

namespace ivanp {

template <typename T> struct is_nothing: std::false_type { };
template <> struct is_nothing<nothing>: std::true_type { };

template <typename T> struct just { using type = T; };

template <typename T> struct just_value {
  using type = T;
  constexpr const T& operator*() const noexcept { return value; }
  constexpr T& operator*() noexcept { return value; }
private:
  T value;
};
template <> struct just_value<void> {
  using type = void;
  constexpr void operator*() const noexcept { }
  constexpr void operator*() noexcept { }
};

template <typename T> struct is_just: std::false_type { };
template <typename T> struct is_just<just<T>>: std::true_type { };
template <typename T> struct is_just<just_value<T>>: std::true_type { };

template <typename T> struct maybe : T {
  static_assert(is_just<T>::value || is_nothing<T>::value);
};

// maybe enable -----------------------------------------------------
template <typename M, typename T = void>
using enable_if_just_t = std::enable_if_t<is_just<M>::value,T>;
template <typename M, typename T = void>
using enable_if_nothing_t = std::enable_if_t<is_nothing<M>::value,T>;

// maybe is ---------------------------------------------------------
template <template<typename> typename Pred, typename M, bool N = false>
struct maybe_is;
template <template<typename> typename Pred, bool N>
struct maybe_is<Pred,nothing,N>: bool_constant<N> { };
template <template<typename> typename Pred, typename T, bool N>
struct maybe_is<Pred,just<T>,N>: bool_constant<Pred<T>::value> { };

#ifdef __cpp_variable_templates
template <template<typename> typename Pred, typename M, bool N = false>
constexpr bool maybe_is_v = maybe_is<Pred,M,N>::value;
#endif

}

#endif
