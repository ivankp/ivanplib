#ifndef IVANP_SCRIBE_JSON_HH
#define IVANP_SCRIBE_JSON_HH

#include <cstring>
#include "ivanp/scribe.hh"

namespace nlohmann {
template <> struct adl_serializer<ivanp::scribe::value_node> {
  static void to_json(json& j, const ivanp::scribe::value_node& node) {
    const char* name = node.type_name();
    if (strlen(name)==2) {
      const char t = name[0], s = name[1];
      if (t=='f') {
        if (s=='8') { j.push_back(node.cast<double  >()); return; }
        if (s=='4') { j.push_back(node.cast<float   >()); return; }
      } else if (t=='u') {
        if (s=='8') { j.push_back(node.cast<uint64_t>()); return; }
        if (s=='4') { j.push_back(node.cast<uint32_t>()); return; }
        if (s=='2') { j.push_back(node.cast<uint16_t>()); return; }
        if (s=='1') { j.push_back(node.cast<uint8_t >()); return; }
      } else if (t=='i') {
        if (s=='8') { j.push_back(node.cast<int64_t >()); return; }
        if (s=='4') { j.push_back(node.cast<int32_t >()); return; }
        if (s=='2') { j.push_back(node.cast<int16_t >()); return; }
        if (s=='1') { j.push_back(node.cast<int8_t  >()); return; }
      }
    }
    if (node.get_type().is_union()) to_json(j,*node);
    else {
      auto a = json::array();
      for (const auto& x : node) to_json(a,x);
      j.push_back(a);
    }
  }
};
}

#endif
