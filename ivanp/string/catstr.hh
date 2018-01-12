#ifndef IVANP_CATSTR_HH
#define IVANP_CATSTR_HH

#include <string>
#include <sstream>
#include <utility>

namespace ivanp {

namespace detail {

template <typename S>
inline void cat_impl(S&) { }

template <typename S, typename T>
inline void cat_impl(S& s, const T& t) { s << t; }

template <typename S, typename T, typename... TT>
inline void cat_impl(S& s, const T& t, const TT&... tt) {
  cat_impl(s,t);
  cat_impl(s,tt...);
}

}

template <typename... TT>
inline std::string cat(const TT&... tt) {
  std::stringstream ss;
  detail::cat_impl(ss,tt...);
  return ss.str();
}
inline std::string cat() { return { }; }

template <typename L>
inline std::string lcat(const L& list, std::string sep=" ") {
  std::stringstream ss;
  auto it = std::begin(list);
  ss << *it;
  ++it;
  for (auto end = std::end(list); it!=end; ++it)
    ss << sep << *it;
  return ss.str();
}

}

#endif
