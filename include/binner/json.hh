#ifndef IVANP_BINNER_JSON_HH
#define IVANP_BINNER_JSON_HH

#include <ostream>
#include "ivanp/binner/binner.hh"
#include "ivanp/unfold.hh"

namespace ivanp {

namespace detail {

template <typename AxisSpec>
void axis_to_json(std::ostream& os, const typename AxisSpec::axis& axis) {
  os << "{\"uf\":" << AxisSpec::under::value
     << ",\"of\":" << AxisSpec::over::value
     << ",\"edges\":[";
  auto n = axis.nedges();
  for (decltype(n) i=0; i<n; ++i) {
    if (i) os << ',';
    os << axis.edge(i);
  }
  os << "]}";
}

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

  UNFOLD((
    os << (I ? "," : ""),
    axis_to_json<std::tuple_element_t<I,AxesSpecs>>(os,hist.template axis<I>())
  ))

  os << "],\"bins\":[";
  bool first = true;
  for (const auto& bin : hist.bins()) {
    if (first) first = false;
    else os << ',';
    to_json(os,bin);
  }
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
    std::make_index_sequence<std::tuple_size<AxesSpecs>::value>{});
}

}

#endif
