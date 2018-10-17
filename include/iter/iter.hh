#ifndef IVANP_ITER_HH
#define IVANP_ITER_HH

#include <iterator>

namespace ivanp {

/*
template <typename C>
constexpr auto begin(C&& c) -> std::enable_if_t<
  std::is_rvalue_reference<C&&>::value,
  std::move_iterator<typename C::iterator>
> { return std::make_move_iterator(c.begin()); }

template <typename C>
constexpr auto begin(C&& c) noexcept -> std::enable_if_t<
  !std::is_rvalue_reference<C&&>::value,
  decltype(std::begin(std::forward<C>(c)))
> { return std::begin(std::forward<C>(c)); }

template <typename C>
constexpr auto end(C&& c) -> std::enable_if_t<
  std::is_rvalue_reference<C&&>::value,
  std::move_iterator<typename C::iterator>
> { return std::make_move_iterator(c.end()); }

template <typename C>
constexpr auto end(C&& c) noexcept -> std::enable_if_t<
  !std::is_rvalue_reference<C&&>::value,
  decltype(std::end(std::forward<C>(c)))
> { return std::end(std::forward<C>(c)); }
*/

}

#endif
