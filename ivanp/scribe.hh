#ifndef IVANP_SCRIBE_HH
#define IVANP_SCRIBE_HH

#include <type_traits>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <array>
#include <tuple>
#include <unordered_map>
#include <cstdint>
#include "ivanp/meta.hh"

#if __has_include
#  if __has_include(<string_view>)
#    define IVANP_SCRIBE_STRING_VIEW_STD
#  elif __has_include(<boost/string_view.hpp>)
#    define IVANP_SCRIBE_STRING_VIEW_BOOST
#  else
#    define IVANP_SCRIBE_STRING_VIEW_NONE
#  endif
#else
#  define IVANP_SCRIBE_STRING_VIEW_NONE
#endif

#ifdef IVANP_SCRIBE_STRING_VIEW_STD
#include <string_view>
namespace ivanp { namespace scribe { using string_view = std::string_view; }}
#elif defined(IVANP_SCRIBE_STRING_VIEW_BOOST)
#include <boost/string_view.hpp>
namespace ivanp { namespace scribe { using string_view = boost::string_view; }}
#else
namespace ivanp { namespace scribe { using string_view = std::string; }}
#endif

namespace ivanp {
namespace scribe {

using type_map = std::unordered_map<std::string,std::string>;
using size_type = uint32_t;

template <typename T, typename SFINAE=void>
struct trait;

template <typename T>
struct trait<T,std::enable_if_t<std::is_arithmetic<T>::value>> {
  static void write_value(std::ostream& o, const T& x) {
    o.write(reinterpret_cast<const char*>(&x), sizeof(x));
  }
  static void write_type_def(type_map& m) { }
  static std::string type_name() {
    return (std::is_floating_point<T>::value ? "f" :
        (std::is_signed<T>::value ? "i" : "u")
      ) + std::to_string(sizeof(T));
  }
};

template <typename T>
struct trait<T, void_t<
  decltype(begin(std::declval<T&>())),
  decltype(end  (std::declval<T&>()))
>> {
  using value_type = std::decay_t<decltype(*std::declval<T>().begin())>;
  using value_trate = trait<value_type>;

  static std::string type_name() { return value_trate::type_name()+"[]"; }
  static void write_value(std::ostream& o, const T& x) {
    const auto _begin = begin(x);
    const auto _end   = end  (x);
    const size_type n = std::distance(_begin,_end);
    o.write(reinterpret_cast<const char*>(&n), sizeof(n));
    for (auto it=_begin; it!=_end; ++it) value_trate::write_value(o,*it);
  }
  static void write_type_def(type_map& m) {
    value_trate::write_type_def(m);
  }
};

template <typename T, std::size_t N>
struct trait<std::array<T,N>,void> {
  using value_type = std::decay_t<T>;
  using value_trate = trait<value_type>;

  static std::string type_name() {
    return value_trate::type_name()+"["+std::to_string(N)+"]";
  }
  static void write_value(std::ostream& o, const std::array<T,N>& x) {
    for (const auto& e : x) value_trate::write_value(o,e);
  }
  static void write_type_def(type_map& m) {
    value_trate::write_type_def(m);
  }
};

class writer {
  std::string file_name;
  std::stringstream o;
  std::vector<std::tuple<std::string,std::string>> root;
  type_map type_defs;
public:
  writer(const std::string& filename): file_name(filename) { }
  writer(std::string&& filename): file_name(std::move(filename)) { }
  ~writer() { close(); }

  template <typename T, typename S>
  writer& write(S&& name, const T& x) {
    using trait = trait<T>;
    root.emplace_back( trait::type_name(), std::forward<S>(name) );
    if (type_defs.find(std::get<0>(root.back()))==type_defs.end())
      trait::write_type_def(type_defs);
    trait::write_value(o,x);
    return *this;
  }

  void close() {
    if (file_name.empty()) throw std::runtime_error(
      "writer is not in a valid state");
    std::ofstream f(file_name,std::ios::binary);
    file_name.clear(); // erase

    f << "{";
    { f << "\"root\":[";
      const auto begin = root.begin();
      const auto end   = root.end  ();
      for (auto it=begin; ; ) {
        f << (it!=begin ? ",[" : "[")
          << "\"" << std::get<0>(*it) << "\","
             "\"" << std::get<1>(*it) << "\"";
        if ((++it)==end) { f << "]"; break; }
        for ( ; std::get<0>(*it)==std::get<0>(*std::prev(it)); ++it) {
          f << ",\"" << std::get<1>(*it) << "\"";
        }
        f << "]";
      }
      f << "],";
    }
    { f << "\"types\":{";
      const auto begin = type_defs.begin();
      const auto end   = type_defs.end  ();
      for (auto it=begin; it!=end; ++it) {
        f << (it!=begin ? ",\"" : "\"") << it->first << "\":" << it->second;
      }
      f << "}";
    }
    f << "}";

    f << o.rdbuf();
    o.str({}); // erase
    o.clear(); // clear errors
  }
};

class reader {
  void* m;
  size_t m_len, head_len;
public:
  reader(const std::string& filename);
  ~reader();
  void close();

  string_view head() const;
};

}}

#endif
