#ifndef IVANP_BINNER_JSON_HH
#define IVANP_BINNER_JSON_HH

#include <ostream>
#include "ivanp/binner.hh"

namespace ivanp {

namespace detail {
template <
  typename Bin,
  typename AxesSpecs,
  typename Container,
  typename Filler,
  size_t... I>
void to_json(
  std::ostream& os,
  const ivanp::binner<Bin,AxesSpecs,Container,Filler>& hist,
  std::index_sequence<I...>
) {
  os << std::boolalpha << "{\"axes\":[";
  {
    if (I) os << ',';
    using spec = std::tuple_element_t<I,AxesSpecs>;
    os << "{\"uf\":" << spec::under::value
       << ",\"of\":" << spec::over::value
       << ",\"edges\":[";
    const auto& axis = hist.axis<I>();
    const auto n = axis.nedges();
    for (axis_size_type i=0; i<n; ++i) {
      if (i) os << ',';
      os << axis.edge(i);
    }
    os << "]}";
  }...;
  os << "],\"bins\":[";
  
  os << "]}";
}
}

template <
  typename Bin,
  typename AxesSpecs,
  typename Container,
  typename Filler>
void to_json(
  std::ostream& os,
  const ivanp::binner<Bin,AxesSpecs,Container,Filler>& hist
) {
  return detail::to_json(os,hist,
    std::make_index_sequence<std::tuple_size<AxesSpecs>::value>);
}

}

#endif
