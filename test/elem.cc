#include <iostream>
#include <vector>

#include "ivanp/elem.hh"
#include "ivanp/meta.hh"

template <typename T>
void test() { std::cout << __PRETTY_FUNCTION__ << std::endl; }

template <typename T>
using add_cptr_t =
  ivanp::compose<std::add_const_t,std::add_pointer_t>::type<T>;

int main(int argc, char* argv[]) {
  test<std::vector<int>>();
  test<ivanp::add_const_elem_t<std::vector<int>>>();
  test<ivanp::add_ptr_elem_t<std::vector<int>>>();
  test<ivanp::add_const_elem_t<ivanp::add_ptr_elem_t<std::vector<int>>>>();

  test<ivanp::compose<std::add_const_t>::type<int>>();
  test<ivanp::compose<std::add_const_t,std::add_pointer_t>::type<int>>();
  test<
    ivanp::transform_elem_t<
      ivanp::compose<
        std::add_const_t,
        std::add_pointer_t
      >::type,
      std::vector<int>
    >
  >();

  test<add_cptr_t<int>>();
}
