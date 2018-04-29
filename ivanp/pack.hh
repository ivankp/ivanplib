#ifndef IVANP_PACK_HH
#define IVANP_PACK_HH

#include "ivanp/maybe.hh"

namespace ivanp {

template <typename...> struct pack_head: maybe<nothing> { };
template <typename T, typename... Other>
struct pack_head<T,Other...>: maybe<just<T>> { };
template <typename... T> using pack_head_t = typename pack_head<T...>::type;

template <typename T, typename... TT>
constexpr T&& get_pack_head(T&& x, TT...) noexcept {
  return static_cast<T&&>(x);
}
constexpr void get_pack_head() noexcept { };

// Find first =======================================================
// first pack element matching predicate
template <template<typename> typename Pred, typename... Args>
struct find_first: maybe<nothing> { };

template <template<typename> typename Pred, typename Arg1, typename... Args>
class find_first<Pred,Arg1,Args...> {
  template <typename, typename = void>
  struct impl: find_first<Pred,Args...> { };
  template <typename Arg>
  struct impl<Arg,std::enable_if_t<Pred<Arg>::value>>: maybe<just<Arg>> { };
public:
  using type = typename impl<Arg1>::type;
};

template <template<typename> typename Pred, typename... Args>
using find_first_t = typename find_first<Pred,Args...>::type;

// Find first index =================================================
namespace detail {
template <template<typename> typename Pred, size_t I, typename... Args>
struct find_first_index_impl: maybe<nothing> { };

template <template<typename> typename Pred, size_t I,
          typename Arg1, typename... Args>
class find_first_index_impl<Pred,I,Arg1,Args...> {
  template <typename, typename = void>
  struct impl: find_first_index_impl<Pred,I+1,Args...> { };
  template <typename Arg>
  struct impl<Arg,std::enable_if_t<Pred<Arg>::value>>
  : maybe<just<std::integral_constant<size_t,I>>> { };
public:
  using type = typename impl<Arg1>::type;
};
}

template <template<typename> typename Pred, typename... Args>
using find_first_index
  = typename detail::find_first_index_impl<Pred,0,Args...>::type;

}

#endif
