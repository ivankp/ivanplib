#include "ivanp/scribe.hh"

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <stdexcept>

namespace ivanp { namespace scribe {

#define SCRIBE_THROW(MSG) throw std::runtime_error("scribe::reader: " MSG);

// https://www.oreilly.com/library/view/linux-system-programming/0596009585/ch04s03.html
reader::reader(const std::string& filename) {
  struct stat sb;
  int fd = ::open(filename.c_str(), O_RDONLY);
  if (fd == -1) SCRIBE_THROW("open");
  if (::fstat(fd, &sb) == -1) SCRIBE_THROW("fstat");
  if (!S_ISREG(sb.st_mode)) SCRIBE_THROW("not a file");
  m_len = sb.st_size;
  m = mmap(0, m_len, PROT_READ, MAP_SHARED, fd, 0);
  if (m == MAP_FAILED) SCRIBE_THROW("mmap");
  if (::close(fd) == -1) SCRIBE_THROW("close");

  int nbraces = 0;
  const char * const a0 = reinterpret_cast<const char*>(m), *a = a0;
  for (;; ++a) {
    if ((decltype(m_len))(a-a0) >= m_len)
      SCRIBE_THROW("reached EOF while reading header");
    const char c = *a;
    if (c=='{') ++nbraces;
    else if (c=='}') --nbraces;
    if (nbraces==0) {
      head_len = (a-a0)+1;
      break;
    }
    else if (nbraces < 0) SCRIBE_THROW("unpaired \'}\' in header");
  }
}
void reader::close() {
  if (munmap(m,m_len) == -1) SCRIBE_THROW("munmap");
}
reader::~reader() { close(); }

string_view reader::head() const {
  return { reinterpret_cast<const char*>(m), head_len };
}

}}
