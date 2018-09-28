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
#include "ivanp/unfold.hh"

using std::cout;
using std::endl;
using boost::lexical_cast;

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

  const auto head = nlohmann::json::parse(a0,a);
  std::map<std::string,type_node,std::less<>> _types;
  std::vector<type_node::child_t> root_types;
  for (const auto& val : head["root"]) {
    auto val_it = val.begin();
    const auto val_end = val.end();
    const std::string name = *val_it;
    const type_node type = y_combinator([&_types](
      auto f, const char* begin, const char* end
    ) -> type_node {
      if (begin==end) THROW("blank type name");
      const string_view name(begin,end-begin);
      auto type_it = _types.find(name);
      if (type_it!=_types.end()) return type_it->second;
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
        std::get<1>(*type.begin()) = subtype;
      } else if (end-s>1 && s==begin && (c=='f'||c=='u'||c=='i')) { // fundamental
        type = { lexical_cast<size_t>(s+1,size_len), 0, false, name };
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
  size_t root_memlen = 0;
  for (const auto& type : root_types) {
    const size_t memlen = std::get<1>(type).memlen();
    if (memlen) root_memlen += memlen;
    else {
      root_memlen = 0;
      break;
    }
  }
  type_node root_type(root_memlen,root_types.size(),false,{});
  std::move(root_types.begin(),root_types.end(),root_type.begin());
  all_types.reserve(_types.size()+1);
  all_types.emplace_back(root_type);
  for (const auto& type : _types) all_types.emplace_back(type.second);
}

void reader::close() {
  for (auto& type : all_types) type.clean();
  if (munmap(m,m_len) == -1) THROW("munmap");
}
reader::~reader() {
  try {
    close();
  } catch (const std::exception& e) {
    std::cerr << "Exception in ~reader(): " << e.what() << std::endl;
  }
}

string_view reader::head() const {
  return { reinterpret_cast<const char*>(m), head_len };
}

void reader::print_types() const {
  for (const auto& type : all_types) {
    cout << type.name() << " <"
         << type.memlen() << ','
         << type.size() << ','
         << type.num_children() << '>'
         << endl;
    for (const auto& sub : type)
      cout <<"  "<< std::get<1>(sub).name() << " " << std::get<0>(sub) << endl;
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
  if (is_array) std::get<0>(*(_p-1)) = { "\0", 1 };
  memcpy(_p,name.data(),name.size());
  *(reinterpret_cast<char*>(_p)+name.size()) = '\0';
  // TEST(name)
  // TEST(size)
  // TEST(num_children())
  // TEST(reinterpret_cast<char*>(_p))
  // TEST(this->name())
  // TEST(reinterpret_cast<const void*>(_p))
  // TEST(reinterpret_cast<const void*>(this->name()))
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

}}