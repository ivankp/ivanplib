#ifndef IVANP_TRAITS_HH
#define IVANP_TRAITS_HH

#include <type_traits>

#define IVANP_MAKE_OP_TRAIT_1(NAME,EXPR) \
template <typename U> \
class NAME { \
  template <typename, typename = void> \
  struct impl : std::false_type { }; \
  template <typename T> \
  struct impl<T, decltype((void)(EXPR))> : std::true_type { }; \
public: \
  using type = impl<U>; \
  static constexpr bool value = type::value; \
};

#define IVANP_MAKE_OP_TRAIT_2(NAME,EXPR) \
template <typename U1, typename U2> \
class NAME { \
  template <typename, typename, typename = void> \
  struct impl : std::false_type { }; \
  template <typename T1, typename T2> \
  struct impl<T1, T2, decltype((void)(EXPR))> : std::true_type { }; \
public: \
  using type = impl<U1,U2>; \
  static constexpr bool value = type::value; \
};

#define IVANP_MAKE_OP_TRAIT_ANY(NAME,EXPR) \
template <typename U, typename... Args> \
class NAME { \
  template <typename, typename = void> \
  struct impl : std::false_type { }; \
  template <typename T> \
  struct impl<T, decltype((void)(EXPR))> : std::true_type { }; \
public: \
  using type = impl<U>; \
  static constexpr bool value = type::value; \
};

namespace ivanp {

// ==================================================================

IVANP_MAKE_OP_TRAIT_ANY( is_callable, std::declval<T&>()(std::declval<Args>()...) )
IVANP_MAKE_OP_TRAIT_ANY( is_constructible, T(std::declval<Args>()...) )

IVANP_MAKE_OP_TRAIT_2( is_assignable, std::declval<T1&>()=std::declval<T2>() )
IVANP_MAKE_OP_TRAIT_1( is_indexable, std::declval<T&>()[0] )

IVANP_MAKE_OP_TRAIT_1( has_pre_increment,  ++std::declval<T&>() )
IVANP_MAKE_OP_TRAIT_1( has_post_increment, std::declval<T&>()++ )
IVANP_MAKE_OP_TRAIT_1( has_pre_decrement,  --std::declval<T&>() )
IVANP_MAKE_OP_TRAIT_1( has_post_decrement, std::declval<T&>()-- )

IVANP_MAKE_OP_TRAIT_2( has_plus_eq,  std::declval<T1&>()+=std::declval<T2>() )
IVANP_MAKE_OP_TRAIT_2( has_minus_eq, std::declval<T1&>()-=std::declval<T2>() )

IVANP_MAKE_OP_TRAIT_1( has_deref, *std::declval<T&>() )

// ==================================================================

} // end ivanp

#endif
