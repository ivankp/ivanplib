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

#include <nlohmann/json.hpp>

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
  std::stringstream o;
  std::vector<std::tuple<std::string,std::string>> root;
  std::map<std::string,std::string,std::less<>> types;
  std::string info;
public:
  template <typename T, typename S>
  writer& operator()(S&& name, const T& x) {
    root.emplace_back( trait<T>::type_name(), std::forward<S>(name) );
    trait<T>::write_value(o,x);
    return *this;
  }
  void write(const std::string& info = { });

  template <typename T, typename... Args>
  void add_type(Args&&... args) {
    types.emplace(
      trait<T>::type_name(),
      trait<T>::type_def(std::forward<Args>(args)...)
    );
  }

  friend std::ostream& operator<<(std::ostream&, const writer&);

  void add_info(const std::string& jstr) { info = jstr; }
  void add_info(std::string&& jstr) { info = std::move(jstr); }
};

template <typename... T>
inline void write_values(std::ostream& o, const T&... x) {
  UNFOLD( trait<T>::write_value(o,x) );
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
    s << '(';
    UNFOLD( s << (I?",":"") << value_trait<I>::type_name() )
    s << ')';
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

#ifdef BOOST_VARIANT_HPP
template <typename T, typename... Ts>
struct trait<boost::variant<T,Ts...>> {
  static std::string type_name() {
    std::ostringstream s;
    s << '[' << trait<T>::type_name();
    UNFOLD( s << ',' << trait<Ts>::type_name() )
    s << ']';
    return s.str();
  }
  struct visitor: public boost::static_visitor<void> {
    std::ostream& o;
    visitor(std::ostream& o): o(o) { }
    template <typename V>
    void operator()(const V& v) const { scribe::write_value(o,v); }
  };
  static void write_value(std::ostream& o, const boost::variant<T,Ts...>& x) {
    scribe::write_value(o,(union_index_type)x.which());
    apply_visitor(visitor(o),x);
  }
};
#endif

class reader;
class value_node;
class iterator;

class type_node {
public:
  friend class reader;
  friend class value_node;
  friend class iterator;
  struct child_t;
private:
  char* p;
  struct flags_t;
  struct flags_size;
  type_node(): p(nullptr) { }
  type_node(size_t memlen, size_type size, flags_t flags, string_view name);
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
  bool is_fundamental() const;
  bool is_null() const;
  size_type num_children() const;
  const child_t* begin() const;
  const child_t* end() const;
  const char* name() const;
  const type_node operator[](size_type i) const;
};
struct type_node::child_t {
  type_node type;
  std::string name;
  const type_node* operator->() const { return &type; }
};
struct type_node::flags_t {
  bool is_array : 1;
  bool is_union : 1;
  bool is_fundamental : 1;
  flags_t() { memset(this, 0, sizeof(flags_t)); }
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
    iterator& operator++();
    value_node operator*() const;
    bool operator!=(const iterator& r) const noexcept
    { return index != r.index; }
    bool operator==(const iterator& r) const noexcept
    { return index == r.index; }
  };
protected:
  char* data;
  type_node type;
  const char* name;
  value_node(): data(nullptr), type(), name(nullptr) { }
  value_node(char* data, type_node type, const char* name)
  : data(data), type(type), name(name) { }
public:
  void* ptr() const { return data; }
  template <typename T>
  T cast() const { return *reinterpret_cast<std::decay_t<T>*>(data); }
  template <typename T>
  T safe_cast() const {
    const auto& type_name = trait<T>::type_name();
    if (type_name!=type.name()) throw error(
      "cannot cast ",type.name()," to ",type_name);
    return cast<T>();
  }

  type_node get_type() const { return type; }
  const char* get_name() const { return name; }
  const char* type_name() const { return type.name(); }
  const char* type_name(size_type i) const { return type[i].name(); }
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
  template <typename T>
  std::enable_if_t<std::is_convertible<T,std::string>::value,value_node>
  operator[](const T& key) const {
    return operator[](static_cast<const std::string&>(key));
  }
  iterator begin() const {
    return { type, (type.is_array() && !type.size())
      ? data+sizeof(size_type) : data };
  }
  iterator end() const { return { size() }; }
  value_node operator*() const;
  size_type array_size() const { return cast<size_type>(); }
  union_index_type union_index() const { return cast<union_index_type>(); }
  size_type size() const {
    const auto n = type.size();
    if (n || !type.is_array()) return n;
    else return array_size();
  }
  size_t memlen() const noexcept { return type.memlen(data); }

  bool operator==(const value_node& r) const noexcept;
  bool operator!=(const value_node& r) const noexcept {
    return !operator==(r);
  }
};

class reader: public value_node {
  char *m;
  size_t m_len;
  std::vector<type_node> all_types;
  nlohmann::json json_head;
public:
  enum class tag { mmap, pipe };
  reader(): value_node(), m(nullptr), m_len(0) { }
  reader(char* file, size_t flen);
  reader(const reader&) = delete;
  reader& operator=(const reader&) = delete;
  reader(reader&& o)
  : value_node(std::move(o)), m(o.m), m_len(o.m_len),
    all_types(std::move(o.all_types)), json_head(std::move(o.json_head))
  {
    o.m = nullptr;
    o.m_len = 0;
  }
  reader& operator=(reader&& o) {
    value_node::operator=(std::move(o));
    m = o.m; o.m = nullptr;
    m_len = o.m_len; o.m_len = 0;
    all_types = std::move(o.all_types);
    json_head = std::move(o.json_head);
    return *this;
  }
  ~reader();

  nlohmann::json& head() noexcept { return json_head; }
  const nlohmann::json& head() const noexcept { return json_head; }
  string_view head_str() const { return { m, size_t(data-m) }; }
  void print_types() const;
  type_node root_type() const noexcept { return type; }
  char* data_ptr() const { return data; }
  size_t data_len() const { return m_len - (data-m); }

  operator bool() const noexcept { return m; }
};

template <typename T>
inline decltype(auto) begin(T& x) { return x.begin(); }
template <typename T>
inline decltype(auto) end(T& x) { return x.end(); }

}}

#endif
