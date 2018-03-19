#include <iostream>
#include <string>

#include "ivanp/debug/type_str.hh"
#include "ivanp/expand.hh"
#include "ivanp/tuple.hh"

using std::cout;
using std::endl;

struct ref_test {
  template <typename T>
  void operator()(T&& x) & {
    ivanp::prt_type<decltype(x)>();
    ivanp::prt_type<decltype(*this)>();
    cout << "lref\n";
  };
  template <typename T>
  void operator()(T&& x) const& {
    ivanp::prt_type<decltype(x)>();
    ivanp::prt_type<decltype(*this)>();
    cout << "const lref\n";
  };
  template <typename T>
  void operator()(T&& x) && {
    ivanp::prt_type<decltype(x)>();
    ivanp::prt_type<decltype(*this)>();
    cout << "rref\n";
  };
};

int main(int argc, char* argv[]) {
  std::tuple<int,double,char> t0 {35,4.5,'c'};

  auto prt = [](const auto& x){ cout << x << ' '; };
  t0 | prt, cout << endl;

  auto& t1 = t0 | [](auto& x){ ++x; };

  ivanp::prt_type<decltype(t1)>();
  t0 | prt, cout << endl;

  const auto t2 = t1 | [](auto& x){ ++x; };
  t0 | prt, cout << endl;

  ivanp::make_array(1,2,3) | [](auto&& x){
    cout << x << ' ';
    ivanp::prt_type<decltype(x)>();
  };

  cout << "\n*** prt_all ***\n";

  auto prt_all = [](const auto&... x){
    EXPAND(cout << x << ' ')
  };

  t0 % prt_all, cout << endl;

  cout << "\n*** ref_test ***\n";

  t2 | ref_test{};
  const ref_test rt;
  t2 | rt;
}
