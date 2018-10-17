#ifndef IVANP_FUNCTIONAL_HH
#define IVANP_FUNCTIONAL_HH

// https://stackoverflow.com/a/40873657/2640636
// http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2016/p0200r0.html

#include <functional>
#include <type_traits>
#include <utility>

namespace ivanp {

template <typename F> class _y_combinator {
  F f;
public:
  template <typename T>
  _y_combinator(T&& f): f(std::forward<T>(f)) { }

  template <typename... Args>
  decltype(auto) operator()(Args&&... args) const {
    return f(std::ref(*this), std::forward<Args>(args)...);
  }
};

template <typename F>
inline _y_combinator<std::decay_t<F>> y_combinator(F&& f) {
  return { std::forward<F>(f) };
}

#ifdef __cpp_deduction_guides
template <typename... F> struct overloaded: F... { using F::operator()...; };
template <typename... F> overloaded(F...) -> overloaded<F...>;
#else
template <typename F, typename... Fs>
struct _overloaded: F, _overloaded<Fs...> {
  template <typename T, typename... Ts>
  _overloaded(T&& f, Ts&&... fs)
  : F(std::forward<T>(f)), _overloaded<Fs...>(std::forward<Ts>(fs)...) { }
  using F::operator();
  using _overloaded<Fs...>::operator();
};
template <typename F> struct _overloaded<F>: F {
  template <typename T> _overloaded(T&& f): F(std::forward<T>(f)) { }
  using F::operator();
};
template <typename... F>
inline _overloaded<F...> overloaded(F&&... f) {
  return { std::forward<F>(f)... };
}
#endif

}

#endif
