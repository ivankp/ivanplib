#ifndef IVANP_MATH_BOOLEAN_HH
#define IVANP_MATH_BOOLEAN_HH

#include <type_traits>

namespace ivanp {

// bool const
template <bool B> using bool_constant = std::integral_constant<bool, B>;

// AND
template <class...> struct conjunction: std::true_type { };
template <class B1> struct conjunction<B1>: B1 { };
template <class B1, class... Bs>
struct conjunction<B1, Bs...>
  : std::conditional_t<bool(B1::value), conjunction<Bs...>, B1> { };
// OR
template <class...> struct disjunction: std::false_type { };
template <class B1> struct disjunction<B1>: B1 { };
template <class B1, class... Bs>
struct disjunction<B1, Bs...>
  : std::conditional_t<bool(B1::value), B1, disjunction<Bs...>> { };
// NOT
template <class B> struct negation: bool_constant<!bool(B::value)> { };

// Only last
template <typename...> struct only_last: std::false_type { };
template <typename B1> struct only_last<B1>: bool_constant<B1::value> { };
template <typename B1, typename... Bs>
struct only_last<B1, Bs...>
  : bool_constant<!B1::value && only_last<Bs...>::value> { };

}

#endif
