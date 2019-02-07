#ifndef IVANP_STRING_HH
#define IVANP_STRING_HH

#include <string>
#include <sstream>
#include <cstring>
#include <array>
#include "ivanp/unfold.hh"

namespace ivanp {

template <typename... T>
inline std::string cat(T&&... x) {
  std::stringstream ss;
  UNFOLD(ss << std::forward<T>(x))
  return ss.str();
}
inline std::string cat() { return { }; }

inline std::string cat(std::string x) { return x; }
inline std::string cat(const char* x) { return x; }

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
  const unsigned len = str.size();
  if (len<N-1) return false;
  return starts_with(str.c_str()+(len-N+1),suffix);
}

struct less_sz {
  bool operator()(const char* a, const char* b) const noexcept {
    return strcmp(a,b) < 0;
  }
};

inline int strcmpi(const char* s1, const char* s2) {
  for (;;++s1,++s2) {
    if (toupper(*s1)!=toupper(*s2)) return (*s1-*s2);
    if (*s1=='\0') return 0;
  }
}


template <size_t N, typename Str>
std::array<Str,N+1> lsplit(const Str& str, typename Str::value_type delim) {
  std::array<Str,N+1> arr;
  auto l = 0;
  size_t i = 0;
  for (; i!=N; ++i) {
    const auto r = str.find(delim,l);
    if (!(r+1)) break;
    arr[i] = str.substr(l,r-l);
    l = r+1;
  }
  arr[i] = str.substr(l);
  return arr;
}

template <size_t N, typename Str>
std::array<Str,N+1> rsplit(const Str& str, typename Str::value_type delim) {
  std::array<Str,N+1> arr;
  auto r = Str::npos-1;
  size_t i = N;
  for (; i!=0; --i) {
    const auto l = str.rfind(delim,r);
    if (!(l+1)) break;
    arr[i] = str.substr(l+1,r-l);
    r = l-1;
  }
  arr[i] = str.substr(0,r+1);
  return arr;
}

}

#endif
