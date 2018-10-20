#include "ivanp/io/mem_file.hh"

#include <cstdio>
#include <cctype>
#include <cstring>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>

#include "ivanp/error.hh"

namespace ivanp {

mem_file mem_file::mmap(const char* name) {
  // https://www.oreilly.com/library/view/linux-system-programming/
  // 0596009585/ch04s03.html
  struct stat sb;
  int fd = ::open(name, O_RDONLY);
  if (fd == -1) throw error("open");
  if (::fstat(fd, &sb) == -1) throw error("fstat");
  if (!S_ISREG(sb.st_mode)) throw error("not a file");
  size_t m_len = sb.st_size;
  char* m = reinterpret_cast<char*>(::mmap(0,m_len,PROT_READ,MAP_SHARED,fd,0));
  if (m == MAP_FAILED) throw error("mmap");
  if (::close(fd) == -1) throw error("close");
  return { m, m_len, mem_file::munmap };
}
void mem_file::munmap(mem_file& f) { ::munmap(f.m,f.len); }

mem_file mem_file::read(const char* name) {
  FILE *f = ::fopen(name,"rb");
  ::fseek(f, 0, SEEK_END);
  size_t fsize = ::ftell(f);
  ::fseek(f, 0, SEEK_SET);
  char* m = reinterpret_cast<char*>(::malloc(fsize));
  ::fread(m, fsize, 1, f);
  ::fclose(f);
  return { m, fsize, mem_file::free };
}
void mem_file::free(mem_file& f) { ::free(f.m); }

mem_file mem_file::pipe(const char* cmd) {
  constexpr size_t buf_len = 1 << 7;
  size_t m_used = 0, m_total = 1<<10;
  char* m = reinterpret_cast<char*>(::malloc(m_total));
  FILE* pipe = ::popen(cmd,"r");
  if (!pipe) throw error("popen");
  for (;;) {
    const size_t n = ::fread(m+m_used, 1, buf_len, pipe);
    m_used += n;
    if (n < buf_len) break;
    if (m_used >= m_total)
      m = reinterpret_cast<char*>(::realloc(m,m_total<<=1));
  }
  if (::pclose(pipe) == -1) throw error("pclose");
  return { m, m_used, mem_file::free };
}

}
