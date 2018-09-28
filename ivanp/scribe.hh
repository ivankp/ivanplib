#ifndef IVANP_SCRIBE_HH
#define IVANP_SCRIBE_HH

#include <cstdint>
#include <type_traits>
#include <sstream>
#include <string>
#include <vector>
#include <array>
#include <tuple>
#include <map>

#include "ivanp/meta.hh"
#include "ivanp/unfold.hh"
#include "ivanp/detect.hh"
#include "ivanp/boolean.hh"

#ifdef __cpp_lib_variant
#include <string_view>
namespace ivanp { namespace scribe { using std::string_view; }}
#else
#include <boost/utility/string_view.hpp>
namespace ivanp { namespace scribe { using boost::string_view; }}
#endif

// #ifdef __cpp_lib_variant
// #include <variant>
// namespace ivanp { namespace scribe { using std::variant; }}
// #else
// #include <boost/variant.hpp>
// namespace ivanp { namespace scribe { using boost::variant; }}
// #endif

namespace ivanp {
namespace scribe {

using writer_type_map = std::map<std::string,std::string,std::less<>>;
using size_type = uint32_t;

template <typename T, typename SFINAE=void>
struct trait;

template <typename T>
struct trait<T,std::enable_if_t<std::is_arithmetic<T>::value>> {
  static void write_value(std::ostream& o, const T& x) {
    o.write(reinterpret_cast<const char*>(&x), sizeof(x));
  }
  static std::string type_name() {
    return (std::is_floating_point<T>::value ? "f" :
        (std::is_signed<T>::value ? "i" : "u")
      ) + std::to_string(sizeof(T));
  }
};

template <>
struct trait<const char*> {
  using value_type = char;
  using value_trate = trait<value_type>;
  static std::string type_name() { return value_trate::type_name()+"#"; }
  static void write_value(std::ostream&, const char*);
};

template <typename T, size_type N>
struct trait<T[N]> {
  using value_type = T;
  using value_trate = trait<value_type>;
  static std::string type_name() {
    return value_trate::type_name()+"#"+std::to_string(N);
  }
  static void write_value(std::ostream& o, const T* x) {
    const size_type n = N;
    o.write(reinterpret_cast<const char*>(&n), sizeof(n));
    for (size_type i=0; i<N; ++i) value_trate::write_value(o,x[i]);
  }
};

template <typename T> using begin_t = decltype(begin(std::declval<T>()));
template <typename T> using   end_t = decltype(end  (std::declval<T>()));
template <typename T>
using is_container = ivanp::conjunction<
  ivanp::is_detected<begin_t,T>,
  ivanp::is_detected<  end_t,T>
>;
template <typename T>
using tuple_size_value_t = decltype(std::tuple_size<T>::value);
template <typename T>
using is_tuple = ivanp::bool_constant<
  ivanp::is_detected<tuple_size_value_t,T>::value
>;

template <typename T>
struct trait<T, void_t<std::enable_if_t<
  is_container<T>::value && !is_tuple<T>::value
>>> {
  using value_type = std::decay_t<decltype(*std::declval<T>().begin())>;
  using value_trate = trait<value_type>;

  static std::string type_name() { return value_trate::type_name()+"#"; }
  static void write_value(std::ostream& o, const T& x) {
    const auto _begin = begin(x);
    const auto _end   = end  (x);
    const size_type n = std::distance(_begin,_end);
    o.write(reinterpret_cast<const char*>(&n), sizeof(n));
    for (auto it=_begin; it!=_end; ++it) value_trate::write_value(o,*it);
  }
};

template <typename T, std::size_t N>
struct trait<std::array<T,N>> {
  using value_type = std::decay_t<T>;
  using value_trate = trait<value_type>;

  static std::string type_name() {
    return value_trate::type_name()+"#"+std::to_string(N);
  }
  static void write_value(std::ostream& o, const std::array<T,N>& x) {
    for (const auto& e : x) value_trate::write_value(o,e);
  }
};

template <typename T>
struct trait<T, void_t<std::enable_if_t<is_tuple<T>::value>>> {
  template <size_t I> using value_trait = trait<std::tuple_element_t<I,T>>;
  template <size_t... I> using _seq = std::index_sequence<I...>;
  using seq = std::make_index_sequence<std::tuple_size<T>::value>;

  template <size_t... I>
  static std::string _type_name(_seq<I...>) {
    std::ostringstream s;
    UNFOLD(( s << '(' << value_trait<I>::type_name() << ')' ))
    return s.str();
  }
  template <size_t... I>
  static void _write_value(std::ostream& o, const T& x, _seq<I...>) {
    UNFOLD( value_trait<I>::write_value(o,std::get<I>(x)) )
  }

  static std::string type_name() { return _type_name(seq{}); }
  static void write_value(std::ostream& o, const T& x) {
    _write_value(o,x,seq{});
  }
};

class writer {
  std::string file_name;
  std::stringstream o;
  std::vector<std::tuple<std::string,std::string>> root;
  writer_type_map types;
public:
  writer(const std::string& filename): file_name(filename) { }
  writer(std::string&& filename): file_name(std::move(filename)) { }
  ~writer() { write(); }

  template <typename T, typename S>
  writer& write(S&& name, const T& x) {
    root.emplace_back( trait<T>::type_name(), std::forward<S>(name) );
    trait<T>::write_value(o,x);
    return *this;
  }

  void write();
};

class reader;

struct type_node {
  char* p;
  using child_t = std::tuple<std::string,type_node>;
  type_node(): p(nullptr) { }
  type_node(
    size_t memlen, size_t size, bool is_array, string_view name
  );
  void clean();
  size_t memlen() const;
  size_t size() const;
  bool is_array() const;
  size_t num_children() const;
  child_t* begin();
  child_t* end();
  const child_t* begin() const;
  const child_t* end() const;
  const char* name() const;
};

class reader {
  void* m;
  size_t m_len, head_len;
  std::vector<type_node> all_types;
  type_node root() const { return all_types.front(); }
public:
  reader(const char* filename);
  reader(const std::string& filename): reader(filename.c_str()) { }
  ~reader();
  void close();

  string_view head() const;

  void print_types() const;

  template <typename... Ts>
  void* get(const Ts&... xs) {
    return nullptr;
  }
};

}}

#endif
