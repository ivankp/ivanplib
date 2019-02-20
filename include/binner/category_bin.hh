#ifndef CATEGORY_BIN_HH
#define CATEGORY_BIN_HH

#include "ivanp/enum_traits.hh"
#include "ivanp/debug/at.hh"
#include "ivanp/error.hh"

namespace ivanp {

template <typename Bin, typename E, typename... Es>
struct category_bin {
  using id_type = unsigned;
  static id_type _id;
  using category = E;

  template <typename...>
  struct next_bin { using type = Bin; };
  template <typename _E, typename... _Es>
  struct next_bin<_E,_Es...> { using type = category_bin<Bin,_E,_Es...>; };

  using bin_type = typename next_bin<Es...>::type;

  std::array< bin_type, enum_traits<E>::size() > bins;

  template <typename... T>
  category_bin& operator()(T&&... args) noexcept {
    std::get<0>(bins)(std::forward<T>(args)...);
    if (_id) bins AT(_id)(std::forward<T>(args)...);
    return *this;
  }
  category_bin& operator+=(const category_bin& b) noexcept {
    for (auto i=bins.size(); i; ) --i, bins AT(i) += b.bins AT(i);
    return *this;
  }

  template <typename _E>
  static std::enable_if_t<std::is_same<_E,E>::value,id_type>& id() {
    return _id;
  }
  template <typename _E>
  static std::enable_if_t<!std::is_same<_E,E>::value,id_type>& id() {
    static_assert( sizeof...(Es),
      "given enum type does not correspond to a category");
    return bin_type::template id<_E>();
  }

  template <typename _E>
  static std::enable_if_t<std::is_same<_E,E>::value,id_type> id(id_type i) {
    if (!(i < enum_traits<E>::size())) throw error(
      "\"",enum_traits<E>::name(),"\" category (size=",
      enum_traits<E>::size(),") index out of range (i=",i,")");
    return _id = i;
  }
  template <typename _E>
  static std::enable_if_t<!std::is_same<_E,E>::value,id_type> id(id_type i) {
    static_assert( sizeof...(Es),
      "given enum type does not correspond to a category");
    return bin_type::template id<_E>(i);
  }

  template <typename T = bin_type>
  std::enable_if_t<!std::is_same<T,Bin>::value,const Bin&> operator*() const {
    return *bins AT(_id);
  }
  template <typename T = bin_type>
  std::enable_if_t< std::is_same<T,Bin>::value,const Bin&> operator*() const {
    return bins AT(_id);
  }
};

template <typename Bin, typename E, typename... Es>
typename category_bin<Bin,E,Es...>::id_type category_bin<Bin,E,Es...>::_id = 0;

}

#endif
