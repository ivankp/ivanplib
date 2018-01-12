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
template <>
constexpr void get_pack_head() noexcept { };

}

#endif
