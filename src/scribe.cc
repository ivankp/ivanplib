#include "ivanp/scribe.hh"

#include <iostream>
#include <fstream>
#include <stdexcept>
#include <algorithm>

#include <boost/lexical_cast.hpp>

#include "ivanp/functional.hh"
#include "ivanp/string.hh"

using std::cout;
using std::endl;
using boost::lexical_cast;

#define TEST(var) \
  std::cout << "\033[36m" #var "\033[0m = " << var << std::endl;

namespace ivanp { namespace scribe {

std::ostream& operator<<(std::ostream& f, const writer& w) {
  f << '{';
  { f << "\"root\":[";
    const auto begin = w.root.begin();
    const auto end   = w.root.end  ();
    for (auto it=begin; it!=end; ) {
      if (it!=begin) f << "],";
      f << "[\"" << std::get<0>(*it) << "\","
            "\"" << std::get<1>(*it) << "\"";
      for (; (++it)!=end && std::get<0>(*it)==std::get<0>(*std::prev(it)); )
        f << ",\"" << std::get<1>(*it) << '\"';
    }
    f << "]]";
  }
  if (!w.types.empty()) {
    f << ",\"types\":{";
    const auto begin = w.types.begin();
    const auto end   = w.types.end  ();
    for (auto it=begin; it!=end; ++it) {
      f << (it!=begin ? ",\"" : "\"") << it->first << "\":["
        << it->second << ']';
    }
    f << "}";
  }
  if (!w.info.empty()) f << ",\"info\":" << w.info;
  f << '}';

  return f << w.o.rdbuf();
  // std::stringstream().swap(w.o); // reset
}

void trait<const char*>::write_value(std::ostream& o, const char* s) {
  const size_type n = strlen(s);
  o.write(reinterpret_cast<const char*>(&n), sizeof(n));
  o.write(s,n);
}

inline size_t memlen(type_node t) noexcept { return t.memlen(); }
inline size_t memlen(type_node::child_t c) noexcept { return c.type.memlen(); }
template <typename T>
size_t memlen_sum(const T& xs) {
  size_t sum = 0;
  for (const auto& x : xs) {
    const size_t len = memlen(x);
    if (len) sum += len;
    else return 0;
  }
  return sum;
}

reader::reader(char* file, size_t flen): m(file), m_len(flen) {
  int nbraces = 0;
  for (data = m;;) {
    if (decltype(m_len)(data-m) >= m_len)
      throw error("reached EOF while reading header");
    const char c = *data;
    if (c=='{') ++nbraces;
    else if (c=='}') --nbraces;
    ++data;
    if (nbraces==0) break;
    else if (nbraces < 0) throw error("unpaired \'}\' in header");
  }

  json_head = nlohmann::json::parse(m,data);
  std::vector<type_node::child_t> root_types;
  for (const auto& val : json_head.at("root")) {
    auto val_it = val.begin();
    const auto val_end = val.end();
    const std::string name = *val_it;
    const type_node root_type = y_combinator([this](
      auto f, const char* begin, const char* end
    ) -> type_node {
      if (begin==end) throw error("blank type name");
      const string_view name(begin,end-begin);
      auto type_it = std::find_if(all_types.begin(),all_types.end(),
        [name](type_node x){ return name == x.name(); });
      if (type_it!=all_types.end()) return *type_it;
      auto s = end;
      while (s!=begin && std::isdigit(*--s)) ;
      const char c = *s;
      const auto size_len = end-s-1;
      type_node type;
      type_node::flags_t flags;
      // memset(&flags, 0, sizeof(flags));
      // array ------------------------------------------------------
      if (c=='#') {
        size_type size = 0; // array length
        if (end-s>1) size = lexical_cast<size_type>(s+1,size_len);
        type_node subtype = f(begin,s);
        flags.is_array = true;
        type = { subtype.memlen()*size, size, flags, name };
        type.begin()->type = subtype;
      // fundamental ------------------------------------------------
      } else if (end-s>1 && s==begin && (c=='f'||c=='u'||c=='i')) {
        flags.is_fundamental = true;
        type = { lexical_cast<size_type>(s+1,size_len), 0, flags, name };
      // tuple or union ---------------------------------------------
      } else if (end-s==1 && [](auto begin, auto end){
          const char b = *begin, e = *--end;
          if (b=='(') {
            if (e!=')') return false;
          } else if (b=='[') {
            if (e!=']') return false;
          } else return false;
          int cnt = 0;
          for (auto s=begin; s!=end; ++s) {
            if (*s==b) ++cnt; else if (*s==e) --cnt;
            if (cnt==0) return false;
          }
          return cnt==1;
        }(begin,end)
      ) {
        const bool is_union = ((*begin)=='[');
        std::vector<type_node> subtypes;
        char b='\0', e='\0';
        int cnt = 0;
        auto _begin = begin + 1;
        const auto _end = end - 1;
        for (auto s=_begin; s!=_end; ++s) {
          const char c = *s;
          if (e) {
            if (c==b) ++cnt; else
            if (c==e) --cnt;
            if (cnt==0) e = '\0';
          } else if (c==',') {
            subtypes.push_back(f(_begin,s));
            _begin = s+1;
          } else if (c=='(' || c=='[' || c=='{') {
            b = c;
            switch (b) {
              case '(': e = ')'; break;
              case '[': e = ']'; break;
              case '{': e = '}'; break;
              default: ;
            }
            ++cnt;
          }
        }
        subtypes.push_back(f(_begin,_end));
        flags.is_union = is_union;
        type = {
          (is_union ? 0 : memlen_sum(subtypes)),
          (size_type)subtypes.size(), flags, name };
        std::transform(subtypes.begin(),subtypes.end(),type.begin(),
          [](auto x) -> type_node::child_t { return { x, { } }; });
        if (is_union) {
          y_combinator([type](auto g, type_node _type) -> void {
            for (auto& child : _type) {
              auto& t = child.type;
              if (strcmp(t.name(),"^")) g(t);
              else { t.clean(); t = type; }
            }
          })(type);
        }
      // null -------------------------------------------------------
      } else if (name=="null" || name=="^") {
        flags.is_fundamental = true; // overwritten later for ^
        type = { 0, 0, flags, name };
      // user defined type ------------------------------------------
      } else {
        std::vector<type_node::child_t> subtypes;
        for (const auto& val : [&]() -> auto& {
          try {
            return json_head.at("types")
              .at(std::string(name.begin(),name.end()));
          } catch (const std::exception& e) {
            throw error("no definition for type \"",name,'\"');
          }
        }()) {
          auto val_it = val.begin();
          const auto val_end = val.end();
          const std::string name = *val_it;
          const type_node subtype = f(name.c_str(), name.c_str()+name.size());
          for (++val_it; val_it!=val_end; ++val_it)
            subtypes.push_back({subtype,*val_it});
        }
        type = {
          memlen_sum(subtypes), (size_type)subtypes.size(), flags, name };
        std::move(subtypes.begin(),subtypes.end(),type.begin());
      }
      if (strcmp(type.name(),"^")) all_types.emplace_back(type);
      return type;
    })(name.c_str(), name.c_str()+name.size());
    for (++val_it; val_it!=val_end; ++val_it)
      root_types.push_back({root_type,*val_it});
  }
  type = { memlen_sum(root_types), (size_type)root_types.size(), {}, {} };
  std::move(root_types.begin(),root_types.end(),type.begin());
}

reader::~reader() {
  if (m) {
    type.clean();
    for (auto& type : all_types) type.clean();
  }
}

void reader::print_types() const {
  for (const auto& type : all_types) {
    cout << type.name() << " <"
         << type.memlen() << ','
         << type.size() << ','
         << type.num_children() << '>'
         << endl;
    for (const auto& sub : type)
      cout <<"  "<< sub.type.name() << " " << sub.name << endl;
  }
}

template <typename... Ts>
inline char* memcpy_pack(char* p, const Ts&... xs) {
  UNFOLD(( memcpy(p,&xs,sizeof(xs)), p+=sizeof(xs) ))
  return p;
}

type_node::type_node(
  size_t memlen, size_type size, flags_t f, string_view name
): p(new char[
      sizeof(memlen) // memlen
    + sizeof(size) // number of elements
    + flags_size::value
    + (f.is_array?1:size)*sizeof(child_t) // children
    + name.size()+1 // name
]){
  child_t* child = reinterpret_cast<child_t*>(
    memcpy_pack(p,memlen,size,f) + (flags_size::value - sizeof(f))
  );
  for (child_t* end=child+(f.is_array?1:size); child!=end; ++child)
    new(child) child_t();
  flags().is_array = f.is_array;
  reinterpret_cast<char*>(
    memcpy(reinterpret_cast<char*>(child),name.data(),name.size())
  )[name.size()] = '\0';
}
void type_node::clean() {
  for (auto& child : *this) child.~child_t();
  delete[] p;
}
size_t type_node::memlen() const {
  return *reinterpret_cast<size_t*>(p);
}
size_type type_node::size() const {
  return *reinterpret_cast<size_type*>(p + sizeof(size_t));
}
type_node::flags_t& type_node::flags() const {
  return *reinterpret_cast<flags_t*>(p + (sizeof(size_t) + sizeof(size_type)));
}
bool type_node::is_array() const {
  return flags().is_array;
}
bool type_node::is_union() const {
  return flags().is_union;
}
bool type_node::is_fundamental() const {
  return flags().is_fundamental;
}
bool type_node::is_null() const {
  return flags().is_fundamental && memlen()==0;
}
size_type type_node::num_children() const {
  return is_array() ? 1 : size();
}
type_node::child_t* type_node::begin() {
  return reinterpret_cast<child_t*>(
    p + (sizeof(size_t) + sizeof(size_type) + flags_size::value));
}
const type_node::child_t* type_node::begin() const {
  return reinterpret_cast<child_t*>(
    p + (sizeof(size_t) + sizeof(size_type) + flags_size::value));
}
type_node::child_t* type_node::end() {
  return begin() + num_children();
}
const type_node::child_t* type_node::end() const {
  return begin() + num_children();
}
const char* type_node::name() const {
  return reinterpret_cast<const char*>(
    p + (sizeof(size_t) + sizeof(size_type) + flags_size::value)
      + num_children()*sizeof(child_t)
  );
}
const type_node type_node::operator[](size_type i) const {
  return (begin()+(is_array() ? 0 : i))->type;
}
const type_node type_node::find(const char* str) const {
  const child_t* _end = end();
  const child_t* it = std::find_if(begin(),_end,[str](const child_t& child){
    return child.name == str;
  });
  if (it==_end) return { };
  return it->type;
}
const size_type type_node::index(const char* str) const {
  const child_t* _begin = begin();
  const child_t* _end = end();
  const child_t* it = std::find_if(_begin,_end,[str](const child_t& child){
    return child.name == str;
  });
  if (it==_end) throw error("type \"",name(),"\" has no member \"",str,"\"");
  return it-_begin;
}

// resolve length of object at given position
size_t type_node::memlen(const char* m) const {
  auto len = memlen();
  if (!len) {
    if (is_array()) {
      size_type n = size();
      if (!n) {
        n = *reinterpret_cast<const size_type*>(m);
        len += sizeof(size_type);
        m += sizeof(size_type);
      }
      auto subtype = begin()->type;
      auto len2 = subtype.memlen();
      if (len2) return len + n*len2;
      for (size_type i=0; i<n; ++i) {
        len2 = subtype.memlen(m);
        len += len2;
        m += len2;
      }
    } else if (is_union()) {
      return (*(begin()+*reinterpret_cast<const union_index_type*>(m)))
        ->memlen(m + sizeof(union_index_type)) + sizeof(union_index_type);
    } else {
      for (const auto& a : *this) {
        const auto len2 = a->memlen(m);
        len += len2;
        m += len2;
      }
    }
  }
  return len;
}

// get by index
value_node value_node::operator[](size_type key) const {
  auto size = type.size();
  char* m = data;
  if (type.is_array()) {
    if (!size) {
      size = *reinterpret_cast<size_type*>(m);
      m += sizeof(size_type);
    }
    if (key >= size) throw error(
      "index ",key," out of bound in \"",type.name(),"\"");
    const auto a = type.begin();
    const auto subtype = a->type;
    const auto len = subtype.memlen();
    if (len) m += len*key;
    else for (size_type i=0; i<key; ++i) m += subtype.memlen(m);
    return { m, subtype, a->name.c_str() };
  } else if (type.is_union()) {
    auto x = **this;
    while (x.get_type().is_union()) x = *x;
    return x[key];
  } else {
    if (key >= size) throw error(
      "index ",key," out of bound in \"",type.name(),"\"");
    auto a = type.begin();
    for (const auto _end=a+key; a!=_end; ++a) {
      m += (*a)->memlen(m);
    }
    return { m, a->type, a->name.c_str() };
  }
}
// get by name
value_node value_node::operator[](const char* key) const {
  if (type.is_union()) {
    auto x = **this;
    while (x.get_type().is_union()) x = *x;
    return x[key];
  }
  char* m = data;
  auto a = type.begin();
  for (const auto _end = type.end();; ++a) {
    if (a==_end) throw error(
      "key \"",key,"\" not found in \"",type.name(),"\"");
    if (!a->name.empty() && a->name==key) break;
    m += (*a)->memlen(m);
  }
  return { m, a->type, a->name.c_str() };
}

// get union element
value_node value_node::operator*() const {
  const auto index = union_index();
  const auto a = type.begin() + index;
  return { data + sizeof(index), a->type, a->name.c_str() };
}

// increment iterator
value_node::iterator& value_node::iterator::operator++() {
  data += type[index].memlen(data);
  ++index;
  return *this;
}

// dereference iterator
value_node value_node::iterator::operator*() const {
  const auto a = (type.begin()+(type.is_array() ? 0 : index));
  return { data, a->type, a->name.c_str() };
}

// compare values without casting with memcmp
bool value_node::operator==(const value_node& r) const noexcept {
  const auto len = memlen();
  if (r.memlen() != len) return false;
  return !memcmp(data,r.data,len);
}

}}
