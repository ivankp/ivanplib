#ifndef IVANP_TYPE_STR_HH
#define IVANP_TYPE_STR_HH

#include "ivanp/utility/literal.hh"

// https://stackoverflow.com/a/20170989/2640636

namespace ivanp {

template <typename T>
constexpr literal type_str() {
#ifdef __clang__
  literal p = __PRETTY_FUNCTION__;
  return literal(p.data() + 31, p.size() - 32);
#elif defined(__GNUC__)
  literal p = __PRETTY_FUNCTION__;
# if __cplusplus < 201402
  return literal(p.data() + 36, p.size() - 37);
# else
  return literal(p.data() + 46, p.size() - 47);
# endif
#elif defined(_MSC_VER)
  literal p = __FUNCSIG__;
  return literal(p.data() + 38, p.size() - 45);
#else
# error type function does not work with this compiler
#endif
}

template <typename T> void prt_type() {
  std::cout << "\033[34;1m" << type_str<T>() << "\033[0m ("
            << (std::is_empty<T>::value ? 0ul : sizeof(T) )
            << ')' << std::endl;
}

}

#endif
