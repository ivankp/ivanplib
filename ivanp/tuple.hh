#ifndef IVANP_TUPLE_HH
#define IVANP_TUPLE_HH

#include <utility>
#include <tuple>
#include <functional>

#include "ivanp/utility/meta.hh"
#include "ivanp/math/boolean.hh"

#ifndef __cpp_fold_expressions
#include "ivanp/utility/expand.hh"
#endif

namespace ivanp {

template <typename T>
using tuple_indices
  = std::make_index_sequence<std::tuple_size<std::decay_t<T>>::value>;

namespace detail {

namespace apply {

template <typename F, typename T, size_t... I>
constexpr decltype(auto) impl(F&& f, T&& t, std::index_sequence<I...>) {
#ifdef __cpp_lib_invoke
  return std::invoke(
    std::forward<F>(f), std::get<I>(std::forward<T>(t))...);
#else
  return std::forward<F>(f)(std::get<I>(std::forward<T>(t))...);
#endif
}

} // end apply

namespace comprehend_tuple {

template <typename T, typename F, size_t I>
using return_type
  = decltype( std::declval<F>()( std::get<I>(std::declval<T>()) ) );

template <typename T, typename F, size_t... I>
constexpr auto impl(T&& t, F&& f, std::index_sequence<I...>)
-> std::enable_if_t<
  !disjunction< std::is_void< return_type<T&&,F&&,I> >... >::value,
  std::tuple< return_type<T&&,F&&,I>... >
> {
#ifdef __cpp_lib_invoke
  return { std::invoke(
      std::forward<F>(f), std::get<I>(std::forward<T>(t))
    )... };
#else
  return { std::forward<F>(f)(std::get<I>(std::forward<T>(t)))... };
#endif
}
template <typename T, typename F, size_t... I>
constexpr auto impl(T&& t, F&& f, std::index_sequence<I...>)
-> std::enable_if_t<
  disjunction< std::is_void< return_type<T&&,F&&,I> >... >::value,
  T&&
> {
#ifdef __cpp_lib_invoke
  (std::invoke(
    std::forward<F>(f), std::get<I>(std::forward<T>(t)) ),...);
#else
  EXPAND( std::forward<F>(f)(std::get<I>(std::forward<T>(t))) )
#endif
  return std::forward<T>(t);
}

} // end comprehend_tuple

template <template<typename> typename Pred, typename Tuple>
class indices_of_impl {
  static constexpr size_t size = std::tuple_size<Tuple>::value;
  template <size_t I, size_t... II> struct impl {
    using type = std::conditional_t<
      Pred<std::tuple_element_t<I,Tuple>>::value,
      typename impl<I+1, II..., I>::type,
      typename impl<I+1, II...>::type >;
  };
  template <size_t... II> struct impl<size,II...> {
    using type = std::index_sequence<II...>;
  };
public:
  using type = typename impl<0>::type;
};

} // end detail

template <typename F, typename T>
constexpr decltype(auto) apply(F&& f, T&& t) {
  return ivanp::detail::apply::impl(
    std::forward<F>(f), std::forward<T>(t), tuple_indices<T>{}
  );
}

// find all indices of types in tuple, for which Pred::value is true
template <template<typename> typename Pred, typename Tuple>
using indices_of = typename detail::indices_of_impl<Pred,Tuple>::type;

} // end namespace ivanp

// call function, using tuple elements as its arguments
template <typename T, typename F>
inline decltype(auto) operator%(T&& t, F&& f) {
  return ivanp::detail::apply::impl(
    std::forward<F>(f), std::forward<T>(t), ivanp::tuple_indices<T>{}
  );
}

// call function for each tuple element
template <typename T, typename F>
inline auto operator|(T&& t, F&& f)
-> decltype(
    ivanp::detail::comprehend_tuple::impl(
      std::forward<T>(t), std::forward<F>(f), ivanp::tuple_indices<T>{} ) )
{
  return ivanp::detail::comprehend_tuple::impl(
    std::forward<T>(t), std::forward<F>(f), ivanp::tuple_indices<T>{}
  );
}

namespace ivanp {

#ifdef _GLIBCXX_ARRAY
template <typename... Args>
auto make_array(Args&&... args)
-> std::array<std::common_type_t<std::decay_t<Args>...>,sizeof...(Args)>
{ return { std::forward<Args>(args)... }; }
#endif

} // end namespace ivanp

#endif
