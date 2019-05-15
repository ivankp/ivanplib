#ifndef IVANP_TUPLE_FOR_EACH_HH
#define IVANP_TUPLE_FOR_EACH_HH

#include <tuple>
#include <type_traits>

namespace ivanp {
namespace detail {

template <size_t I, typename T, typename F, typename = void>
struct element_callback_ret_trait;
template <size_t I, typename T, typename F>
struct element_callback_ret_trait<I,T,F,std::enable_if_t<
  !(I < std::tuple_size<std::decay_t<T>>::value)
>> {
  constexpr static bool is_valid = false;
  constexpr static bool is_valid_void = false;
  constexpr static bool is_valid_not_void = false;
};
template <size_t I, typename T, typename F>
struct element_callback_ret_trait<I,T,F,std::enable_if_t<
  (I < std::tuple_size<std::decay_t<T>>::value)
>> {
  constexpr static bool is_valid = true;
  constexpr static bool is_valid_void = std::is_void< decltype(
    std::declval<F>()(std::get<I>(std::declval<T>()))
  ) >::value;
  constexpr static bool is_valid_not_void = !is_valid_void;
};

template <size_t I, typename T, typename F>
auto for_each_impl(T&&, F&)
-> std::enable_if_t< !element_callback_ret_trait<I,T&&,F>::is_valid, bool>
{
  return false;
}
template <size_t I, typename T, typename F>
auto for_each_impl(T&& xs, F& f)
-> std::enable_if_t<
  element_callback_ret_trait<I,T&&,F>::is_valid_not_void,
  bool>
{
  if (!f(std::get<I>(xs))) return for_each_impl<I+1>(xs,f);
  return true;
}
template <size_t I, typename T, typename F>
auto for_each_impl(T&& xs, F& f)
-> std::enable_if_t<
  element_callback_ret_trait<I,T&&,F>::is_valid_void,
  bool>
{
  f(std::get<I>(xs));
  return for_each_impl<I+1>(xs,f);
}

}

template <typename T, typename F>
bool for_each(T&& xs, F&& f) {
  return detail::for_each_impl<0>(xs,f);
}

}

#endif
