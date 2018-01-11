#ifndef IVANP_TRANSFORM_HH
#define IVANP_TRANSFORM_HH

#include "ivanp/utility/tuple.hh"
#include "ivanp/utility/pack.hh"
#include "ivanp/utility/detect.hh"
#include "ivanp/iter/iter.hh"

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

} // end namespace ivanp

#endif
