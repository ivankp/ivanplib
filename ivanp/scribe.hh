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
#include "ivanp/error.hh"

#ifdef __cpp_lib_string_view
#include <string_view>
namespace ivanp { namespace scribe { using std::string_view; }}
#else
#include <boost/utility/string_view.hpp>
namespace ivanp { namespace scribe { using boost::string_view; }}
#endif

namespace ivanp {
namespace scribe {

struct error: ivanp::error { using ivanp::error::error; };

using size_type = uint32_t;
using union_index_type = uint8_t;

namespace detail {
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
}

template <typename T, typename SFINAE=void>
struct trait;

class writer {
  std::string file_name;
  std::stringstream o;
  std::vector<std::tuple<std::string,std::string>> root;
  std::map<std::string,std::string,std::less<>> types;
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

  template <typename T>
  void add_type() {
    types.emplace( trait<T>::type_name(), trait<T>::type_def() );
  }
};

template <typename T>
void write_value(std::ostream& o, const T& x) {
  trait<T>::write_value(o,x);
}

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
  static std::string type_name() { return trait<char>::type_name()+"#"; }
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
    for (size_type i=0; i<N; ++i) value_trate::write_value(o,x[i]);
  }
};

template <typename T>
struct trait<T, void_t<std::enable_if_t<
  detail::is_container<T>::value && !detail::is_tuple<T>::value
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
struct trait<T, void_t<std::enable_if_t<detail::is_tuple<T>::value>>> {
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

class reader;
class value_node;
class iterator;

class type_node {
  friend class reader;
  friend class value_node;
  friend class iterator;
  char* p;
  struct child_t;
  struct flags_t;
  struct flags_size;
  type_node(): p(nullptr) { }
  type_node(
    size_t memlen, size_type size, flags_t flags, string_view name
  );
  void clean();
  flags_t& flags() const;
  child_t* begin();
  child_t* end();
public:
  size_t memlen() const;
  size_t memlen(const char* /*memory pointer*/) const;
  size_type size() const;
  bool is_array() const;
  bool is_union() const;
  size_type num_children() const;
  const child_t* begin() const;
  const child_t* end() const;
  const char* name() const;
  type_node operator[](size_type i) const;
};
struct type_node::child_t {
  type_node type;
  std::string name;
  const type_node* operator->() const { return &type; }
};
struct type_node::flags_t {
  bool is_array : 1;
  bool is_union : 1;
};
struct type_node::flags_size: std::integral_constant<unsigned,
  (sizeof(size_t)-sizeof(size_type)+sizeof(flags_t)) % sizeof(size_t)
  + sizeof(flags_t)
> {};

class value_node {
  class iterator {
    size_type index;
    type_node type;
    char* data;
  public:
    iterator(type_node t, char* d): index(0), type(t), data(d) { }
    iterator(size_type i): index(i) { }
    value_node operator*() const { return { data, type[index] }; }
    iterator& operator++();
    bool operator!=(const iterator& r) const noexcept
    { return index != r.index; }
    bool operator==(const iterator& r) const noexcept
    { return index == r.index; }
  };
protected:
  char* data;
  type_node type;
  value_node(): data(nullptr), type() { }
  value_node(char* data, type_node type): data(data), type(type) { }
public:
  const char* type_name() const { return type.name(); }
  size_type size() const {
    if (type.is_union()) return 0;
    const auto n = type.size();
    if (n || !type.is_array()) return n;
    else return *reinterpret_cast<size_type*>(data);
  }
  template <typename T>
  const T& cast() const { return *reinterpret_cast<const T*>(data); }
  template <typename T>
  const T& safe_cast() const {
    const auto& type_name = trait<T>::type_name();
    if (type_name!=type.name()) throw error(
      "cannot cast ",type.name()," to ",type_name);
    return cast<T>();
  }
  value_node operator[](size_type) const;
  template <typename T>
  std::enable_if_t<std::is_integral<T>::value,value_node>
  operator[](T key) const {
    return operator[](static_cast<size_type>(key));
  }
  value_node operator[](const char*) const;
  value_node operator[](const std::string& s) const {
    return operator[](s.c_str());
  }
  iterator begin() const {
    return { type, (type.is_array() && !type.size())
      ? data+sizeof(size_type) : data };
  }
  iterator end() const { return { size() }; }
  value_node operator*() const;
};

class reader: public value_node {
  char *m;
  size_t m_len;
  std::vector<type_node> all_types;
public:
  reader(const char* filename);
  reader(const std::string& filename): reader(filename.c_str()) { }
  ~reader();
  void close();

  string_view head() const;
  void print_types() const;
  type_node root_type() const { return type; }
  void* data_ptr() const { return data; }
};

}}

#endif
