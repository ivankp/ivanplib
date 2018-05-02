#ifndef IVANP_UTILITY_HH
#define IVANP_UTILITY_HH

#define IVANP_TRIPPLE_FCN(CODE) \
  noexcept(noexcept(CODE)) -> decltype(CODE) { return CODE; }

#include <string>

namespace ivanp {

template <typename T>
struct named_ptr {
  using type = T;
  type *p;
  std::string name;

  named_ptr(): p(nullptr), name() { }
  named_ptr(const named_ptr& n) = default;
  named_ptr(named_ptr&& n) = default;
  template <typename Name>
  named_ptr(T* ptr, Name&& name): p(ptr), name(std::forward<Name>(name)) { }

  inline type& operator*() const noexcept { return *p; }
  inline type* operator->() const noexcept { return p; }
};

struct deref {
  template <typename T>
  inline auto operator()(T&& x) const
  IVANP_TRIPPLE_FCN(*std::forward<T>(x))
};

}

#endif
