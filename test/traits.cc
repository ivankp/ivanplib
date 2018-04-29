#include <iostream>
#include <string>

#include "ivanp/traits.hh"

#define TEST(var) \
  std::cout << "\033[36m" #var "\033[0m = " << var << std::endl;

using namespace ivanp;

int main(int argc, char* argv[]) {

  TEST(( is_constructible<std::string,const char*>::value )) // 1
  TEST(( is_constructible<std::string,int>::value )) // 0
  TEST(( is_constructible<std::string,int,char>::value )) // 1
  TEST(( is_constructible<std::string,double>::value )) // 0

}
