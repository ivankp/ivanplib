#ifndef IVANP_TRAITS_HH
#define IVANP_TRAITS_HH

namespace ivanp {

template <typename T, typename... Args> // T(Args...)
class is_callable {
  template <typename, typename = void> struct impl: std::false_type { };
  template <typename U> struct impl<U,
    void_t<decltype( std::declval<U&>()(std::declval<Args>()...) )>
  > : std::true_type { };
public:
  static constexpr bool value = impl<T>::value;
};

template <typename T, typename... Args> // T(Args...)
class is_constructible {
  template <typename, typename = void> struct impl: std::false_type { };
  template <typename U> struct impl<U,
    void_t<decltype( U(std::declval<Args>()...) )>
  > : std::true_type { };
public:
  static constexpr bool value = impl<T>::value;
};

template <typename T, typename Arg> // T = Arg
class is_assignable {
  template <typename, typename = void> struct impl: std::false_type { };
  template <typename U> struct impl<U,
    void_t<decltype( std::declval<U&>() = std::declval<Arg>() )>
  > : std::true_type { };
public:
  static constexpr bool value = impl<T>::value;
};

}

#endif
