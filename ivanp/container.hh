#ifndef IVANP_TRANSFORM_HH
#define IVANP_TRANSFORM_HH

#include "ivanp/tuple.hh"
// #include "ivanp/utility/pack.hh"
#include "ivanp/utility/detect.hh"
// #include "ivanp/iter/iter.hh"

/*
namespace ivanp {

namespace detail {

template <typename F, typename... Cont>
using zip_type = std::vector<std::tuple<rm_rref_t<
  decltype(std::declval<F>()(ivanp::begin(std::declval<Cont>())))>... >>;

struct deref {
  template <typename T>
  inline auto operator()(T&& x) const
  noexcept(noexcept(*std::forward<T>(x)))
  -> decltype(*std::forward<T>(x))
  { return *std::forward<T>(x); }
};

}

template <typename F, typename... Cont>
inline auto zip(F&& f, Cont&&... cont) ->
  std::enable_if_t<sizeof...(Cont), detail::zip_type<F,Cont&&...> >
{
  auto it = std::make_tuple(ivanp::begin(std::forward<Cont>(cont))...);
  const auto end0 = ivanp::end(get_pack_head(std::forward<Cont>(cont)...));

  // ivanp::prt_type<std::tuple<Cont&&...>>();
  // ivanp::prt_type<decltype(std::get<0>(it))>();
  ivanp::prt_type<decltype(*std::get<0>(it))>();
  ivanp::prt_type<decltype(it | f)>();

  detail::zip_type<F,Cont&&...> vec;
  vec.reserve( std::distance( std::get<0>(it), end0 ) );

  while (std::get<0>(it) != end0) {
    it | [](auto& x){
      // ivanp::prt_type<decltype(x)>();
      std::cout << (*x) << ' ';
    }, std::cout << std::endl;
    vec.emplace_back(it | f);
    it | [](auto& x){ std::cout << (*x) << ' '; }, std::cout << std::endl;
    it | [](auto& x){ ++x; };
  }

  return vec;
}
template <typename... Cont>
inline auto zip(Cont&&... cont) -> detail::zip_type<detail::deref,Cont&&...>
{
  return zip( detail::deref{}, std::forward<Cont>(cont)... );
}
*/

namespace ivanp {

namespace detail {

struct deref {
  template <typename T>
  inline auto operator()(T&& x) const
  noexcept(noexcept(*std::forward<T>(x)))
  -> decltype(*std::forward<T>(x))
  { return *std::forward<T>(x); }
};

template <typename... C>
using zip_type = std::vector<std::tuple<
  std::decay_t<decltype(*std::begin(std::declval<C>()))>...>>;

// template <typename F, typename... C>
// using zip_apply_type = std::vector<std::tuple<
//   std::decay_t<decltype(*std::begin(std::declval<C>()))>...>>;

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
};

}

template <typename... C>
inline detail::zipper<C&&...> zip(C&&... c) {
  return { std::forward<C>(c)... };
}

} // end namespace ivanp

template <typename C, typename F>
inline auto operator|(C&& c, F&& f)
-> std::enable_if_t<
  !ivanp::is_detected<ivanp::tuple_indices,C>::value
>
{
  for (auto&& x : c) std::forward<F>(f)(x);
}

#endif
