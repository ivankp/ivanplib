#ifndef IVANP_SCRIBE_JSON_HH
#define IVANP_SCRIBE_JSON_HH

#include <cstring>
#include "ivanp/scribe.hh"

namespace nlohmann {
template <> struct adl_serializer<ivanp::scribe::value_node> {
  static void to_json(json& j, const ivanp::scribe::value_node& node) {
    const auto type = node.get_type();
    if (type.is_fundamental()) {
      json x;
      if (!type.is_null()) {
        assign_any_value(x,node);
        if (j.is_array()) j.push_back(std::move(x));
        else j = std::move(x);
      }
    }
    else if (type.is_union()) to_json(j,*node);
    else {
      auto a = json::array();
      for (const auto& x : node) to_json(a,x);
      if (j.is_array()) j.push_back(std::move(a));
      else j = std::move(a);
    }
  }
};
}

#endif
