#include <iostream>
#include <vector>
#include <string>

#include "ivanp/debug/type_str.hh"
#include "ivanp/string.hh"
#include "ivanp/container.hh"
#include "ivanp/splice.hh"

using std::cout;
using std::endl;

int main(int argc, char* argv[]) {
  std::vector<int> ints { 1, 4 };
  std::vector<double> doubles { 2.2, 23. };
  std::vector<std::string> strings { "hello", "world" };

  /*
  auto z1 = ivanp::zip(
    // ints,
    // doubles,
    // strings,
    // std::initializer_list<const char*>{ "a", "b" },
    std::string{"test"}
    // "test"
  );
  ivanp::prt_type<decltype(z1)>();
  for (const auto& t : z1)
    t | [](const auto& x){ cout << x << ' '; }, cout << endl;
  */

  // cout << *std::begin(std::string{"test"}) << endl;

  strings | [](auto&& x){
    cout << x << ' ';
    ivanp::prt_type<decltype(x)>();
  };

  const char* s1 = "12";
  const char* s2 = "ab";
  cout << static_cast<const void*>(s1) << endl;
  ivanp::detail::zip_iter<const char*,const char*> zi { s1, s2 };
  ivanp::prt_type<decltype(zi)>();
  zi.base() | [](auto x){ cout << x << ' '; }, cout << endl;
  cout << *std::get<0>(zi) << endl;
  cout << static_cast<const void*>(std::get<0>(zi)) << endl;
  cout << static_cast<const void*>(&std::get<0>(*zi)) << endl;
  ivanp::prt_type<decltype(*zi)>();
  *zi | [](auto& x){
    cout << static_cast<const void*>(&x) <<": "<< x << ' ';
  }, cout << endl;
  ++zi;
  zi.base() | [](auto x){ cout << x << ' '; }, cout << endl;

  ivanp::zip(strings,ints,doubles,strings) | [](auto&& z){
    ivanp::prt_type<decltype(z)>();
    z | [](auto& x){ cout << x << ' '; }, cout << endl;
  };

  auto comp = doubles | [](auto x){ return ivanp::cat(x); };
  comp += "hello" | [](auto x){ return ivanp::cat(x); };
  comp | [](auto x){ cout << x << ' '; }, cout << endl;

  ivanp::zip(strings,ints) | [](auto&& z) -> std::string {
    return z % [](auto&&... x){ return ivanp::cat(x...); };
  } | [](auto x){ cout << x << ' '; }, cout << endl;
}
