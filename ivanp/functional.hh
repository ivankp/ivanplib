#ifndef IVANP_FUNCTIONAL_HH
#define IVANP_FUNCTIONAL_HH

// https://stackoverflow.com/a/40873657/2640636
// http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2016/p0200r0.html

#include <functional>
#include <type_traits>
#include <utility>

namespace ivanp {

template <typename F> class y_combinator_result {
  F f;
public:
  template <typename T>
  y_combinator_result(T&& f): f(std::forward<T>(f)) { }

  template <typename... Args>
  decltype(auto) operator()(Args&&... args) const {
    return f(std::ref(*this), std::forward<Args>(args)...);
  }
};

template <typename F>
y_combinator_result<std::decay_t<F>> y_combinator(F&& f) {
  return { std::forward<F>(f) };
}

}

#endif
