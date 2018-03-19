#ifndef IVANP_STRING_HH
#define IVANP_STRING_HH

#include <string>
#include <sstream>
#include <utility>
#include <cstring>
#include "ivanp/expand.hh"

namespace ivanp {

template <typename... T>
inline std::string cat(T&&... x) {
  std::stringstream ss;
  EXPAND(ss << std::forward<T>(x))
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

template <typename Str, unsigned N>
inline bool starts_with(const Str& str, const char(&prefix)[N]) {
  for (unsigned i=0; i<N-1; ++i)
    if (str[i]!=prefix[i]) return false;
  return true;
}

template <unsigned N>
inline bool ends_with(const char* str, const char(&suffix)[N]) {
  const unsigned len = strlen(str);
  if (len<N-1) return false;
  return starts_with(str+(len-N+1),suffix);
}
template <unsigned N>
inline bool ends_with(const std::string& str, const char(&suffix)[N]) {
  return ends_with<N>(str.c_str(),suffix);
}

struct less_sz {
  bool operator()(const char* a, const char* b) const noexcept {
    return strcmp(a,b) < 0;
  }
};

}

#endif
