#include <iostream>

#include "ivanp/seq/seq.hh"
#include "ivanp/debug/type_str.hh"

int main(int argc, char* argv[]) {

  using seq1 = std::make_index_sequence<3>;
  ivanp::prt_type<seq1>();

  using add5 = ivanp::seq::advance<seq1,5>;
  using seq2 = add5::type;
  ivanp::prt_type<add5>();
  ivanp::prt_type<seq2>();
}
