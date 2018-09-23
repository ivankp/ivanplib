#ifndef IVANP_SCRIBE_HH
#define IVANP_SCRIBE_HH

#include <type_traits>
#include <fstream>
#include <sstream>
#include <vector>
#include <array>
#include <tuple>
#include <unordered_map>
#include <cstdint>
#include "ivanp/meta.hh"

namespace ivanp {

using scribe_type_map = std::unordered_map<std::string,std::string>;
using scribe_size_type = uint32_t;

template <typename T, typename SFINAE=void>
struct scribe_trait;

template <typename T>
struct scribe_trait<T,std::enable_if_t<std::is_arithmetic<T>::value>> {
  static void write_value(std::ostream& o, const T& x) {
    o.write(reinterpret_cast<const char*>(&x), sizeof(x));
  }
  static void write_type_def(scribe_type_map& m) { }
  static std::string type_name() {
    return (std::is_floating_point<T>::value ? "f" :
        (std::is_signed<T>::value ? "i" : "u")
      ) + std::to_string(sizeof(T));
  }
};

template <typename T>
struct scribe_trait<T, void_t<
  decltype(std::declval<const T&>().begin()),
  decltype(std::declval<const T&>().end())
>> {
  using value_type = std::decay_t<decltype(*std::declval<T>().begin())>;
  using value_trate = scribe_trait<value_type>;

  static std::string type_name() { return value_trate::type_name()+"[]"; }
  static void write_value(std::ostream& o, const T& x) {
    const auto begin = x.begin();
    const auto end   = x.end  ();
    const scribe_size_type n = std::distance(begin,end);
    o.write(reinterpret_cast<const char*>(&n), sizeof(n));
    for (auto it=begin; it!=end; ++it) value_trate::write_value(o,*it);
  }
  static void write_type_def(scribe_type_map& m) {
    value_trate::write_type_def(m);
  }
};

template <typename T, std::size_t N>
struct scribe_trait<std::array<T,N>,void> {
  using value_type = std::decay_t<T>;
  using value_trate = scribe_trait<value_type>;

  static std::string type_name() {
    return value_trate::type_name()+"["+std::to_string(N)+"]";
  }
  static void write_value(std::ostream& o, const std::array<T,N>& x) {
    for (const auto& e : x) value_trate::write_value(o,e);
  }
  static void write_type_def(scribe_type_map& m) {
    value_trate::write_type_def(m);
  }
};

class scribe_writer {
  std::string file_name;
  std::stringstream o;
  std::vector<std::tuple<std::string,std::string>> root;
  scribe_type_map type_defs;
public:
  scribe_writer(const std::string& filename): file_name(filename) { }
  scribe_writer(std::string&& filename): file_name(std::move(filename)) { }
  ~scribe_writer() { close(); }

  template <typename T, typename S>
  scribe_writer& write(S&& name, const T& x) {
    using trait = scribe_trait<T>;
    root.emplace_back( trait::type_name(), std::forward<S>(name) );
    if (type_defs.find(std::get<0>(root.back()))==type_defs.end())
      trait::write_type_def(type_defs);
    trait::write_value(o,x);
    return *this;
  }

  void close() {
    if (file_name.empty()) throw std::runtime_error(
      "scribe_writer is not in a valid state");
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

class scribe_reader {
  void* m;
  size_t m_len;
public:
  scribe_reader(const std::string& filename);
  ~scribe_reader();
  void close();
};

}

#endif
