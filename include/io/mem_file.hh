#ifndef IVANP_IO_MEMFILE_HH
#define IVANP_IO_MEMFILE_HH

#include <cstddef>

namespace ivanp {

class mem_file {
  char* m;
  size_t len;
  using df_t = void (*)(mem_file&); // destructor function
  df_t df;
  static void free  (mem_file&);
  static void munmap(mem_file&);
public:
  mem_file(): m(nullptr), len(0), df(nullptr) { }
  mem_file(char* m, size_t len, df_t df): m(m), len(len), df(df) { }
  mem_file(const mem_file&) = delete;
  mem_file& operator=(const mem_file& f) = delete;
  mem_file(mem_file&& f): m(f.m), len(f.len), df(f.df) {
    f.m = nullptr;
    f.len = 0;
    f.df = nullptr;
  }
  mem_file& operator=(mem_file&& f) {
    m = f.m; f.m = nullptr;
    len = f.len; f.len = 0;
    df = f.df; f.df = nullptr;
    return *this;
  }
  ~mem_file() { if (m) df(*this); }

  static mem_file mmap(const char*);
  static mem_file read(const char*);
  static mem_file pipe(const char*);

  char* mem() const noexcept { return m; }
  size_t size() const noexcept { return len; }
};

}

#endif
