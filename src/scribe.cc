#include "ivanp/scribe.hh"

#include <cstdio>
#include <cctype>
#include <cstring>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>

#include <iostream>
#include <fstream>
#include <stdexcept>
#include <algorithm>

#include <boost/lexical_cast.hpp>

#include <nlohmann/json.hpp>

#include "ivanp/functional.hh"

using std::cout;
using std::endl;
using boost::lexical_cast;

#define TEST(var) \
  std::cout << "\033[36m" #var "\033[0m = " << var << std::endl;

namespace ivanp { namespace scribe {

void writer::write() {
  if (file_name.empty()) throw error("invalid state");
  std::ofstream f(file_name,std::ios::binary);
  file_name.clear(); // erase

  f << '{';
  { f << "\"root\":[";
    const auto begin = root.begin();
    const auto end   = root.end  ();
    for (auto it=begin; ; ) {
      f << (it!=begin ? ",[" : "[")
        << "\"" << std::get<0>(*it) << "\","
           "\"" << std::get<1>(*it) << "\"";
      if ((++it)==end) { f << "]"; break; }
      for ( ; std::get<0>(*it)==std::get<0>(*std::prev(it)); ++it) {
        f << ",\"" << std::get<1>(*it) << '\"';
      }
      f << ']';
    }
    f << ']';
  }
  if (!types.empty()) {
    f << ",\"types\":{";
    const auto begin = types.begin();
    const auto end   = types.end  ();
    for (auto it=begin; it!=end; ++it) {
      f << (it!=begin ? ",\"" : "\"") << it->first << "\":["
        << it->second << ']';
    }
    f << "}";
  }
  f << '}';

  f << o.rdbuf();
  o.str({}); // erase
  o.clear(); // clear errors
}

void trait<const char*>::write_value(std::ostream& o, const char* s) {
  const size_type n = strlen(s);
  o.write(reinterpret_cast<const char*>(&n), sizeof(n));
  o.write(s,n);
}

template <typename T, typename F>
size_t memlen_sum(const T& xs, F f) {
  size_t sum = 0;
  for (const auto& x : xs) {
    const size_t memlen = f(x).memlen();
    if (memlen) sum += memlen;
    else return 0;
  }
  return sum;
}

// https://www.oreilly.com/library/view/linux-system-programming/0596009585/ch04s03.html
reader::reader(const char* filename) {
  struct stat sb;
  int fd = ::open(filename, O_RDONLY);
  if (fd == -1) throw error("open");
  if (::fstat(fd, &sb) == -1) throw error("fstat");
  if (!S_ISREG(sb.st_mode)) throw error("not a file");
  m_len = sb.st_size;
  m = reinterpret_cast<char*>(mmap(0, m_len, PROT_READ, MAP_SHARED, fd, 0));
  if (m == MAP_FAILED) throw error("mmap");
  if (::close(fd) == -1) throw error("close");

  int nbraces = 0;
  for (data = m;;) {
    if ((decltype(m_len))(data-m) >= m_len)
      throw error("reached EOF while reading header");
    const char c = *data;
    if (c=='{') ++nbraces;
    else if (c=='}') --nbraces;
    ++data;
    if (nbraces==0) break;
    else if (nbraces < 0) throw error("unpaired \'}\' in header");
  }

  const auto head = nlohmann::json::parse(m,data);
  std::vector<type_node::child_t> root_types;
  for (const auto& val : head.at("root")) {
    auto val_it = val.begin();
    const auto val_end = val.end();
    const std::string name = *val_it;
    const type_node root_type = y_combinator([this,&head](
      auto f, const char* begin, const char* end
    ) -> type_node {
      if (begin==end) throw error("blank type name");
      const string_view name(begin,end-begin);
      auto type_it = std::find_if(all_types.begin(),all_types.end(),
        [name](type_node x){ return !name.compare(x.name()); });
      if (type_it!=all_types.end()) return *type_it;
      auto s = end;
      while (s!=begin && std::isdigit(*--s)) ;
      const char c = *s;
      const auto size_len = end-s-1;
      type_node type;
      if (c=='#') { // array
        size_t size = 0; // array length
        if (end-s>1) size = lexical_cast<size_t>(s+1,size_len);
        type_node subtype = f(begin,s);
        type = { subtype.memlen()*size, size, true, name };
        type.begin()->type = subtype;
      } else if (end-s>1 && s==begin && (c=='f'||c=='u'||c=='i')) { // fundamental
        type = { lexical_cast<size_t>(s+1,size_len), 0, false, name };
      } else if (end-s==1 && [begin,end](){
          if (*begin!='(' || *(end-1)!=')') return false;
          int cnt = 0, prev_cnt = 0;
          for (auto s=begin; s!=end; ++s) {
            prev_cnt = cnt;
            if (*s=='(') ++cnt; else if (*s==')') --cnt;
            if (cnt==0 && prev_cnt==0) return false;
          }
          return !cnt;
        }()
      ) {
        std::vector<type_node> subtypes;
        int cnt = 0;
        auto a = begin+1;
        for (auto s=begin; s!=end; ++s) {
          if (*s=='(') ++cnt; else if (*s==')') --cnt;
          if (!cnt) subtypes.push_back(f(a,s)), a = s+2;
        }
        type = {
          memlen_sum(subtypes,[](const auto& x){ return x; }),
          subtypes.size(), false, name };
        std::transform(subtypes.begin(),subtypes.end(),type.begin(),
          [](auto x) -> type_node::child_t { return { x, { } }; });
      } else { // user defined type
        std::vector<type_node::child_t> subtypes;
        for (const auto& val : [&]() -> auto& {
          try {
            return head.at("types").at(std::string(name.begin(),name.end()));
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
          memlen_sum(subtypes,[](const auto& x){ return x.type; }),
          subtypes.size(),false,name };
        std::move(subtypes.begin(),subtypes.end(),type.begin());
      }
      all_types.emplace_back(type);
      return all_types.back();
    })(name.c_str(), name.c_str()+name.size());
    for (++val_it; val_it!=val_end; ++val_it)
      root_types.push_back({root_type,*val_it});
  }
  type = {
    memlen_sum(root_types,[](const auto& x){ return x.type; }),
    root_types.size(),false,{} };
  std::move(root_types.begin(),root_types.end(),type.begin());
}

void reader::close() {
  type.clean();
  for (auto& type : all_types) type.clean();
  if (munmap(m,m_len) == -1) throw error("munmap");
}
reader::~reader() {
  try {
    close();
  } catch (const std::exception& e) {
    std::cerr << "Exception in ~reader(): " << e.what() << std::endl;
  }
}

string_view reader::head() const { return { m, size_t(data-m) }; }

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
  size_t memlen, size_t size, bool is_array, string_view name
): p(new char[
      sizeof(memlen) // memlen
    + sizeof(size) // number of elements
    + sizeof(is_array)
    + (is_array?1:size)*sizeof(child_t) // children
    + name.size()+1  // name
]){
  child_t* _p = reinterpret_cast<child_t*>(memcpy_pack(p,memlen,size,is_array));
  for (child_t* end=_p+(is_array?1:size); _p!=end; ++_p) new(_p) child_t();
  if (is_array) (_p-1)->name = { "\0", 1 };
  memcpy(_p,name.data(),name.size());
  *(reinterpret_cast<char*>(_p)+name.size()) = '\0';
}
void type_node::clean() {
  for (auto& child : *this) child.~child_t();
  delete[] p;
}
size_t type_node::memlen() const {
  return *reinterpret_cast<size_t*>(p);
}
size_t type_node::size() const {
  return *reinterpret_cast<size_t*>(p + sizeof(size_t));
}
bool type_node::is_array() const {
  return *reinterpret_cast<bool*>(p + sizeof(size_t) + sizeof(size_t));
}
size_t type_node::num_children() const {
  return is_array() ? 1 : size();
}
type_node::child_t* type_node::begin() {
  return reinterpret_cast<child_t*>(
    p + sizeof(size_t) + sizeof(size_t) + sizeof(bool));
}
const type_node::child_t* type_node::begin() const {
  return reinterpret_cast<child_t*>(
    p + sizeof(size_t) + sizeof(size_t) + sizeof(bool));
}
type_node::child_t* type_node::end() {
  return begin() + num_children();
}
const type_node::child_t* type_node::end() const {
  return begin() + num_children();
}
const char* type_node::name() const {
  return reinterpret_cast<const char*>(
    p + sizeof(size_t) + sizeof(size_t) + sizeof(bool)
      + num_children()*sizeof(child_t)
  );
}

size_t type_node::memlen(const char* m) const {
  auto len = memlen();
  if (!len) {
    if (is_array()) {
      auto n = size();
      if (!n) {
        n = *reinterpret_cast<const size_type*>(m);
        len += sizeof(size_type);
        m += sizeof(size_type);
      }
      auto subtype = begin()->type;
      auto len2 = subtype.memlen();
      if (len2) return len + n*len2;
      for (size_t i=0; i<n; ++i) {
        len2 = subtype.memlen(m);
        len += len2;
        m += len2;
      }
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

node node::operator[](size_type key) const {
  auto size = type.size();
  char* m = data;
  if (type.is_array()) {
    if (!size) {
      size = *reinterpret_cast<size_type*>(m);
      m += sizeof(size_type);
    }
    if (key >= size) throw error("index ",key," out of bound");
    auto subtype = type.begin()->type;
    const auto len = subtype.memlen();
    if (len) m += len*key;
    else for (size_type i=0; i<key; ++i) m += subtype.memlen(m);
    return { m, subtype };
  } else {
    if (key >= size) throw error("index ",key," out of bound");
    auto a = type.begin();
    for (const auto _end=a+key; a!=_end; ++a) {
      m += (*a)->memlen(m);
    }
    return { m, a->type };
  }
}
node node::operator[](const char* key) const {
  if (type.is_array()) throw error("cannot use string as array index");
  char* m = data;
  auto a = type.begin();
  for (const auto _end = type.end();; ++a) {
    if (a==_end) throw error("key \"",key,"\" not found");
    if (a->name==key) break;
    m += (*a)->memlen(m);
  }
  return { m, a->type };
}

}}
