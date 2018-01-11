#ifndef IVANP_SEQ_HH
#define IVANP_SEQ_HH

#include <utility>

namespace ivanp { namespace seq {

template <typename Seq, typename Seq::value_type Addend> struct advance;
template <typename T, T... I, typename std::integer_sequence<T,I...>::value_type Addend>
struct advance<std::integer_sequence<T,I...>, Addend> {
  using type = std::integer_sequence<T,(I+Addend)...>;
};


}}

#endif
