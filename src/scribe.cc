#include "ivanp/scribe.hh"

#include <cstdio>
#include <cctype>
#include <cstring>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>

#include <fstream>
#include <stdexcept>

#include <boost/lexical_cast.hpp>

#include <nlohmann/json.hpp>

#include "ivanp/functional.hh"
#include "ivanp/unfold.hh"

using boost::lexical_cast;

#include <iostream>
#define TEST(var) \
  std::cout << "\033[36m" #var "\033[0m = " << var << std::endl;

namespace ivanp { namespace scribe {

#define THROW(MSG) throw std::runtime_error("scribe::writer: " MSG);

void writer::write() {
  if (file_name.empty()) THROW("invalid state");
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
      f << (it!=begin ? ",\"" : "\"") << it->first << "\":" << it->second;
    }
    f << "}";
  }
  // if (!types.empty()) {
  //   f << ",\"types\":{";
  //   bool first = true;
  //   for (const auto& type : types) {
  //     if (first) first = false;
  //     else f << ',';
  //     f << '\"' << type.first << "\":[";
  //     bool first = true;
  //     for (const auto& member : type.second) {
  //       if (first) first = false;
  //       else f << ',';
  //       f << '\"' << member << '\"';
  //     }
  //   }
  //   f << '}';
  // }
  f << '}';

  f << o.rdbuf();
  o.str({}); // erase
  o.clear(); // clear errors
}

#undef THROW
#define THROW(MSG) throw std::runtime_error("scribe::reader: " MSG);

// https://www.oreilly.com/library/view/linux-system-programming/0596009585/ch04s03.html
reader::reader(const char* filename) {
  struct stat sb;
  int fd = ::open(filename, O_RDONLY);
  if (fd == -1) THROW("open");
  if (::fstat(fd, &sb) == -1) THROW("fstat");
  if (!S_ISREG(sb.st_mode)) THROW("not a file");
  m_len = sb.st_size;
  m = mmap(0, m_len, PROT_READ, MAP_SHARED, fd, 0);
  if (m == MAP_FAILED) THROW("mmap");
  if (::close(fd) == -1) THROW("close");

  int nbraces = 0;
  const char * const a0 = reinterpret_cast<const char*>(m), *a = a0;
  for (;;) {
    if ((decltype(m_len))(a-a0) >= m_len)
      THROW("reached EOF while reading header");
    const char c = *a;
    if (c=='{') ++nbraces;
    else if (c=='}') --nbraces;
    ++a;
    if (nbraces==0) {
      head_len = (a-a0);
      break;
    }
    else if (nbraces < 0) THROW("unpaired \'}\' in header");
  }

  /*
  std::vector<node> root_nodes(end-it);
  for (const auto& x : _head["root"]) {
    auto it = x.begin();
    const auto end = x.end();
    const std::string type = *it;
    ++it;
    for (; it!=end; ++it) {
      y_combinator([&it](auto f, const std::string& type, auto& n) -> void {
        if (type.back()=='#') {
          const auto subtype = type.substr(0,type.size()-1);
          node _n { std::vector<node>{}, subtype };
          visit(ivanp::overloaded(
            [&](std::map<std::string,node>& n){
              f(subtype,*n.emplace(*it,std::move(_n)).first);
            },
            [&](std::vector<node>& n){
              n.emplace_back(std::move(_n));
              f(subtype,n.back());
            }
          ),n);
        }
      })(type,_root);
    }
  }
  */
  /*
  auto& root_type = types.emplace().first->second;
  for (const auto& x : _head["root"]) {
    auto it = x.begin();
    const auto end = x.end();
    const std::string type = *it;
    size_t sz = 0;
    const char t0 = type[0];
    if ((t0=='i' || t0=='u' || t0=='f') && std::isdigit(type[1])) {
      bool all_digits = true;
      for (unsigned i=2; i<type.size(); ++i) {
        if (!std::isdigit(type[i])) {
          all_digits = false;
          break;
        }
      }
      if (all_digits) sz = atoi(type.c_str()+1);
    }
    root_type.reserve(end-(++it));
    for (; it!=end; ++it) {
      root_type.push_back({it->get<std::string>(), type, sz});
    }
  }
  */
  const auto head = nlohmann::json::parse(a0,a);
  // {"root":[["i1##","str1","str2","str3"],["i1##2","str4"],["f8","pi"],["u4","3u"]]}
  std::map<std::string,type_node,std::less<>> _types;
  std::vector<std::tuple<std::string,type_node>> root_types;
  for (const auto& val : head["root"]) {
    auto val_it = val.begin();
    const auto val_end = val.end();
    const std::string name = *val_it;
    const type_node type = y_combinator([&_types](
      auto f, const char* begin, const char* end
    ) -> type_node {
      if (begin==end) THROW("blank type name");
      const size_t name_len = end-begin;
      const string_view name(begin,name_len);
      auto type_it = _types.find(name);
      if (type_it!=_types.end()) return type_it->second;
      auto s = end;
      while (s!=begin && std::isdigit(*--s)) ;
      const char c = *s;
      const auto size_len = end - s -1;
      type_node type;
      if (c=='#') { // array
        size_t size = 0; // array length
        if (end-s>1) size = lexical_cast<size_t>(s+1,size_len);
        type_node subtype = f(begin,s-1);
        type = { subtype.memlen()*size, size, 1, begin, name_len };
      } else if (end-s>1 && s==begin && (c=='f'||c=='u'||c=='i')) { // fundamental
        type = { lexical_cast<size_t>(s+1,size_len), 0, 0, begin, name_len };
      } else { // user defined type
      }
      // return _types.emplace(
      //   std::piecewise_construct,
      //   std::forward_as_tuple(name),
      //   std::forward_as_tuple(type)
      return _types.emplace(name,type).first->second;
    })(name.c_str(), name.c_str()+name.size());
    for (++val_it; val_it!=val_end; ++val_it) {
      root_types.emplace_back(val_it->get<std::string>(),type);
    }
  }
}

void reader::close() {
  if (munmap(m,m_len) == -1) THROW("munmap");
}
reader::~reader() {
  close();
  for (auto type : types) type.clean();
}

string_view reader::head() const {
  return { reinterpret_cast<const char*>(m), head_len };
}

template <typename... Ts>
inline char* memcpy_pack(char* p, const Ts&... xs) {
  UNFOLD(( memcpy(p,&xs,sizeof(xs)), p+=sizeof(xs) ))
  return p;
}

type_node::type_node(
  size_t memlen, size_t size, bool is_array,
  const char* name, size_t name_len
): p(new char[
      sizeof(memlen) // memlen
    + sizeof(size) // number of elements
    + sizeof(is_array) // is array
    + ( is_array
        ? sizeof(type_node)
        : size*(sizeof(type_node)+sizeof(char*)) ) // children
    + name_len+1  // name
]){
  char* _p = memcpy_pack(p,memlen,size,is_array);
  if (is_array) _p += sizeof(type_node);
  else _p += size*(sizeof(type_node)+sizeof(char*));
  memcpy(_p,name,name_len);
  *(_p+name_len) = '\0';
}
void type_node::clean() { delete[] p; }
// void type_node::set_memlen(size_t len) { memcpy(p,&len,sizeof(len)); }

}}
