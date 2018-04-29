#ifndef IVANP_IS_STD_VECTOR_HH
#define IVANP_IS_STD_VECTOR_HH

namespace ivanp {

template <typename T> struct is_std_vector: std::false_type { };
template <typename T, typename Alloc>
struct is_std_vector<std::vector<T,Alloc>>: std::true_type { };

}

#endif
