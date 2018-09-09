#ifndef CATEGORY_BIN_HH
#define CATEGORY_BIN_HH

#include "ivanp/enum_traits.hh"

namespace ivanp {

template <typename Bin, typename E, typename... Es>
struct category_bin {
  static unsigned _id;
  using category = E;

  template <typename...>
  struct next_bin { using type = Bin; };
  template <typename _E, typename... _Es>
  struct next_bin<_E,_Es...> { using type = category_bin<Bin,_E,_Es...>; };

  using bin_type = typename next_bin<Es...>::type;

  std::array< bin_type, enum_traits<E>::size > bins;

  template <typename... T>
  inline category_bin& operator()(T&&... args) noexcept {
    std::get<0>(bins)(std::forward<T>(args)...);
    if (_id) bins[_id](std::forward<T>(args)...);
    return *this;
  }
  inline category_bin& operator+=(const category_bin& b) noexcept {
    for (auto i=bins.size(); i; ) --i, bins[i] += b.bins[i];
    return *this;
  }

  template <typename _E>
  inline static std::enable_if_t<std::is_same<_E,E>::value,unsigned>& id() {
    return _id;
  }
  template <typename _E>
  inline static std::enable_if_t<!std::is_same<_E,E>::value,unsigned>& id() {
    static_assert( sizeof...(Es),
      "given enum type does not correspond to a category");
    return bin_type::template id<_E>();
  }
};

template <typename Bin, typename E, typename... Es>
unsigned category_bin<Bin,E,Es...>::_id = 0;

}

#endif
