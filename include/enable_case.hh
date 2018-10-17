#ifndef IVANP_ENABLE_CASE_HH
#define IVANP_ENABLE_CASE_HH

#include <type_traits>
#include "ivanp/boolean.hh"

namespace ivanp {

namespace detail {

template <template<size_t,class...> class, typename, typename...>
struct is_case_impl;

template <template<size_t,class...> class Pred, size_t... I, typename... Args>
struct is_case_impl<Pred,std::index_sequence<I...>,Args...>
  : only_last<Pred<I,Args...>...> { };

} // end namespace detail

template <template<size_t,class...> class Pred, size_t I, typename... Args>
using is_case = detail::is_case_impl<Pred, std::make_index_sequence<I+1>, Args...>;

template <template<size_t,class...> class Pred, size_t I, typename... Args>
using enable_case = std::enable_if_t<is_case<Pred,I,Args...>::value>;

}

#endif
