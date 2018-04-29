#include <iostream>
#include <cstring>
#include <vector>
#include <array>
#include <map>

#include <boost/optional.hpp>

// #define PROGRAM_OPTIONS_STD_REGEX
#define PROGRAM_OPTIONS_ALLOW_INT_AS_FLOAT
#include "ivanp/program_options.hh"

#define TEST(var) \
  std::cout << "\033[36m" #var "\033[0m = " << var << std::endl;

using std::cout;
using std::cerr;
using std::endl;
using namespace std::string_literals;

// TODO: make sure all tuples are forwarded so get returns the right ref type

void double_parser(const char* str, double& x) {
  x = std::atof(str) * 2;
}

template <typename F> auto as_value(F f) {
  return ( struct { F f; operator decltype(f())() { return f(); } } ){ f };
}

namespace ivanp { namespace po {
template <>
inline void arg_parser<std::string>(const char* arg, std::string& var) {
  var = arg;
  for (unsigned i=0; var[i]!='\0'; ++i)
    if (var[i]=='s') var[i] = '*';
}
}}

int main(int argc, char* argv[]) {
  cout << std::boolalpha;
  double d = 0, d2;
  std::vector<int> i;
  std::string s;
  // boost::optional<std::string> s;
  // const char* s;
  bool b = false;
  boost::optional<bool> o;
  std::tuple<double,int> tup {1,2};
  std::map<std::string,double> m;
  bool dash = false;

  try {
    using namespace ivanp::po;
    if (program_options() // test for help
      (dash,"-","dash")
      (d,'d',"Double",switch_init(0.1),double_parser)
      (d2,"--d2","1-d",
        // switch_init(std::tie(d))
        default_init(as_value([&d]{ return 1.-d; }))
      )
      (b,'b',"bool switch")
      (o,'o',"optional")
      (i,{"-i","--int"},
       "Position of the first character in the string to be considered in the search.\n"
       "If this is greater than the string length, the function never finds matches.\n"
       "Note: The first character is denoted by a value of 0 (not 1): A value of 0 means that the entire string is searched.")
      (i,"--count","Count",pos(),
        [](const char* str, decltype(i)& x){ x.push_back(strlen(str)); })
      (s,std::forward_as_tuple(
          's', [](const char* arg){ return arg[0]=='s'; }),
        "starts with \'s\'",
        name("-s,[stars with s]"))
      // (&s,".*\\.txt","ends with .txt")
      (tup,{"--tup","-t"},ivanp::cat(ivanp::type_str<decltype(tup)>()))
      (m,'m',ivanp::cat(ivanp::type_str<decltype(m)>()),multi())
      .help_prefix("Test for program options")
      .help_suffix("There's no reason to stay alive if you can't do deadlift!")
      .parse(argc,argv/*,true*/)) return 0;
  } catch (const std::exception& e) {
    cerr <<"\033[31m"<< e.what() <<"\033[0m"<< endl;
    return 1;
  }

  TEST( d )
  TEST( d2 )
  cout << "i:";
  for (int i : i) cout << ' ' << i;
  cout << endl;
  // TEST( i )
  // if (s) cout << "s = " << *s << endl;
  // else cout << "s undefined" << endl;
  TEST( s )
  TEST( b )
  if (o) cout << "o = " << *o << endl;
  else cout << "o undefined" << endl;
  TEST( std::get<0>(tup) )
  TEST( std::get<1>(tup) )
  cout << "m:";
  for (const auto& x : m) cout << ' ' << x.first << ':' << x.second;
  cout << endl;
  TEST( dash )

  return 0;
}
