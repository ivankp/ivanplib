#ifndef IVANP_TRANSFORM_HH
#define IVANP_TRANSFORM_HH

#include <iterator>

#include "ivanp/tuple.hh"
#include "ivanp/detect.hh"

namespace ivanp {

template <typename T>
auto reserve(size_t n) {
  std::vector<T> _v;
  _v.reserve(n);
  return _v;
}

template <typename C>
using detect_size = decltype(std::declval<C>().size());

template <typename C>
constexpr auto size(const C& c) -> decltype(c.size()) { return c.size(); }
template <typename C>
inline std::enable_if_t<
  !ivanp::is_detected<detect_size,C>::value, size_t >
size(const C& c) { return std::distance( std::begin(c), std::end(c) ); }
template <class T, size_t N>
constexpr size_t size(const T (&array)[N]) noexcept { return N; }

namespace detail {

template <typename... T>
class zip_iter : public std::tuple<T...> {
public:
  using std::tuple<T...>::tuple;
  inline std::tuple<T...>& base() noexcept { return *this; }
  inline const std::tuple<T...>& base() const noexcept { return *this; }
  inline zip_iter& operator++() {
    base() | [](auto& x){ ++x; };
    return *this;
  }
  inline std::tuple<decltype(*std::declval<T>())...> operator*() {
    // without specifying return type
    // lambdas return by value
    return base() | [](auto& x) -> decltype(*x) { return *x; };
  }
  inline std::tuple<decltype(*std::declval<T>())...> operator*() const {
    return base() | [](auto& x) -> decltype(*x) { return *x; };
  }
};

template <typename... T>
class zipper : std::tuple<T...> {
public:
  using begin_iter =
    zip_iter<std::decay_t<decltype(std::begin(std::declval<T>()))>...>;
  using end_iter =
    zip_iter<std::decay_t<decltype(std::end(std::declval<T>()))>...>;
private:
  template <size_t... I>
  inline begin_iter begin(std::index_sequence<I...>) {
    return { std::begin(std::get<I>(*this))... };
  }
  template <size_t... I>
  inline end_iter end(std::index_sequence<I...>) {
    return { std::end(std::get<I>(*this))... };
  }
public:
  using std::tuple<T...>::tuple;
  inline begin_iter begin() {
    return begin(std::index_sequence_for<T...>{});
  }
  inline end_iter end() {
    return end(std::index_sequence_for<T...>{});
  }
  inline size_t size() const { return std::get<0>(*this).size(); }
};

namespace comprehend_container {

template <typename C, typename F>
using return_type = std::decay_t<decltype(
  std::declval<F>()( *std::begin(std::declval<C>()) ) )>;

}

} // end detail

template <typename... C>
inline detail::zipper<C&&...> zip(C&&... c) {
  return { std::forward<C>(c)... };
}

} // end namespace ivanp

template <typename C, typename F>
inline auto operator|(C&& c, F&& f)
-> std::enable_if_t<
  !ivanp::is_detected<ivanp::tuple_indices,C>::value &&
  std::is_void<ivanp::detail::comprehend_container::return_type<C&,F&&>>::value,
  C&& >
{
  for (auto&& x : std::forward<C>(c)) std::forward<F>(f)(x);
  return std::forward<C>(c);
}

template <typename C, typename F>
inline auto operator|(C&& c, F&& f)
-> std::enable_if_t<
  !ivanp::is_detected<ivanp::tuple_indices,C>::value &&
  !std::is_void<ivanp::detail::comprehend_container::return_type<C&,F&&>>::value,
  std::vector<ivanp::detail::comprehend_container::return_type<C&,F&&>> >
{
  std::vector<ivanp::detail::comprehend_container::return_type<C&,F&&>> vec;
  vec.reserve(ivanp::size(c));

  for (auto&& x : std::forward<C>(c))
    vec.emplace_back( std::forward<F>(f)(x) );

  return vec;
}

// template <typename T, typename F>
// inline decltype(auto) operator|(std::initializer_list<T> c, F&& f) {
//   return c | std::forward<F>(f);
// }

template <typename T>
inline std::vector<T>& operator+=(std::vector<T>& v, const std::vector<T>& u) {
  v.reserve(v.size()+u.size());
  v.insert(v.end(), u.begin(), u.end());
  return v;
}
template <typename T>
inline std::vector<T>& operator+=(std::vector<T>& v, std::vector<T>&& u) {
  v.reserve(v.size()+u.size());
  v.insert(v.end(),
    std::make_move_iterator(u.begin()),
    std::make_move_iterator(u.end())
  );
  return v;
}

#endif
