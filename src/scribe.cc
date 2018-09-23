#include "ivanp/scribe.hh"

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>

#include <iostream>

namespace ivanp {

#define SCRIBE_THROW(MSG) throw std::runtime_error("scribe_reader: " MSG);

// https://www.oreilly.com/library/view/linux-system-programming/0596009585/ch04s03.html
scribe_reader::scribe_reader(const std::string& filename) {
  struct stat sb;
  int fd = ::open(filename.c_str(), O_RDONLY);
  if (fd == -1) SCRIBE_THROW("open");
  if (::fstat(fd, &sb) == -1) SCRIBE_THROW("fstat");
  if (!S_ISREG(sb.st_mode)) SCRIBE_THROW("not a file");
  m_len = sb.st_size;
  m = mmap(0, m_len, PROT_READ, MAP_SHARED, fd, 0);
  if (m == MAP_FAILED) SCRIBE_THROW("mmap");
  if (::close(fd) == -1) SCRIBE_THROW("close");

  std::cout.write(reinterpret_cast<const char*>(m),m_len);
}
void scribe_reader::close() {
  if (munmap(m,m_len) == -1) SCRIBE_THROW("munmap");
}
scribe_reader::~scribe_reader() { close(); }

}
