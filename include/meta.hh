#ifndef IVANP_META_HH
#define IVANP_META_HH

namespace ivanp {

template <typename T> struct rm_rref { using type = T; };
template <typename T> struct rm_rref<T&&> { using type = T; };
template <typename T> using rm_rref_t = typename rm_rref<T>::type;

template <typename T>
struct add_const_to_ref { using type = T; };
template <typename T>
struct add_const_to_ref<T&> { using type = const T&; };
template <typename T>
using add_const_to_ref_t = typename add_const_to_ref<T>::type;

template <typename T>
using decay_ptr_t =
  std::remove_cv_t< std::remove_pointer_t< std::remove_reference_t< T > > >;

// nothing
struct nothing { };

#ifndef IVANP_VOID_T_DEFECT
// void_t
template <typename...> using void_t = void;
// subst_t
template <typename T, typename...> using subst_t = T;
#else
// void_t
template <typename...> struct make_void { using type = void; };
template <typename... Ts> using void_t = typename make_void<Ts...>::type;
// subst_t
template <typename T, typename...> struct make_subst { using type = T; };
template <typename T, typename... _>
using subst_t = typename make_subst<T,_...>::type;
#endif

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

// compose
template <template<typename> typename F, template<typename> typename... Fs>
struct compose {
  template <typename T>
  using type = F<typename compose<Fs...>::template type<T>>;
};
template <template<typename> typename F>
struct compose<F> {
  template <typename T>
  using type = F<T>;
};

// is_defined, has to be at least declared
template <typename T, typename = void>
struct is_defined: std::false_type { };
template <typename T>
struct is_defined<T,decltype(typeid(T),void())>: std::true_type { };

}

#endif
