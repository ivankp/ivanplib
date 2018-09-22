#ifndef IVANP_SCRIBE_HH
#define IVANP_SCRIBE_HH

#include <type_traits>
#include <fstream>
#include <sstream>
#include <vector>
#include <tuple>
#include <unordered_map>
#include <iostream>

namespace ivanp {

using scribe_type_map = std::unordered_map<std::string,std::string>;

template <typename T, typename SFINAE=void>
struct scribe_trait;

template <typename T>
struct scribe_trait<T,std::enable_if_t<std::is_arithmetic<T>::value>> {
  static std::string type_name() { return typeid(T).name(); }
  static unsigned size() noexcept { return 1; }
  static void write_value(std::ostream& o, const T& x) {
    o.write(reinterpret_cast<const char*>(&x), sizeof(x));
  }
  static void write_type_def(scribe_type_map& m) { }
};

template <typename T>
struct scribe_trait<T, decltype(
  std::declval<const T&>().begin(),
  std::declval<const T&>().end(),
  void()
)> {
private:
  using value_type = std::decay_t<decltype(*std::declval<T>().begin())>;
public:
  static std::string type_name() {
    return scribe_trait<value_type>::type_name();
  }
  static unsigned size() noexcept { return 0; }
  static void write_value(std::ostream& o, const T& x) {
    const auto begin = x.begin();
    const auto end   = x.end  ();
    const unsigned n = std::distance(begin,end);
    o.write(reinterpret_cast<const char*>(&n), sizeof(n));
    for (auto it=begin; it!=end; ++it)
      scribe_trait<value_type>::write_value(o,*it);
  }
  static void write_type_def(scribe_type_map& m) {
    scribe_trait<value_type>::write_type_def(m);
  }
};

class scribe_writer {
  std::string file_name;
  std::stringstream o;
  std::vector<std::tuple<std::string,unsigned,std::string>> root;
  scribe_type_map type_defs;
public:
  scribe_writer(const std::string& filename): file_name(filename) { }
  scribe_writer(std::string&& filename): file_name(std::move(filename)) { }
  ~scribe_writer() { close(); }

  template <typename T, typename S>
  scribe_writer& write(S&& name, const T& x) {
    using trait = scribe_trait<T>;
    root.emplace_back(
      trait::type_name(),
      trait::size(),
      std::forward<S>(name)
    );
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
      for (auto it=begin; it!=end; ++it) {
        f << (it!=begin ? ",[" : "[")
          << "\"" << std::get<0>(*it) << "\","
                  << std::get<1>(*it) << ","
             "\"" << std::get<2>(*it) << "\"]";
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

}

#endif
