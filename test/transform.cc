#include <iostream>
#include <vector>
#include <string>

#include "ivanp/debug/type_str.hh"
#include "ivanp/transform/transform.hh"
#include "ivanp/string/catstr.hh"

using std::cout;
using std::endl;

int main(int argc, char* argv[]) {
  std::vector<int> ints { 1, 4, 64, 32, 23 };
  std::vector<double> doubles { 2.2, 23., 5, 0.2, 85 };

  auto t1 = std::make_tuple(1,'a',5.5);
  auto t2 = t1 | [](auto x){ return ivanp::cat(x); };

  ivanp::prt_type<decltype(t1)>();
  ivanp::prt_type<decltype(t2)>();

  t1 | [](auto& x){ cout << x << endl; };

  ivanp::prt_type<decltype(ivanp::zip(ints, doubles))>();
  ivanp::prt_type<typename decltype(ivanp::zip(ints, doubles))::value_type>();

  auto zip = ivanp::zip(
    // [](auto x) -> std::decay_t<decltype(*x)> { return std::move(*x); },
    ints,
    // doubles);
    // std::move(doubles));
    // std::vector<std::string>{"A1","B2","C3","D4","E5"},
    std::vector<std::string>{"a1","b2","c3","d4","e5"});

  ivanp::prt_type<decltype(zip)>();
  cout << zip.size() << endl;

  for (auto x : zip) x | [](auto& x){ cout <<' '<< x; }, cout << endl;

}
