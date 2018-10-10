#ifndef IVANP_SCOPE_HH
#define IVANP_SCOPE_HH

#define CONCATENATE_IMPL(s1, s2) s1##s2
#define CONCATENATE(s1, s2) CONCATENATE_IMPL(s1, s2)
#ifdef ANONYMOUS_VARIABLE_COUNTER
#define ANONYMOUS_VARIABLE(str) CONCATENATE(str, ANONYMOUS_VARIABLE_COUNTER)
#else
#define ANONYMOUS_VARIABLE(str) CONCATENATE(str, __LINE__)
#endif

#define SCOPE_EXIT \
  auto ANONYMOUS_VARIABLE(SCOPE_EXIT_) \
  = ::ivanp::detail::scope_guard_on_exit() + [&]()

namespace ivanp {

template <typename F>
class scope_guard {
  F f;
  scope_guard() = delete;
  scope_guard(const scope_guard&) = delete;
  scope_guard& operator=(const scope_guard&) = delete;
public:
  scope_guard(F&& f): f(std::move(f)) { }
  ~scope_guard() { f(); }
  scope_guard(scope_guard&& r): f(std::move(r.f)) { }
};

template <typename F>
inline scope_guard<F> make_scope_guard(F&& f) {
  return { std::forward<F>(f) };
}

namespace detail {
  enum class scope_guard_on_exit { };
  template <typename F>
  scope_guard<F> operator+(scope_guard_on_exit, F&& f) {
    return scope_guard<F>(std::forward<F>(f));
  }
}

}

#endif
