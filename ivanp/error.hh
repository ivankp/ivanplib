#ifndef IVANP_ERROR_HH
#define IVANP_ERROR_HH

#include <stdexcept>
#include "ivanp/string.hh"

namespace ivanp {

struct error : std::runtime_error {
  using std::runtime_error::runtime_error;
  template <typename... T>
  error(T&&... x): std::runtime_error(cat(x...)) { };
  error(const char* str): std::runtime_error(str) { };
};

}

#endif
