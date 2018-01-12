#ifndef IVANP_META_HH
#define IVANP_META_HH

namespace ivanp {

template <typename T> struct rm_rref { using type = T; };
template <typename T> struct rm_rref<T&&> { using type = T; };
template <typename T> using rm_rref_t = typename rm_rref<T>::type;

// nothing
struct nothing { };

// void_t
template <typename... T> struct make_void { typedef void type; };
template <typename... T> using void_t = typename make_void<T...>::type;

// substitute_t
template <typename T, typename...> using substitute_t = T;

// zero
template <typename T> constexpr T zero(T) { return 0; }

// make constant
template <typename T> constexpr const T& as_const(T& x) noexcept { return x; }

// curry
template <template<typename,typename> typename Pred, typename T1>
struct bind_first {
  template <typename T2> using type = Pred<T1,T2>;
};
template <template<typename,typename...> typename Pred, typename... T2>
struct bind_tail {
  template <typename T1> using type = Pred<T1,T2...>;
};

}

#endif
