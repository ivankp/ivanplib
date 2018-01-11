#ifndef IVANP_PACK_HH
#define IVANP_PACK_HH

namespace ivanp {

template <typename T, typename...>
struct pack_head { using type = T; };
template <typename... T>
using pack_head_t = typename pack_head<T...>::type;


template <typename T, typename... TT>
constexpr T&& get_pack_head(T&& x, TT...) noexcept {
  return static_cast<T&&>(x);
}

}

#endif
