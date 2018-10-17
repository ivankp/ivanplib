#ifndef IVANP_SPLICE_HH
#define IVANP_SPLICE_HH

#include "ivanp/tuple.hh"
#include "ivanp/container.hh"

namespace ivanp {

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

}

template <typename... C>
inline detail::zipper<C&&...> zip(C&&... c) {
  return { std::forward<C>(c)... };
}

}

#endif
