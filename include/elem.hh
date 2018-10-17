#ifndef IVANP_ELEM_HH
#define IVANP_ELEM_HH

#include <type_traits>

namespace ivanp {

template <template<typename> class, typename T>
struct transform_elem {
  using type = T; // default to doing nothing
};

#ifdef _GLIBCXX_UTILITY
template <template<typename> class F, typename T1, typename T2>
struct transform_elem<F,std::pair<T1,T2>> {
  using type = std::pair<F<T1>,F<T2>>;
};
#endif

#ifdef _GLIBCXX_TUPLE
template <template<typename> class F, typename... Ts>
struct transform_elem<F,std::tuple<Ts...>> {
  using type = std::tuple<F<Ts>...>;
};
#endif

#ifdef _GLIBCXX_ARRAY
template <template<typename> class F, typename T, size_t N>
struct transform_elem<F,std::array<T,N>> {
  using type = std::array<F<T>,N>;
};
#endif

#ifdef _GLIBCXX_VECTOR
template <template<typename> class F, typename T, typename Alloc>
struct transform_elem<F,std::vector<T,Alloc>> {
  using type = std::vector<F<T>,Alloc>;
};
#endif

template <template<typename> class F, typename T> using transform_elem_t
= typename transform_elem<F,T>::type;

template <typename T> using rm_const_elem_t
= transform_elem_t<std::remove_const_t,T>;
template <typename T> using rm_volatile_elem_t
= transform_elem_t<std::remove_volatile_t,T>;
template <typename T> using rm_cv_elem_t
= transform_elem_t<std::remove_cv_t,T>;
template <typename T> using rm_ptr_elem_t
= transform_elem_t<std::remove_pointer_t,T>;

template <typename T> using add_const_elem_t
= transform_elem_t<std::add_const_t,T>;
template <typename T> using add_volatile_elem_t
= transform_elem_t<std::add_volatile_t,T>;
template <typename T> using add_cv_elem_t
= transform_elem_t<std::add_cv_t,T>;
template <typename T> using add_ptr_elem_t
= transform_elem_t<std::add_pointer_t,T>;

}

#endif
